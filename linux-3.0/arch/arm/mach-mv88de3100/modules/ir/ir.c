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
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/input.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <mach/galois_generic.h>
#include <mach/sm.h>
#include <mach/ir_key_def.h>

static struct {
	MV_IR_KEY_CODE_t ir_key;
	int input_key;
} keymap_tab[] = {
	{MV_IR_KEY_NULL,		KEY_RESERVED},	/* no key */
	{MV_IR_KEY_POWER,		KEY_POWER},
	{MV_IR_KEY_OPENCLOSE,		KEY_OPEN},

	/* digital keys */
	{MV_IR_KEY_DIGIT_1,		KEY_1},
	{MV_IR_KEY_DIGIT_2,		KEY_2},
	{MV_IR_KEY_DIGIT_3,		KEY_3},
	{MV_IR_KEY_DIGIT_4,		KEY_4},
	{MV_IR_KEY_DIGIT_5,		KEY_5},
	{MV_IR_KEY_DIGIT_6,		KEY_6},
	{MV_IR_KEY_DIGIT_7,		KEY_7},
	{MV_IR_KEY_DIGIT_8,		KEY_8},
	{MV_IR_KEY_DIGIT_9,		KEY_9},
	{MV_IR_KEY_DIGIT_0,		KEY_0},

	/* for BD */
	{MV_IR_KEY_INFO,		KEY_INFO},
	{MV_IR_KEY_SETUPMENU,		KEY_HOME},
	{MV_IR_KEY_CANCEL,		KEY_CANCEL},	/* no key */

	{MV_IR_KEY_DISCMENU,		KEY_CONTEXT_MENU},
	{MV_IR_KEY_TITLEMENU,		KEY_TITLE},
	{MV_IR_KEY_SUBTITLE,		KEY_SUBTITLE},
	{MV_IR_KEY_ANGLE,		KEY_ANGLE},
	{MV_IR_KEY_AUDIO,		KEY_AUDIO},
	{MV_IR_KEY_SEARCH,		KEY_SEARCH},
	{MV_IR_KEY_ZOOM,		KEY_ZOOM},
	{MV_IR_KEY_DISPLAY,		KEY_SCREEN},

	{MV_IR_KEY_REPEAT,		KEY_MEDIA_REPEAT},
	{MV_IR_KEY_REPEAT_AB,		KEY_AB},
	{MV_IR_KEY_MARKER,		KEY_MARKER},
	{MV_IR_KEY_EXIT,		KEY_EXIT},
	{MV_IR_KEY_A,			KEY_RED},
	{MV_IR_KEY_B,			KEY_GREEN},
	{MV_IR_KEY_C,			KEY_YELLOW},
	{MV_IR_KEY_D,			KEY_BLUE},

	/* IR misc around ENTER */
	{MV_IR_KEY_CLEAR,		KEY_CLEAR},
	{MV_IR_KEY_VIDEO_FORMAT,	KEY_VIDEO},
	{MV_IR_KEY_STEP,		KEY_STEP},
	{MV_IR_KEY_RETURN,		KEY_ESC},

	/* up down left right enter */
	{MV_IR_KEY_UP,			KEY_UP},
	{MV_IR_KEY_DOWN,		KEY_DOWN},
	{MV_IR_KEY_LEFT,		KEY_LEFT},
	{MV_IR_KEY_RIGHT,		KEY_RIGHT},
	{MV_IR_KEY_ENTER,		KEY_ENTER},

	/* for BD */
	{MV_IR_KEY_SLOW,		KEY_SLOW},
	{MV_IR_KEY_PAUSE,		KEY_PAUSE},
	{MV_IR_KEY_PLAY,		KEY_PLAY},
	{MV_IR_KEY_STOP,		KEY_STOP},
	{MV_IR_KEY_PLAY_PAUSE,		KEY_PLAYPAUSE},	/* no key */

	{MV_IR_KEY_SKIP_BACKWARD,	KEY_PREVIOUS},
	{MV_IR_KEY_SKIP_FORWARD,	KEY_NEXT},
	{MV_IR_KEY_SLOW_BACKWARD,	KEY_RESERVED},	/* no key */
	{MV_IR_KEY_SLOW_FORWARD,	KEY_RESERVED},	/* no key */
	{MV_IR_KEY_FAST_BACKWARD,	KEY_REWIND},
	{MV_IR_KEY_FAST_FORWARD,	KEY_FORWARD},

	/* bottom keys */
	{MV_IR_KEY_F1,			KEY_F1},
	{MV_IR_KEY_F2,			KEY_F2},
	{MV_IR_KEY_F3,			KEY_F3},
	{MV_IR_KEY_F4,			KEY_F4},
	{MV_IR_KEY_F5,			KEY_F5},
	{MV_IR_KEY_F6,			KEY_F6},
	{MV_IR_KEY_F7,			KEY_F7},
	{MV_IR_KEY_F8,			KEY_F8},

	/* for future */
	{MV_IR_KEY_VOL_PLUS,		KEY_VOLUMEUP},		/* no key */
	{MV_IR_KEY_VOL_MINUS,		KEY_VOLUMEDOWN},	/* no key */
	{MV_IR_KEY_VOL_MUTE,		KEY_MUTE},		/* no key */
	{MV_IR_KEY_CHANNEL_PLUS,	KEY_CHANNELUP},		/* no key */
	{MV_IR_KEY_CHANNEL_MINUS,	KEY_CHANNELDOWN},	/* no key */

	/* obsoleted keys */
	{MV_IR_KEY_MENU,		KEY_MENU},
	{MV_IR_KEY_INPUTSEL,		KEY_RESERVED},
	{MV_IR_KEY_ANYNET,		KEY_RESERVED},
	{MV_IR_KEY_TELEVISION,		KEY_TV},
	{MV_IR_KEY_CHANNEL_LIST,	KEY_RESERVED},
	{MV_IR_KEY_TVPOWER,		KEY_RESERVED},
};

