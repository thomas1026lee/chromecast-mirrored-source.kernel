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
#include <linux/platform_device.h>
#include <linux/mmc/host.h>
#include <mach/gpio.h>
#include <mach/mmc_priv.h>
#include <mach/galois_platform.h>

#define SDIO_CONTROLLER_VERSION_REG_OFFSET	0x264
#define SDIO_CONTROLLER_VERSION_30		3
#define SDIO_CONTROLLER_VERSION_20		0

#define SDIO_DLL_MST_STATUS_OFFSET		0x25C
#define DELAY_CTRL4_MASK			0x1FF
#define DELAY_CTRL4_OFFSET			18

void set_sdio_controller_version(void)
{
#if defined(CONFIG_MV88DE3100_SDHC_300) && defined(CONFIG_MV88DE3100_BG2_A0)
	printk("--------------set sdio version 3.0----------------\n");
	writel(SDIO_CONTROLLER_VERSION_30,
		MEMMAP_CHIP_CTRL_REG_BASE + SDIO_CONTROLLER_VERSION_REG_OFFSET);
	if (SDIO_CONTROLLER_VERSION_30 != readl(MEMMAP_CHIP_CTRL_REG_BASE + SDIO_CONTROLLER_VERSION_REG_OFFSET))
		printk(KERN_CRIT"--------------set sdio version error----------------\n");
#endif
}

/* set the power of sd card */
void set_sdio0_voltage(int voltage)
{
#if (defined(SDIO0_SET_VOLTAGE_1V8) && defined(SDIO0_SET_VOLTAGE_3V3))
	if (MMC_SIGNAL_VOLTAGE_180 == voltage)
		SDIO0_SET_VOLTAGE_1V8();
	else
		SDIO0_SET_VOLTAGE_3V3();
#endif
}

/* set the power of sd card */
void set_sdio1_voltage(int voltage)
{
#if (defined(SDIO1_SET_VOLTAGE_1V8) && defined(SDIO1_SET_VOLTAGE_3V3))
	if (MMC_SIGNAL_VOLTAGE_180 == voltage)
		SDIO1_SET_VOLTAGE_1V8();
	else
		SDIO1_SET_VOLTAGE_3V3();
#endif
}

/* Get PAD voltage of SDIO0 */
static int get_sdio0_pad_voltage(void)
{
#ifdef CONFIG_MV88DE3100_BG2_CD
	u32 val;
	val = readl(MEMMAP_CHIP_CTRL_REG_BASE + 0x016C);

	if(val & (1<<9))
		return MMC_SIGNAL_VOLTAGE_180;
	else
		return MMC_SIGNAL_VOLTAGE_330;

#else
	/* No available pad voltage information */
	return -1;
#endif /* CONFIG_MV88DE3100_BG2_CD */
}

/*
 * Read the status using the sdioDllMstStatus register.
 * This is the delay value that needs to be programmed into
 * the delay element of MMC controller
 */
unsigned int get_tx_hold_delay(void)
{
	unsigned int value;
	value = readl(MEMMAP_CHIP_CTRL_REG_BASE + SDIO_DLL_MST_STATUS_OFFSET);
	return (value >> DELAY_CTRL4_OFFSET) & DELAY_CTRL4_MASK;
}

#ifdef CONFIG_MV88DE3100_SDHC
#ifdef CONFIG_MV88DE3100_SDHC_INTERFACE0
static struct sdhci_mv_mmc_platdata sdhc_plat_data0 = {
#ifdef SDHC_INTERFACE0_FLAG
	.flags = SDHC_INTERFACE0_FLAG,
#endif
	.set_controller_version = set_sdio_controller_version,
	.set_sdio_voltage = set_sdio0_voltage,
	.get_sdio_tx_hold_delay = get_tx_hold_delay,
	.get_sdio_pad_voltage	= get_sdio0_pad_voltage,
};
#ifdef CONFIG_MV88DE3100_SDHC_200
static struct resource mv88de3100_sdhc_resource0[] = {
	[0] = {
		.start  = MEMMAP_SDIO_REG_BASE,
		.end    = MEMMAP_SDIO_REG_BASE + MEMMAP_SDIO_REG_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_SDIO_INTERRUPT,
		.end    = IRQ_SDIO_INTERRUPT,
		.flags  = IORESOURCE_IRQ,
	},
 };
