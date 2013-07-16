/******************************************************************************* 
* Copyright (C) Marvell International Ltd. and its affiliates 
* 
* Marvell GPL License Option 
* 
* If you received this File from Marvell, you may opt to use, redistribute and/or 
* modify this File in accordance with the terms and conditions of the General 
* Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
* available along with the File in the license.txt file or by writing to the Free 
* Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
* on the worldwide web at http://www.gnu.org/licenses/gpl.txt.  
* 
* THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
* WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
* DISCLAIMED.  The GPL License provides additional details about this warranty 
* disclaimer.  
********************************************************************************/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/io.h>
#include <linux/pm.h>
#include <mach/galois_platform.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <mach/galois_generic.h>
#include <mach/sm.h>

#define w32smSysCtl_SM_CTRL { \
	unsigned int uSM_CTRL_ISO_EN		:  1; \
	unsigned int uSM_CTRL_SM2SOC_SW_INTR	:  1; \
	unsigned int uSM_CTRL_SOC2SM_SW_INTR	:  1; \
	unsigned int uSM_CTRL_REV_0		:  2; \
	unsigned int uSM_CTRL_ADC_SEL		:  4; \
	unsigned int uSM_CTRL_ADC_PU		:  1; \
	unsigned int uSM_CTRL_ADC_CKSEL		:  2; \
	unsigned int uSM_CTRL_ADC_START		:  1; \
	unsigned int uSM_CTRL_ADC_RESET		:  1; \
	unsigned int uSM_CTRL_ADC_BG_RDY	:  1; \
	unsigned int uSM_CTRL_ADC_CONT		:  1; \
	unsigned int uSM_CTRL_ADC_BUF_EN	:  1; \
	unsigned int uSM_CTRL_ADC_VREF_SEL	:  1; \
	unsigned int uSM_CTRL_ADC_TST_SAR	:  1; \
	unsigned int uSM_CTRL_ADC_ROTATE_SEL	:  1; \
	unsigned int uSM_CTRL_TSEN_EN		:  1; \
	unsigned int uSM_CTRL_TSEN_CLK_SEL	:  1; \
	unsigned int uSM_CTRL_TSEN_MODE_SEL	:  1; \
	unsigned int uSM_CTRL_TSEN_ADC_CAL	:  2; \
	unsigned int uSM_CTRL_TSEN_TST_SEL	:  2; \
	unsigned int RSVDx14_b27		:  5; \
}

#define RA_smSysCtl_SM_CTRL	0x0014

typedef union {
	unsigned int u32;
	struct w32smSysCtl_SM_CTRL;
} T32smSysCtl_SM_CTRL;

#ifdef SM_DEBUG
#define DPRINT(fmt, args...) printk(fmt, ## args)
#else
#define DPRINT(fmt, args...)
#endif

#define SM_Q_PUSH( pSM_Q ) {				\
	pSM_Q->m_iWrite += SM_MSG_SIZE;			\
	if( pSM_Q->m_iWrite >= SM_MSGQ_SIZE )		\
		pSM_Q->m_iWrite -= SM_MSGQ_SIZE;	\
	pSM_Q->m_iWriteTotal += SM_MSG_SIZE; }

#define SM_Q_POP( pSM_Q ) {				\
	pSM_Q->m_iRead += SM_MSG_SIZE;			\
	if( pSM_Q->m_iRead >= SM_MSGQ_SIZE )		\
		pSM_Q->m_iRead -= SM_MSGQ_SIZE;		\
	pSM_Q->m_iReadTotal += SM_MSG_SIZE; }

static MV_SM_MsgQ *Dispatch_Pool;

static struct {
	int m_iModuleID;
	bool m_bWaitQueue;
	wait_queue_head_t *m_pQueue;
	int m_bMsgRev;
} SMModules[MAX_MSG_TYPE] = {
	{ MV_SM_ID_SYS, false, NULL, 0},
	{ MV_SM_ID_COMM, false, NULL, 0},
	{ MV_SM_ID_IR, false, NULL, 0},
	{ MV_SM_ID_KEY, false, NULL, 0},
	{ MV_SM_ID_POWER, false, NULL, 0},
	{ MV_SM_ID_WD, false, NULL, 0},
	{ MV_SM_ID_TEMP, false, NULL, 0},
	{ MV_SM_ID_VFD, false, NULL, 0},
	{ MV_SM_ID_SPI, false, NULL, 0},
	{ MV_SM_ID_I2C, false, NULL, 0},
	{ MV_SM_ID_UART, false, NULL, 0},
	{ MV_SM_ID_CEC, false, NULL, 0},
};

