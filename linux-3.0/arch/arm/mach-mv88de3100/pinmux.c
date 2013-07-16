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
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/galois_generic.h>
#include <mach/kpinmux.h>
#include <mach/galois_platform.h>


/* Pinmux driver */

#define PINMUX_PRINT printk

static DEFINE_SPINLOCK(pinmux_lock);

#define	FirstGrpLastID				12

#define	SOC_NUM_PINMUX_GRPS			29
#define	SM_NUM_PINMUX_GRPS			12


static char soc_pinmux_set[SOC_NUM_PINMUX_GRPS];
static char sm_pinmux_set[SM_NUM_PINMUX_GRPS];

static unsigned int soc_pinmuxtab[SOC_NUM_PINMUX_GRPS][3] = {
	{MSK32Gbl_pinMux_gp0, LSb32Gbl_pinMux_gp0, bGbl_pinMux_gp0},
	{MSK32Gbl_pinMux_gp1, LSb32Gbl_pinMux_gp1, bGbl_pinMux_gp1},
	{MSK32Gbl_pinMux_gp2, LSb32Gbl_pinMux_gp2, bGbl_pinMux_gp2},
	{MSK32Gbl_pinMux_gp3, LSb32Gbl_pinMux_gp3, bGbl_pinMux_gp3},
	{MSK32Gbl_pinMux_gp4, LSb32Gbl_pinMux_gp4, bGbl_pinMux_gp4},
	{MSK32Gbl_pinMux_gp5, LSb32Gbl_pinMux_gp5, bGbl_pinMux_gp5},
	{MSK32Gbl_pinMux_gp6, LSb32Gbl_pinMux_gp6, bGbl_pinMux_gp6},
	{MSK32Gbl_pinMux_gp7, LSb32Gbl_pinMux_gp7, bGbl_pinMux_gp7},
	{MSK32Gbl_pinMux_gp8, LSb32Gbl_pinMux_gp8, bGbl_pinMux_gp8},
	{MSK32Gbl_pinMux_gp9, LSb32Gbl_pinMux_gp9, bGbl_pinMux_gp9},
	{MSK32Gbl_pinMux_gp10, LSb32Gbl_pinMux_gp10, bGbl_pinMux_gp10},
	{MSK32Gbl_pinMux_gp11, LSb32Gbl_pinMux_gp11, bGbl_pinMux_gp11},
	{MSK32Gbl_pinMux_gp12, LSb32Gbl_pinMux_gp12, bGbl_pinMux_gp12},
	{MSK32Gbl_pinMux_gp13, LSb32Gbl_pinMux_gp13, bGbl_pinMux_gp13},
	{MSK32Gbl_pinMux_gp14, LSb32Gbl_pinMux_gp14, bGbl_pinMux_gp14},
	{MSK32Gbl_pinMux_gp15, LSb32Gbl_pinMux_gp15, bGbl_pinMux_gp15},
	{MSK32Gbl_pinMux_gp16, LSb32Gbl_pinMux_gp16, bGbl_pinMux_gp16},
	{MSK32Gbl_pinMux_gp17, LSb32Gbl_pinMux_gp17, bGbl_pinMux_gp17},
	{MSK32Gbl_pinMux_gp18, LSb32Gbl_pinMux_gp18, bGbl_pinMux_gp18},
	{MSK32Gbl_pinMux_gp19, LSb32Gbl_pinMux_gp19, bGbl_pinMux_gp19},
	{MSK32Gbl_pinMux_gp20, LSb32Gbl_pinMux_gp20, bGbl_pinMux_gp20},
	{MSK32Gbl_pinMux_gp21, LSb32Gbl_pinMux_gp21, bGbl_pinMux_gp21},
	{MSK32Gbl_pinMux_gp22, LSb32Gbl_pinMux_gp22, bGbl_pinMux_gp22},
	{MSK32Gbl_pinMux_gp23, LSb32Gbl_pinMux_gp23, bGbl_pinMux_gp23},
	{MSK32Gbl_pinMux_gp24, LSb32Gbl_pinMux_gp24, bGbl_pinMux_gp24},
	{MSK32Gbl_pinMux_gp25, LSb32Gbl_pinMux_gp25, bGbl_pinMux_gp25},
	{MSK32Gbl_pinMux_gp26, LSb32Gbl_pinMux_gp26, bGbl_pinMux_gp26},
	{MSK32Gbl_pinMux_gp27, LSb32Gbl_pinMux_gp27, bGbl_pinMux_gp27},
	{MSK32Gbl_pinMux_gp28, LSb32Gbl_pinMux_gp28, bGbl_pinMux_gp28},
};

