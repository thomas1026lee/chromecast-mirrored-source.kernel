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
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/platform_device.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <mach/galois_generic.h>
#include <mach/galois_platform.h>

#include "pwm.h"

#define GALOIS_PWMCH0		0
#define GALOIS_PWMCH1		1
#define GALOIS_PWMCH2		2
#define GALOIS_PWMCH3		3
#define GALOIS_PWMCH_NUM	4

#define GA_REG_WORD32_READ(a, pv)	(*pv = __raw_readl(a))
#define GA_REG_WORD32_WRITE(a, v)	__raw_writel(v, a)

int opened_pwmch[GALOIS_PWMCH_NUM];
int pwmch_base = MEMMAP_PWM_REG_BASE;

static int galois_pwmch_open(struct inode *inode, struct file *file)
{
	unsigned int pwmch;

	pwmch = iminor(inode);
	if (pwmch >= GALOIS_PWMCH_NUM) {
			printk("error: unknown PWM channel %d.\n", pwmch);
			return -ENODEV;
	}

	if (opened_pwmch[pwmch]) {
		printk("error: pwm channel %d has been opened.\n", pwmch);
		return -EBUSY;
	}

	opened_pwmch[pwmch] = 1;
	file->private_data = (void *)pwmch;
	return 0;
}

static int galois_pwmch_release(struct inode *inode, struct file *file)
{
	unsigned int pwmch = (unsigned int)file->private_data;

	if (pwmch >= GALOIS_PWMCH_NUM) {
		printk("error: unknown PWM channel %d.\n", pwmch);
		return -ENODEV;
	}

	opened_pwmch[pwmch] = 0;
	return 0;
}

static ssize_t galois_pwmch_read(struct file *file, char __user * buf,
		size_t count, loff_t *ppos)
{
	printk("error: please use ioctl.\n");
	return -EFAULT;
}

static ssize_t galois_pwmch_write(struct file *file, const char __user * buf,
		size_t count, loff_t *ppos)
{
	printk("error: please use ioctl.\n");
	return -EFAULT;
}

static long galois_pwmch_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	unsigned int pwmch = (unsigned int)file->private_data;
	galois_pwm_rw_t pwm_rw;

	if ((pwmch >= GALOIS_PWMCH_NUM) || (opened_pwmch[pwmch] == 0)) {
		printk("Oops: bad channel, or closed channel %d.\n", pwmch);
		return -ENODEV;
	}

	switch(cmd) {
		case PWM_IOCTL_READ:
			if (copy_from_user(&pwm_rw, (void __user *)arg, sizeof(galois_pwm_rw_t))) {
				printk("PWM: copy_from_user failed\n");
				return -EFAULT;
			}
			if (((pwm_rw.offset & 0x3) != 0) || (pwm_rw.offset > 0xC)) {
				printk("PWM: error register offset 0x%x.\n", pwm_rw.offset);
				return -ENOMEM;
			}
			GA_REG_WORD32_READ(pwmch_base + (pwmch*0x10) + pwm_rw.offset, &pwm_rw.data);
			if (copy_to_user((void __user *)arg, &pwm_rw, sizeof(galois_pwm_rw_t))) {
				printk("PWM: copy_to_user failed\n");
				return -EFAULT;
			}
			break;
		case PWM_IOCTL_WRITE:
			if (copy_from_user(&pwm_rw, (void __user *)arg, sizeof(galois_pwm_rw_t))) {
				printk("PWM: copy_from_user failed\n");
				return -EFAULT;
			}
			if (((pwm_rw.offset & 0x3) != 0) || (pwm_rw.offset > 0xC)) {
				printk("PWM: error register offset 0x%x.\n", pwm_rw.offset);
				return -ENOMEM;
			}
			GA_REG_WORD32_WRITE(pwmch_base + (pwmch*0x10) + pwm_rw.offset, pwm_rw.data);
			break;
		default:
			printk("PWM: unknown ioctl command.\n");
			return -EINVAL;
	}
	return 0;
}

static struct file_operations galois_pwmch_fops = {
	.owner		= THIS_MODULE,
	.open		= galois_pwmch_open,
	.release	= galois_pwmch_release,
	.write		= galois_pwmch_write,
	.read		= galois_pwmch_read,
	.unlocked_ioctl	= galois_pwmch_ioctl,
};

/*
 * PWMCH controller list
 */
static struct {
	uint minor;
	char *name;
} pwmchdev_list[] = {
	{GALOIS_PWMCH0_MINOR, GALOIS_PWMCH0_NAME},
	{GALOIS_PWMCH1_MINOR, GALOIS_PWMCH1_NAME},
	{GALOIS_PWMCH2_MINOR, GALOIS_PWMCH2_NAME},
	{GALOIS_PWMCH3_MINOR, GALOIS_PWMCH3_NAME},
};

/* PWMCH may be needed to bring up other drivers */
static struct class *pwmch_class;
#ifdef CONFIG_PM
struct saved_pwm_status_t {
	u32 enable;
	u32 control;
	u32 duty;
	u32 termcnt;
};

