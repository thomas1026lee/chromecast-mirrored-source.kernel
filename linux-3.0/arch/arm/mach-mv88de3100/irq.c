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
#include <linux/irq.h>
#include <linux/cpu_pm.h>

#include <asm/io.h>
#include <mach/galois_platform.h>
#include <asm/hardware/gic.h>
#include <asm/mach/irq.h>

#define APB_ICTL_IRQ_INTEN_L		0x00
#define APB_ICTL_IRQ_INTMASK_L		0x08
#define APB_ICTL_IRQ_FINALSTATUS_L	0x30

#define APB_ICTL_INTEN		APB_ICTL_INST0_BASE + APB_ICTL_IRQ_INTEN_L
#define APB_ICTL_INTMASK	APB_ICTL_INST0_BASE + APB_ICTL_IRQ_INTMASK_L
#define APB_ICTL_FINALSTATUS	APB_ICTL_INST0_BASE + APB_ICTL_IRQ_FINALSTATUS_L
#define SM_ICTL_INTEN		SM_APB_ICTL1_BASE + APB_ICTL_IRQ_INTEN_L
#define SM_ICTL_INTMASK		SM_APB_ICTL1_BASE + APB_ICTL_IRQ_INTMASK_L
#define SM_ICTL_FINALSTATUS	SM_APB_ICTL1_BASE + APB_ICTL_IRQ_FINALSTATUS_L

static void apb_enable_irq(struct irq_data *d)
{
	u32 u;
	int pin = d->irq - GALOIS_APB_IRQ_START;

	u = __raw_readl(APB_ICTL_INTMASK);
	u &= ~(1 << (pin & 31));
	__raw_writel(u, APB_ICTL_INTMASK);

	u = __raw_readl(APB_ICTL_INTEN);
	u |= (1 << (pin & 31));
	__raw_writel(u, APB_ICTL_INTEN);
}

static void apb_disable_irq(struct irq_data *d)
{
	u32 u;
	int pin = d->irq - GALOIS_APB_IRQ_START;

	u = __raw_readl(APB_ICTL_INTMASK);
	u |= (1 << (pin & 31));
	__raw_writel(u, APB_ICTL_INTMASK);

	u = __raw_readl(APB_ICTL_INTEN);
	u &= ~(1 << (pin & 31));
	__raw_writel(u, APB_ICTL_INTEN);
}

static void apb_mask_irq(struct irq_data *d)
{
	u32 u;
	int pin = d->irq - GALOIS_APB_IRQ_START;

	u = __raw_readl(APB_ICTL_INTMASK);
	u |= (1 << (pin & 31));
	__raw_writel(u, APB_ICTL_INTMASK);
}

static void apb_unmask_irq(struct irq_data *d)
{
	u32 u;
	int pin = d->irq - GALOIS_APB_IRQ_START;

	u = __raw_readl(APB_ICTL_INTMASK);
	u &= ~(1 << (pin & 31));
	__raw_writel(u, APB_ICTL_INTMASK);
}

static void sm_enable_irq(struct irq_data *d)
{
	u32 u;
	int pin = d->irq - GALOIS_SM_IRQ_START;

	u = __raw_readl(SM_ICTL_INTMASK);
	u &= ~(1 << (pin & 31));
	__raw_writel(u, SM_ICTL_INTMASK);

	u = __raw_readl(SM_ICTL_INTEN);
	u |= (1 << (pin & 31));
	__raw_writel(u, SM_ICTL_INTEN);
}

static void sm_disable_irq(struct irq_data *d)
{
	u32 u;
	int pin = d->irq - GALOIS_SM_IRQ_START;

	u = __raw_readl(SM_ICTL_INTMASK);
	u |= (1 << (pin & 31));
	__raw_writel(u, SM_ICTL_INTMASK);

	u = __raw_readl(SM_ICTL_INTEN);
	u &= ~(1 << (pin & 31));
	__raw_writel(u, SM_ICTL_INTEN);
}

static void sm_mask_irq(struct irq_data *d)
{
	u32 u;
	int pin = d->irq - GALOIS_SM_IRQ_START;

	u = __raw_readl(SM_ICTL_INTMASK);
	u |= (1 << (pin & 31));
	__raw_writel(u, SM_ICTL_INTMASK);
}