#ifdef CONFIG_MV88DE3100_IR
extern void girl_sm_int(void);
#else
#define girl_sm_int()	do {} while (0)
#endif

int bsm_msg_send(int id, void *msg, int len)
{
	MV_SM_MsgQ *q = (MV_SM_MsgQ*)(SOC_DTCM(SM_CPU0_INPUT_QUEUE_ADDR));
	MV_SM_Message *smsg;
	int msgdata = *((int*)msg);

	if (id < 1 || id > MAX_MSG_TYPE || len > SM_MSG_BODY_SIZE)
		return -1;

	if (q->m_iWrite < 0 || q->m_iWrite >= SM_MSGQ_SIZE)
		return -1;

	/* message queue full, ignore the newest message */
	if (q->m_iRead == q->m_iWrite && q->m_iReadTotal != q->m_iWriteTotal)
		return -1;

	if (MV_SM_ID_POWER == id &&
		(MV_SM_POWER_SYS_RESET == msgdata ||
		MV_SM_POWER_WARMDOWN_REQUEST == msgdata ||
		MV_SM_POWER_WARMDOWN_REQUEST_2 == msgdata)) {
		printk("Linux warm down.\n");
		/* wait for 50ms to make sure that NAND transaction is done */
		mdelay(50);
	}

	smsg = (MV_SM_Message*)(&(q->m_Queue[q->m_iWrite]));
	smsg->m_iModuleID = id;
	smsg->m_iMsgLen = len;
	memcpy(smsg->m_pucMsgBody, msg, len);
	SM_Q_PUSH(q);

	return 0;
}
EXPORT_SYMBOL(bsm_msg_send);

int bsm_msg_receive(int id, void *pMsgBody, int *piLen)
{
	MV_SM_MsgQ *pDisp_Q;
	MV_SM_Message *pDisp_Msg;

	if( id < 1 || id > MAX_MSG_TYPE )
		return -1;

	pDisp_Q = &Dispatch_Pool[id-1];
	/* check whether there is new message */
	if( pDisp_Q->m_iReadTotal == pDisp_Q->m_iWriteTotal) {
		DPRINT("kernel, no msg\n");
		return -1;
	}

	/* If some keys was sent by SM but not received by kernel
	 * ignore these keys, and read the newest key
	 */
	while (pDisp_Q->m_iReadTotal < (pDisp_Q->m_iWriteTotal-SM_MSG_SIZE))
		SM_Q_POP(pDisp_Q);

	pDisp_Msg = (MV_SM_Message*)(&(pDisp_Q->m_Queue[pDisp_Q->m_iRead]));
	DPRINT("bsm_msg_receive ID = %d, rt = 0x%x, wt = 0x%x, key=0x%08x.\n", id, pDisp_Q->m_iReadTotal, pDisp_Q->m_iWriteTotal,*(int*)&(pDisp_Msg->m_pucMsgBody[0]));
	*piLen = pDisp_Msg->m_iMsgLen;
	if (*piLen > SM_MSG_BODY_SIZE)
		return -1;
	memcpy(pMsgBody, pDisp_Msg->m_pucMsgBody, *piLen);
	SM_Q_POP(pDisp_Q);

	return 0;
}
EXPORT_SYMBOL(bsm_msg_receive);

