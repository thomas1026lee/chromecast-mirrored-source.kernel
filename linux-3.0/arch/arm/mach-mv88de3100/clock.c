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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <mach/galois_platform.h>

#include "clock.h"

#ifdef CONFIG_MV88DE3100_ASIC
static inline u32 rdlc(int offset)
{
	return __raw_readl(MEMMAP_CHIP_CTRL_REG_BASE + offset);
}

static u32 vcodiv[] = {10, 15, 20, 25, 30, 40, 50, 60, 80};
static u32 clkdiv[] = {1, 2, 4, 6, 8, 12, 1, 1};

static inline u32 cpu0_get_divider(void)
{
	u32 val;

	val = rdlc(RA_GBL_CLKSWITCH);
	if ((val >> 8) & 0x1)
		return 3;
	if ((val >> 7) & 0x1) {
		u32 clksel = (rdlc(RA_GBL_CLKSELECT) >> 9) & 0x7;
		return clkdiv[clksel];
	} else
		return 1;
}

static inline u32 cfg_get_divider(void)
{
	u32 val;

	val = rdlc(RA_GBL_CLKSWITCH);
	if ((val >> 17) & 0x1)
		return 3;
	if ((val >> 16) & 0x1) {
		u32 clksel = (rdlc(RA_GBL_CLKSELECT) >> 26) & 0x7;
		return clkdiv[clksel];
	} else
		return 1;
}

static inline u32 perif_get_divider(void)
{
	u32 val;

	val = rdlc(RA_GBL_CLKSWITCH);
	if ((val >> 26) & 0x1)
		return 3;
	if ((val >> 25) & 0x1) {
		u32 clksel = (rdlc(RA_GBL_CLKSELECT1) >> 12) & 0x7;
		return clkdiv[clksel];
	} else
		return 1;
}

static inline u32 sdioxin_get_divider(void)
{
	u32 val;

	val = rdlc(RA_GBL_SDIOXINCLKCTRL);
	if ((val >> 6) & 0x1)
		return 3;
	if ((val >> 5) & 0x1) {
		u32 clksel = (val >> 7) & 0x7;
		return clkdiv[clksel];
	} else
		return 1;
}

static inline u32 sdio1xin_get_divider(void)
{
	u32 val;

	val = rdlc(RA_GBL_SDIO1XINCLKCTRL);
	if ((val >> 6) & 0x1)
		return 3;
	if ((val >> 5) & 0x1) {
		u32 clksel = (val >> 7) & 0x7;
		return clkdiv[clksel];
	} else
		return 1;
}

/* input clock is 25MHZ */
#define CLKIN 25
unsigned long get_pll(struct clk *clk)
{
	u32 val, fbdiv, rfdiv, vcodivsel;
	unsigned long pll;

	val = rdlc(clk->ctl);
	fbdiv = (val >> 6) & 0x1ff;
	rfdiv = (val >> 1) & 0x1f;
	if (rfdiv == 0)
		rfdiv = 1;
	pll = CLKIN * fbdiv / rfdiv;
	val = rdlc(clk->ctl1);
	vcodivsel = (val >> 7) & 0xf;
	pll = pll * 10 / vcodiv[vcodivsel];

	return pll;
}
#else
static unsigned long cpu0_get_rate(struct clk *clk)
{
	return GALOIS_CPU_CLK;
}

static unsigned long cfg_get_rate(struct clk *clk)
{
	return GALOIS_TIMER_CLK;
}
#endif

CLK(cpu0, RA_GBL_CPUPLLCTL, RA_GBL_CPUPLLCTL1);
CLK(cfg, RA_GBL_SYSPLLCTL, RA_GBL_SYSPLLCTL1);
#ifdef CONFIG_MV88DE3100_ASIC
CLK(perif, RA_GBL_SYSPLLCTL, RA_GBL_SYSPLLCTL1);
CLK(sdioxin, RA_GBL_SYSPLLCTL, RA_GBL_SYSPLLCTL1);
CLK(sdio1xin, RA_GBL_SYSPLLCTL, RA_GBL_SYSPLLCTL1);
#endif

static struct clk_lookup clks[] = {
	CLK_LOOKUP(cpu0),
	CLK_LOOKUP(cfg),
#ifdef CONFIG_MV88DE3100_ASIC
	CLK_LOOKUP(perif),
	CLK_LOOKUP(sdioxin),
	CLK_LOOKUP(sdio1xin),
#endif
};

void __init mv88de3100_clk_init(void)
{
	clkdev_add_table(clks, ARRAY_SIZE(clks));
}

int clk_enable(struct clk *clk)
{
	return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	unsigned long rate;

	if (clk->ops->getrate)
		rate = clk->ops->getrate(clk);
	else
		rate = 0;

	return rate;
}
EXPORT_SYMBOL(clk_get_rate);