static void sm_unmask_irq(struct irq_data *d)
{
	u32 u;
	int pin = d->irq - GALOIS_SM_IRQ_START;

	u = __raw_readl(SM_ICTL_INTMASK);
	u &= ~(1 << (pin & 31));
	__raw_writel(u, SM_ICTL_INTMASK);
}

static struct irq_chip apb_irq_chip = {
	.name		= "apb_ictl",
	.irq_enable	= apb_enable_irq,
	.irq_disable	= apb_disable_irq,
	.irq_mask	= apb_mask_irq,
	.irq_unmask	= apb_unmask_irq,
};

static struct irq_chip sm_irq_chip = {
	.name		= "sm_ictl",
	.irq_enable	= sm_enable_irq,
	.irq_disable	= sm_disable_irq,
	.irq_mask	= sm_mask_irq,
	.irq_unmask	= sm_unmask_irq,
};

static void apb_irq_demux(unsigned int irq, struct irq_desc *desc)
{
	int res;
	unsigned long cause;
	struct irq_chip *chip = irq_desc_get_chip(desc);

	chained_irq_enter(chip, desc);
	cause = __raw_readl(APB_ICTL_FINALSTATUS);
	while (cause) {
		res = ffs(cause) - 1;
		cause &= ~(1 << res);
		generic_handle_irq(res + GALOIS_APB_IRQ_START);
	}
	chained_irq_exit(chip, desc);

}

static void sm_irq_demux(unsigned int irq, struct irq_desc *desc)
{
	int res;
	unsigned long cause;
	struct irq_chip *chip = irq_desc_get_chip(desc);

	chained_irq_enter(chip, desc);
	cause = __raw_readl(SM_ICTL_FINALSTATUS);
	while (cause) {
		res = ffs(cause) - 1;
		cause &= ~(1 << res);
		generic_handle_irq(res + GALOIS_SM_IRQ_START);
	}
	chained_irq_exit(chip, desc);
}

#ifdef CONFIG_CPU_PM
static u32 apb_ictl_intmask;
static u32 apb_ictl_inten;
static u32 sm_ictl_intmask;
static u32 sm_ictl_inten;

static void galois_irq_save(void)
{
	apb_ictl_intmask = __raw_readl(APB_ICTL_INTMASK);
	apb_ictl_inten = __raw_readl(APB_ICTL_INTEN);
	sm_ictl_intmask = __raw_readl(SM_ICTL_INTMASK);
	sm_ictl_inten = __raw_readl(SM_ICTL_INTEN);
}

static void galois_irq_restore(void)
{
	__raw_writel(apb_ictl_intmask, APB_ICTL_INTMASK);
	__raw_writel(apb_ictl_inten, APB_ICTL_INTEN);
	__raw_writel(sm_ictl_intmask, SM_ICTL_INTMASK);
	__raw_writel(sm_ictl_inten, SM_ICTL_INTEN);
}

static int gic_notifier(struct notifier_block *self, unsigned long cmd,
			void *v)
{
	switch (cmd) {
		case CPU_CLUSTER_PM_ENTER:
			galois_irq_save();
			break;
		case CPU_CLUSTER_PM_ENTER_FAILED:
		case CPU_CLUSTER_PM_EXIT:
			galois_irq_restore();
			break;
	}

	return NOTIFY_OK;
}

static struct notifier_block irq_notifier_block = {
	.notifier_call = gic_notifier,
};

static void __init irq_pm_init(void)
{
	cpu_pm_register_notifier(&irq_notifier_block);
}
#else
static void __init irq_pm_init(void)
{
}
#endif

void __init mv88de3100_init_irq(void)
{
	int i;

	gic_init(0, 29, __io(MEMMAP_GIC_DIST_BASE), __io(MEMMAP_GIC_CPU_BASE));

	for (i = GALOIS_APB_IRQ_START; i < GALOIS_SM_IRQ_START; i++)
		irq_set_chip_and_handler(i, &apb_irq_chip, handle_level_irq);
	for (i = GALOIS_SM_IRQ_START; i < NR_IRQS; i++)
		irq_set_chip_and_handler(i, &sm_irq_chip, handle_level_irq);

	irq_set_chained_handler(IRQ_ICTLINST0CPUIRQ, apb_irq_demux);
	irq_set_chained_handler(IRQ_SM2SOCHWINT0, sm_irq_demux);
	irq_pm_init();
}
