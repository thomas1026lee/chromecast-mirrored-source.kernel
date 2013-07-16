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
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/irqs.h>
#include <mach/galois_generic.h>
#include <mach/galois_platform.h>
#include <mach/io.h>
#include <mach/timex.h>
#include <mach/sm.h>
#include "common.h"

extern void tauros3_shutdown(void);
extern void tauros3_restart(void);
extern void mv88de3100_close_phys_cores(void);
extern void mv88de3100_cpu_suspend(long);
extern void mv88de3100_cpu_resume(void);
extern void resume_prepare_cpus(void);

void EnterStandby_2(void)
{
	int msg[2];

	msg[0] = MV_SM_POWER_WARMDOWN_REQUEST_2;
	msg[1] = virt_to_phys(mv88de3100_cpu_resume);

	bsm_msg_send(MV_SM_ID_POWER, msg, 2*sizeof(int));
}

/*
 * task freezed, deviced stopped
 */
static int mv88de3100_pm_enter(suspend_state_t state)
{
	if (state != PM_SUSPEND_MEM)
		return -EINVAL;

	flush_cache_all();
#ifdef CONFIG_CACHE_TAUROS3
	tauros3_shutdown();
#endif
	mv88de3100_cpu_suspend(PHYS_OFFSET - PAGE_OFFSET);

	/* come back */
	cpu_init();

#ifdef CONFIG_CACHE_TAUROS3
	tauros3_restart();
#endif
#ifdef CONFIG_SMP
	resume_prepare_cpus();
#endif
	mv88de3100_close_phys_cores();
	printk("Welcome back\n");
	return 0;
}

static struct platform_suspend_ops mv88de3100_pm_ops = {
	.valid = suspend_valid_only_mem,
	.enter = mv88de3100_pm_enter,
};

static int __init mv88de3100_pm_init(void)
{
	suspend_set_ops(&mv88de3100_pm_ops);
	return 0;
}
late_initcall(mv88de3100_pm_init);
