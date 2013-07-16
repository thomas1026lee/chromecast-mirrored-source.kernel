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

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/io.h>
#include <plat/usb.h>
#include <linux/usb/mv_usb.h>
#include <mach/gpio.h>

#ifdef CONFIG_MV88DE3100_BG2_A0
#define PHY_PLL	0x5560
#else
#define PHY_PLL	0x54C0
#endif

int mv_udc_phy_init(unsigned int base)
{
	/* Initialize the USB PHY REGS */

	/* program PLL divider */
	writel(PHY_PLL, (base + USB_PHY_PLL_REG));
	/* power up PLL */
	writel(0x2235, (base + USB_PHY_PLL_CONTROL_REG));
	/* power up analog */
	writel(0x5680, (base + USB_PHY_ANALOG_REG));
	/* Set LPF_COEFF to '10' */
	writel(0xAA79, (base + USB_PHY_RX_CTRL_REG));
	/* Power up OTG */
	writel(0x10  , (base + 0x5C));

	printk(" UDC PHY initialized\n");
	return 0;
}

#ifdef MV88DE3100_USB_VBUS_DRV_GPIO
int set_usb1_vbus(unsigned int on)
{
	if(on) {
		/* turn on the vbus */
		GPIO_PortWrite(MV88DE3100_USB_VBUS_DRV_GPIO, 1);
	} else {
		/* turn off the vbus */
		GPIO_PortWrite(MV88DE3100_USB_VBUS_DRV_GPIO, 0);
	}
	return 0;
}
#else
int set_usb1_vbus(unsigned int on)
{
	return 0;
}
#endif
