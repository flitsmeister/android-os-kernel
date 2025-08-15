// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>

#include <linux/gpio.h>
#include "fsa4480_i2c.h"
//#include "../../../../drivers/misc/mediatek/typec/tcpc/inc/tcpm.h"
//#include "../typec/tcpc/inc/tcpm.h"
//#include "mt6358-accdet.h"
//extern void mid_hp_detect(int state);
#define FSA4480_I2C_NAME	"fsa4480"

#define HL5280_DEVICE_REG_VALUE 0x49

#define FSA4480_DEVICE_ID  0x00
#define FSA4480_SWITCH_SETTINGS 0x04
#define FSA4480_SWITCH_CONTROL  0x05
#define FSA4480_SWITCH_STATUS0  0x06
#define FSA4480_SWITCH_STATUS1  0x07
#define FSA4480_SLOW_L          0x08
#define FSA4480_SLOW_R          0x09
#define FSA4480_SLOW_MIC        0x0A
#define FSA4480_SLOW_SENSE      0x0B
#define FSA4480_SLOW_GND        0x0C
#define FSA4480_DELAY_L_R       0x0D
#define FSA4480_DELAY_L_MIC     0x0E
#define FSA4480_DELAY_L_SENSE   0x0F
#define FSA4480_DELAY_L_AGND    0x10
#define FSA4480_FUN_EN          0x12
#define FSA4480_JACK_STATUS     0x17
#define FSA4480_RESET           0x1E
#define FSA4480_CURRENT_SOURCE_SETTING 0x1F

#define BCT4480_SWITCH_SETTINGS 0x04
#define BCT4480_SWITCH_CONTROL  0x05
#define BCT4480_SWITCH_STATUS1  0x07
#define BCT4480_SLOW_L          0x08
#define BCT4480_SLOW_R          0x09
#define BCT4480_SLOW_MIC        0x0A
#define BCT4480_SLOW_SENSE      0x0B
#define BCT4480_SLOW_GND        0x0C
#define BCT4480_DELAY_L_R       0x0D
#define BCT4480_DELAY_L_MIC     0x0E
#define BCT4480_DELAY_L_SENSE   0x0F
#define BCT4480_DELAY_L_AGND    0x10

#undef dev_dbg
#define dev_dbg dev_info

enum switch_vendor {
    BCT4480 = 0,
    HL5280
};
struct regmap *mid_regmap;//kzhkzh add for read and write interface!!!
struct device *mid_dev;

struct fsa4480_priv {
	struct regmap *regmap;
	struct device *dev;
	struct tcpc_device *tcpc_dev;
	struct notifier_block pd_nb;
	atomic_t usbc_mode;
	struct work_struct usbc_analog_work;
	struct blocking_notifier_head fsa4480_notifier;
	struct mutex notification_lock;
	int hs_det_pin;
	int irq;
	enum switch_vendor vendor;
	bool plug_state;
};

struct fsa4480_priv *fsa_priv;
	
struct fsa4480_reg_val {
	u16 reg;
	u8 val;
};

static const struct regmap_config fsa4480_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = FSA4480_CURRENT_SOURCE_SETTING,
};

static const struct fsa4480_reg_val fsa_reg_i2c_defaults[] = {
	{FSA4480_SLOW_L, 0x00},
	{FSA4480_SLOW_R, 0x00},
	{FSA4480_SLOW_MIC, 0x00},
	{FSA4480_SLOW_SENSE, 0x00},
	{FSA4480_SLOW_GND, 0x00},
	{FSA4480_DELAY_L_R, 0x00},
	{FSA4480_DELAY_L_MIC, 0x00},
	{FSA4480_DELAY_L_SENSE, 0x00},
	{FSA4480_DELAY_L_AGND, 0x09},
	{FSA4480_SWITCH_SETTINGS, 0x98},
};

static const struct fsa4480_reg_val bct_reg_i2c_defaults[] = {
	{BCT4480_SLOW_L, 0x00},
	{BCT4480_SLOW_R, 0x00},
	{BCT4480_SLOW_MIC, 0x00},
	{BCT4480_SLOW_SENSE, 0x00},
	{BCT4480_SLOW_GND, 0x00},
	{BCT4480_DELAY_L_R, 0x00},
	{BCT4480_DELAY_L_MIC, 0x00},
	{BCT4480_DELAY_L_SENSE, 0x00},
	{BCT4480_DELAY_L_AGND, 0x09},
	{BCT4480_SWITCH_SETTINGS, 0x98},
};

unsigned int mid_hl5280_read(u8 reg)//kzh add for codec read ,返回值为address对应value
{
		unsigned int val; 

		regmap_read(mid_regmap, reg, &val);
		printk(" kzhkzh HL5280 Read register reg:0x%x,value:0x%x\n",reg,val);
        return val;
}

