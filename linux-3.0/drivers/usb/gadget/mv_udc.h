
#ifndef __MV_UDC_H
#define __MV_UDC_H

#define DQH_ALIGNMENT		2048
#define DTD_ALIGNMENT		64
#define DMA_BOUNDARY		4096

#define EP_DIR_IN	1
#define EP_DIR_OUT	0

#define DMA_ADDR_INVALID	(~(dma_addr_t)0)

#define EP0_MAX_PKT_SIZE	64
/* ep0 transfer state */
#define WAIT_FOR_SETUP		0
#define DATA_STATE_XMIT		1
#define DATA_STATE_NEED_ZLP	2
#define WAIT_FOR_OUT_STATUS	3
#define DATA_STATE_RECV		4

struct mv_udc {
	struct usb_gadget		gadget;
	struct usb_gadget_driver	*driver;
	spinlock_t			lock;
	struct completion		*done;
	struct platform_device		*dev;
	int				irq;

	struct mv_cap_regs __iomem	*cap_regs;
	struct mv_op_regs __iomem	*op_regs;
	void __iomem				*base_regs;
	unsigned int			phy_regs;
	unsigned int			max_eps;
	struct mv_dqh			*ep_dqh;
	size_t				ep_dqh_size;
	dma_addr_t			ep_dqh_dma;

	struct dma_pool			*dtd_pool;
	struct mv_ep			*eps;

	struct mv_dtd			*dtd_head;
	struct mv_dtd			*dtd_tail;
	unsigned int			dtd_entries;

	struct mv_req			*status_req;
	struct usb_ctrlrequest		local_setup_buff;

	unsigned int		resume_state;	/* USB state to resume */
	unsigned int		usb_state;	/* USB current state */
	unsigned int		ep0_state;	/* Endpoint zero state */
	unsigned int		ep0_dir;

	unsigned int		dev_addr;

	int			errors;
	unsigned		softconnect:1,
				vbus_active:1,
				remote_wakeup:1,
				softconnected:1,
				force_fs:1,
				active:1;
	unsigned int		power;
	struct work_struct event_work;

	struct otg_transceiver	*transceiver;

	struct mv_usb_platform_data	*pdata;
};

/* endpoint data structure */
struct mv_ep {
	struct usb_ep		ep;
	struct mv_udc		*udc;
	struct list_head	queue;
	struct mv_dqh		*dqh;
	const struct usb_endpoint_descriptor	*desc;
	u32			direction;
	char			name[14];
	unsigned		stopped:1,
				wedge:1,
				ep_type:2,
				ep_num:8;
};

/* request data structure */
struct mv_req {
	struct usb_request	req;
	struct mv_dtd		*dtd, *head, *tail;
	struct mv_ep		*ep;
	struct list_head	queue;
	unsigned		dtd_count;
	unsigned		mapped:1;
};

#define EP_QUEUE_HEAD_MULT_POS			30
#define EP_QUEUE_HEAD_ZLT_SEL			0x20000000
#define EP_QUEUE_HEAD_MAX_PKT_LEN_POS		16
#define EP_QUEUE_HEAD_MAX_PKT_LEN(ep_info)	(((ep_info)>>16)&0x07ff)
#define EP_QUEUE_HEAD_IOS			0x00008000
#define EP_QUEUE_HEAD_NEXT_TERMINATE		0x00000001
#define EP_QUEUE_HEAD_IOC			0x00008000
#define EP_QUEUE_HEAD_MULTO			0x00000C00
#define EP_QUEUE_HEAD_STATUS_HALT		0x00000040
#define EP_QUEUE_HEAD_STATUS_ACTIVE		0x00000080
#define EP_QUEUE_CURRENT_OFFSET_MASK		0x00000FFF
#define EP_QUEUE_HEAD_NEXT_POINTER_MASK		0xFFFFFFE0
#define EP_QUEUE_FRINDEX_MASK			0x000007FF
#define EP_MAX_LENGTH_TRANSFER			0x4000

struct mv_dqh {
	/* Bits 16..26 Bit 15 is Interrupt On Setup */
	u32	max_packet_length;
	u32	curr_dtd_ptr;		/* Current dTD Pointer */
	u32	next_dtd_ptr;		/* Next dTD Pointer */
	/* Total bytes (16..30), IOC (15), INT (8), STS (0-7) */
	u32	size_ioc_int_sts;
	u32	buff_ptr0;		/* Buffer pointer Page 0 (12-31) */
	u32	buff_ptr1;		/* Buffer pointer Page 1 (12-31) */
	u32	buff_ptr2;		/* Buffer pointer Page 2 (12-31) */
	u32	buff_ptr3;		/* Buffer pointer Page 3 (12-31) */
	u32	buff_ptr4;		/* Buffer pointer Page 4 (12-31) */
	u32	reserved1;
	/* 8 bytes of setup data that follows the Setup PID */
	u8	setup_buffer[8];
	u32	reserved2[4];
};


#define DTD_NEXT_TERMINATE		(0x00000001)
#define DTD_IOC				(0x00008000)
#define DTD_STATUS_ACTIVE		(0x00000080)
#define DTD_STATUS_HALTED		(0x00000040)
#define DTD_STATUS_DATA_BUFF_ERR	(0x00000020)
#define DTD_STATUS_TRANSACTION_ERR	(0x00000008)
#define DTD_RESERVED_FIELDS		(0x00007F00)
#define DTD_ERROR_MASK			(0x68)
#define DTD_ADDR_MASK			(0xFFFFFFE0)
#define DTD_PACKET_SIZE			0x7FFF0000
#define DTD_LENGTH_BIT_POS		(16)

struct mv_dtd {
	u32	dtd_next;
	u32	size_ioc_sts;
	u32	buff_ptr0;		/* Buffer pointer Page 0 */
	u32	buff_ptr1;		/* Buffer pointer Page 1 */
	u32	buff_ptr2;		/* Buffer pointer Page 2 */
	u32	buff_ptr3;		/* Buffer pointer Page 3 */
	u32	buff_ptr4;		/* Buffer pointer Page 4 */
	u32	scratch_ptr;
	/* 32 bytes */
	dma_addr_t td_dma;		/* dma address for this td */
	struct mv_dtd *next_dtd_virt;
};

extern int mv_udc_phy_init(unsigned int base);

#endif