static void bsm_msg_dispatch(void)
{
	MV_SM_MsgQ *pSM_Q = (MV_SM_MsgQ*)(SOC_DTCM(SM_CPU0_OUTPUT_QUEUE_ADDR));
	MV_SM_MsgQ *pDisp_Q;
	MV_SM_Message *pSM_Msg, *pDisp_Msg;
	int i;

	if (pSM_Q->m_iRead < 0 || pSM_Q->m_iRead >= SM_MSGQ_SIZE || pSM_Q->m_iReadTotal > pSM_Q->m_iWriteTotal)
		return;

	/* if buffer was overflow written, only the last messages are
	 * saved in queue. move read pointer into the same position of
	 * write pointer and keep buffer full.
	 */
	if (pSM_Q->m_iWriteTotal - pSM_Q->m_iReadTotal > SM_MSGQ_SIZE) {
		int iTotalDataSize = pSM_Q->m_iWriteTotal - pSM_Q->m_iReadTotal;

		pSM_Q->m_iReadTotal += iTotalDataSize - SM_MSGQ_SIZE;
		pSM_Q->m_iRead += iTotalDataSize % SM_MSGQ_SIZE;
		if (pSM_Q->m_iRead >= SM_MSGQ_SIZE)
			pSM_Q->m_iRead -= SM_MSGQ_SIZE;
	}

	while (pSM_Q->m_iReadTotal < pSM_Q->m_iWriteTotal) {
		pSM_Msg = (MV_SM_Message*)(&(pSM_Q->m_Queue[pSM_Q->m_iRead]));
		for (i = 0; i < MAX_MSG_TYPE; i++) {
			if (pSM_Msg->m_iModuleID == i + 1) {
				/* move from SM Queue to Dispatch queue */
				pDisp_Q = &Dispatch_Pool[i];
				pDisp_Msg = (MV_SM_Message*)(&(pDisp_Q->m_Queue[pDisp_Q->m_iWrite]));

				if (pDisp_Q->m_iRead == pDisp_Q->m_iWrite && pDisp_Q->m_iReadTotal != pDisp_Q->m_iWriteTotal) {
					/* Dispatch Queue full, do nothing now, the
					 * newest message will be lost
					 */
				} else {
					memcpy(pDisp_Msg, pSM_Msg, sizeof(MV_SM_Message));
					DPRINT("Received SM msg ID %d, Len %d.\n", pSM_Msg->m_iModuleID, pSM_Msg->m_iMsgLen);
					SM_Q_PUSH(pDisp_Q);
				}

				if (pSM_Msg->m_iModuleID == MV_SM_ID_IR)
					girl_sm_int();

				if (SMModules[i].m_bWaitQueue) {
					wake_up_interruptible(SMModules[i].m_pQueue);
					SMModules[i].m_bMsgRev = 1;
				}
				break;
			}
		}
		SM_Q_POP(pSM_Q);
	}
}

static irqreturn_t bsm_intr(int irq, void *dev_id)
{
	T32smSysCtl_SM_CTRL reg;

	reg.u32 = readl_relaxed(SM_APBC(SM_SM_SYS_CTRL_REG_BASE + RA_smSysCtl_SM_CTRL));
	reg.uSM_CTRL_SM2SOC_SW_INTR = 0;
	writel_relaxed(reg.u32, SM_APBC(SM_SM_SYS_CTRL_REG_BASE + RA_smSysCtl_SM_CTRL));

	bsm_msg_dispatch();

	return IRQ_HANDLED;
}

static ssize_t berlin_sm_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	int len, ret, i;
	MV_SM_Message SM_Msg;
	int id = (int)(*ppos);

	if (count > SM_MSG_SIZE)
		return 0;

	for (i = 0; i < MAX_MSG_TYPE; i++) {
		if (SMModules[i].m_iModuleID == id) {
			if (SMModules[i].m_bWaitQueue) {
				DPRINT("mod %d wait queue,wait here\n",SMModules[i].m_iModuleID);
				wait_event_interruptible(*(SMModules[i].m_pQueue), SMModules[i].m_bMsgRev== 1);
				SMModules[i].m_bMsgRev = 0;
			}
			break;
		}
	}

	ret = bsm_msg_receive(id, SM_Msg.m_pucMsgBody, &len);
	if (ret < 0)
		return 0;

	SM_Msg.m_iModuleID = id;
	SM_Msg.m_iMsgLen = len;
	ret = copy_to_user(buf, (void *)&SM_Msg, SM_MSG_SIZE);
	if (ret)
		return 0;
	else
		return count;
}