static int mid_hl5280_write(u8 reg, unsigned char value)
{
		regmap_write(mid_regmap, reg, value);
		printk(" kzhkzh HL5280 Write register reg:0x%x,value:0x%x\n",reg,value);
        return 0;
}


void fsa4480_usbc_update_settings(u32 switch_control, u32 switch_enable)
{
	if(!mid_regmap) {
		printk("%s: regmap invalid\n", __func__);
		return;
	}

	regmap_write(mid_regmap, FSA4480_SWITCH_SETTINGS, 0x80);
	regmap_write(mid_regmap, FSA4480_SWITCH_CONTROL, switch_control);
	/* FSA4480 chip hardware requirement */
	usleep_range(50, 55);
	regmap_write(mid_regmap, FSA4480_SWITCH_SETTINGS, switch_enable);
}

/*
*kzhkzh add for HL5280检测到audio设备插入时设置5280通道使能和切换寄存器设置
*/
void mid_hl5280_detect(int state)
{
	unsigned int jack_status = 0;
	
	if(state == 1)
	{
		pr_info("[HL5280] %s: plug in \n", __func__);
		/* activate switches */
		if(fsa_priv->vendor == HL5280){  
			fsa4480_usbc_update_settings(0x00, 0x9F);
			mid_hl5280_write(FSA4480_CURRENT_SOURCE_SETTING, 0x07);
			usleep_range(1000, 1005);
			mid_hl5280_write(FSA4480_FUN_EN, 0x45);
			usleep_range(10000, 10005);
			pr_info("[HL5280] %s: set reg[0x%x] done.\n", __func__, FSA4480_FUN_EN);
			jack_status = mid_hl5280_read(FSA4480_JACK_STATUS);
			pr_info("[HL5280] %s: reg[0x%x]=0x%x.\n", __func__, FSA4480_JACK_STATUS, jack_status);
		}
		else if(fsa_priv->vendor == BCT4480){ 
			fsa4480_usbc_update_settings(0x00, 0x9F);
			mid_hl5280_write(0x12, 0x01);
			usleep_range(1000, 1005);
			pr_info("[BCT4480] %s: set reg[0x12] done.\n", __func__);
		}
	
        /* Hefeng.Wu@RM.MM.AudioDriver.HeadsetDet, 2020/06/12,
         * ifdetect fail under 700uA, use 100uA detect again */
        if(1 == jack_status) {
            pr_info("[HL5280] %s: use 100uA detect again\n", __func__);
            fsa4480_usbc_update_settings(0x00, 0x9F);
            mid_hl5280_write(FSA4480_CURRENT_SOURCE_SETTING, 0x01);
            usleep_range(1000, 1005);

            mid_hl5280_write(FSA4480_FUN_EN, 0x45);
            usleep_range(10000, 10005);
			jack_status = mid_hl5280_read(FSA4480_JACK_STATUS);
            pr_info("[HL5280] %s: reg[0x%x]=0x%x.\n", __func__, FSA4480_JACK_STATUS, jack_status);
        }

		if(jack_status & 0x2) {
			//for 3 pole, mic switch to SBU2
			pr_info("[HL5280] %s: set mic to sbu2 for 3 pole.\n", __func__);
			fsa4480_usbc_update_settings(0x00, 0x9F);
			usleep_range(4000, 4005);
		}
		
		pr_info("[HL5280] %s: reg[0x%x]=0x%x.\n", __func__, FSA4480_SWITCH_STATUS0, mid_hl5280_read(FSA4480_SWITCH_STATUS0));
		pr_info("[HL5280] %s: reg[0x%x]=0x%x.\n", __func__, FSA4480_SWITCH_STATUS1, mid_hl5280_read(FSA4480_SWITCH_STATUS1));
		
	}else{
		pr_info("[HL5280] %s: plug out \n", __func__);
				/* deactivate switches */
		if(fsa_priv->vendor == BCT4480){
			mid_hl5280_write(0x12, 0x00);
		}
		fsa4480_usbc_update_settings(0x18, 0x98);
	}
}
EXPORT_SYMBOL_GPL(mid_hl5280_detect);

static void fsa4480_update_reg_defaults(struct regmap *regmap)
{
	u8 i;

	for (i = 0; i < ARRAY_SIZE(fsa_reg_i2c_defaults); i++)
		regmap_write(regmap, fsa_reg_i2c_defaults[i].reg,
				   fsa_reg_i2c_defaults[i].val);
}


static void bct4480_update_reg_defaults(struct regmap *regmap)
{
	u8 i;

	for (i = 0; i < ARRAY_SIZE(bct_reg_i2c_defaults); i++)
		regmap_write(regmap, bct_reg_i2c_defaults[i].reg,
				   bct_reg_i2c_defaults[i].val);
}



