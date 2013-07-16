/*
 * linux/drivers/mmc/host/sdhci-mv.c
 *
 * Authors: Hz Chen
 * Copyright (C) 2011 Marvell Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/bitops.h>
#include <linux/leds.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/mmc/host.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <mach/galois_generic.h>
#include <mach/mmc_priv.h>
#include <mach/gpio.h>
#include "sdhci.h"

#define SD_CLOCK_BURST_SIZE_SETUP	0x10A
#define SDCLK_SEL			0x100
#define SDCLK_DELAY_SHIFT		9
#define SDCLK_DELAY_MASK		0x1f

#define SD_CFG_FIFO_PARAM		0x100
#define SDCFG_GEN_PAD_CLK_ON		(1<<6)
#define SDCFG_GEN_PAD_CLK_CNT_MASK	0xFF
#define SDCFG_GEN_PAD_CLK_CNT_SHIFT	24

#define TX_CFG_REG			0x118
#define TX_HOLD_DELAY0_DEF_VALUE	0xC0
#define TX_HOLD_DELAY0_MASK		0x1FF

#define SD_SPI_MODE			0x108
#define SD_CE_ATA_1			0x10C

#define SD_CE_ATA_2			0x10E
#define SDCE_MISC_INT			(1<<2)
#define SDCE_MISC_INT_EN		(1<<1)

#define SDIO_SOC_FREQ_SDIOXIN  20
#define SDIO_SOC_FREQ_SDIO1XIN 21

#define INTERFACE_INVALID	-1
#define INTERFACE_SDIO0		0
#define INTERFACE_SDIO1		1
#define INTERFACE_MMC		2

extern unsigned int GaloisGetFrequency(int FreqID);
static bool controller_version_initialized;

DEFINE_LED_TRIGGER(mv_sdhci_led_trigger);

#define MAX_SLOTS			1
struct sdhci_mv_chip;
struct sdhci_mv_slot;

struct sdhci_mv_fixes {
	unsigned int		quirks;
	int			(*probe)(struct sdhci_mv_chip*);
	int			(*probe_slot)(struct sdhci_mv_slot*);
	void			(*remove_slot)(struct sdhci_mv_slot*, int);
	int			(*suspend)(struct sdhci_mv_chip*,
					pm_message_t);
	int			(*resume)(struct sdhci_mv_chip*);
};

struct sdhci_mv_slot {
	struct sdhci_mv_chip	*chip;
	struct sdhci_host	*host;
};

struct sdhci_mv_chip {
	unsigned int		quirks;
	const struct sdhci_mv_fixes *fixes;
	int			num_slots;	/* Slots on controller */
	struct sdhci_mv_slot	*slots[MAX_SLOTS]; /* Pointers to host slots */
	void			(*set_sdio_controller_version)(void);
	void			(*set_sdio_voltage)(int voltage);
	unsigned int		(*get_sdio_tx_hold_delay)(void);
	int			(*get_sdio_pad_voltage)(void);
};

static void sdhci_mv_remove_slot(struct sdhci_mv_slot *slot)
{
	int dead;
	u32 scratch;
	dead = 0;
	scratch = readl(slot->host->ioaddr + SDHCI_INT_STATUS);
	if (scratch == (u32)-1)
		dead = 1;
	sdhci_remove_host(slot->host, dead);
	if (slot->chip->fixes && slot->chip->fixes->remove_slot)
		slot->chip->fixes->remove_slot(slot, dead);
	sdhci_free_host(slot->host);
}

static int sdhci_mvfix_probe(struct sdhci_mv_chip *chip)
{
	return 0;
}

static void set_tx_delay(struct sdhci_host *host)
{
	struct sdhci_mv_slot *slot;
	unsigned int value, reg;

	slot = sdhci_priv(host);
	if (slot->chip->get_sdio_tx_hold_delay)
		value = slot->chip->get_sdio_tx_hold_delay();
	else
		value = TX_HOLD_DELAY0_DEF_VALUE;
	value &= TX_HOLD_DELAY0_MASK;
	reg = sdhci_readl(host, TX_CFG_REG);
	reg &= ~TX_HOLD_DELAY0_MASK;
	reg |= value;
	sdhci_writel(host, reg, TX_CFG_REG);
}

