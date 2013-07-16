/*
 * arch/arm/mm/cache-tauros3.c - Tauros3 L2 cache controller support
 *
 * Copyright (C) 2008 Marvell Semiconductor
 * Original Author: Lennert Buytenhek
 * Modified by Peter Liao <pliao@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * References:
 * - PJ1 CPU Core Datasheet,
 *   Document ID MV-S104837-01, Rev 0.7, January 24 2008.
 * - PJ4 CPU Core Datasheet,
 *   Document ID MV-S105190-00, Rev 1.1, March 14 2008.
 */
#include <linux/module.h>
#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <asm/cacheflush.h>
#include <asm/hardware/cache-tauros3.h>
#include <asm/io.h>
#include <asm/setup.h>

#define CACHE_LINE_SIZE		32
static void __iomem *tauros3_base = NULL;
static u32 dgb_control_default = 0;
static u32 hidden_dgb_control_default = 0;
static u32 tauros3_way_mask;   /* Bitmask of active ways */
static u32 tauros3_size;

static DEFINE_SPINLOCK(tauros3_lock);
static int tauros3_forcewa;

struct forcewa_policy {
	const char	policy[16];
	unsigned int	value;
};

static struct forcewa_policy forcewa_policies[] = {
	{
		.policy		= "default",
		.value		= TAUROS3_AWCACHE,
	}, {
		.policy		= "nowa",
		.value		= TAUROS3_FORCE_NO_WA,
	}, {
		.policy		= "wa",
		.value		= TAUROS3_FORCE_WA,
	}
};

#ifdef CONFIG_PROC_FS
static unsigned char *associativity[] = { "8-way", "16-way" };

static unsigned char *wsize[] = { "reserved(16KB)",
	"16KB",
	"32KB",
	"64KB",
	"128KB",
	"256KB",
	"512KB",
	"reserved(512KB)"
};

static int proc_tauros3_info_read(char *page, char **start, off_t off,
				  int count, int *eof, void *data)
{
	char *p = page;
	int len;
	__u32 aux;
	int i;

	p += sprintf(p, "Tauros3 Information:\n");

	aux = readl_relaxed(tauros3_base + TAUROS3_CACHE_ID);
	p += sprintf(p, "Implementer : %#02x\n", (aux >> 24) & 0xff);
	p += sprintf(p, "Cache ID    : %#02x\n", (aux >> 10) & 0x3f);
	p += sprintf(p, "Part Number : %#02x\n", (aux >> 6) & 0xf);
	p += sprintf(p, "RTL release : %#02x\n", (aux >> 0) & 0x3f);

	aux = readl_relaxed(tauros3_base + TAUROS3_CACHE_TYPE);
	p += sprintf(p, "System L2 Cache Associativity : %s\n",
		     associativity[(aux >> 6) & 0x1]);
	p += sprintf(p, "System L2 Cache Way size      : %s\n",
		     wsize[(aux >> 8) & 0x7]);

	aux = readl_relaxed(tauros3_base + TAUROS3_PREFETCH_CTRL);
	p += sprintf(p, "TAUROS3_PREFETCH_CTRL : %#08x\n", aux);

	aux = readl_relaxed(tauros3_base + TAUROS3_AUX_CTRL);
	p += sprintf(p, "TAUROS3_AUX_CTRL : %#08x\n", aux);

	aux &= TAUROS3_WA_MASK;
	for (i = 0; i < ARRAY_SIZE(forcewa_policies); i++) {
		if (forcewa_policies[i].value == aux) {
			p += sprintf(p, "\tWrite Allocate: %s\n", forcewa_policies[i].policy);
			break;
		}
	}

	aux = readl_relaxed(tauros3_base + TAUROS3_DEBUG_CTRL);
	p += sprintf(p, "TAUROS3_DEBUG_CTRL : %#08x\n", aux);

	aux = readl_relaxed(tauros3_base + TAUROS3_HIDDEN_DEBUG_REG);
	p += sprintf(p, "TAUROS3_HIDDEN_DEBUG_REG : %#08x\n", aux);

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}