static int fsa4480_probe(struct i2c_client *i2c,
			 const struct i2c_device_id *id)
{
	int rc = 0;
	unsigned int reg_value = 0;
	printk(" ######## sunth %s:%d \n",__func__,__LINE__);
	fsa_priv = devm_kzalloc(&i2c->dev, sizeof(*fsa_priv),
				GFP_KERNEL);			
	if(!fsa_priv)
		return -ENOMEM;
	fsa_priv->dev = &i2c->dev;
	printk(" ######## sunth %s:%d i2c->addr:0x%x\n",__func__,__LINE__,i2c->addr);
	fsa_priv->regmap = devm_regmap_init_i2c(i2c, &fsa4480_regmap_config);
	if(IS_ERR_OR_NULL(fsa_priv->regmap)) {
		dev_err(fsa_priv->dev, "%s: Failed to initialize regmap: %d\n",
			__func__, rc);
		if(!fsa_priv->regmap) {
			rc = -EINVAL;
			goto err_data;
		}
		rc = PTR_ERR(fsa_priv->regmap);
		goto err_data;
	}

	mid_regmap = fsa_priv->regmap;//kzhkzh add for read and write interface!!!
	mid_dev = fsa_priv->dev;//kzhkzh add for mid_hl5280_detect!!!

	regmap_read(fsa_priv->regmap, FSA4480_DEVICE_ID, &reg_value);
	printk("%s: device id reg value: 0x%x\n", __func__, reg_value);
	if(HL5280_DEVICE_REG_VALUE == reg_value) {
		dev_err(fsa_priv->dev, "%s: switch chip is HL5280\n", __func__);
		fsa_priv->vendor = HL5280;
	} else{
		dev_err(fsa_priv->dev, "%s: switch chip is BCT4480\n", __func__);
		fsa_priv->vendor = BCT4480;
	}

	if(fsa_priv->vendor == HL5280){
		dev_err(fsa_priv->dev, "%s: fsa4480_update_reg_defaults\r\n", __func__);
		fsa4480_update_reg_defaults(fsa_priv->regmap);
	}
	else if(fsa_priv->vendor == BCT4480){
		dev_err(fsa_priv->dev, "%s: bct4480_update_reg_defaults\r\n", __func__);
		bct4480_update_reg_defaults(fsa_priv->regmap);
	}
	
	fsa_priv->plug_state = false;

	mutex_init(&fsa_priv->notification_lock);
	i2c_set_clientdata(i2c, fsa_priv);

	printk(" ######## sunth %s:%d probe success!!!!!!!\n",__func__,__LINE__);
	return 0;

err_data:
/* 	if(gpio_is_valid(fsa_priv->hs_det_pin)) {
		gpio_free(fsa_priv->hs_det_pin);
	} */
	devm_kfree(&i2c->dev, fsa_priv);
	return rc;
}

static int fsa4480_remove(struct i2c_client *i2c)
{
	struct fsa4480_priv *fsa_priv =
			(struct fsa4480_priv *)i2c_get_clientdata(i2c);

	if(!fsa_priv)
		return -EINVAL;

	fsa4480_usbc_update_settings(0x18, 0x98);
	//cancel_work_sync(&fsa_priv->usbc_analog_work);
	pm_relax(fsa_priv->dev);
	mutex_destroy(&fsa_priv->notification_lock);
	dev_set_drvdata(&i2c->dev, NULL);

	return 0;
}

/* static const struct of_device_id fsa4480_i2c_dt_match[] = {
	{ .compatible = "mediatek,fsa4480", },
	{ }
};
 */
static const struct of_device_id fsa4480_i2c_dt_match[] = {
	{.compatible = "mediatek,fsa4480"},
	{},
};
MODULE_DEVICE_TABLE(of, fsa4480_i2c_dt_match);
static const struct i2c_device_id fsa4480_id[] = {
	{FSA4480_I2C_NAME, 0 },
	{}
};
static struct i2c_driver fsa4480_i2c_driver = {
	.driver = {
		.name = FSA4480_I2C_NAME,
		.of_match_table = fsa4480_i2c_dt_match,
		//.of_match_table = of_match_ptr(fsa4480_i2c_dt_match),
	},
	.probe = fsa4480_probe,
	.remove = fsa4480_remove,
	.id_table = fsa4480_id,
};

static int __init fsa4480_init(void)
{
	int rc;
	printk(" ######## kzhkzh %s:%d \n",__func__,__LINE__);
	rc = i2c_add_driver(&fsa4480_i2c_driver);
	if(rc){
		printk("fsa4480: Failed to register I2C driver: %d\n", rc);
	}
	printk(" ######## kzhkzh %s:%d rc:%d\n",__func__,__LINE__,rc);
	return rc;
}
//late_initcall_sync(fsa4480_init);
//module_init(fsa4480_init);

static void __exit fsa4480_exit(void)
{
	i2c_del_driver(&fsa4480_i2c_driver);
}

module_init(fsa4480_init);
module_exit(fsa4480_exit);
MODULE_DESCRIPTION("FSA4480 I2C driver");
MODULE_LICENSE("GPL v2");
