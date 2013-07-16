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
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>

#include <mach/twsi_bus.h>

#include "i2c_master.h"
#include "i2c_bus.h"

static int mv88de3100_i2c_doxfer(struct i2c_adapter *adap, struct i2c_msg *msgs)
{
	unsigned int addr = (msgs->addr & 0x7f) << 1;
	int res;

	if (msgs->flags & I2C_M_RD) {
		addr |= 1;
		res = galois_twsi_i2c_writeread(adap->nr, addr,
						TWSI_7BIT_SLAVE_ADDR, NULL, 0,
						msgs->buf, msgs->len);
	} else
		res = galois_twsi_i2c_writeread(adap->nr, addr,
						TWSI_7BIT_SLAVE_ADDR,
						msgs->buf, msgs->len,
						NULL, 0);
	return res;
}

static int mv88de3100_i2c_xfer(struct i2c_adapter *adap,
			struct i2c_msg *msgs, int num)
{
	int res, i;

	for (i = 0; i < num; i++) {
		res = mv88de3100_i2c_doxfer(adap, &(msgs[i]));
		if (res < 0)
			return res;
	}

	return 0;
}

/* declare our i2c functionality */
static u32 mv88de3100_i2c_func(struct i2c_adapter *adap)
{
        return I2C_FUNC_I2C /*| I2C_FUNC_10BIT_ADDR*/;
}

static const struct i2c_algorithm mv88de3100_i2c_algo = {
        .master_xfer            = mv88de3100_i2c_xfer,
        .functionality          = mv88de3100_i2c_func,
};

static int __devinit mv88de3100_i2c_probe(struct platform_device *dev)
{
	struct i2c_adapter *adap;
	int ret;

	adap = kzalloc(sizeof(*adap), GFP_KERNEL);
	if (!adap)
		return -ENOMEM;

	adap->owner = THIS_MODULE;
	adap->algo  = &mv88de3100_i2c_algo;
	adap->algo_data = adap;
	strlcpy(adap->name, "MV88DE3100 I2C Adaptor", sizeof(adap->name));
	adap->dev.parent = &dev->dev;
	if (dev->id >= 0) {
		/* static adapter numbering */
		adap->nr = dev->id;
		ret = i2c_add_numbered_adapter(adap);
	} else
		/* dynamic adapter numbering */
		ret = i2c_add_adapter(adap);
	if (ret < 0) {
		printk(KERN_ERR "failed to register i2c adapter\n");
		kfree(adap);
		return ret;
	}

	ret = galois_twsi_i2c_open(adap->nr);
	if (ret < 0) {
		printk(KERN_ERR "failed to open galois i2c adapter\n");
		i2c_del_adapter(adap);
		kfree(adap);
		return ret;
	}

	platform_set_drvdata(dev, adap);

	printk(KERN_INFO "i2c adapter %d added\n", adap->nr);

	return 0;
}

static int __devexit mv88de3100_i2c_remove(struct platform_device *dev)
{
	struct i2c_adapter *adap = platform_get_drvdata(dev);
	if (adap) {
		platform_set_drvdata(dev, NULL);
		i2c_del_adapter(adap);
		galois_twsi_i2c_close(adap->nr);
		kfree(adap);
	}
	return 0;
}

static struct platform_driver mv88de3100_i2c_driver = {
	.probe		= mv88de3100_i2c_probe,
	.remove		= mv88de3100_i2c_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "mv88de3100-i2c",
	},
};

static int __init i2c_adap_mv88de3100_init(void)
{
	return platform_driver_register(&mv88de3100_i2c_driver);
}

static void __exit i2c_adap_mv88de3100_exit(void)
{
	platform_driver_unregister(&mv88de3100_i2c_driver);
}

module_init(i2c_adap_mv88de3100_init);
module_exit(i2c_adap_mv88de3100_exit);

MODULE_DESCRIPTION("MV88DE3100 Linux I2C Bus driver");
MODULE_AUTHOR("YH, <zhengyh@marvell.com>");
MODULE_LICENSE("GPL");