static int proc_tauros3_info_write(struct file *file,
				   const char __user * buffer,
				   unsigned long count, void *data)
{
	u8 param[2][32];
	u32 configs[2] = { 0 };
	u8 *buffer_tmp = kmalloc(count + 16, GFP_KERNEL);
	int i;

	memset(buffer_tmp, 0x0, sizeof(buffer_tmp));

	if (copy_from_user(buffer_tmp, buffer, count))
		return -EFAULT;

	sscanf(buffer_tmp, "%s %s\n", param[0], param[1]);

	for (i = 0; i < 2; i++)
		configs[i] = simple_strtoul(param[i], NULL, 0);

	if (strcmp(param[0], "reset") == 0)
		writel_relaxed(dgb_control_default,
		       tauros3_base + TAUROS3_DEBUG_CTRL);
	else
		writel_relaxed(configs[0], tauros3_base + TAUROS3_DEBUG_CTRL);

	if (strcmp(param[1], "reset") == 0)
		writel_relaxed(hidden_dgb_control_default,
		       tauros3_base + TAUROS3_HIDDEN_DEBUG_REG);
	else
		writel_relaxed(configs[1],
		       tauros3_base + TAUROS3_HIDDEN_DEBUG_REG);


	if (buffer_tmp)
		kfree(buffer_tmp);

	return count;
}

#ifdef CONFIG_CACHE_TAUROS3_EVENT_MONITOR_ENABLE
static unsigned int last_counter[3] = { 0, 0, 0 };

static unsigned char *event_name[] = {
	"Counter Disabled",
	"CO",
	"DRHIT",
	"DRREQ",
	"DWHIT",
	"DWREQ",
	"DWTREQ",
	"IRHIT",
	"IRREQ",
	"WA",
	"PF",
	"MRHIT",
	"MRREQ",
	"Counter Disabled",
	"Counter Disabled",
	"Counter Disabled"
};

static int proc_tauros3_counter_read(char *page, char **start, off_t off,
				     int count, int *eof, void *data)
{
	char *p = page;
	int len, i, cfg0, cfg1, cfg2;
	unsigned int counter[3], delta_counter[3];

	cfg0 = readl_relaxed(tauros3_base + TAUROS3_EVENT_CNT0_CFG);
	cfg1 = readl_relaxed(tauros3_base + TAUROS3_EVENT_CNT1_CFG);
	cfg2 = readl_relaxed(tauros3_base + TAUROS3_EVENT_CNT2_CFG);

	p += sprintf(p, "Tauros3 Event Counter Information:\n\n");
	p += sprintf(p, "TAUROS3_EVENT_CNT_CTRL : %#08x\n",
		     readl_relaxed(tauros3_base + TAUROS3_EVENT_CNT_CTRL));
	p += sprintf(p, "TAUROS3_EVENT_CNT0_CFG : %#08x[%s]\n", cfg0,
		     event_name[(cfg0 >> 2) & 0xF]);
	p += sprintf(p, "TAUROS3_EVENT_CNT1_CFG : %#08x[%s]\n", cfg1,
		     event_name[(cfg1 >> 2) & 0xF]);
	p += sprintf(p, "TAUROS3_EVENT_CNT2_CFG : %#08x[%s]\n", cfg2,
		     event_name[(cfg2 >> 2) & 0xF]);

	counter[0] = readl_relaxed(tauros3_base + TAUROS3_EVENT_CNT0_VAL);
	counter[1] = readl_relaxed(tauros3_base + TAUROS3_EVENT_CNT1_VAL);
	counter[2] = readl_relaxed(tauros3_base + TAUROS3_EVENT_CNT2_VAL);

	for (i = 0; i < 3; i++)
		delta_counter[i] = counter[i] - last_counter[i];

	p += sprintf(p,
		     "\n=========================================================================\n");
	p += sprintf(p, "currnet counter 0 1 2 : %12u     %12u     %12u\n",
		     counter[0], counter[1], counter[2]);
	p += sprintf(p, "delta   counter 0 1 2 : %12u     %12u     %12u\n",
		     delta_counter[0], delta_counter[1], delta_counter[2]);

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	memcpy((unsigned char *) last_counter, (unsigned char *) counter,
	       sizeof(last_counter));

	return len;
}


