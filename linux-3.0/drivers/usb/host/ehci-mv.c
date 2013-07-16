/*
 * linux/drivers/usb/host/ehci-mv.c
 *
 * Authors: Hongzhan Chen <hzchen@marvell.com>
 * Copyright (C) 2011 Marvell Ltd.
 *
 * Based on "ehci-au1xxx.c"
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/version.h>
#include <mach/galois_platform.h>
#include <plat/usb.h>

#define MV_USB_CHIP_ID_OFFSET	0
#define MV_USB_CHIP_ID_MASK		0x3F
#define MV_USB_CHIP_USB20		0x05
#define MV_USB_CHIP_CAP_OFFSET	(0x100)

#define MV_USB_PHY_REG_BASE_USB0	0xF7B74000
#define MV_USB_PHY_REG_BASE_USB1	0xF7B78000
#define MV_USB_PHY_PLL_REG			0x04
#define MV_USB_PHY_PLL_CONTROL_REG	0x08
#define MV_USB_PHY_ANALOG_REG		0x34
#define MV_USB_PHY_RX_CTRL_REG		0x20
#ifdef CONFIG_MV88DE3100_BG2_A0
#define PHY_PLL	0x5560
#else
#define PHY_PLL	0x54C0
#endif



#define MV_USB_MEMMAP_USB1_REG_BASE 		0xF7EE0000

#if (BERLIN_CHIP_VERSION != BERLIN_BG2CD_A0)
#error "should not be here for BG2CD"
#define MV_USB_RESET_TRIGGER			0x0007C	/* RA_Gbl_ResetTrigger */
#define MV_USB_USB0_SYNC_RESET			25	/* LSb32Gbl_ResetTrigger_usb0SyncReset */
#define MV_USB_USB1_SYNC_RESET			26	/* LSb32Gbl_ResetTrigger_usb1SyncReset */
#else /* (BERLIN_CHIP_VERSION == BERLIN_BG2CD_A0) */
#define MV_USB_RESET_TRIGGER		0x0178	/* RA_Gbl_ResetTrigger */
#define MV_USB_USB0_SYNC_RESET		23	/* LSb32Gbl_ResetTrigger_usb0SyncReset */
#define MV_USB_USB1_SYNC_RESET		24	/* LSb32Gbl_ResetTrigger_usb1SyncReset */
#endif /* (BERLIN_CHIP_VERSION != BERLIN_BG2CD_A0) */

#define USBMODE_OFFSET		0x1a8
#define USBMODE_CM_HOST		3

extern int usb_disabled(void);

static void usb_ehci_mv_gpioset(struct ehci_hcd *ehci, int val)
{
	u32 read;

	ehci_writel(ehci, 0x00, (u32 __iomem *)(APB_GPIO_INST0_BASE + 0x08));
	read = ehci_readl(ehci, (u32 __iomem *)(APB_GPIO_INST0_BASE + 0x04));
	ehci_writel(ehci, read | 0x04, (u32 __iomem *)(APB_GPIO_INST0_BASE + 0x04));
	read = ehci_readl(ehci, (u32 __iomem *)(APB_GPIO_INST0_BASE + 0x00));

	if (val)
		ehci_writel(ehci, read | 0x04, (u32 __iomem *)(APB_GPIO_INST0_BASE + 0x00));
	else
		ehci_writel(ehci, read & 0xfffffffb, (u32 __iomem *)(APB_GPIO_INST0_BASE + 0x00));
}

static void mv_start_ehc(struct usb_hcd *hcd)
{
	u32 temp;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	usb_ehci_mv_gpioset(ehci, 0);

#ifndef CONFIG_MV88DE3100_PM_DISABLE_USB1
	if (hcd->rsrc_start == MV_USB_MEMMAP_USB1_REG_BASE) {
		ehci_writel(ehci, PHY_PLL, (u32 __iomem *)(MV_USB_PHY_REG_BASE_USB1 + MV_USB_PHY_PLL_REG));
		ehci_writel(ehci, 0x2235, (u32 __iomem *)(MV_USB_PHY_REG_BASE_USB1 + MV_USB_PHY_PLL_CONTROL_REG));
		ehci_writel(ehci, 0x5680, (u32 __iomem *)(MV_USB_PHY_REG_BASE_USB1 + MV_USB_PHY_ANALOG_REG));
		ehci_writel(ehci, 0xAA79, (u32 __iomem *)(MV_USB_PHY_REG_BASE_USB1
							+ MV_USB_PHY_RX_CTRL_REG));

		/* set USBMODE to host mode */
		temp = ehci_readl(ehci, hcd->regs + USBMODE_OFFSET);
		ehci_writel(ehci, temp | USBMODE_CM_HOST, hcd->regs + USBMODE_OFFSET);

		ehci_writel(ehci, 1 << MV_USB_USB1_SYNC_RESET, (u32 __iomem *)(MEMMAP_CHIP_CTRL_REG_BASE + MV_USB_RESET_TRIGGER));
	} else
#endif
	{
		ehci_writel(ehci, PHY_PLL, (u32 __iomem *)(MV_USB_PHY_REG_BASE_USB0 + MV_USB_PHY_PLL_REG));
		ehci_writel(ehci, 0x2235, (u32 __iomem *)(MV_USB_PHY_REG_BASE_USB0 + MV_USB_PHY_PLL_CONTROL_REG));
		ehci_writel(ehci, 0x5680, (u32 __iomem *)(MV_USB_PHY_REG_BASE_USB0 + MV_USB_PHY_ANALOG_REG));
		ehci_writel(ehci, 0xAA79, (u32 __iomem *)(MV_USB_PHY_REG_BASE_USB0
							+ MV_USB_PHY_RX_CTRL_REG));
		ehci_writel(ehci, 1 << MV_USB_USB0_SYNC_RESET, (u32 __iomem *)(MEMMAP_CHIP_CTRL_REG_BASE + MV_USB_RESET_TRIGGER));
	}

	usb_ehci_mv_gpioset(ehci, 1);
}