static unsigned int sm_pinmuxtab[SM_NUM_PINMUX_GRPS][3] = {
	{MSK32smSysCtl_SM_GSM_SEL_GSM0_SEL, LSb32smSysCtl_SM_GSM_SEL_GSM0_SEL, bsmSysCtl_SM_GSM_SEL_GSM0_SEL},
	{MSK32smSysCtl_SM_GSM_SEL_GSM1_SEL, LSb32smSysCtl_SM_GSM_SEL_GSM1_SEL, bsmSysCtl_SM_GSM_SEL_GSM1_SEL},
	{MSK32smSysCtl_SM_GSM_SEL_GSM2_SEL, LSb32smSysCtl_SM_GSM_SEL_GSM2_SEL, bsmSysCtl_SM_GSM_SEL_GSM2_SEL},
	{MSK32smSysCtl_SM_GSM_SEL_GSM3_SEL, LSb32smSysCtl_SM_GSM_SEL_GSM3_SEL, bsmSysCtl_SM_GSM_SEL_GSM3_SEL},
	{MSK32smSysCtl_SM_GSM_SEL_GSM4_SEL, LSb32smSysCtl_SM_GSM_SEL_GSM4_SEL, bsmSysCtl_SM_GSM_SEL_GSM4_SEL},
	{MSK32smSysCtl_SM_GSM_SEL_GSM5_SEL, LSb32smSysCtl_SM_GSM_SEL_GSM5_SEL, bsmSysCtl_SM_GSM_SEL_GSM5_SEL},
	{MSK32smSysCtl_SM_GSM_SEL_GSM6_SEL, LSb32smSysCtl_SM_GSM_SEL_GSM6_SEL, bsmSysCtl_SM_GSM_SEL_GSM6_SEL},
	{MSK32smSysCtl_SM_GSM_SEL_GSM7_SEL, LSb32smSysCtl_SM_GSM_SEL_GSM7_SEL, bsmSysCtl_SM_GSM_SEL_GSM7_SEL},
	{MSK32smSysCtl_SM_GSM_SEL_GSM8_SEL, LSb32smSysCtl_SM_GSM_SEL_GSM8_SEL, bsmSysCtl_SM_GSM_SEL_GSM8_SEL},
	{MSK32smSysCtl_SM_GSM_SEL_GSM9_SEL, LSb32smSysCtl_SM_GSM_SEL_GSM9_SEL, bsmSysCtl_SM_GSM_SEL_GSM9_SEL},
	{MSK32smSysCtl_SM_GSM_SEL_GSM10_SEL, LSb32smSysCtl_SM_GSM_SEL_GSM10_SEL, bsmSysCtl_SM_GSM_SEL_GSM10_SEL},
	{MSK32smSysCtl_SM_GSM_SEL_GSM11_SEL, LSb32smSysCtl_SM_GSM_SEL_GSM11_SEL, bsmSysCtl_SM_GSM_SEL_GSM11_SEL},
};

static ssize_t galois_pinmux_read(struct file *file, char __user * buf,
		size_t count, loff_t *ppos)
{
	printk("error: please use ioctl.\n");
	return -EFAULT;
}

static ssize_t galois_pinmux_write(struct file *file, const char __user * buf,
		size_t count, loff_t *ppos)
{
	printk("error: please use ioctl.\n");
	return -EFAULT;
}

static void pinmux_print_stat(void)
{
	T32Gbl_pinMux pinmux_soc;
	T32Gbl_pinMux1 pinmux_soc1;
	T32smSysCtl_SM_GSM_SEL pinmux_sm;

	pinmux_soc.u32 = readl(MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_pinMux);
	pinmux_soc1.u32 = readl(MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_pinMux1);
	pinmux_sm.u32 = readl(SM_SYS_CTRL_REG_BASE + RA_smSysCtl_SM_GSM_SEL);

	printk("current setting: %x:%x:%x\n", pinmux_soc.u32, pinmux_soc1.u32,
		pinmux_sm.u32);

}