static int proc_tauros3_counter_write(struct file *file,
				      const char __user * buffer,
				      unsigned long count, void *data)
{
	u8 param[4][32];
	u32 configs[4] = { 0 };
	u8 *buffer_tmp = kmalloc(count + 16, GFP_KERNEL);
	int i;

	memset(buffer_tmp, 0x0, sizeof(buffer_tmp));

	if (copy_from_user(buffer_tmp, buffer, count)) {
		if (buffer_tmp)
			kfree(buffer_tmp);
		return -EFAULT;
	}

	sscanf(buffer_tmp, "%s %s %s %s\n", param[0], param[1], param[2],
	       param[3]);

	if (strcmp(param[0], "reset") == 0) {
		writel_relaxed(0, tauros3_base + TAUROS3_EVENT_CNT_CTRL);
		writel_relaxed(0, tauros3_base + TAUROS3_EVENT_CNT0_CFG);
		writel_relaxed(0, tauros3_base + TAUROS3_EVENT_CNT1_CFG);
		writel_relaxed(0, tauros3_base + TAUROS3_EVENT_CNT2_CFG);
		writel_relaxed(0, tauros3_base + TAUROS3_EVENT_CNT0_VAL);
		writel_relaxed(0, tauros3_base + TAUROS3_EVENT_CNT1_VAL);
		writel_relaxed(0, tauros3_base + TAUROS3_EVENT_CNT2_VAL);

		memset((unsigned char *) last_counter, 0,
		       sizeof(last_counter));

		goto out;
	}

	for (i = 0; i < 4; i++)
		configs[i] = simple_strtoul(param[i], NULL, 0);

	writel_relaxed(configs[1], tauros3_base + TAUROS3_EVENT_CNT0_CFG);
	writel_relaxed(configs[2], tauros3_base + TAUROS3_EVENT_CNT1_CFG);
	writel_relaxed(configs[3], tauros3_base + TAUROS3_EVENT_CNT2_CFG);
	writel_relaxed(configs[0], tauros3_base + TAUROS3_EVENT_CNT_CTRL);

      out:

	if (buffer_tmp)
		kfree(buffer_tmp);

	return count;
}
#endif				/* CONFIG_CACHE_TAUROS3_EVENT_MONITOR_ENABLE */
#endif				/* CONFIG_PROC_FS */


static inline void cache_wait_way(void __iomem *reg, unsigned long mask)
{

	/* wait for cache operation by line or way to complete */
	while (readl_relaxed(reg) & mask)
		;
}

static inline void cache_wait(void __iomem *reg, unsigned long mask)
{
	/* cache operations by line are atomic on TAUROS3 */
}

static inline void cache_sync(void)
{
	void __iomem *base = tauros3_base;
	writel_relaxed(0, base + TAUROS3_CACHE_SYNC);
	cache_wait(base + TAUROS3_CACHE_SYNC, 1);
}

static inline void tauros3_clean_line(unsigned long addr)
{
	void __iomem *base = tauros3_base;
	cache_wait(base + TAUROS3_CLEAN_LINE_PA, 1);
	writel_relaxed(addr, base + TAUROS3_CLEAN_LINE_PA);
}

static inline void tauros3_inv_line(unsigned long addr)
{
	void __iomem *base = tauros3_base;
	cache_wait(base + TAUROS3_INV_LINE_PA, 1);
	writel_relaxed(addr, base + TAUROS3_INV_LINE_PA);
}

static inline void tauros3_flush_line(unsigned long addr)
{
	void __iomem *base = tauros3_base;
	cache_wait(base + TAUROS3_CLEAN_INV_LINE_PA, 1);
	writel_relaxed(addr, base + TAUROS3_CLEAN_INV_LINE_PA);
}