static struct saved_pwm_status_t saved_pwm_status[GALOIS_PWMCH_NUM];

static int pwmch_suspend(struct device *dev)
{
	/* save opened pwmchs' status */
	int i;

	memset(saved_pwm_status, 0x0, sizeof(struct saved_pwm_status_t) * GALOIS_PWMCH_NUM);

	for ( i = 0; i < GALOIS_PWMCH_NUM; i++)
		if (opened_pwmch[i]) {
			printk("backup pwm status for channel %d\n", i);
			GA_REG_WORD32_READ(pwmch_base + (i * 0x10) + ENABLE_REG_OFFSET, &saved_pwm_status[i].enable);
			GA_REG_WORD32_READ(pwmch_base + (i * 0x10) + CONTROL_REG_OFFSET, &saved_pwm_status[i].control);
			GA_REG_WORD32_READ(pwmch_base + (i * 0x10) + DUTY_REG_OFFSET, &saved_pwm_status[i].duty);
			GA_REG_WORD32_READ(pwmch_base + (i * 0x10) + TERMCNT_REG_OFFSET, &saved_pwm_status[i].termcnt);
		}

	return 0;
}

static int pwmch_resume(struct device *dev)
{
	/* restore opened pwmchs' status */
	int i;
	for ( i = 0; i < GALOIS_PWMCH_NUM; i++)
		if (opened_pwmch[i]) {
			printk("restore pwm status for channel %d\n", i);
			GA_REG_WORD32_WRITE(pwmch_base + (i * 0x10) + ENABLE_REG_OFFSET, saved_pwm_status[i].enable);
			GA_REG_WORD32_WRITE(pwmch_base + (i * 0x10) + CONTROL_REG_OFFSET, saved_pwm_status[i].control);
			GA_REG_WORD32_WRITE(pwmch_base + (i * 0x10) + DUTY_REG_OFFSET, saved_pwm_status[i].duty);
			GA_REG_WORD32_WRITE(pwmch_base + (i * 0x10) + TERMCNT_REG_OFFSET, saved_pwm_status[i].termcnt);
		}

	return 0;
}

static struct dev_pm_ops pwmch_pm_ops = {
	.suspend	= pwmch_suspend,
	.resume		= pwmch_resume,
};
#endif

static struct platform_driver pwmch_drv = {
	.driver = {
		.name	= "mv88de3100_pwm",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &pwmch_pm_ops,
#endif
	},
};

static struct platform_device pwmch_dev = {
	.name = "mv88de3100_pwm",
};

static int __init pwmch_dummy_probe(struct platform_device *dev)
{
	if (dev == &pwmch_dev)
		return 0;
	return -ENODEV;
}

static int __init galois_pwmch_init(void)
{
	int i;

	for (i = 0; i < GALOIS_PWMCH_NUM; i++)
		opened_pwmch[i] = 0;

	/* register char device */
	printk("PWM register major %d\n", GALOIS_PWMCH_MAJOR);
	if (register_chrdev(GALOIS_PWMCH_MAJOR, "pwmch", &galois_pwmch_fops))
		printk("unable to get major %d for pwmch.\n", GALOIS_PWMCH_MAJOR);

	pwmch_class = class_create(THIS_MODULE, "pwmch");
	for (i = 0; i < ARRAY_SIZE(pwmchdev_list); i++) {
		printk("minor=%d\n", pwmchdev_list[i].minor);
		device_create(pwmch_class, NULL,
			MKDEV(GALOIS_PWMCH_MAJOR, pwmchdev_list[i].minor), NULL, pwmchdev_list[i].name);
	}

	platform_device_register(&pwmch_dev);
	platform_driver_probe(&pwmch_drv, pwmch_dummy_probe);

	return 0;
}

static void __exit galois_pwmch_exit(void)
{
	int i;

	for (i = 0; i < GALOIS_PWMCH_NUM; i++)
		if (opened_pwmch[i]) {
			printk("PWM channel %d is still opened, exit with error.\n", i);
			return;
		}


	for (i = 0; i < ARRAY_SIZE(pwmchdev_list); i++) {
		printk("minor=%d\n", pwmchdev_list[i].minor);
		device_destroy(pwmch_class, MKDEV(GALOIS_PWMCH_MAJOR, pwmchdev_list[i].minor));
	}
	class_destroy(pwmch_class);

	printk("PWM unregister major %d\n", GALOIS_PWMCH_MAJOR);
	unregister_chrdev(GALOIS_PWMCH_MAJOR, "pwmch");

	platform_device_unregister(&pwmch_dev);
	platform_driver_unregister(&pwmch_drv);
}

module_init(galois_pwmch_init);
module_exit(galois_pwmch_exit);

MODULE_AUTHOR("Marvell-Galois");
MODULE_DESCRIPTION("Galois PWMCH Driver");
MODULE_LICENSE("GPL");