static const struct sdhci_mv_fixes sdhci0_fixes = {
	.probe		= sdhci_mvfix_probe,
	.quirks     = SDHCI_QUIRK_BROKEN_ADMA |
		          SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN |
		          SDHCI_QUIRK_NO_HISPD_BIT |
#ifdef CONFIG_MV88DE3100_SDHC_300
		          SDHCI_QUIRK_INVERTED_WRITE_PROTECT |
#endif
				  SDHCI_QUIRK_BROKEN_TIMEOUT_VAL,
};

static const struct sdhci_mv_fixes sdhci1_fixes = {
	.probe		= sdhci_mvfix_probe,
	.quirks     = SDHCI_QUIRK_BROKEN_ADMA |
		          SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN |
#ifdef CONFIG_MV88DE3100_SDHC_300
		          SDHCI_QUIRK_INVERTED_WRITE_PROTECT |
#endif
				  SDHCI_QUIRK_BROKEN_TIMEOUT_VAL,
};

static unsigned int sdhci_mv_get_max_clock(struct sdhci_host *host)
{
	/* Host controller reports the wrong base clock frequency,
	 * get the correct frequency
	 */
	struct clk *sdio_clk;

	if (host->irq == IRQ_SDIO_INTERRUPT)
		sdio_clk = clk_get(NULL, "sdioxin");
	else
		sdio_clk = clk_get(NULL, "sdio1xin");
	BUG_ON(IS_ERR(sdio_clk));

	return clk_get_rate(sdio_clk);
}

static void sdhci_mv_switch_voltage(struct sdhci_host *host, int signal_voltage)
{
	struct sdhci_mv_slot *slot;

	if (host->version < SDHCI_SPEC_300)
		return;

	slot = sdhci_priv(host);
	if (slot->chip->set_sdio_voltage) {
		slot->chip->set_sdio_voltage(signal_voltage);
		/* Wait for 1ms */
		usleep_range(1000, 1500);
	}
}

static void sdhci_mv_plat_reset_exit(struct sdhci_host *host, u8 mask)
{
	struct sdhci_mv_slot *slot;

	slot = sdhci_priv(host);
	if(slot->chip->get_sdio_pad_voltage) {
		int voltage;
		u16 ctrl;

		/* Retrieve PAD voltage */
		voltage = slot->chip->get_sdio_pad_voltage();

		/* Set SDHCI Host Control2 register if necessary */
		ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
		switch(voltage) {
		case MMC_SIGNAL_VOLTAGE_330:
			ctrl &= ~SDHCI_CTRL_VDD_180;
			break;
		case MMC_SIGNAL_VOLTAGE_180:
			ctrl |= SDHCI_CTRL_VDD_180;
			break;
		default:
			break;
		}
		sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);
	}
}

static struct sdhci_ops sdhci_mv_ops = {
	.get_max_clock	= sdhci_mv_get_max_clock,
	.switch_voltage = sdhci_mv_switch_voltage,
	.platform_reset_exit	= sdhci_mv_plat_reset_exit,
};

static struct sdhci_mv_slot * __init sdhci_mv_probe_slot(
	struct platform_device *pdev, struct sdhci_mv_chip *chip, int bar)
{
	struct sdhci_mv_slot *slot;
	struct sdhci_host *host;
	struct resource *res;
	struct sdhci_mv_mmc_platdata *pdata = pdev->dev.platform_data;
	int ret = 0;

	res = platform_get_resource(pdev, IORESOURCE_IRQ,0);
	if (!res) {
		printk("There's no irq resource for the SDHCI platform device.\n");
		return ERR_PTR(-ENODEV);
	}

	host = sdhci_alloc_host(&pdev->dev, sizeof(struct sdhci_mv_slot));
	if (IS_ERR(host)) {
		return (void *)host;
	}

	slot = sdhci_priv(host);
	slot->chip = chip;
	slot->host = host;

	host->hw_name = "MVSD";
	host->ops = &sdhci_mv_ops;
	host->quirks = chip->quirks;
	host->irq = res->start;