static void tauros3_cache_sync(void)
{
	unsigned long flags;

	spin_lock_irqsave(&tauros3_lock, flags);
	cache_sync();
	spin_unlock_irqrestore(&tauros3_lock, flags);
}

static void tauros3_flush_all(void)
{
	unsigned long flags;

	/* clean and invalidate all ways */
	spin_lock_irqsave(&tauros3_lock, flags);
	writel_relaxed(tauros3_way_mask, tauros3_base + TAUROS3_CLEAN_INV_WAY);
	cache_wait_way(tauros3_base + TAUROS3_CLEAN_INV_WAY, tauros3_way_mask);
	cache_sync();
	spin_unlock_irqrestore(&tauros3_lock, flags);
}

static void tauros3_clean_all(void)
{
	unsigned long flags;

	/* clean all ways */
	spin_lock_irqsave(&tauros3_lock, flags);
	writel_relaxed(0x1, tauros3_base + TAUROS3_CLEAN_ALL);
	cache_wait_way(tauros3_base + TAUROS3_CLEAN_ALL, 1);
	cache_sync();
	spin_unlock_irqrestore(&tauros3_lock, flags);
}

static inline void tauros3_inv_all(void)
{
	unsigned long flags;

	/* invalidate all ways */
	spin_lock_irqsave(&tauros3_lock, flags);
	writel_relaxed(0x1, tauros3_base + TAUROS3_INV_ALL);
	cache_wait_way(tauros3_base + TAUROS3_INV_ALL, 0x1);
	cache_sync();
	spin_unlock_irqrestore(&tauros3_lock, flags);
}

#ifndef CONFIG_SMP
/*
 * Linux primitives.
 *
 * Note that the end addresses passed to Linux primitives are
 * noninclusive.
 */
static void tauros3_inv_range(unsigned long start, unsigned long end)
{
	void __iomem *base = tauros3_base;
	unsigned long flags;

	spin_lock_irqsave(&tauros3_lock, flags);
	if (start & (CACHE_LINE_SIZE - 1)) {
		start &= ~(CACHE_LINE_SIZE - 1);
		tauros3_flush_line(start);
		start += CACHE_LINE_SIZE;
	}

	if (end & (CACHE_LINE_SIZE - 1)) {
		end &= ~(CACHE_LINE_SIZE - 1);
		tauros3_flush_line(end);
	}

	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			tauros3_inv_line(start);
			start += CACHE_LINE_SIZE;
		}

		if (blk_end < end) {
			spin_unlock_irqrestore(&tauros3_lock, flags);
			spin_lock_irqsave(&tauros3_lock, flags);
		}
	}
	cache_wait(base + TAUROS3_INV_LINE_PA, 1);
	cache_sync();
	spin_unlock_irqrestore(&tauros3_lock, flags);
}

static void tauros3_clean_range(unsigned long start, unsigned long end)
{
	void __iomem *base = tauros3_base;
	unsigned long flags;

	if ((end - start) >= tauros3_size) {
		tauros3_clean_all();
		return;
	}

	spin_lock_irqsave(&tauros3_lock, flags);
	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			tauros3_clean_line(start);
			start += CACHE_LINE_SIZE;
		}

		if (blk_end < end) {
			spin_unlock_irqrestore(&tauros3_lock, flags);
			spin_lock_irqsave(&tauros3_lock, flags);
		}
	}
	cache_wait(base + TAUROS3_CLEAN_LINE_PA, 1);
	cache_sync();
	spin_unlock_irqrestore(&tauros3_lock, flags);
}

