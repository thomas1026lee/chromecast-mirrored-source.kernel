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
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/suspend.h>
#include <mach/galois_platform.h>
#include <asm/system.h>

#include "clock.h"

static unsigned long target_cpu_speed[NR_CPUS];

static struct cpufreq_frequency_table freq_table[] = {
	{ 0, 400000 },
	{ 1, 600000 },
	{ 2, 725000 },
	{ 3, 800000 },
	{ 4, 1000000 },
	{ 5, 1200000 },
	{ 6, CPUFREQ_TABLE_END },
};

struct cpupll_ctl {
	u32 ctl;
	u32 ctl1;
};

static struct cpupll_ctl cpupll_setting[] = {
	{ 0x8D48A005, 0x010832AA },
	{ 0x9948B005, 0x010832AD },
	{ 0x4A529D05, 0x0108312A },
	{ 0x8D48A005, 0x0108312A },
	{ 0x9548A805, 0x0108312C },
	{ 0x9948B005, 0x0108312D },
};

static struct clk *cpu_clk;

static int bg2_verify_speed(struct cpufreq_policy *policy)
{
	return cpufreq_frequency_table_verify(policy, freq_table);
}

static unsigned int bg2_getspeed(unsigned int cpu)
{
	return clk_get_rate(cpu_clk) / 1000;
}

static unsigned long bg2_cpu_highest_speed(void)
{
	unsigned long rate = 0;
	int i;

	for_each_online_cpu(i)
		rate = max(rate, target_cpu_speed[i]);
	return rate;
}

static int bg2_update_cpu_speed(unsigned long rate, unsigned int idx)
{
	struct cpufreq_freqs freqs;
	u32 val;
	unsigned long long t;

	freqs.old = bg2_getspeed(0);
	freqs.new = rate;

	if (freqs.old == freqs.new)
		return 0;

	for_each_online_cpu(freqs.cpu)
		cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

#ifdef CONFIG_CPU_FREQ_DEBUG
	printk("cpufreq-bg2: transition: %u --> %u\n",
	       freqs.old, freqs.new);
#endif
	/* bypass on */
	val = readl((MEMMAP_CHIP_CTRL_REG_BASE + RA_GBL_CLKSWITCH));
	val |= (1 << 2);
	writel(val, (MEMMAP_CHIP_CTRL_REG_BASE + RA_GBL_CLKSWITCH));

	/* pll bypass enable */
	val = readl(MEMMAP_CHIP_CTRL_REG_BASE + RA_GBL_CPUPLLCTL1);
	val |= (1 << 15);
	writel(val, (MEMMAP_CHIP_CTRL_REG_BASE + RA_GBL_CPUPLLCTL1));

	/* reset on */
	val = (1 << 8);
	writel(val, (MEMMAP_CHIP_CTRL_REG_BASE + RA_GBL_RESETTRIGGER));

	/* Go */
	writel(cpupll_setting[idx].ctl, (MEMMAP_CHIP_CTRL_REG_BASE + RA_GBL_CPUPLLCTL));
	writel(cpupll_setting[idx].ctl1, (MEMMAP_CHIP_CTRL_REG_BASE + RA_GBL_CPUPLLCTL1));

	/* keep reset high for at least 2us */
	t = sched_clock();
	while (sched_clock() - t < 2000)
		writel(val, (MEMMAP_CHIP_CTRL_REG_BASE + RA_GBL_RESETTRIGGER));

	udelay(50);

	/* make sure pll locked */
	while (!(readl(MEMMAP_CHIP_CTRL_REG_BASE + RA_GBL_PLLSTATUS) & (1 << 2)))
		cpu_relax();

	/* pll bypass enable */
	val = readl(MEMMAP_CHIP_CTRL_REG_BASE + RA_GBL_CPUPLLCTL1);
	val &= ~(1 << 15);
	writel(val, (MEMMAP_CHIP_CTRL_REG_BASE + RA_GBL_CPUPLLCTL1));

	/* bypass off */
	val = readl((MEMMAP_CHIP_CTRL_REG_BASE + RA_GBL_CLKSWITCH));
	val &= ~(1 << 2);
	writel(val, (MEMMAP_CHIP_CTRL_REG_BASE + RA_GBL_CLKSWITCH));

	for_each_online_cpu(freqs.cpu)
		cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	return 0;
}

static int bg2_target(struct cpufreq_policy *policy,
		      unsigned int target_freq,
		      unsigned int relation)
{
	unsigned int idx;
	unsigned int freq;

	cpufreq_frequency_table_target(policy, freq_table, target_freq,
		relation, &idx);

	freq = freq_table[idx].frequency;

	target_cpu_speed[policy->cpu] = freq;

	return bg2_update_cpu_speed(bg2_cpu_highest_speed(), idx);
}

static int bg2_cpu_init(struct cpufreq_policy *policy)
{

	cpufreq_frequency_table_get_attr(freq_table, policy->cpu);
	policy->cur = clk_get_rate(cpu_clk) / 1000;

	/* set the transition latency value */
	policy->cpuinfo.transition_latency = 3300000;

	cpumask_setall(policy->cpus);

	return cpufreq_frequency_table_cpuinfo(policy, freq_table);
}

static struct freq_attr *bg2_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver bg2_cpufreq_driver = {
	.verify		= bg2_verify_speed,
	.target		= bg2_target,
	.get		= bg2_getspeed,
	.init		= bg2_cpu_init,
	.name		= "BG2",
	.attr		= bg2_cpufreq_attr,
};

static int __init bg2_cpufreq_init(void)
{
	cpu_clk = clk_get(NULL, "cpu0");
	if (IS_ERR(cpu_clk))
		return -ENODEV;

	return cpufreq_register_driver(&bg2_cpufreq_driver);
}

static void __exit bg2_cpufreq_exit(void)
{
        cpufreq_unregister_driver(&bg2_cpufreq_driver);
}

MODULE_AUTHOR("Jisheng Zhang <jszhang@marvell.com>");
MODULE_DESCRIPTION("cpufreq driver for Marvell BG2");
MODULE_LICENSE("GPL");
module_init(bg2_cpufreq_init);
module_exit(bg2_cpufreq_exit);
