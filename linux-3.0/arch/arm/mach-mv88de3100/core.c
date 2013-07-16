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
#include <asm/page.h>
#include <asm/memory.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <mach/galois_platform.h>
#include <asm/mach/time.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include "common.h"
#include "clock.h"

/*
 * the virtual address mapped should be < PAGE_OFFSET or >= VMALLOC_END.
 * Generally, it should be >= VMALLOC_END.
 */
static struct map_desc mv88de3100_io_desc[] __initdata = {
	{/* map phys space 0xF6000000~0xF9000000(doesn't cover Flash area) to same virtual space*/
		.virtual = 0xF6000000,
		.pfn = __phys_to_pfn(0xF6000000),
		.length = 0x03000000,
		.type = MT_DEVICE
	},
	/* Don't map ROM,vectors space: 0xFF800000~0xFFFFFFFF */
};

#ifndef CONFIG_MV88DE3100_SM
#define SM_APB_WDT_VER 0x3130332a

#define	SM_APB_ICTL_BASE	(SOC_SM_APB_REG_BASE)
#define	SM_APB_WDT0_BASE	(SOC_SM_APB_REG_BASE + 0x1000)
#define	SM_APB_WDT1_BASE	(SOC_SM_APB_REG_BASE + 0x2000)
#define	SM_APB_WDT2_BASE	(SOC_SM_APB_REG_BASE + 0x3000)

#define	RA_smSysCtl_SM_WDT_MASK	0x0034

static void BFM_HOST_Bus_Write32(unsigned int addr, unsigned int val)
{
	writel(val, addr);
}
static void BFM_HOST_Bus_Read32(unsigned int addr, unsigned int *val)
{
	if(val) *val =readl(addr);
}

// use watchdog to reset Soc when SM no used
static int Galois_SM_WDT(unsigned int wdt_instance, unsigned int rst_type, int int_mode)
{
    unsigned int read,data,counter,stat, raw_status,wdt_base, ctrl_base,ictl_base;
    volatile int i,j,iResult;
    iResult=0;

    if (wdt_instance>5)
    {
        printk(" Invalid WDT instance.\n");
        iResult = 1;
    }
    ctrl_base = SM_SYS_CTRL_REG_BASE;
    ictl_base = SM_APB_ICTL_BASE; //SM_APB_ICTL0_BASE;

    // select WDT register base
    switch (wdt_instance)
    {
    	case 0:
    		wdt_base = SM_APB_WDT0_BASE;
    		break;
    	case 1:
    		wdt_base = SM_APB_WDT1_BASE;
    		break;
    	case 2:
    		wdt_base = SM_APB_WDT2_BASE;
    		break;
    	case 3:
    		wdt_base = APB_WDT_INST0_BASE; //APB_WDT0_BASE;
    		break;
    	case 4:
    		wdt_base = APB_WDT_INST1_BASE; //APB_WDT1_BASE;
    		break;
    	case 5:
    		wdt_base = APB_WDT_INST2_BASE; //APB_WDT2_BASE;
    		break;
    	default:
    		wdt_base = SM_APB_WDT0_BASE;
    		break;
    }

    // check SPI ID
    BFM_HOST_Bus_Read32((wdt_base + 0x18), &read);
    if (read != SM_APB_WDT_VER)
    {
       printk(" WDT ID incorrect.\n");
       iResult = 1;
    }
    // setup RST MASK register
    data=0x3F;
    if (rst_type==0)
    	data &=~( 1 <<(wdt_instance+3));                // soc reset
    else
     	data &=~( 1 <<(wdt_instance));
	
     BFM_HOST_Bus_Write32( (ctrl_base+RA_smSysCtl_SM_WDT_MASK),data);       // sm reset
     BFM_HOST_Bus_Read32((ctrl_base+RA_smSysCtl_SM_WDT_MASK),&read);

     // setup WDT mode and time out value
     if (int_mode==0)
     	BFM_HOST_Bus_Write32( (wdt_base+0x00),0x10);
     else
     	BFM_HOST_Bus_Write32( (wdt_base+0x00),0x12);      //
     BFM_HOST_Bus_Write32( (wdt_base+0x04),0x0A);          // time out around 3 sec
     BFM_HOST_Bus_Read32((wdt_base+0x00),&read);
     read |=0x01;                                          // enable WDT
     BFM_HOST_Bus_Write32( (wdt_base+0x00),read);
     BFM_HOST_Bus_Write32( (wdt_base+0x0C),0x76);          // restart counter
     // kick dog three times
     for (i=0;i<1; i++)
     {
     	for (;;)
     	{
     		for (j=0;j<100;j++);
     		BFM_HOST_Bus_Read32((wdt_base+0x08),&counter);// current counter
     		BFM_HOST_Bus_Read32((wdt_base+0x10),&stat);// read STAT
     		if (stat ==1)
     		{
     			printk(" watchdog Stat = 1, Break!\n");
        		break;
        	}
        	BFM_HOST_Bus_Read32((wdt_base+0x08),&counter);       // current counter
        	if(int_mode==0 & counter < 0x10000) break;
        }
        // kick dog, pet dog whatever ..
        BFM_HOST_Bus_Read32((ictl_base+0x18),&raw_status);
        BFM_HOST_Bus_Read32((wdt_base+0x14),&read);           // read EOI to clear interrupt stat
        BFM_HOST_Bus_Write32( (wdt_base+0x0C),0x76);          // restart counter

        BFM_HOST_Bus_Read32((ictl_base+0x18),&raw_status);
        BFM_HOST_Bus_Read32((wdt_base+0x10),&stat);
    }
    printk(" Wait for RESET!!! \n");
    while(1)
    {
    	for (j=0;j<10000;j++);
    	BFM_HOST_Bus_Read32((wdt_base+0x08),&counter);
        if(counter>0x30000)
        {
        }
        else
        {
        	break;
        }
    }
    for (j=0;j<100000;j++);
    return iResult;
}
static galois_soc_watchdog_reset(void)
{
	Galois_SM_WDT(3, 1, 0);
	for (;;);
}
#endif