static void mv_stop_ehc(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	usb_ehci_mv_gpioset(ehci, 0);
}

/* This is hardware specific init function, do following
 * halt ==> init ==> reset ==> power_off
 * please notice, ehci_init constructs the internal data structures,
 * so  needs to be invoked once before any operations, but once only.
 * Any hardware specific stuff can be added here.
 * 			He */
static int ehci_mv_setup(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int retval;

	mv_start_ehc(hcd);

	ehci_halt(ehci);

	/*
	 * data structure init
	 */
	retval = ehci_init(hcd);
	if (retval)
		return retval;

	hcd->has_tt = 1;
	ehci->sbrn = 0x20;

	ehci_reset(ehci);

	ehci_port_power(ehci, 0);

	return retval;
}

static int ehci_mv_start( struct usb_hcd *hcd)
{
	int retval;
	struct ehci_hcd  *ehci = hcd_to_ehci(hcd);

	retval = ehci_run(hcd);

	ehci_writel(ehci, 0x07, hcd->regs + 0x090);
	ehci_writel(ehci, 0x13, hcd->regs + 0x1a8);

	return retval;

}

static int usb_ehci_mv_probe(const struct hc_driver *driver, struct usb_hcd **hcd_out,
			     struct platform_device *pdev)
{
	struct mv_usb_platform_data *pdata = pdev->dev.platform_data;
	int retval, tmp;
	int irq;
	struct usb_hcd *hcd;
	struct ehci_hcd *ehci;
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&pdev->dev,
			"Found HC with no IRQ. Check %s setup!\n",
			dev_name(&pdev->dev));
		return -ENODEV;
	}
	irq = res->start;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev,
			"Found HC with no register addr. Check %s setup!\n",
			dev_name(&pdev->dev));
		return -ENODEV;
	}
	/* get chip ID register*/
	tmp = readl(res->start + MV_USB_CHIP_ID_OFFSET);
	if ((tmp & MV_USB_CHIP_ID_MASK) != MV_USB_CHIP_USB20){
		dev_err(&pdev->dev, "chip version is not USB2.0\n");
		return -ENODEV;
	}

	hcd = usb_create_hcd(driver, &pdev->dev, dev_name(&pdev->dev));
	if (!hcd) {
		retval = -ENOMEM;
		goto err1;
	}

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);
	hcd->irq = irq;

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_err(&pdev->dev, "ioremap failed \n");
		retval = -EFAULT;
		goto err3;
	}

	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs + MV_USB_CHIP_CAP_OFFSET;
	ehci->regs = hcd->regs + MV_USB_CHIP_CAP_OFFSET +
		HC_LENGTH(ehci, readl(&ehci->caps->hc_capbase));

	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

	if (pdata && (pdata->mode == MV_USB_MODE_OTG)) {
#ifdef CONFIG_USB_OTG
		ehci->transceiver = otg_get_transceiver();
		if (!ehci->transceiver) {
			dev_err(&pdev->dev, "unable to find transceiver\n");
			retval = -ENODEV;
			goto err_get_transceiver;
		}

		retval = otg_set_host(ehci->transceiver, &hcd->self);
		if (retval < 0) {
			dev_err(&pdev->dev,
				"unable to register with transceiver\n");
			retval = -ENODEV;
			goto err_set_host;
		}
		dev_info(&pdev->dev, "%s : irq %d, otg_set_host successful\n",
			__FUNCTION__, hcd->irq);
		return retval;

#else
		dev_info(&pdev->dev,
			"MV_USB_MODE_OTG must have CONFIG_USB_OTG enabled\n");
		goto err_get_transceiver;
#endif
	} else if (pdata->mode == MV_USB_MODE_HOST){
		if (pdata && pdata->set_vbus)
			pdata->set_vbus(1);
		retval = usb_add_hcd(hcd, irq, IRQF_DISABLED | IRQF_SHARED);
		if (retval == 0) {
			dev_info(&pdev->dev, "%s : usb_add_hcd successful\n", __FUNCTION__);
			return retval;
		}
	}

	dev_err(&pdev->dev, "failed to add hcd with err %d\n",
		retval);

