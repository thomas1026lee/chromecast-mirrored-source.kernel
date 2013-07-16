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

#ifndef __MV_PLATFORM_USB_H
#define __MV_PLATFORM_USB_H

#include <linux/usb/mv_usb.h>
#include <linux/notifier.h>

enum pxa_ehci_type {
	EHCI_UNDEFINED = 0,
	PXA_U2OEHCI,  /* pxa 168, 9xx */
	PXA_SPH, /* pxa 168, 9xx SPH */
	MMP3_HSIC, /* mmp3 hsic */
	MMP3_FSIC, /* mmp3 fsic */
};

enum usb_port_speed {
	USB_PORT_SPEED_FULL = 0,	/* full speed: 0x0 */
	USB_PORT_SPEED_LOW,		/* low speed: 0x1 */
	USB_PORT_SPEED_HIGH,		/* high speed: 0x2 */
	USB_PORT_SPEED_UNKNOWN,		/* unknown speed: 0x3 */
};


enum {
	MV_USB_MODE_OTG,
	MV_USB_MODE_HOST,
	MV_USB_MODE_DEVICE,
};

enum {
	VBUS_LOW	= 0,
	VBUS_HIGH	= 1 << 0,
};

enum charger_type {
	NULL_CHARGER            = 0,
	DEFAULT_CHARGER,
	VBUS_CHARGER,
	AC_CHARGER_STANDARD,
	AC_CHARGER_OTHER,
};

int mv_udc_phy_init(unsigned int base);
int set_usb1_vbus(unsigned int on);
void set_usb_mem_pri(void);


extern int mv_udc_register_client(struct notifier_block *nb);
extern int mv_udc_unregister_client(struct notifier_block *nb);

struct mv_usb_addon_irq {
	unsigned int	irq;
	int		(*poll)(void);
	void		(*init)(void);
};

struct mv_usb_platform_data {
	unsigned int		clknum;
	char			**clkname;
	struct mv_usb_addon_irq	*id;	/* Only valid for OTG. ID pin change*/
	struct mv_usb_addon_irq	*vbus;	/* valid for OTG/UDC. VBUS change*/
	/* only valid for HCD. OTG or Host only*/
	unsigned int		mode;

	/* This flag is used for that needs id pin checked by otg */
	unsigned int	disable_otg_clock_gating:1;
	/* Force a_bus_req to be asserted */
	unsigned int	otg_force_a_bus_req:1;

	int     (*phy_init)(unsigned int regbase);
	void    (*phy_deinit)(unsigned int regbase);
	int	(*set_vbus)(unsigned int vbus);
	int	(*private_init)(struct mv_op_regs *opregs,
					unsigned int phyregs);
};

extern int pxa_usb_phy_init(unsigned int base);
extern void pxa_usb_phy_deinit(unsigned int base);

#ifdef CONFIG_USB_EHCI_PXA_U2H_HSIC
extern int mmp3_hsic_phy_init(unsigned int base);
extern void mmp3_hsic_phy_deinit(unsigned int base);
extern int mmp3_hsic_private_init(struct mv_op_regs *opregs,
					unsigned int phyregs);
#endif

#endif