#define MAX_LEN 10

static DECLARE_WAIT_QUEUE_HEAD(girl_wait);

static DEFINE_SPINLOCK(my_lock);

static struct {
	int number;
	int girl_buf[MAX_LEN];
	int in;
	int out;
} mygirl;

static void girl_key_buf_write(int key)
{
	spin_lock(&my_lock);
	mygirl.girl_buf[mygirl.in] = key;
	mygirl.in++;
	mygirl.number++;
	if (mygirl.in == MAX_LEN)
		mygirl.in = 0;
	spin_unlock(&my_lock);
}

static int girl_key_buf_read(void)
{
	int key;
	spin_lock(&my_lock);
	key = mygirl.girl_buf[mygirl.out];
	mygirl.out++;
	mygirl.number--;
	if (mygirl.out == MAX_LEN)
		mygirl.out = 0;
	spin_unlock(&my_lock);
	return key;
}

static struct input_dev *ir_input = NULL;

static int ir_input_open(struct input_dev *dev)
{
	int msg;
	msg = MV_SM_IR_Linuxready;
	bsm_msg_send(MV_SM_ID_IR, &msg, 4);
	return 0;
}

static void ir_report_key(int ir_key)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(keymap_tab); i++) {
		if (keymap_tab[i].ir_key == (ir_key & 0xFFFF)) {
			if (ir_key & 0x80000000)
				input_event(ir_input, EV_KEY, keymap_tab[i].input_key, 2);
			else
				input_event(ir_input, EV_KEY, keymap_tab[i].input_key, !(ir_key & 0x8000000));
			input_sync(ir_input);
		}
	}
}

/*
 * when sm receives msg and the msg is to ir, SM ISR will call this function.
 * SM needs to work under interrupt mode
 */
void girl_sm_int(void)
{
	int msg[8], len, ret, ir_key;

	ret = bsm_msg_receive(MV_SM_ID_IR, msg, &len);

	if(ret != 0 ||  len != 4)
		return;

	ir_key = msg[0];
	ir_report_key(ir_key);
	girl_key_buf_write(ir_key);
	wake_up_interruptible(&girl_wait);
}