err_set_host:
	if (ehci->transceiver)
		otg_put_transceiver(ehci->transceiver);
err_get_transceiver:
	iounmap(hcd->regs);
err3:
	usb_put_hcd(hcd);
err1:
	dev_err(&pdev->dev, "init %s fail, %d\n",
			pdev->name, retval);

	return retval;
}

/**
 * usb_ehci_hcd_mv_remove - shutdown processing for ci13511-based HCDs
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_ehci_hcd_mv_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 *
 */
void usb_ehci_mv_remove(struct usb_hcd *hcd, struct platform_device *pdev)
{
	mv_stop_ehc(hcd);
	if (hcd->rh_registered)
		usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	usb_put_hcd(hcd);
}

static const struct hc_driver ehci_mv_hc_driver = {
	.description = hcd_name,
	.product_desc = "Marvell On-Chip EHCI Host Controller",
	.hcd_priv_size = sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq = ehci_irq,
	.flags = HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 */
	.reset = ehci_mv_setup,
	.start = ehci_mv_start,
	.stop = ehci_stop,
	.shutdown = ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue = ehci_urb_enqueue,
	.urb_dequeue = ehci_urb_dequeue,
	.endpoint_disable = ehci_endpoint_disable,
	.endpoint_reset = ehci_endpoint_reset,

	/*
	 * scheduling support
	 */
	.get_frame_number = ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data = ehci_hub_status_data,
	.hub_control = ehci_hub_control,
	.bus_suspend = ehci_bus_suspend,
	.bus_resume = ehci_bus_resume,
	.relinquish_port = ehci_relinquish_port,
	.port_handed_over = ehci_port_handed_over,

	.clear_tt_buffer_complete = ehci_clear_tt_buffer_complete,
};

static int ehci_hcd_mv_drv_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd = NULL;
	int ret;

	if (usb_disabled()){
		return -ENODEV;
	}

	ret = usb_ehci_mv_probe(&ehci_mv_hc_driver, &hcd, pdev);

	return ret;
}

static int ehci_hcd_mv_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_ehci_mv_remove(hcd, pdev);

	return 0;
}

#ifdef CONFIG_PM
static int ehci_hcd_mv_drv_suspend(struct platform_device *pdev,
				   pm_message_t message)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	unsigned long flags;
	int rc = 0;

	/* For otg, don't need to do suspend when OTG is in device state*/
	if (ehci->transceiver && !hcd->rh_registered) {
		return 0;
	}

	if (time_before(jiffies, ehci->next_statechange))
		msleep(10);

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible.  The PM and USB cores make sure that
	 * the root hub is either suspended or stopped.
	 */
	ehci_prepare_ports_for_controller_suspend(ehci, device_may_wakeup(&pdev->dev));
	spin_lock_irqsave(&ehci->lock, flags);
	ehci_writel(ehci, 0, &ehci->regs->intr_enable);
	(void)ehci_readl(ehci, &ehci->regs->intr_enable);

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	spin_unlock_irqrestore(&ehci->lock, flags);
	mv_stop_ehc(hcd);

	return rc;
}

static int ehci_hcd_mv_drv_resume(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	/* For OTG, don't need resume when OTG is in device state*/
	if (ehci->transceiver && !hcd->rh_registered) {
		return 0;
	}

	mv_start_ehc(hcd);

	if (time_before(jiffies, ehci->next_statechange))
		msleep(100);

	/* Mark hardware accessible again as we are out of D3 state by now */
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	ehci_prepare_ports_for_controller_resume(ehci);
	ehci_dbg(ehci, "lost power, restarting\n");
	usb_root_hub_lost_power(hcd->self.root_hub);

	(void) ehci_halt(ehci);
	(void) ehci_reset(ehci);

	/* emptying the schedule aborts any urbs */
	spin_lock_irq(&ehci->lock);
	if (ehci->reclaim)
		end_unlink_async(ehci);
	ehci_work(ehci);
	spin_unlock_irq(&ehci->lock);

	ehci_writel(ehci, ehci->command, &ehci->regs->command);
	ehci_writel(ehci, FLAG_CF, &ehci->regs->configured_flag);
	ehci_readl(ehci, &ehci->regs->command);	/* unblock posted writes */

	ehci_port_power(ehci, 0);
	ehci_writel(ehci, 0x07, hcd->regs + 0x090);
	ehci_writel(ehci, 0x13, hcd->regs + 0x1a8);

	hcd->state = HC_STATE_SUSPENDED;

	return 0;
}
#else
#define ehci_hcd_mv_drv_suspend NULL
#define ehci_hcd_mv_drv_resume NULL
#endif

MODULE_ALIAS("mv88de3100_ehci");

static struct platform_driver ehci_hcd_mv_driver = {
	.probe 		= ehci_hcd_mv_drv_probe,
	.remove 	= ehci_hcd_mv_drv_remove,
	.shutdown 	= usb_hcd_platform_shutdown,
	.suspend	= ehci_hcd_mv_drv_suspend,
	.resume		= ehci_hcd_mv_drv_resume,
	.driver = {
		.name = "mv88de3100_ehci",
	}
};