	if (pdata) {
		if (pdata->flags & MV_FLAG_CARD_PERMANENT) {
			host->quirks |= SDHCI_QUIRK_BROKEN_CARD_DETECTION;
			host->mmc->caps |= MMC_CAP_NONREMOVABLE;
		}
		if (pdata->flags & MV_FLAG_HOST_OFF_CARD_ON)
			host->quirks2 = SDHCI_QUIRK2_HOST_OFF_CARD_ON;

		if (pdata->flags & MV_FLAG_SD_8_BIT_CAPABLE_SLOT)
			host->mmc->caps |= MMC_CAP_8_BIT_DATA;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		printk("There's no mem resource for the SDHCI platform device.\n");
		goto release;
	}

	host->ioaddr = (void __iomem *)res->start;

	if (chip->fixes && chip->fixes->probe_slot) {
		ret = chip->fixes->probe_slot(slot);
		if (ret)
			goto release;
	}

	host->mmc->pm_caps = MMC_PM_KEEP_POWER | MMC_PM_WAKE_SDIO_IRQ;

	ret = sdhci_add_host(host);
	if (ret)
		goto remove;
	set_tx_delay(host);

	return slot;

remove:
	if (chip->fixes && chip->fixes->remove_slot)
		chip->fixes->remove_slot(slot, 0);

release:
	sdhci_free_host(host);
	return ERR_PTR(ret);
}

static int sdhci_mv_get_interface(struct platform_device *pdev)
{
	struct resource *res;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_IRQ,0);
	if (!res) {
		printk("There's no irq resource for the SDHCI platform device.\n");
		return INTERFACE_INVALID;
	}

	if ((res->start != IRQ_SDIO_INTERRUPT) && (res->start != IRQ_SDIO1_INTERRUPT) && (res->start != IRQ_MMC_INTERRUPT)) {
		printk("There's no correct irq resource for the SDHCI platform device.\n");
		return INTERFACE_INVALID;
	}

	switch (res->start) {
	case IRQ_SDIO_INTERRUPT:
		ret = INTERFACE_SDIO0;
		break;
	case IRQ_SDIO1_INTERRUPT:
		ret = INTERFACE_SDIO1;
		break;
	case IRQ_MMC_INTERRUPT:
		ret = INTERFACE_MMC;
		break;
	default:
		ret = INTERFACE_INVALID;
		break;
	}

	return ret;
}

static void sdhci_mv_set_pinmux(struct platform_device *pdev)
{
	unsigned int regval1;

	switch (sdhci_mv_get_interface(pdev)) {
	case INTERFACE_SDIO0:
		/* Check whether it is B0 or above board revision */
		regval1 = *(volatile unsigned int *)0xF7EA01D0;
		if (regval1 >= 0xB0)  {
		    GPIO_PortSetInOut(1, 0);
		    GPIO_PortWrite(1, 1);
		} else {
		    GPIO_PortSetInOut(6, 0);
		    GPIO_PortWrite(6, 1);
		}
		break;
	case INTERFACE_SDIO1:
		GPIO_PortSetInOut(7, 0);
		GPIO_PortWrite(7, 1);
		break;
	default:
		break;
	}
}