static int galois_ir_open(struct inode *inode, struct file *file)
{
	int msg;

	msg = MV_SM_IR_Linuxready;
	bsm_msg_send(MV_SM_ID_IR, &msg, 4);

	return 0;
}


static ssize_t galois_ir_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	int ret;
	int key;

	key = girl_key_buf_read();
	ret = put_user(key, (unsigned int __user *)buf);

	if (ret)
		return ret;
	else
		return sizeof(key);
}

static ssize_t galois_ir_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	printk("error: doesn't support write.\n");
	return -EFAULT;
}

static unsigned int galois_ir_poll(struct file *file, poll_table *wait)
{
	int tmp;
	spin_lock(&my_lock);
	tmp = mygirl.number;
	spin_unlock(&my_lock);
	if (!tmp)
		poll_wait(file, &girl_wait, wait);
	spin_lock(&my_lock);
	tmp = mygirl.number;
	spin_unlock(&my_lock);
	if (tmp)
		return (POLLIN | POLLRDNORM);
	return 0;
}

static struct file_operations galois_ir_fops = {
	.owner		= THIS_MODULE,
	.open		= galois_ir_open,
	.write		= galois_ir_write,
	.read		= galois_ir_read,
	.poll		= galois_ir_poll,
};

static struct miscdevice ir_dev = {
	.minor	= GALOIS_IR_MISC_MINOR,
	.name	= GALOIS_IR_NAME,
	.fops	= &galois_ir_fops,
};

#ifdef CONFIG_PM
static int ir_resume(struct device *dev)
{
	int msg = MV_SM_IR_Linuxready;

	bsm_msg_send(MV_SM_ID_IR, &msg, sizeof(msg));
	return 0;
}

static struct dev_pm_ops ir_pm_ops = {
	.resume		= ir_resume,
};
#endif

static struct platform_driver galois_ir_drv = {
	.driver = {
		.name	= "mv88de3100_ir",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &ir_pm_ops,
#endif
	},
};

static struct platform_device galois_ir_dev = {
	.name = "mv88de3100_ir",
};

static int __init ir_dummy_probe(struct platform_device *dev)
{
	if (dev == &galois_ir_dev)
		return 0;
	return -ENODEV;
}

static int __init galois_ir_init(void)
{
	int i, error;

	if (misc_register(&ir_dev))
		return -ENODEV;

	ir_input = input_allocate_device();
	if (!ir_input) {
		printk("error: failed to alloc input device for IR.\n");
		misc_deregister(&ir_dev);
		return -ENOMEM;
	}

	ir_input->name = "Inafra-Red";
	ir_input->phys = "Inafra-Red/input0";
	ir_input->id.bustype = BUS_HOST;
	ir_input->id.vendor = 0x0001;
	ir_input->id.product = 0x0001;
	ir_input->id.version = 0x0100;

	ir_input->open = ir_input_open;

	for(i = 0; i < ARRAY_SIZE(keymap_tab); i++)
		__set_bit(keymap_tab[i].input_key, ir_input->keybit);
	__set_bit(EV_KEY, ir_input->evbit);

	error = input_register_device(ir_input);
	if (error) {
		printk("error: failed to register input device for IR\n");
		misc_deregister(&ir_dev);
		input_free_device(ir_input);
		return error;
	}

	platform_device_register(&galois_ir_dev);
	platform_driver_probe(&galois_ir_drv, ir_dummy_probe);

	return 0;
}

static void __exit galois_ir_exit(void)
{
	misc_deregister(&ir_dev);
	input_unregister_device(ir_input);
	input_free_device(ir_input);
	platform_device_unregister(&galois_ir_dev);
	platform_driver_unregister(&galois_ir_drv);
}

module_init(galois_ir_init);
module_exit(galois_ir_exit);

MODULE_AUTHOR("Marvell-Galois");
MODULE_DESCRIPTION("Galois Infra-Red Driver");
MODULE_LICENSE("GPL");