static int soc_pinmuxsetgrp(int grpID, int mode)
{
	int pinmux, mask, shift, bits;
	unsigned int offset;
	unsigned int curr_value;

	if (grpID >= SOC_NUM_PINMUX_GRPS)
		return -1;

	mask = soc_pinmuxtab[grpID][0];
	shift = soc_pinmuxtab[grpID][1];
	bits = soc_pinmuxtab[grpID][2];
	if (mode >= (0x1<<bits))
		return -1;
	if (grpID <= FirstGrpLastID) {
		printk("change pinmux register RA_Gbl_pinMux\n");
		offset = RA_Gbl_pinMux;	//in first grp
	} else {
		printk("change pinmux register RA_Gbl_pinMux1\n");
		offset = RA_Gbl_pinMux1; //in second grp
	}

	pinmux = readl(MEMMAP_CHIP_CTRL_REG_BASE + offset);
	curr_value = (pinmux & mask)>>shift;
	if (curr_value == mode) {
		printk("current group value is %d, do not need to change\n",
				curr_value);
		return 0;			//	do not need to change
	}

	printk(" current value is: 0x%x\n", pinmux);
	pinmux &= (~(mask));
	pinmux |= (mode << shift);
	writel(pinmux, MEMMAP_CHIP_CTRL_REG_BASE+ offset);
	soc_pinmux_set[grpID] = 1;

	printk(" after change, the value is 0x%x\n", pinmux);
	return 0;
}

static int sm_pinmuxsetgrp(int grpID, int mode)
{
	int pinmux, mask, shift, bits;
	unsigned int curr_value;

	if (grpID >= SM_NUM_PINMUX_GRPS)
		return -1; // void groups

	mask = sm_pinmuxtab[grpID][0];
	shift = sm_pinmuxtab[grpID][1];
	bits = sm_pinmuxtab[grpID][2];
	if (mode >= (0x1<<bits))
		return -1;
	printk("change SM pinmux register\n");

	pinmux = readl(SM_SYS_CTRL_REG_BASE + RA_smSysCtl_SM_GSM_SEL);
	curr_value = (pinmux & mask)>>shift;
	if (curr_value == mode) {
		printk("current group value is %d, do not need to change\n",
			curr_value);
		return 0;			//	do not need to change
	}

	printk(" current value is: 0x%x\n", pinmux);
	pinmux &= (~(mask));
	pinmux |= (mode << shift);
	writel(pinmux, SM_SYS_CTRL_REG_BASE + RA_smSysCtl_SM_GSM_SEL);
	sm_pinmux_set[grpID] = 1;

	printk(" after change, the value is 0x%x\n", pinmux);
	return 0;

}


static int soc_pinmuxgetgrp(int grpID, int *mode)
{
	int pinmux, mask, shift, bits;
	unsigned int offset;

	if (grpID >= SOC_NUM_PINMUX_GRPS)
		return -1; // void groups

	mask = soc_pinmuxtab[grpID][0];
	shift = soc_pinmuxtab[grpID][1];
	bits = soc_pinmuxtab[grpID][2];

	if (grpID <= FirstGrpLastID)
		offset = RA_Gbl_pinMux;	//in first grp
	else
		offset = RA_Gbl_pinMux1; //in second grp

	pinmux = readl(MEMMAP_CHIP_CTRL_REG_BASE + offset);
	pinmux &= mask;
	pinmux = (pinmux >> shift);
	pinmux &= ((0x1 <<bits) -1);
	*mode = pinmux;
	return 0;
}

static int sm_pinmuxgetgrp(int grpID, int *mode)
{
	int pinmux, mask, shift, bits;

	if (grpID >= SM_NUM_PINMUX_GRPS)
		return -1; // void groups

	mask = sm_pinmuxtab[grpID][0];
	shift = sm_pinmuxtab[grpID][1];
	bits = sm_pinmuxtab[grpID][2];

	pinmux = readl(SM_SYS_CTRL_REG_BASE + RA_smSysCtl_SM_GSM_SEL);

	pinmux &= mask;
	pinmux = (pinmux >> shift);
	pinmux &= ((0x1 <<bits) -1);
	*mode = pinmux;

	return 0;

}