/*
 * do very early than any initcalls, it's better in setup_arch()
 */
static void __init soc_initialize(void)
{
	mv88de3100_clk_init();
}

static void __init mv88de3100_map_io(void)
{
	iotable_init(mv88de3100_io_desc, ARRAY_SIZE(mv88de3100_io_desc));
	soc_initialize(); /* some ugly to put here, but it works:-) */
}

/*
 * mv88de3100 SoC initialization, in arch_initcall()
 */
static void __init mv88de3100_init(void)
{
	mv88de3100_devs_init();
}

#ifdef CONFIG_CACHE_TAUROS3
#include <asm/hardware/cache-tauros3.h>
static int __init mv88de3015_l2x0_init(void)
{
	return tauros3_init(__io(MEMMAP_CPU_SL2C_REG_BASE), 0x0, 0x0);
}
early_initcall(mv88de3015_l2x0_init);
#elif defined(CONFIG_CACHE_L2X0)
#include <asm/hardware/cache-l2x0.h>
static int __init mv88de3015_l2x0_init(void)
{
	l2x0_init(__io(MEMMAP_CPU_SL2C_REG_BASE), 0x30000000, 0xfe0fffff);

	return 0;
}
early_initcall(mv88de3015_l2x0_init);
#endif

#if !defined(CONFIG_MV88DE3100_SM)
void galois_arch_reset(char mode, const char *cmd)
{
	galois_soc_watchdog_reset();
}
#endif

void __init __attribute__((weak))
mv88de31xx_android_fixup(struct tag *tag, char **cmdline)
{
	return;
}

static void __init mv88de3100_fixup(struct machine_desc *desc, struct tag *tag,
		char **cmdline, struct meminfo *mi)
{
	struct tag *tmp_tag = tag;
	/*
	 * MV88DE3100 only receives ATAG_CORE, ATAG_MEM, ATAG_CMDLINE, ATAG_NONE
	 * Here change ATAG_MEM to be controlled by Linux itself.
	 */
	if (tmp_tag->hdr.tag == ATAG_CORE) {
		for (; tmp_tag->hdr.size; tmp_tag = tag_next(tmp_tag)) {
			if (tmp_tag->hdr.tag == ATAG_MEM) {
				tmp_tag->u.mem.size = CONFIG_MV88DE3100_CPU0MEM_SIZE;
				tmp_tag->u.mem.start = CONFIG_MV88DE3100_CPU0MEM_START;
				break;
			}
		}
	}
	/* Invoke android specific fixup handler */
	mv88de31xx_android_fixup(tag, cmdline);
}

#define ARM_BOOT_PARAMS_ADDR (CONFIG_MV88DE3100_CPU0MEM_START + 0x00000100)

MACHINE_START(MV88DE3100, "MV88DE3100")
	/* Maintainer: Jisheng Zhang<jszhang@marvell.com> */
	.boot_params	= ARM_BOOT_PARAMS_ADDR,
	.fixup		= mv88de3100_fixup,
	.map_io		= mv88de3100_map_io,
	.init_irq	= mv88de3100_init_irq,
	.timer		= &apbt_timer,
	.init_machine	= mv88de3100_init,
MACHINE_END