static void tauros3_flush_range(unsigned long start, unsigned long end)
{
	void __iomem *base = tauros3_base;
	unsigned long flags;

#if 0
	if ((end - start) >= tauros3_size) {
		tauros3_flush_all();
		return;
	}
#endif

	spin_lock_irqsave(&tauros3_lock, flags);
	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			tauros3_flush_line(start);
			start += CACHE_LINE_SIZE;
		}

		if (blk_end < end) {
			spin_unlock_irqrestore(&tauros3_lock, flags);
			spin_lock_irqsave(&tauros3_lock, flags);
		}
	}
	cache_wait(base + TAUROS3_CLEAN_INV_LINE_PA, 1);
	cache_sync();
	spin_unlock_irqrestore(&tauros3_lock, flags);
}
#endif

void tauros3_shutdown(void)
{
	unsigned long flags;

	BUG_ON(num_online_cpus() > 1);

	local_irq_save(flags);

	if (readl_relaxed(tauros3_base + TAUROS3_CTRL) & 1) {
		int w;
		for (w = 0; w < 8; w++) {
			writel_relaxed(tauros3_way_mask,
				tauros3_base + TAUROS3_LOCKDOWN_WAY_D + (w * 8));
			writel_relaxed(tauros3_way_mask,
				tauros3_base + TAUROS3_LOCKDOWN_WAY_I + (w * 8));
		}
		tauros3_flush_all();
		writel_relaxed(0, tauros3_base + TAUROS3_CTRL);
		for (w = 0; w < 8; w++) {
			writel_relaxed(0, tauros3_base + TAUROS3_LOCKDOWN_WAY_D + (w * 8));
			writel_relaxed(0, tauros3_base + TAUROS3_LOCKDOWN_WAY_I + (w * 8));
		}
	}

	local_irq_restore(flags);
}

static void tauros3_enable(__u32 aux_val, __u32 aux_mask)
{
	__u32 aux;
	__u32 way_size = 0;
	int ways;

	/* 1. Write to TAUROS3 Auxiliary Control Register, 0x104
	 *    Setting up Associativity, Way Size, and Latencies
	 */
	aux = readl_relaxed(tauros3_base + TAUROS3_AUX_CTRL);

	aux &= ~aux_mask;
	aux |= aux_val;

#if defined(CONFIG_CACHE_TAUROS3_FORCE_WRITE_ALLOCATE)
	aux &= ~TAUROS3_WA_MASK;
	aux |= TAUROS3_FORCE_WA;
#elif defined(CONFIG_CACHE_TAUROS3_FORCE_NO_WRITE_ALLOCATE)
	aux &= ~TAUROS3_WA_MASK;
	aux |= TAUROS3_FORCE_NO_WA;
#endif
	/* Write allocate policy is overwritten by command line.
	 */
	if (tauros3_forcewa) {
		aux &= ~TAUROS3_WA_MASK;
		aux |= forcewa_policies[tauros3_forcewa].value;
	}

#ifdef CONFIG_CACHE_TAUROS3_PREFETCH_ENABLE
	aux &= ~(0x1 << 28);
	aux |= (0x1 << 28);
#endif

#ifdef CONFIG_CACHE_TAUROS3_L2_ECC_ENABLE
	aux &= ~(0x1 << 21);
	aux |= (0x1 << 21);
#endif

#ifdef CONFIG_CACHE_TAUROS3_EVENT_MONITOR_ENABLE
	aux &= ~(0x1 << 20);
	aux |= (0x1 << 20);
#endif

	writel_relaxed(aux, tauros3_base + TAUROS3_AUX_CTRL);

	/* AUX2
	 */
	aux = readl_relaxed(tauros3_base + TAUROS3_HIDDEN_DEBUG_REG);

#ifdef CONFIG_CACHE_TAUROS3_L2LINEFILL_BURST8_ENABLE
	aux &= ~(0x1 << 2);
	aux |= (0x1 << 2);
#endif

	writel_relaxed(aux, tauros3_base + TAUROS3_HIDDEN_DEBUG_REG);

	/* 1.1 Write to TAUROS3 Prefetch Control Register, 0xf60
	 * Setting up Prefetch Offset
	 */
#ifdef CONFIG_CACHE_TAUROS3_PREFETCH_ENABLE
	aux = readl_relaxed(tauros3_base + TAUROS3_PREFETCH_CTRL);
	aux &= ~TAUROS3_PREFETCH_MASK;
	aux |= (CONFIG_CACHE_TAUROS3_PREFETCH_OFFSET & TAUROS3_PREFETCH_MASK);
	writel_relaxed(aux, tauros3_base + TAUROS3_PREFETCH_CTRL);
#endif

	/* 2. Secure write to TAUROS3 Invalidate by Way, 0x77c
	 *    Setting up Associativity, Way Size, and Latencies
	 */
	tauros3_inv_all();

	/* 3. Write to the Lockdown D and Lockdown I Register 9 if required
	 */
#ifdef CONFIG_CACHE_TAUROS3_FORCE_WRITE_THROUGH
	aux = readl_relaxed(tauros3_base + TAUROS3_DEBUG_CTRL);
	aux |= (1 << 1);
	writel_relaxed(aux, tauros3_base + TAUROS3_DEBUG_CTRL);
#endif

	/* 4. Write to interrupt clear register, 0x220, to clear any residual raw interrupt set.
	 */
	writel_relaxed(0x1FF, tauros3_base + TAUROS3_INTR_CLEAR);

	/* 5. Write to Control Register 1, bit-0 to enable the cache
	 */
	writel_relaxed(1, tauros3_base + TAUROS3_CTRL);

	aux = readl_relaxed(tauros3_base + TAUROS3_CACHE_TYPE);
	if ((aux >> 6) & 0x1)
		ways = 16;
	else
		ways = 8;

	tauros3_way_mask = (1 << ways) - 1;

	/*
	 * L2 cache Size =  Way size * Number of ways
	 */
	way_size = (aux >>8) & 0x7;
	way_size = 1 << (way_size + 3);
	tauros3_size = ways * way_size * SZ_1K;

	dgb_control_default = readl_relaxed(tauros3_base + TAUROS3_DEBUG_CTRL);
	hidden_dgb_control_default =
	    readl_relaxed(tauros3_base + TAUROS3_HIDDEN_DEBUG_REG);

}