static long galois_pinmux_ioctl(struct file *file, unsigned int cmd,
			unsigned long arg)
{
	galois_pinmux_data_t pinmux_data;
	char requester_name[256];
	int requester_len;
	int ret = 0;

	switch(cmd) {
	case PINMUX_IOCTL_SETMOD:
		if (copy_from_user(&pinmux_data, (void __user *)arg,
				  sizeof(galois_pinmux_data_t)))
			return -EFAULT;

		requester_len = strnlen_user(pinmux_data.requster, 256);
		if (!requester_len)
			printk("no requester name for pinmux ioctl\n");
		else {
			memset(requester_name, 0, 256);
			if(copy_from_user(requester_name,
					(void __user *)pinmux_data.requster,
					requester_len))
				return -EFAULT;

			if (pinmux_data.owner)
				printk("%s wants to set SM pinmux group %d to %d\n",
					requester_name, pinmux_data.group,
					pinmux_data.value);
			else
				printk("%s want to set SOC pinmux group %d to %d\n",
					requester_name, pinmux_data.group,
					pinmux_data.value);
		}

		spin_lock(&pinmux_lock);
		if (!pinmux_data.owner) {
			if (soc_pinmuxsetgrp(pinmux_data.group, pinmux_data.value)) {
				printk("Warning: failed to set pinmux\n");
				ret = -EFAULT;
			}
		} else {
			if (sm_pinmuxsetgrp(pinmux_data.group, pinmux_data.value)) {
				printk("Warning: failed to set pinmux\n");
				ret = -EFAULT;
			}
		}
		spin_unlock(&pinmux_lock);
		return ret;

	case PINMUX_IOCTL_GETMOD:
		printk("GETMOD ioctl for pinmux\n");

		if (copy_from_user(&pinmux_data, (void __user *)arg,
				  sizeof(galois_pinmux_data_t)))
			return -EFAULT;

		spin_lock(&pinmux_lock);
		if (!pinmux_data.owner) {
			if(soc_pinmuxgetgrp(pinmux_data.group, &pinmux_data.value)){
				printk("Warning: failed to read pinmux for group %d\n",
						pinmux_data.group);
				ret = -EFAULT;
			}
		} else {
			if(sm_pinmuxgetgrp(pinmux_data.group, &pinmux_data.value)){
				printk("Warning: failed to read pinmux for group %d\n",
						pinmux_data.group);
				ret = -EFAULT;
			}
		}
		printk("Getmod: value for group %d is %d\n",
			pinmux_data.group, pinmux_data.value);
		if (copy_to_user((void __user *)arg, &pinmux_data,
				sizeof(galois_pinmux_data_t))) {
			printk("copy to user error\n");
			ret = -EFAULT;
		}
		spin_unlock(&pinmux_lock);
		return ret;

	case PINMUX_IOCTL_PRINTSTATUS:
		printk("PRINTSTATUS ioctl for pinmux");
		spin_lock(&pinmux_lock);
		pinmux_print_stat();
		spin_unlock(&pinmux_lock);
		break;

	default:
		printk("unknown ioctl for pinmux\n");
		return -EPERM;
	}
	return ret;
}

static int galois_pinmux_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "galois_pinmux_open\n");
	return 0;
}

static struct file_operations galois_pinmux_fops = {
	.owner		= THIS_MODULE,
	.open		= galois_pinmux_open,
	.write		= galois_pinmux_write,
	.read		= galois_pinmux_read,
	.unlocked_ioctl	= galois_pinmux_ioctl,
};

static struct miscdevice pinmux_dev = {
	.minor	= GALOIS_PINMUX_MINOR,
	.name	= GALOIS_PINMUX_NAME,
	.fops	= &galois_pinmux_fops,
};

static int __init galois_pinmux_init(void)
{
	int i;
	T32Gbl_pinMux pinmux_soc;
	T32Gbl_pinMux1 pinmux_soc1;
	T32smSysCtl_SM_GSM_SEL pinmux_sm;

	printk("galois_pinmux_init\n");
	/* by default, no one has changed the pinmux */
	for (i = 0; i < SOC_NUM_PINMUX_GRPS; i++)
		soc_pinmux_set[i]=0;
	for (i = 0; i < SM_NUM_PINMUX_GRPS; i++)
		sm_pinmux_set[i]=0;

	if (misc_register(&pinmux_dev))
		return -ENODEV;

	/* print out current pinmux setting */
	pinmux_soc.u32 = readl(MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_pinMux);
	pinmux_soc1.u32 = readl(MEMMAP_CHIP_CTRL_REG_BASE + RA_Gbl_pinMux1);
	pinmux_sm.u32 = readl(SM_SYS_CTRL_REG_BASE + RA_smSysCtl_SM_GSM_SEL);

	printk("current setting: %x:%x:%x\n", pinmux_soc.u32, pinmux_soc1.u32,
		pinmux_sm.u32);
	return 0;
}

static void __exit galois_pinmux_exit(void)
{
	printk("galois_pinmux_exit\n");
	misc_deregister(&pinmux_dev);
}

module_init(galois_pinmux_init);
module_exit(galois_pinmux_exit);