#elif defined(CONFIG_MV88DE3100_SDHC_300)
static struct resource mv88de3100_sdhc_resource0[] = {
	[0] = {
		.start  = MEMMAP_SDIO3_REG_BASE,
		.end    = MEMMAP_SDIO3_REG_BASE + 0x200 - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_SDIO_INTERRUPT,
		.end    = IRQ_SDIO_INTERRUPT,
		.flags  = IORESOURCE_IRQ,
	},
 };
#else
#error
#endif

struct platform_device mv88de3100_sdhc_dev0 = {
	.name = "mv_sdhci",
	.id = 0,
	.dev = {
		.platform_data = &sdhc_plat_data0,
	},
	.resource = mv88de3100_sdhc_resource0,
	.num_resources = ARRAY_SIZE(mv88de3100_sdhc_resource0),
};
#endif

#ifdef CONFIG_MV88DE3100_SDHC_INTERFACE1
static struct sdhci_mv_mmc_platdata sdhc_plat_data1 = {
#ifdef SDHC_INTERFACE1_FLAG
	.flags = SDHC_INTERFACE1_FLAG,
#endif
	.set_controller_version = set_sdio_controller_version,
	.set_sdio_voltage = set_sdio1_voltage,
	.get_sdio_tx_hold_delay = get_tx_hold_delay,
};
#ifdef CONFIG_MV88DE3100_SDHC_200
static struct resource mv88de3100_sdhc_resource1[] = {
	[0] = {
		.start  = MEMMAP_SDIO1_REG_BASE,
		.end    = MEMMAP_SDIO1_REG_BASE + MEMMAP_SDIO1_REG_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_SDIO1_INTERRUPT,
		.end    = IRQ_SDIO1_INTERRUPT,
		.flags  = IORESOURCE_IRQ,
	},
 };
#elif defined(CONFIG_MV88DE3100_SDHC_300)
static struct resource mv88de3100_sdhc_resource1[] = {
	[0] = {
		.start  = MEMMAP_SDIO3_REG_BASE + 0x800,
		.end    = MEMMAP_SDIO3_REG_BASE + 0x800 + 0x200 - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_SDIO1_INTERRUPT,
		.end    = IRQ_SDIO1_INTERRUPT,
		.flags  = IORESOURCE_IRQ,
	},
 };
#else
#error
#endif

struct platform_device mv88de3100_sdhc_dev1 = {
	.name = "mv_sdhci",
	.id = 1,
	.dev = {
		.platform_data = &sdhc_plat_data1,
	},
	.resource = mv88de3100_sdhc_resource1,
	.num_resources = ARRAY_SIZE(mv88de3100_sdhc_resource1),
};
#endif

#ifdef CONFIG_MV88DE3100_MMC_INTERFACE
static struct sdhci_mv_mmc_platdata mmc_plat_data = {
	.clk_delay_cycles = 0x1f,
	.flags = MV_FLAG_CARD_PERMANENT |
		 MV_FLAG_SD_8_BIT_CAPABLE_SLOT,
	.set_controller_version = set_sdio_controller_version,
	.get_sdio_tx_hold_delay = get_tx_hold_delay,
};

static struct resource mv88de3100_mmc_resource0[] = {
	[0] = {
		.start  = MEMMAP_SDIO3_REG_BASE + 0x1000,
		.end    = MEMMAP_SDIO3_REG_BASE + 0x1000 + MEMMAP_SDIO3_REG_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_MMC_INTERRUPT,
		.end    = IRQ_MMC_INTERRUPT,
		.flags  = IORESOURCE_IRQ,
	},
 };

struct platform_device mv88de3100_mmc_dev = {
	.name = "mv_sdhci",
	.id = -1,
	.dev = {
		.platform_data = &mmc_plat_data,
	},
	.resource = mv88de3100_mmc_resource0,
	.num_resources = ARRAY_SIZE(mv88de3100_mmc_resource0),
};
#endif
#endif