void tauros3_restart(void)
{
	tauros3_enable(0, 0);
}

int __init tauros3_init(void __iomem * base, __u32 aux_val, __u32 aux_mask)
{

#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *res;
	struct proc_dir_entry *res_file;

	res = proc_mkdir("tauros3", NULL);
	if (!res)
		return -ENOMEM;

	/* Create information proc file
	 */
	res_file = create_proc_entry("info", S_IWUSR | S_IRUGO, res);
	if (!res)
		return -ENOMEM;

	res_file->read_proc = proc_tauros3_info_read;
	res_file->write_proc = proc_tauros3_info_write;
#ifdef CONFIG_CACHE_TAUROS3_EVENT_MONITOR_ENABLE
	/* Create counter proc file
	 */
	res_file = create_proc_entry("counter", S_IWUSR | S_IRUGO, res);
	if (!res)
		return -ENOMEM;

	res_file->read_proc = proc_tauros3_counter_read;
	res_file->write_proc = proc_tauros3_counter_write;
#endif				/* CONFIG_CACHE_TAUROS3_EVENT_MONITOR_ENABLE */
#endif

	tauros3_base = base;

	tauros3_enable(aux_val, aux_mask);

#ifndef CONFIG_SMP
	outer_cache.inv_range = tauros3_inv_range;
	outer_cache.clean_range = tauros3_clean_range;
	outer_cache.flush_range = tauros3_flush_range;
#endif
	outer_cache.sync = tauros3_cache_sync;

	printk(KERN_INFO "Tauros3: System L2 cache support initialised\n");

	return 0;
}

static int __init early_tauros3_forcewa(char *p)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(forcewa_policies); i++) {
		int len = strlen(forcewa_policies[i].policy);

		if (memcmp(p, forcewa_policies[i].policy, len) == 0) {
			tauros3_forcewa = i;
			break;
		}
	}
	return 0;
}
early_param("tauros3forcewa", early_tauros3_forcewa);