static ssize_t berlin_sm_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	MV_SM_Message SM_Msg;
	int ret;
	int id = (int)(*ppos);

	if (count < 4 || count > SM_MSG_BODY_SIZE)
		return -EINVAL;

	if (copy_from_user(SM_Msg.m_pucMsgBody, buf, count))
		return -EFAULT;

	ret = bsm_msg_send(id, SM_Msg.m_pucMsgBody, count);
	if (ret < 0)
		return -EFAULT;
	else
		return count;
}

static long berlin_sm_ioctl(struct file *file, unsigned int cmd,
			    unsigned long arg)
{
	int i, ret = 0;
	switch (cmd) {
	case SM_Enable_WaitQueue:
		for (i = 0; i < MAX_MSG_TYPE;i++) {
			if (SMModules[i].m_iModuleID == arg) {
				SMModules[i].m_bWaitQueue = true;
				SMModules[i].m_pQueue = kmalloc(sizeof(wait_queue_head_t), GFP_KERNEL);
				init_waitqueue_head(SMModules[i].m_pQueue);
				break;
			}
		}
		break;
	case SM_Disable_WaitQueue:
		for (i = 0; i < MAX_MSG_TYPE; i++) {
			if (SMModules[i].m_iModuleID == arg) {
				SMModules[i].m_bWaitQueue = false;
				kfree(SMModules[i].m_pQueue);
				SMModules[i].m_pQueue = NULL;
			}
			break;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static struct file_operations berlin_sm_fops = {
	.owner		= THIS_MODULE,
	.write		= berlin_sm_write,
	.read		= berlin_sm_read,
	.unlocked_ioctl	= berlin_sm_ioctl,
	.llseek		= default_llseek,
};

static struct miscdevice sm_dev = {
	.minor	= BERLIN_SM_MISC_MINOR,
	.name	= BERLIN_SM_NAME,
	.fops	= &berlin_sm_fops,
};

void galois_arch_reset(char mode, const char *cmd)
{
#define SM_MSG_EXTRA_BUF_ADDR (MEMMAP_SM_REG_BASE+SM_ITCM_ALIAS_SIZE+SM_DTCM_SIZE-512)
	int len = 0;
	int msg = MV_SM_POWER_SYS_RESET;
	void *p = __io(SM_MSG_EXTRA_BUF_ADDR);

	if (cmd != NULL) {
		len = strlen(cmd) + 1;
		if (len > SM_MSG_EXTRA_BUF_SIZE - sizeof(len)) {
			len = SM_MSG_EXTRA_BUF_SIZE - sizeof(len);
			printk("cut off too long reboot args to %d bytes\n", len);
		}
		printk("reboot cmd is %s@%d\n", cmd, len);
		memcpy(p, &len, sizeof(len));
		memcpy(p + sizeof(len), cmd, len);
	} else
		memset(p, 0, sizeof(int));

	flush_cache_all();

	bsm_msg_send(MV_SM_ID_POWER, &msg, sizeof(int));
	for (;;);
}

static void galois_power_off(void)
{
	int msg = MV_SM_POWER_WARMDOWN_REQUEST;
	bsm_msg_send(MV_SM_ID_POWER, &msg, sizeof(msg));
	for (;;);
}

static int __init berlin_sm_init(void)
{
	Dispatch_Pool = kzalloc(sizeof(MV_SM_MsgQ) * MAX_MSG_TYPE, GFP_KERNEL);
	if (!Dispatch_Pool)
		return -ENOMEM;

	if (misc_register(&sm_dev))
		return -ENODEV;

	pm_power_off = galois_power_off;

	return request_irq(IRQ_SM2SOCINT, bsm_intr, 0, "bsm", &sm_dev);
}

static void __exit berlin_sm_exit(void)
{
	misc_deregister(&sm_dev);
	free_irq(IRQ_SM2SOCINT, &sm_dev);
	kfree(Dispatch_Pool);
}

module_init(berlin_sm_init);
module_exit(berlin_sm_exit);

MODULE_AUTHOR("Marvell-Galois");
MODULE_DESCRIPTION("System Manager Driver");
MODULE_LICENSE("GPL");