static int sdhci_mv_probe(struct platform_device *pdev)
{
	struct sdhci_mv_mmc_platdata *pdata = pdev->dev.platform_data;
	struct sdhci_mv_chip *chip;
	struct sdhci_mv_slot *slot;
	u8 slots = 1,first_bar = 0;
	int ret = 0, i;

	sdhci_mv_set_pinmux(pdev);

	chip = kzalloc(sizeof(struct sdhci_mv_chip), GFP_KERNEL);
	if (!chip) {
		ret = -ENOMEM;
		goto err;
	}
	if (pdata) {
		chip->set_sdio_controller_version = pdata->set_controller_version;
		chip->get_sdio_tx_hold_delay = pdata->get_sdio_tx_hold_delay;
		chip->get_sdio_pad_voltage = pdata->get_sdio_pad_voltage;
	}
	if (!controller_version_initialized)
		if (chip->set_sdio_controller_version) {
			chip->set_sdio_controller_version();
			controller_version_initialized = true;
		}

	chip->fixes = (sdhci_mv_get_interface(pdev) == INTERFACE_SDIO0)? &sdhci0_fixes : &sdhci1_fixes;
	if (chip->fixes) {
		chip->quirks = chip->fixes->quirks;
	}

	platform_set_drvdata(pdev, chip);
	if (chip->fixes && chip->fixes->probe) {
		ret = chip->fixes->probe(chip);
		if (ret)
			goto free;
	}

	for (i = 0;i < slots;i++) {
		slot = sdhci_mv_probe_slot(pdev, chip, first_bar + i);
		if (IS_ERR(slot)) {
			for (i--;i >= 0;i--)
				sdhci_mv_remove_slot(chip->slots[i]);
			ret = PTR_ERR(slot);
			goto free;
		}

		chip->slots[i] = slot;
	}
	chip->num_slots = slots;
	if (pdata)
		chip->set_sdio_voltage = pdata->set_sdio_voltage;
	return 0;

free:
	platform_set_drvdata(pdev, NULL);
	kfree(chip);
err:
	return ret;
}

static int __devexit sdhci_mv_remove(struct platform_device *pdev)
{
	int i;
	struct sdhci_mv_chip *chip;
	chip = platform_get_drvdata(pdev);
	if (chip) {
		for (i = 0;i < chip->num_slots; i++)
			sdhci_mv_remove_slot(chip->slots[i]);
		platform_set_drvdata(pdev, NULL);
		kfree(chip);
	}
	return 0;
}

#ifdef CONFIG_PM

static int __devexit sdhci_mv_suspend( struct platform_device *pdev, pm_message_t state)
{
	struct sdhci_mv_chip *chip;
	struct sdhci_mv_slot *slot;
	int i,ret;
	controller_version_initialized = false;
	chip = platform_get_drvdata(pdev);

	if (chip) {
		for (i = 0;i < chip->num_slots; i++) {
			slot = chip->slots[i];
			if (!slot)
				continue;
			ret = sdhci_suspend_host(slot->host, state);
			if (ret) {
				for (i--;i >= 0;i--)
					sdhci_resume_host(chip->slots[i]->host);
				return ret;
			}
		}
	}

	return 0;
}

static int __devexit sdhci_mv_resume( struct platform_device *pdev)
{
	struct sdhci_mv_chip *chip;
	struct sdhci_mv_slot *slot;
	int ret, i;

	chip = platform_get_drvdata(pdev);

	if (!chip)
		return 0;

	if (!controller_version_initialized)
		if (chip->set_sdio_controller_version) {
			chip->set_sdio_controller_version();
			controller_version_initialized = true;
		}

	for (i = 0;i < chip->num_slots;i++) {
		slot = chip->slots[i];
		if (!slot)
			continue;

		ret = sdhci_resume_host(slot->host);
		if (ret)
			return ret;
		set_tx_delay(slot->host);
	}

	return 0;
}

#else

#define sdhci_mv_suspend NULL
#define sdhci_mv_resume NULL

#endif

static struct platform_driver mv_sdhci_driver = {
	.probe		= sdhci_mv_probe,
	.remove		= __devexit_p(sdhci_mv_remove),
	.driver		= {
	    .name	= "mv_sdhci",
	    .owner	= THIS_MODULE,
	},

	.suspend        = sdhci_mv_suspend,
	.resume	        = sdhci_mv_resume,
};


static int __init mv_sdhci_init(void)
{
	led_trigger_register_simple("mv-sdhci", &mv_sdhci_led_trigger);
	return platform_driver_register(&mv_sdhci_driver);
}


static void __exit mv_sdhci_exit(void)
{
	led_trigger_unregister_simple(mv_sdhci_led_trigger);
	platform_driver_unregister(&mv_sdhci_driver);
}

module_init(mv_sdhci_init);
module_exit(mv_sdhci_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hz Chen");
MODULE_DESCRIPTION("Marvell MV88DE3100 MMC,SD,SDIO Host Controller driver");
