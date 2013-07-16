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
#include <linux/init.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/ahci_platform.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <asm/pmu.h>
#include <mach/galois_platform.h>
#include <mach/irqs.h>
#include <mach/mmc_priv.h>
#include <plat/pxa3xx_nand_debu.h>
#include <plat/usb.h>

static u64 mv88de3100_dma_mask = 0xffffffffULL;
#define MEMMAP_USB1_REG_CAP_BASE		 (MEMMAP_USB1_REG_BASE + 0x100)

/*
 * PMU
 */
static struct resource pmu_resource[] = {
	[0] = {
		.start	= IRQ_PMU_CPU0,
		.end	= IRQ_PMU_CPU0,
		.flags	= IORESOURCE_IRQ,
	},
	[1] = {
		.start	= IRQ_PMU_CPU1,
		.end	= IRQ_PMU_CPU1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device pmu_dev = {
	.name		= "arm-pmu",
	.id		= ARM_PMU_DEVICE_CPU,
	.num_resources	= ARRAY_SIZE(pmu_resource),
	.resource	= pmu_resource,
};

/*
 * UART device
 */
#define APB_UART_USR	0x7c
static struct plat_serial8250_port platform_serial_ports[] = {
	{
		.membase = (void *)(APB_UART_INST0_BASE),
		.mapbase = (unsigned long)(APB_UART_INST0_BASE),
		.irq = IRQ_SM_UART0,
		.uartclk = GALOIS_UART_TCLK,
		.regshift = 2,
		.iotype = UPIO_DWAPB,
		.flags = UPF_FIXED_TYPE | UPF_SKIP_TEST,
		.type = PORT_16550A,
		/* read USR to clear busy int */
		.private_data = (void *)(APB_UART_INST0_BASE + APB_UART_USR),
	},
#ifdef CONFIG_ARCH_MV88DE3100_BG2
	{
		.membase = (void *)(APB_UART_INST1_BASE),
		.mapbase = (unsigned long)(APB_UART_INST1_BASE),
		.irq = IRQ_SM_UART1,
		.uartclk = GALOIS_UART_TCLK,
		.regshift = 2,
		.iotype = UPIO_DWAPB,
		.flags = UPF_FIXED_TYPE | UPF_SKIP_TEST,
		.type = PORT_16550A,

		/* read USR to clear busy int */
		.private_data = (void *)(APB_UART_INST1_BASE + APB_UART_USR),
	},
#endif
	{}
};

static struct platform_device mv88de3100_serial_dev = {
	.name = "serial8250",
	.id = PLAT8250_DEV_PLATFORM,
	.dev = {
		.platform_data = &platform_serial_ports,
	},
};

#ifdef CONFIG_MV88DE3100_ETHERNET
/*
 * Ethernet device
 */
static struct resource mv88de3100_eth_resource[] = {
	[0] = {
		.start	= MEMMAP_ETHERNET1_REG_BASE,
		.end	= MEMMAP_ETHERNET1_REG_BASE + MEMMAP_ETHERNET1_REG_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_ETH1IRQ,
		.end	= IRQ_ETH1IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device mv88de3100_eth_dev = {
	.name = "mv88de3100_eth1",
	.id	= -1,
	.num_resources = ARRAY_SIZE(mv88de3100_eth_resource),
	.resource = mv88de3100_eth_resource,
	.dev = {
		.dma_mask = &mv88de3100_dma_mask,
		.coherent_dma_mask = 0xffffffff,
	},
};
#endif


#if defined(CONFIG_MV88DE3100_PXA3XX_NFC) || defined(CONFIG_MV88DE3100_PXA3XX_NFC_MODULE)
static struct resource mv_nand_resources[] = {
	[0] = {
		.start	= MEMMAP_NAND_FLASH_REG_BASE,
		.end	= MEMMAP_NAND_FLASH_REG_BASE + MEMMAP_NAND_FLASH_REG_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_INTRPB,
		.end	= IRQ_INTRPB,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct pxa3xx_nand_platform_data mv88de3100_nand_platform_data = {
	.controller_attrs = PXA3XX_ARBI_EN | PXA3XX_NAKED_CMD_EN
		| PXA3XX_DMA_EN,
	.cs_num		= 1,
	.parts[0]	= NULL,
	.nr_parts[0]	= 0
};

static struct platform_device pxa3xx_nand_dev = {
	.name		= "mv_nand",
	.id		= -1,
	.dev	= {
		.platform_data = &mv88de3100_nand_platform_data,
		.dma_mask = &mv88de3100_dma_mask,
		.coherent_dma_mask = 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(mv_nand_resources),
	.resource	= mv_nand_resources,
};
#endif

#ifdef CONFIG_MV88DE3100_USB_EHCI_HCD
static char *mv88de3100_usb_clock_name[] = {
	[0] = "INVALID",
};

static struct mv_usb_platform_data mv88de3100_usb1_pdata = {
	.clknum		= 0,
	.clkname	= mv88de3100_usb_clock_name,
	.id			= NULL,
	.vbus		= NULL,
#ifdef CONFIG_USB_OTG
	.mode		= MV_USB_MODE_OTG,
#else
	.mode		= MV_USB_MODE_HOST,
#endif
	.phy_init	= mv_udc_phy_init,
	.set_vbus	= set_usb1_vbus,
	.otg_force_a_bus_req = 1,
};

static struct mv_usb_platform_data mv88de3100_usb0_pdata = {
	.clknum		= 0,
	.clkname	= mv88de3100_usb_clock_name,
	.id			= NULL,
	.vbus		= NULL,
	.mode		= MV_USB_MODE_HOST,
	.phy_init	= mv_udc_phy_init,
	.set_vbus	= NULL,
	.otg_force_a_bus_req = 1,
};


/*
 * USB Host Controller device
 */
static struct resource mv88de3100_usb0_resource[] = {
	[0] = {
		.start	= MEMMAP_USB0_REG_BASE,
		.end	= MEMMAP_USB0_REG_BASE + MEMMAP_USB0_REG_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_USB0INTERRUPT,
		.end	= IRQ_USB0INTERRUPT,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device mv88de3100_usb0_dev = {
	.name = "mv88de3100_ehci",
	.id	= 0,
	.num_resources = ARRAY_SIZE(mv88de3100_usb0_resource),
	.resource = mv88de3100_usb0_resource,
	.dev = {
		.dma_mask = &mv88de3100_dma_mask,
		.coherent_dma_mask = 0xffffffff,
		.platform_data		= (void *)&mv88de3100_usb0_pdata,
	},
};

static struct resource mv88de3100_usb1_resource[] = {
	[0] = {
		.start	= MEMMAP_USB1_REG_BASE,
		.end	= MEMMAP_USB1_REG_BASE + MEMMAP_USB1_REG_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_USB1INTERRUPT,
		.end	= IRQ_USB1INTERRUPT,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device mv88de3100_usb1_dev = {
	.name = "mv88de3100_ehci",
	.id	= 1,
	.num_resources = ARRAY_SIZE(mv88de3100_usb1_resource),
	.resource = mv88de3100_usb1_resource,
	.dev = {
		.dma_mask = &mv88de3100_dma_mask,
		.coherent_dma_mask = 0xffffffff,
		.platform_data		= (void *)&mv88de3100_usb1_pdata,
	},
};
#endif /* CONFIG_MV88DE3100_USB_EHCI_HCD */

#ifdef CONFIG_USB_PXA_U2O
/********************************************************************
 * The registers read/write routines
 ********************************************************************/

static struct resource mv88de3100_u2o_resources[] = {
	[0] = {
		.start	= MEMMAP_USB1_REG_BASE,
		.end	= MEMMAP_USB1_REG_BASE + 0xff,
		.flags	= IORESOURCE_MEM,
		.name	= "baseregs",
	},
	[1] = {
		.start	= MEMMAP_USB1_REG_CAP_BASE,
		.end	= MEMMAP_USB1_REG_CAP_BASE + USB_REG_RANGE,
		.flags	= IORESOURCE_MEM,
		.name	= "capregs",
	},
	[2] = {
		.start	= USB_PHY_REG_BASE_1,
		.end	= USB_PHY_REG_BASE_1 + USB_PHY_RANGE,
		.flags	= IORESOURCE_MEM,
		.name	= "phyregs",
	},
	[3] = {
		.start	= IRQ_USB1INTERRUPT,
		.end	= IRQ_USB1INTERRUPT,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device mv88de3100_device_u2o = {
	.name		= "mv88de3100-udc",
	.id		= -1,
	.dev		= {
		.dma_mask	= &mv88de3100_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= (void *)&mv88de3100_usb1_pdata,
	},
	.num_resources	= ARRAY_SIZE(mv88de3100_u2o_resources),
	.resource	= mv88de3100_u2o_resources,
};

#ifdef CONFIG_USB_PXA_U2O_OTG
static struct resource mv88de3100_u2ootg_resources[] = {
	[0] = {
		.start	= MEMMAP_USB1_REG_CAP_BASE,
		.end	= MEMMAP_USB1_REG_CAP_BASE + USB_REG_RANGE,
		.flags	= IORESOURCE_MEM,
		.name	= "capregs",
	},
	[1] = {
		.start	= USB_PHY_REG_BASE_1,
		.end	= USB_PHY_REG_BASE_1 + USB_PHY_RANGE,
		.flags	= IORESOURCE_MEM,
		.name	= "phyregs",
	},
	[2] = {
		.start	= IRQ_USB1INTERRUPT,
		.end	= IRQ_USB1INTERRUPT,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device mv88de3100_device_u2ootg = {
	.name		= "mv88de3100-otg",
	.id		= -1,
	.dev		= {
		.dma_mask	= &mv88de3100_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= (void *)&mv88de3100_usb1_pdata,
	},
	.num_resources	= ARRAY_SIZE(mv88de3100_u2ootg_resources),
	.resource	= mv88de3100_u2ootg_resources,
};
#endif

#endif


/*
 * SATA device
 */
#define PORT_VSR_ADDR		0x78	/* port Vendor Specific Register Address */
#define PORT_VSR_DATA		0x7c	/* port Vendor Specific Register Data */
#define PORT_SCR_CTL		0x2c	/* SATA phy register: SControl */
#define MAX_PORTS		2
#define MAX_MAP			((1 << MAX_PORTS) - 1)

static inline void disable_sata_port(int port)
{
	void *base = __io(MEMMAP_SATA_REG_BASE + 0x100 + (port * 0x80));
	__raw_writel(0x80 + 0x26, base + PORT_VSR_ADDR);
	__raw_writel(0x8122, base + PORT_VSR_DATA);
	__raw_writel(0x80 + 0x1, base + PORT_VSR_ADDR);
	__raw_writel(0x8901, base + PORT_VSR_DATA);
}

#ifdef CONFIG_SATA_AHCI_PLATFORM
static struct resource mv88de3100_sata_resource[] = {
	[0] = {
		.start	= MEMMAP_SATA_REG_BASE,
		.end	= MEMMAP_SATA_REG_BASE + MEMMAP_SATA_REG_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_SATAINTR,
		.end	= IRQ_SATAINTR,
		.flags	= IORESOURCE_IRQ,
	},
};

static void port_init(struct device *dev, void __iomem *addr)
{
	u32 tmp;

	/* improve TX hold time for Gen2 */
	writel(0x80 + 0x5C, addr + PORT_VSR_ADDR);
	writel(0x0D97, addr + PORT_VSR_DATA);
	writel(0x80 + 0x4E, addr + PORT_VSR_ADDR);
	writel(0x5155, addr + PORT_VSR_DATA);
	/* Change clock to 25Mhz (It's 40Mhz by default) */
	writel(0x80 + 0x1, addr + PORT_VSR_ADDR);
	tmp = readl(addr + PORT_VSR_DATA);
	tmp &= 0xFFE0;
	tmp |= 0x01;
	writel(tmp, addr + PORT_VSR_DATA);

	/* After a clock change, we should calibrate */
	writel(0x80 + 0x2, addr + PORT_VSR_ADDR);
	tmp = readl(addr + PORT_VSR_DATA);
	tmp |= 0x8000;
	writel(tmp, addr + PORT_VSR_DATA);

	/* Wait for calibration to Complete */
	while (1) {
		writel(0x80 + 0x2, addr + PORT_VSR_ADDR);
		tmp = readl(addr + PORT_VSR_DATA);
		if (tmp & 0x4000)
			break;
	}

	/* Setup SATA PHY */

	/* Set SATA PHY to be 40-bit data bus mode */
	writel(0x80 + 0x23, addr + PORT_VSR_ADDR);
	tmp = readl(addr + PORT_VSR_DATA);
	tmp &= ~0xC00;
	tmp |= 0x800;
	writel(tmp, addr + PORT_VSR_DATA);

	/* Set SATA PHY PLL to use maxiumum PLL rate to TX and RX */
	writel(0x80 + 0x2, addr + PORT_VSR_ADDR);
	tmp = readl(addr + PORT_VSR_DATA);
	tmp |= 0x100;
	writel(tmp, addr + PORT_VSR_DATA);

	/* Set SATA PHY to support GEN1/GEN2/GEN3 */
	writel(0x00000030, addr + PORT_SCR_CTL);
	mdelay(10);
	writel(0x00000031, addr + PORT_SCR_CTL);
	mdelay(10);
	writel(0x00000030, addr + PORT_SCR_CTL);
	mdelay(10);

	/* improve SATA signal integrity */
	writel(0x80 + 0x0D, addr + PORT_VSR_ADDR);
	writel(0xC926, addr + PORT_VSR_DATA);
	writel(0x80 + 0x0F, addr + PORT_VSR_ADDR);
	writel(0x9A4E, addr + PORT_VSR_DATA);
}

static int mv_ahci_init(struct device *dev, void __iomem *base)
{
	int i;
	unsigned int port_map;
	struct ahci_platform_data *pdata = dev->platform_data;

	port_map = pdata->force_port_map ? pdata->force_port_map : MAX_MAP;
	for (i = 0; i < MAX_PORTS; i++)
		if (port_map & (1 << i))
			port_init(dev, base + 0x100 + (i * 0x80));

	return 0;
}

static struct ahci_platform_data mv_ahci_pdata = {
	.init		= mv_ahci_init,
	.force_port_map	= 1,
};

static struct platform_device mv_ahci_dev = {
	.name		= "ahci",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(mv88de3100_sata_resource),
	.resource	= mv88de3100_sata_resource,
	.dev		= {
		.platform_data		= &mv_ahci_pdata,
		.dma_mask		= &mv88de3100_dma_mask,
		.coherent_dma_mask	= 0xffffffff,
	},
};
#endif

#ifdef CONFIG_MV88DE3100_LINUX_I2C
static struct platform_device mv_i2c0_dev = {
	.name		= "mv88de3100-i2c",
	.id		= 0,
};

static struct platform_device mv_i2c1_dev = {
	.name		= "mv88de3100-i2c",
	.id		= 1,
};

static struct platform_device mv_i2c2_dev = {
	.name		= "mv88de3100-i2c",
	.id		= 2,
};

static struct platform_device mv_i2c3_dev = {
	.name		= "mv88de3100-i2c",
	.id		= 3,
};
#endif

#ifdef CONFIG_MV88DE3100_WDT
static struct resource mv88de3100_wdt0_resource[] = {
	[0] = {
		.start	= APB_WDT_INST0_BASE,
		.end	= APB_WDT_INST0_BASE + 0x100 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_APB_WDTINST0,
		.end	= IRQ_APB_WDTINST0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device mv88de3100_wdt0_dev = {
	.name		= "mv88de3100_wdt",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(mv88de3100_wdt0_resource),
	.resource	= mv88de3100_wdt0_resource,
};
#endif

static struct platform_device *mv88de3100_platform_devs[] __initdata = {
	&pmu_dev,
	&mv88de3100_serial_dev,
#ifdef CONFIG_SATA_AHCI_PLATFORM
	&mv_ahci_dev,
#endif
#ifdef CONFIG_MV88DE3100_ETHERNET
	&mv88de3100_eth_dev,
#endif
#if defined(CONFIG_MV88DE3100_PXA3XX_NFC) || defined(CONFIG_MV88DE3100_PXA3XX_NFC_MODULE)
	&pxa3xx_nand_dev,
#endif
#ifdef CONFIG_MV88DE3100_SDHC
#ifndef CONFIG_MV88DE3100_PM_DISABLE_SDIO
#ifdef CONFIG_MV88DE3100_MMC_INTERFACE
	&mv88de3100_mmc_dev,
#endif
#ifdef CONFIG_MV88DE3100_SDHC_INTERFACE0
	&mv88de3100_sdhc_dev0,
#endif
#ifdef CONFIG_MV88DE3100_SDHC_INTERFACE1
	&mv88de3100_sdhc_dev1,
#endif
#endif
#endif
#ifdef CONFIG_MV88DE3100_USB_EHCI_HCD
#ifdef CONFIG_MV88DE3100_EHCI_USB0
	&mv88de3100_usb0_dev,
#endif
#ifndef CONFIG_MV88DE3100_PM_DISABLE_USB1
#ifdef CONFIG_USB_PXA_U2O_OTG
	&mv88de3100_device_u2ootg,
	&mv88de3100_device_u2o,
#endif
	&mv88de3100_usb1_dev,
#endif
#endif
#ifdef CONFIG_MV88DE3100_LINUX_I2C
	&mv_i2c0_dev,
	&mv_i2c1_dev,
	&mv_i2c2_dev,
	&mv_i2c3_dev,
#endif
#ifdef CONFIG_MV88DE3100_WDT
	&mv88de3100_wdt0_dev,
#endif
};

void __init mv88de3100_devs_init(void)
{
	platform_add_devices(mv88de3100_platform_devs, ARRAY_SIZE(mv88de3100_platform_devs));
}

#ifdef CONFIG_MV88DE3100_BG2_A0
int mv88de3100_close_phys_cores(void)
{
	u32 val;

#ifdef CONFIG_MV88DE3100_PM_DISABLE_2ND_SATA
	/* we never use sata 2nd port*/
	disable_sata_port(1);
#endif /* CONFIG_MV88DE3100_PM_DISABLE_2ND_SATA */

#ifdef CONFIG_MV88DE3100_PM_DISABLE_ETH
	/* we never use eth0 */
	val = __raw_readl(0xF7EA0150);
	val &= ~(1<<7);
	__raw_writel(val, 0xF7EA0150);
#endif /* CONFIG_MV88DE3100_PM_DISABLE_ETH */

#ifndef CONFIG_SATA_AHCI_PLATFORM
#ifdef CONFIG_MV88DE3100_PM_DISABLE_1ST_SATA
	disable_sata_port(0);
	/* if we don't use 1st sata port we can turn off sata clock */
	val = __raw_readl(0xF7EA0150);
	val &= ~(1<<9);
	__raw_writel(val, 0xF7EA0150);
#endif /* CONFIG_MV88DE3100_PM_DISABLE_1ST_SATA */
#endif

#ifdef CONFIG_MV88DE3100_PM_DISABLE_SDIO
	*((unsigned int *)0xF7EA0150) = *((unsigned int *)0xF7EA0150) & ~(1<<14);
#endif
#ifdef CONFIG_MV88DE3100_PM_DISABLE_USB1
	*((unsigned int *)0xF7EE0800) = 0;	/* turn off PHY layer */
	*((unsigned int *)0xF7EA0150) = *((unsigned int *)0xF7EA0150) & ~(1<<12);
#endif
#ifdef CONFIG_MV88DE3100_PM_DISABLE_SPI_TWSI
	*((unsigned int *)0xF7EA0150) = *((unsigned int *)0xF7EA0150) & ~(1<<10);
#endif
	return 0;
}

late_initcall(mv88de3100_close_phys_cores);
#endif

#ifdef CONFIG_MV88DE3100_BG2_CD

#define	RA_Gbl_clkEnable			0x0150

int mv88de3100_close_phys_cores(void)
{
	u32 val;

	/* Turn off USB0/1 and ETH1 clock if they are not used */
	val = __raw_readl(MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_clkEnable);

#if !defined(CONFIG_MV88DE3100_USB_EHCI_HCD)
	val &= ~(1 << 11);	/* usb0CoreClkEn */
	val &= ~(1 << 12);	/* usb1CoreClkEn */
#endif /* !defined(CONFIG_MV88DE3100_USB_EHCI_HCD) */

#if !defined(CONFIG_MV88DE3100_ETHERNET)
	val &= ~(1 << 8);	/* geth1CoreClkEn */
#endif /* !defined(CONFIG_MV88DE3100_ETHERNET) */

	__raw_writel(val, MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_clkEnable);

#if !defined(CONFIG_MV88DE3100_USB_EHCI_HCD)
	/* Power down USB1 PHY */
	__raw_writel(0x1680, (USB_PHY_REG_BASE_1 + USB_PHY_ANALOG_REG));
#endif /* !defined(CONFIG_MV88DE3100_USB_EHCI_HCD) */

#if !defined(CONFIG_MV88DE3100_ETHERNET)
	/* Power down ETH1 PHY */
	val = __raw_readl(0xF7FE1404);
	val |= (1 << 0);
	__raw_writel(val, 0xF7FE1404);
#endif /* !defined(CONFIG_MV88DE3100_ETHERNET) */

	return 0;
}

late_initcall(mv88de3100_close_phys_cores);
#endif /* CONFIG_MV88DE3100_BG2_CD */
