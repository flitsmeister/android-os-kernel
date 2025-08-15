// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/types.h>
#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#endif
//#include <mt-plat/mtk_boot.h>
//#include <mt-plat/upmu_common.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include "sgm415xx_sub.h"
#include <mt-plat/v1/charger_class.h>
#include <mt-plat/v1/mtk_charger.h>
#include "mtk_charger_intf.h"
#include <mt-plat/upmu_common.h>
/**********************************************************
 *
 *   [I2C Slave Setting]
 *
 *********************************************************/

#define SGM4154x_SUB_REG_NUM    (0xF)

static struct wakeup_source *otg_in_wakelock;
int sgm4154x_sub_hw_exist;

static const unsigned int CS_VTH_SUB[] = {
	0, 5000, 10000, 15000, 20000,
	25000, 30000, 35000, 40000, 50000,
	60000, 70000, 80000, 90000, 100000,
	110000, 130000, 150000, 170000, 190000,
	210000, 230000, 250000, 270000, 300000,
	330000, 360000, 390000, 420000, 450000,
	480000, 510000, 540000, 600000, 660000,
	720000, 780000, 840000, 900000, 960000,
	1020000, 1080000, 1140000, 1200000, 1260000,
	1320000, 1380000, 1440000, 1500000, 1620000,
	1740000, 1860000, 1980000, 2100000, 2220000,
	2340000, 2460000, 2580000, 2700000, 2820000,
	2940000, 3000000, 3000000, 3000000
};
/* SGM4154x_SUB REG06 BOOST_LIM[5:4], uV */
static const unsigned int BOOST_VOLT_LIMIT[] = {
	4850000, 5000000, 5150000, 5300000		
};
 /* SGM4154x_SUB REG02 BOOST_LIM[7:7], uA */
#if (defined(__SGM41542_CHIP_ID__) || defined(__SGM41541_CHIP_ID__)|| defined(__SGM41543_CHIP_ID__)|| defined(__SGM41543D_CHIP_ID__))
static const unsigned int BOOST_CURRENT_LIMIT[] = {
	1200000, 2000000
};
#else
static const unsigned int BOOST_CURRENT_LIMIT[] = {
	500000, 1200000
};
#endif

#if (defined(__SGM41513_CHIP_ID__) || defined(__SGM41513A_CHIP_ID__) || defined(__SGM41513D_CHIP_ID__))

static const unsigned int IPRECHG_CURRENT_STABLE[] = {
	5000, 10000, 15000, 20000, 30000, 40000, 50000, 60000,
	80000, 100000, 120000, 140000, 160000, 180000, 200000, 240000
};

static const unsigned int ITERM_CURRENT_STABLE[] = {
	5000, 10000, 15000, 20000, 30000, 40000, 50000, 60000,
	80000, 100000, 120000, 140000, 160000, 180000, 200000, 240000
};
#endif
#if 0
static enum power_supply_usb_type sgm4154x_sub_usb_type[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_CDP,	
};
#endif
static const struct charger_properties sgm4154x_sub_chg_props = {
	.alias_name = SGM4154x_SUB_NAME,
};


enum {
	SGM_DP_DM_VOL_HIZ,
	SGM_DP_DM_VOL_0P0,
	SGM_DP_DM_VOL_0P6,
	SGM_DP_DM_VOL_3P3,
};

enum SGM4154x_SUB_QC_VOLT {
	QC_20_5000mV,
	QC_20_9000mV,
	QC_20_12000mV,		
};

/**********************************************************
 *
 *   [Global Variable]
 *
 *********************************************************/
static struct power_supply_desc sgm4154x_sub_power_supply_desc;
static struct charger_device *s_chg_dev_otg;

/**********************************************************
 *
 *   [I2C Function For Read/Write sgm4154x_sub]
 *
 *********************************************************/
static int __sgm4154x_sub_read_byte(struct sgm4154x_sub_device *sgm, u8 reg, u8 *data)
{
    s32 ret;

    ret = i2c_smbus_read_byte_data(sgm->client, reg);
    if (ret < 0) {
        pr_err("i2c read fail: can't read from reg 0x%02X\n", reg);
        return ret;
    }

    *data = (u8) ret;

    return 0;
}

static int __sgm4154x_sub_write_byte(struct sgm4154x_sub_device *sgm, int reg, u8 val)
{
    s32 ret;

    ret = i2c_smbus_write_byte_data(sgm->client, reg, val);
    if (ret < 0) {
        pr_err("i2c write fail: can't write 0x%02X to reg 0x%02X: %d\n",
               val, reg, ret);
        return ret;
    }
    return 0;
}

static int sgm4154x_sub_read_reg(struct sgm4154x_sub_device *sgm, u8 reg, u8 *data)
{
	int ret;

	mutex_lock(&sgm->i2c_rw_lock);
	ret = __sgm4154x_sub_read_byte(sgm, reg, data);
	mutex_unlock(&sgm->i2c_rw_lock);

	return ret;
}
#if 0
static int sgm4154x_sub_write_reg(struct sgm4154x_sub_device *sgm, u8 reg, u8 val)
{
	int ret;

	mutex_lock(&sgm->i2c_rw_lock);
	ret = __sgm4154x_sub_write_byte(sgm, reg, val);
	mutex_unlock(&sgm->i2c_rw_lock);

	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);

	return ret;
}
#endif
static int sgm4154x_sub_update_bits(struct sgm4154x_sub_device *sgm, u8 reg,
					u8 mask, u8 val)
{
	int ret;
	u8 tmp;

	mutex_lock(&sgm->i2c_rw_lock);
	ret = __sgm4154x_sub_read_byte(sgm, reg, &tmp);
	if (ret) {
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= val & mask;

	ret = __sgm4154x_sub_write_byte(sgm, reg, tmp);
	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);

out:
	mutex_unlock(&sgm->i2c_rw_lock);
	return ret;
}

/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/

 static int sgm4154x_sub_set_watchdog_timer(struct sgm4154x_sub_device *sgm, int time)
{
	int ret;
	u8 reg_val;

	if (time == 0)
		reg_val = SGM4154x_SUB_WDT_TIMER_DISABLE;
	else if (time == 40)
		reg_val = SGM4154x_SUB_WDT_TIMER_40S;
	else if (time == 80)
		reg_val = SGM4154x_SUB_WDT_TIMER_80S;
	else
		reg_val = SGM4154x_SUB_WDT_TIMER_160S;	

	ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_5,
				SGM4154x_SUB_WDT_TIMER_MASK, reg_val);

	return ret;
}

 #if 0
 static int sgm4154x_sub_get_term_curr(struct sgm4154x_sub_device *sgm)
{
	int ret;
	u8 reg_val;
	int curr;
	int offset = SGM4154x_SUB_TERMCHRG_I_MIN_uA;

	ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_CTRL_3, &reg_val);
	if (ret)
		return ret;

	reg_val &= SGM4154x_SUB_TERMCHRG_CUR_MASK;
	curr = reg_val * SGM4154x_SUB_TERMCHRG_CURRENT_STEP_uA + offset;
	return curr;
}

static int sgm4154x_sub_get_prechrg_curr(struct sgm4154x_sub_device *sgm)
{
	int ret;
	u8 reg_val;
	int curr;
	int offset = SGM4154x_SUB_PRECHRG_I_MIN_uA;

	ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_CTRL_3, &reg_val);
	if (ret)
		return ret;

	reg_val = (reg_val&SGM4154x_SUB_PRECHRG_CUR_MASK)>>4;
	curr = reg_val * SGM4154x_SUB_PRECHRG_CURRENT_STEP_uA + offset;
	return curr;
}
#endif

static int sgm4154x_sub_adc_read_charge_current(struct sgm4154x_sub_device *sgm, u32 *ibus)
{
	int ret;
	u8 ichg;
    unsigned int curr;
	
	ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_CTRL_2, &ichg);
	if (ret)
		return ret;	

	ichg &= SGM4154x_SUB_ICHRG_I_MASK;
#if (defined(__SGM41513_CHIP_ID__) || defined(__SGM41513A_CHIP_ID__) || defined(__SGM41513D_CHIP_ID__))	
	if (ichg <= 0x8)
		curr = ichg * 5000;
	else if (ichg <= 0xF)
		curr = 40000 + (ichg - 0x8) * 10000;
	else if (ichg <= 0x17)
		curr = 110000 + (ichg - 0xF) * 20000;
	else if (ichg <= 0x20)
		curr = 270000 + (ichg - 0x17) * 30000;
	else if (ichg <= 0x30)
		curr = 540000 + (ichg - 0x20) * 60000;
	else if (ichg <= 0x3C)
		curr = 1500000 + (ichg - 0x30) * 120000;
	else
		curr = 3000000;
#else
	curr = ichg * SGM4154x_SUB_ICHRG_I_STEP_uA;
#endif	
	*ibus =  curr;
	
	return ret;
}

static int sgm4154x_sub_get_ibus_adc(struct charger_device *chgdev, u32 *ibus)
{
    struct sgm4154x_sub_device *sgm = dev_get_drvdata(&chgdev->dev);
    
    return sgm4154x_sub_adc_read_charge_current(sgm, ibus);
}

static int sgm4154x_sub_set_term_curr(struct sgm4154x_sub_device *sgm, int uA)
{
	u8 reg_val;
#if (defined(__SGM41513_CHIP_ID__) || defined(__SGM41513A_CHIP_ID__) || defined(__SGM41513D_CHIP_ID__))	
	
	for(reg_val = 1; reg_val < 16 && uA >= ITERM_CURRENT_STABLE[reg_val]; reg_val++)
		;
	reg_val--;
#else
	if (uA < SGM4154x_SUB_TERMCHRG_I_MIN_uA)
		uA = SGM4154x_SUB_TERMCHRG_I_MIN_uA;
	else if (uA > SGM4154x_SUB_TERMCHRG_I_MAX_uA)
		uA = SGM4154x_SUB_TERMCHRG_I_MAX_uA;
	
	reg_val = (uA - SGM4154x_SUB_TERMCHRG_I_MIN_uA) / SGM4154x_SUB_TERMCHRG_CURRENT_STEP_uA;
#endif

	return sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_3,
				  SGM4154x_SUB_TERMCHRG_CUR_MASK, reg_val);
}

static int sgm4154x_sub_set_prechrg_curr(struct sgm4154x_sub_device *sgm, int uA)
{
	u8 reg_val;
	
#if (defined(__SGM41513_CHIP_ID__) || defined(__SGM41513A_CHIP_ID__) || defined(__SGM41513D_CHIP_ID__))
	for(reg_val = 1; reg_val < 16 && uA >= IPRECHG_CURRENT_STABLE[reg_val]; reg_val++)
		;
	reg_val--;
#else
	if (uA < SGM4154x_SUB_PRECHRG_I_MIN_uA)
		uA = SGM4154x_SUB_PRECHRG_I_MIN_uA;
	else if (uA > SGM4154x_SUB_PRECHRG_I_MAX_uA)
		uA = SGM4154x_SUB_PRECHRG_I_MAX_uA;

	reg_val = (uA - SGM4154x_SUB_PRECHRG_I_MIN_uA) / SGM4154x_SUB_PRECHRG_CURRENT_STEP_uA;
#endif
	reg_val = reg_val << 4;
	return sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_3,
				  SGM4154x_SUB_PRECHRG_CUR_MASK, reg_val);
}

static int sgm4154x_sub_set_ichrg_curr(struct charger_device *chg_dev, unsigned int uA)
{
	int ret;
	u8 reg_val;
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
	
	printk("xuchen sgm4154x_sub_set_ichrg_curr set uA=%d\n",uA);
	if (uA < SGM4154x_SUB_ICHRG_I_MIN_uA)
		uA = SGM4154x_SUB_ICHRG_I_MIN_uA;
	else if ( uA > sgm->init_data.max_ichg)
		uA = sgm->init_data.max_ichg;
#if (defined(__SGM41513_CHIP_ID__) || defined(__SGM41513A_CHIP_ID__) || defined(__SGM41513D_CHIP_ID__))
	if (uA <= 40000)
		reg_val = uA / 5000;	
	else if (uA <= 110000)
		reg_val = 0x08 + (uA -40000) / 10000;	
	else if (uA <= 270000)
		reg_val = 0x0F + (uA -110000) / 20000;	
	else if (uA <= 540000)
		reg_val = 0x17 + (uA -270000) / 30000;	
	else if (uA <= 1500000)
		reg_val = 0x20 + (uA -540000) / 60000;	
	else if (uA <= 2940000)
		reg_val = 0x30 + (uA -1500000) / 120000;
	else 
		reg_val = 0x3d;
#else
	reg_val = uA / SGM4154x_SUB_ICHRG_I_STEP_uA;
#endif
	printk("xuchen sgm4154x_sub_update_bits SGM4154x_SUB_CHRG_CTRL_2 set reg_val=0x%02x\n",reg_val);
	ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_2,
				  SGM4154x_SUB_ICHRG_I_MASK, reg_val);
	
	return ret;
}

static int sgm4154x_sub_get_current(struct charger_device *chg_dev, unsigned int *ichg)
{
	u8 reg_val;
	unsigned int ret = 0;
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);

	pr_info("%s enter!\n", __func__);

	/* Get current level */
	ret=sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_CTRL_2, &reg_val);
	pr_info("pass value = 0x%x\n", reg_val);
	/* Parsing */
	*ichg = CS_VTH_SUB[reg_val&0x3f];

	return ret;
}

static int sgm4154x_sub_set_chrg_volt(struct charger_device *chg_dev, unsigned int chrg_volt)
{
	int ret;
	u8 reg_val;
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
	
	if (chrg_volt < SGM4154x_SUB_VREG_V_MIN_uV)
		chrg_volt = SGM4154x_SUB_VREG_V_MIN_uV;
	else if (chrg_volt > sgm->init_data.max_vreg)
		chrg_volt = sgm->init_data.max_vreg;
	
	
	reg_val = (chrg_volt-SGM4154x_SUB_VREG_V_MIN_uV) / SGM4154x_SUB_VREG_V_STEP_uV;
	reg_val = reg_val<<3;
	ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_4,
				  SGM4154x_SUB_VREG_V_MASK, reg_val);

	return ret;
}

static int sgm4154x_sub_get_chrg_volt(struct charger_device *chg_dev,unsigned int *volt)
{
	int ret;
	u8 vreg_val;
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);

	ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_CTRL_4, &vreg_val);
	if (ret)
		return ret;	

	vreg_val = (vreg_val & SGM4154x_SUB_VREG_V_MASK)>>3;

	if (15 == vreg_val)
		*volt = 4352000; //default
	else if (vreg_val < 25)	
		*volt = vreg_val*SGM4154x_SUB_VREG_V_STEP_uV + SGM4154x_SUB_VREG_V_MIN_uV;	

	return 0;
}

static int sgm4154x_sub_get_vindpm_offset_os(struct sgm4154x_sub_device *sgm)
{
	int ret;
	u8 reg_val;

	ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_CTRL_f, &reg_val);
	if (ret)
		return ret;	

	reg_val = reg_val & SGM4154x_SUB_VINDPM_OS_MASK;	

	return reg_val;
}

static int sgm4154x_sub_set_vindpm_offset_os(struct sgm4154x_sub_device *sgm,u8 offset_os)
{
	int ret;	
	
	ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_f,
				  SGM4154x_SUB_VINDPM_OS_MASK, offset_os);
	
	if (ret){
		pr_err("%s fail\n",__func__);
		return ret;
	}
	
	return ret;
}
static int sgm4154x_sub_set_input_volt_lim(struct charger_device *chg_dev, unsigned int vindpm)
{
	int ret;
	unsigned int offset;
	u8 reg_val;
	u8 os_val;
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
	
	if (vindpm < SGM4154x_SUB_VINDPM_V_MIN_uV ||
	    vindpm > SGM4154x_SUB_VINDPM_V_MAX_uV)
 		return -EINVAL;	
	
	if (vindpm < 5900000){
		os_val = 0;
		vindpm = 4200000; //fox modify for static vindpm = 4200000;
		offset = 3900000;
	}		
	else if (vindpm >= 5900000 && vindpm < 7500000){
		os_val = 1;
		offset = 5900000; //uv
	}		
	else if (vindpm >= 7500000 && vindpm < 10500000){
		os_val = 2;
		offset = 7500000; //uv
	}		
	else{
		os_val = 3;
		offset = 10500000; //uv
	}		
	
	sgm4154x_sub_set_vindpm_offset_os(sgm,os_val);
	reg_val = (vindpm - offset) / SGM4154x_SUB_VINDPM_STEP_uV;	

	printk("xuchen sgm4154x_sub_set_input_volt_lim set reg_val=0x%02x\n",reg_val);
	ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_6,
				  SGM4154x_SUB_VINDPM_V_MASK, reg_val); 

	return ret;
}

static int sgm4154x_sub_get_input_volt_lim(struct sgm4154x_sub_device *sgm)
{
	int ret;
	int offset;
	u8 vlim;
	int temp;

	ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_CTRL_6, &vlim);
	if (ret)
		return ret;
	
	temp = sgm4154x_sub_get_vindpm_offset_os(sgm);
	if (0 == temp)
		offset = 3900000; //uv
	else if (1 == temp)
		offset = 5900000;
	else if (2 == temp)
		offset = 7500000;
	else if (3 == temp)
		offset = 10500000;
	
	temp = offset + (vlim & 0x0F) * SGM4154x_SUB_VINDPM_STEP_uV;
	return temp;
}


static int sgm4154x_sub_set_input_curr_lim(struct charger_device *chg_dev, unsigned int iindpm)
{
	int ret;
	u8 reg_val;
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
	
	printk("xuchen sgm4154x_sub_set_input_curr_lim iindpm=%d\n",iindpm);
	if (iindpm < SGM4154x_SUB_IINDPM_I_MIN_uA ||
			iindpm > SGM4154x_SUB_IINDPM_I_MAX_uA)
		return -EINVAL;	

#if (defined(__SGM41513_CHIP_ID__) || defined(__SGM41513A_CHIP_ID__) || defined(__SGM41513D_CHIP_ID__))
	reg_val = (iindpm-SGM4154x_SUB_IINDPM_I_MIN_uA) / SGM4154x_SUB_IINDPM_STEP_uA;
#else		
	if (iindpm >= SGM4154x_SUB_IINDPM_I_MIN_uA && iindpm <= 3100000)//default
		reg_val = (iindpm-SGM4154x_SUB_IINDPM_I_MIN_uA) / SGM4154x_SUB_IINDPM_STEP_uA;
	else if (iindpm > 3100000 && iindpm < SGM4154x_SUB_IINDPM_I_MAX_uA)
		reg_val = 0x1E;
	else
		reg_val = SGM4154x_SUB_IINDPM_I_MASK;
#endif
	ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_0,
				  SGM4154x_SUB_IINDPM_I_MASK, reg_val);
	return ret;
}

static int sgm4154x_sub_get_input_curr_lim(struct charger_device *chg_dev,unsigned int *ilim)
{
	int ret;	
	u8 reg_val;
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
	
	ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_CTRL_0, &reg_val);
	if (ret)
		return ret;	
	if (SGM4154x_SUB_IINDPM_I_MASK == (reg_val & SGM4154x_SUB_IINDPM_I_MASK))
		*ilim =  SGM4154x_SUB_IINDPM_I_MAX_uA;
	else
		*ilim = (reg_val & SGM4154x_SUB_IINDPM_I_MASK)*SGM4154x_SUB_IINDPM_STEP_uA + SGM4154x_SUB_IINDPM_I_MIN_uA;

	return 0;
}

static int sgm4154x_sub_get_min_ichg(struct charger_device *chg_dev, u32 *uA)
{
	*uA = 200000;

	return 0;
}

static int32_t sgm4154x_sub_set_dpdm(
	struct sgm4154x_sub_device *sgm, uint8_t dp_val, uint8_t dm_val)
{
	uint8_t data_reg = 0;
	
	uint8_t mask = SGM4154x_SUB_DP_VSEL_MASK|SGM4154x_SUB_DM_VSEL_MASK;
	
	data_reg  = (dp_val & SGM4154x_SUB_DP_VSEL_MASK) << SGM4154x_SUB_DP_VOLT_SHIFT;
	data_reg |= (dm_val & SGM4154x_SUB_DM_VSEL_MASK) << SGM4154x_SUB_DM_VOLT_SHIFT;
	
	
	return sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_d,
				  mask, data_reg);

}

static int sgm4154x_sub_charging_set_hvdcp20(
		struct sgm4154x_sub_device *sgm, uint32_t vbus_target)
{
	int32_t ret = 0;

	pr_err("Set vbus target %dv\n", vbus_target);
	switch (vbus_target) {
	case 5:
		ret = sgm4154x_sub_set_dpdm(sgm, SGM_DP_DM_VOL_0P6, SGM_DP_DM_VOL_0P0);
		sgm4154x_sub_set_input_volt_lim(sgm->chg_dev, 4200000);
		break;
	case 9:		
		ret = sgm4154x_sub_set_dpdm(sgm, SGM_DP_DM_VOL_3P3, SGM_DP_DM_VOL_0P6);
		sgm4154x_sub_set_input_volt_lim(sgm->chg_dev, 8000000);
		break;
	case 12:		
		ret = sgm4154x_sub_set_dpdm(sgm, SGM_DP_DM_VOL_0P6, SGM_DP_DM_VOL_0P6);
		sgm4154x_sub_set_input_volt_lim(sgm->chg_dev, 11000000);
		break;
	default:
		ret = sgm4154x_sub_set_dpdm(sgm, SGM_DP_DM_VOL_HIZ, SGM_DP_DM_VOL_HIZ);
		sgm4154x_sub_set_input_volt_lim(sgm->chg_dev, 4200000);
		break;
	}

	return (ret < 0) ? ret : 0;
}

int32_t sgm4154x_sub_charging_enable_hvdcp30(struct sgm4154x_sub_device *sgm, bool enable)
{
//	sgm4154x_sub_dbg("enable %d\n", enable);
	//enter continuous mode DP 0.6, DM 3.3
	return sgm4154x_sub_set_dpdm(sgm, SGM_DP_DM_VOL_0P6, SGM_DP_DM_VOL_3P3);
}

int32_t sgm4154x_sub_charging_set_hvdcp30(struct sgm4154x_sub_device *sgm, bool increase)
{
	int32_t ret = 0;

//	sgm4154x_sub_dbg("increase %d\n", increase);
	if (increase) {
		//DP 3.3, DM 3.3
		ret = sgm4154x_sub_set_dpdm(sgm, SGM_DP_DM_VOL_3P3, SGM_DP_DM_VOL_3P3);
		if (ret < 0)
			return ret;
		//need test
		msleep(100);
		//DP 0.6, DM 3.3
		ret = sgm4154x_sub_set_dpdm(sgm, SGM_DP_DM_VOL_0P6, SGM_DP_DM_VOL_3P3);
		if (ret < 0)
			return ret;
		msleep(100);
	} else {
		//DP 0.6, DM 3.3
		ret = sgm4154x_sub_set_dpdm(sgm, SGM_DP_DM_VOL_0P6, SGM_DP_DM_VOL_0P6);
		if (ret < 0)
			return ret;
		//need test
		msleep(100);
		//DP 0.6, DM 3.3
		ret = sgm4154x_sub_set_dpdm(sgm, SGM_DP_DM_VOL_0P6, SGM_DP_DM_VOL_3P3);
		if (ret < 0)
			return ret;
		msleep(100);
	}
	return 0;
}

bool sgm4154x_sub_is_hvdcp(struct sgm4154x_sub_device *sgm,enum SGM4154x_SUB_QC_VOLT val)
{	
    int i = 20;	
	int vlim;
	u8 temp;

	vlim = sgm4154x_sub_get_input_volt_lim(sgm);
	
	if (QC_20_9000mV == val)
	{
		sgm4154x_sub_set_input_volt_lim(sgm->chg_dev,8000000); //8v
	}
	else if (QC_20_12000mV == val)
	{
		sgm4154x_sub_set_input_volt_lim(sgm->chg_dev,11000000); //11v
	}
	else
	{
		sgm4154x_sub_set_input_volt_lim(sgm->chg_dev,4200000); //4.5v
		return 0;
	}
	mdelay(1);
	while(i--){		
	 	sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_CTRL_a, &temp);
		
		if(0 == (temp&0x40)){
			return 1;
		}
		else if(1 == !!(temp&0x40) && i == 1){
			sgm4154x_sub_set_input_volt_lim(sgm->chg_dev,vlim);
			return 0;
		}
		mdelay(10);
	}	
	return 0;
}

static int sgm4154x_sub_get_state(struct sgm4154x_sub_device *sgm,
			     struct sgm4154x_sub_state *state)
{
	u8 chrg_stat;
	u8 fault;
	u8 chrg_param_0,chrg_param_1,chrg_param_2;
	int ret;

	ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_STAT, &chrg_stat);
	if (ret){
		ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_STAT, &chrg_stat);
		if (ret){
			pr_err("%s read SGM4154x_SUB_CHRG_STAT fail\n",__func__);
			return ret;
		}
	}
	state->chrg_type = chrg_stat & SGM4154x_SUB_VBUS_STAT_MASK;
	state->chrg_stat = chrg_stat & SGM4154x_SUB_CHG_STAT_MASK;
	state->online = !!(chrg_stat & SGM4154x_SUB_PG_STAT);
	state->therm_stat = !!(chrg_stat & SGM4154x_SUB_THERM_STAT);
	state->vsys_stat = !!(chrg_stat & SGM4154x_SUB_VSYS_STAT);
	
	pr_err("%s chrg_type =%d,chrg_stat =%d online = %d\n",__func__,state->chrg_type,state->chrg_stat,state->online);
	

	ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_FAULT, &fault);
	if (ret){
		pr_err("%s read SGM4154x_SUB_CHRG_FAULT fail\n",__func__);
		return ret;
	}
	state->chrg_fault = fault;	
	state->ntc_fault = fault & SGM4154x_SUB_TEMP_MASK;
	state->health = state->ntc_fault;
	ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_CTRL_0, &chrg_param_0);
	if (ret){
		pr_err("%s read SGM4154x_SUB_CHRG_CTRL_0 fail\n",__func__);
		return ret;
	}
	state->hiz_en = !!(chrg_param_0 & SGM4154x_SUB_HIZ_EN);
	
	ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_CTRL_5, &chrg_param_1);
	if (ret){
		pr_err("%s read SGM4154x_SUB_CHRG_CTRL_5 fail\n",__func__);
		return ret;
	}
	state->term_en = !!(chrg_param_1 & SGM4154x_SUB_TERM_EN);
	
	ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_CTRL_a, &chrg_param_2);
	if (ret){
		pr_err("%s read SGM4154x_SUB_CHRG_CTRL_a fail\n",__func__);
		return ret;
	}
	state->vbus_gd = !!(chrg_param_2 & SGM4154x_SUB_VBUS_GOOD);

	return 0;
}
#if 0
static int sgm4154x_sub_set_hiz_en(struct charger_device *chg_dev, bool hiz_en)
{
	u8 reg_val;
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
	
	dev_notice(sgm->dev, "%s:%d", __func__, hiz_en);
	reg_val = hiz_en ? SGM4154x_SUB_HIZ_EN : 0;

	return sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_0,
				  SGM4154x_SUB_HIZ_EN, reg_val);
}
#endif
static int sgm4154x_sub_enable_charger(struct sgm4154x_sub_device *sgm)
{
    int ret;
    
    ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_1, SGM4154x_SUB_CHRG_EN,
                     SGM4154x_SUB_CHRG_EN);
	
    return ret;
}

static int sgm4154x_sub_disable_charger(struct sgm4154x_sub_device *sgm)
{
    int ret;
    
    ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_1, SGM4154x_SUB_CHRG_EN,
                     0);
    return ret;
}

static int sgm4154x_sub_charging_switch(struct charger_device *chg_dev,bool enable)
{
	int ret;
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
	pr_info("sgm4154x_sub_charging_switch enable:%d\n",enable);
	if (enable)
	{
		ret = sgm4154x_sub_enable_charger(sgm);
		sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_0,
            SGM4154x_SUB_HIZ_EN, 0);
		printk("sgm4154x_sub_charging_switch 111 enable:%d\n",enable);
	}
	else
	{
	    ret = sgm4154x_sub_disable_charger(sgm);
	    sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_0,
            SGM4154x_SUB_HIZ_EN, SGM4154x_SUB_HIZ_EN);
		printk("sgm4154x_sub_charging_switch 222 enable:%d\n",enable);
	}
	return ret;
}

static int sgm4154x_sub_set_recharge_volt(struct sgm4154x_sub_device *sgm, int mV)
{
	u8 reg_val;
	
	reg_val = (mV - SGM4154x_SUB_VRECHRG_OFFSET_mV) / SGM4154x_SUB_VRECHRG_STEP_mV;

	return sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_4,
				  SGM4154x_SUB_VRECHARGE, reg_val);
}

static int sgm4154x_sub_set_wdt_rst(struct sgm4154x_sub_device *sgm, bool is_rst)
{
	u8 val;
	
	if (is_rst)
		val = SGM4154x_SUB_WDT_RST_MASK;
	else
		val = 0;
	return sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_1,
				  SGM4154x_SUB_WDT_RST_MASK, val);	
}

/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/
static int sgm4154x_sub_dump_register(struct charger_device *chg_dev)
{

	unsigned char i = 0;
	unsigned int ret = 0;
	unsigned char sgm4154x_sub_reg[SGM4154x_SUB_REG_NUM+1] = { 0 }; 
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
		
	for (i = 0; i < SGM4154x_SUB_REG_NUM+1; i++) {
		ret = sgm4154x_sub_read_reg(sgm,i, &sgm4154x_sub_reg[i]);
		if (ret != 0) {
			pr_info("[sgm4154x_sub] i2c transfor error\n");
			return 1;
		}
		pr_info("%s,[0x%x]=0x%x ",__func__, i, sgm4154x_sub_reg[i]);
	}
	
	return 0;
}


/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/
static int sgm4154x_sub_hw_chipid_detect(struct sgm4154x_sub_device *sgm)
{
	int ret = 0;
	u8 val = 0;
	ret = sgm4154x_sub_read_reg(sgm,SGM4154x_SUB_CHRG_CTRL_b,&val);
	if (ret < 0)
	{
		pr_info("[%s] read SGM4154x_SUB_CHRG_CTRL_b fail\n", __func__);
		return ret;
	}		
	val = val & SGM4154x_SUB_PN_MASK;
	pr_info("[%s] Reg[0x0B]=0x%x\n", __func__,val);
	
	return val;
}

static int sgm4154x_sub_reset_watch_dog_timer(struct charger_device
		*chg_dev)
{
	int ret;
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);

	pr_info("charging_reset_watch_dog_timer\n");

	ret = sgm4154x_sub_set_wdt_rst(sgm,0x1);	/* RST watchdog */	

	return ret;
}


static int sgm4154x_sub_get_charging_status(struct charger_device *chg_dev,
				       bool *is_done)
{
	//struct sgm4154x_sub_state state;
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
	//sgm4154x_sub_get_state(sgm, &state);

	if (sgm->state.chrg_stat == SGM4154x_SUB_TERM_CHRG)
		*is_done = true;
	else
		*is_done = false;

	return 0;
}

static int sgm4154x_sub_set_en_timer(struct sgm4154x_sub_device *sgm)
{
	int ret;	

	ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_5,
				SGM4154x_SUB_SAFETY_TIMER_EN, SGM4154x_SUB_SAFETY_TIMER_EN);

	return ret;
}

static int sgm4154x_sub_set_disable_timer(struct sgm4154x_sub_device *sgm)
{
	int ret;	

	ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_5,
				SGM4154x_SUB_SAFETY_TIMER_EN, 0);

	return ret;
}

static int sgm4154x_sub_enable_safetytimer(struct charger_device *chg_dev,bool en)
{
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
	int ret = 0;

	if (en)
		ret = sgm4154x_sub_set_en_timer(sgm);
	else
		ret = sgm4154x_sub_set_disable_timer(sgm);
	return ret;
}

static int sgm4154x_sub_get_is_safetytimer_enable(struct charger_device
		*chg_dev,bool *en)
{
	int ret = 0;
	u8 val = 0;
	
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
	
	ret = sgm4154x_sub_read_reg(sgm,SGM4154x_SUB_CHRG_CTRL_5,&val);
	if (ret < 0)
	{
		pr_info("[%s] read SGM4154x_SUB_CHRG_CTRL_5 fail\n", __func__);
		return ret;
	}
	*en = !!(val & SGM4154x_SUB_SAFETY_TIMER_EN);
	return 0;
}

#if (defined(__SGM41542_CHIP_ID__)|| defined(__SGM41516D_CHIP_ID__)|| defined(__SGM41543D_CHIP_ID__))
static int sgm4154x_sub_en_pe_current_partern(struct charger_device
		*chg_dev,bool is_up)
{
	int ret = 0;	
	
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
	
	ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_d,
				SGM4154x_SUB_EN_PUMPX, SGM4154x_SUB_EN_PUMPX);
	if (ret < 0)
	{
		pr_info("[%s] read SGM4154x_SUB_CHRG_CTRL_d fail\n", __func__);
		return ret;
	}
	if (is_up)
		ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_d,
				SGM4154x_SUB_PUMPX_UP, SGM4154x_SUB_PUMPX_UP);
	else
		ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_d,
				SGM4154x_SUB_PUMPX_DN, SGM4154x_SUB_PUMPX_DN);
	return ret;
}
#endif

static enum power_supply_property sgm4154x_sub_power_supply_props[] = {
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	//POWER_SUPPLY_PROP_USB_TYPE,
	//POWER_SUPPLY_PROP_CHARGING_ENABLED,
	POWER_SUPPLY_PROP_PRESENT
};

static int sgm4154x_sub_property_is_writeable(struct power_supply *psy,
					 enum power_supply_property prop)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
	case POWER_SUPPLY_PROP_PRECHARGE_CURRENT:
	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
	//case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		return true;
	default:
		return false;
	}
}
static int sgm4154x_sub_charger_set_property(struct power_supply *psy,
		enum power_supply_property prop,
		const union power_supply_propval *val)
{
	//struct sgm4154x_sub_device *sgm = power_supply_get_drvdata(psy);
	int ret = -EINVAL;

	switch (prop) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = sgm4154x_sub_set_input_curr_lim(s_chg_dev_otg, val->intval);
		break;
/*	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		sgm4154x_sub_charging_switch(s_chg_dev_otg,val->intval);		
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
		ret = sgm4154x_sub_set_input_volt_lim(s_chg_dev_otg, val->intval);
		break;*/
	default:
		return -EINVAL;
	}

	return ret;
}

static int sgm4154x_sub_charger_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sgm4154x_sub_device *sgm = power_supply_get_drvdata(psy);
	struct sgm4154x_sub_state state;
	int ret = 0;

	mutex_lock(&sgm->lock);
	//ret = sgm4154x_sub_get_state(sgm, &state);
	state = sgm->state;
	mutex_unlock(&sgm->lock);
	if (ret)
		return ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (!state.chrg_type || (state.chrg_type == SGM4154x_SUB_OTG_MODE))
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else if (!state.chrg_stat)
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else if (state.chrg_stat == SGM4154x_SUB_TERM_CHRG)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		switch (state.chrg_stat) {		
		case SGM4154x_SUB_PRECHRG:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
			break;
		case SGM4154x_SUB_FAST_CHRG:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
			break;		
		case SGM4154x_SUB_TERM_CHRG:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
			break;
		case SGM4154x_SUB_NOT_CHRGING:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
			break;
		default:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
		}
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = SGM4154x_SUB_MANUFACTURER;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = SGM4154x_SUB_NAME;
		break;

	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = state.online;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = state.vbus_gd;
		break;
	case POWER_SUPPLY_PROP_TYPE:
		val->intval = sgm4154x_sub_power_supply_desc.type;
		break;	

	case POWER_SUPPLY_PROP_HEALTH:
		if (state.chrg_fault & 0xF8)
			val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		else
			val->intval = POWER_SUPPLY_HEALTH_GOOD;

		switch (state.health) {
		case SGM4154x_SUB_TEMP_HOT:
			val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
			break;
		case SGM4154x_SUB_TEMP_WARM:
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
			break;
		case SGM4154x_SUB_TEMP_COOL:
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
			break;
		case SGM4154x_SUB_TEMP_COLD:
			val->intval = POWER_SUPPLY_HEALTH_COLD;
			break;
		}
		break;

	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		//val->intval = state.vbus_adc;
		break;

	case POWER_SUPPLY_PROP_CURRENT_NOW:
		//val->intval = state.ibus_adc;
		break;

/*	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
		ret = sgm4154x_sub_get_input_volt_lim(sgm);
		if (ret < 0)
			return ret;

		val->intval = ret;
		break;*/

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:		
		break;
#if 0
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		val->intval = !state.hiz_en;
		break;
#endif
	default:
		return -EINVAL;
	}

	return ret;
}

#if 0
static bool sgm4154x_sub_state_changed(struct sgm4154x_sub_device *sgm,
				  struct sgm4154x_sub_state *new_state)
{
	struct sgm4154x_sub_state old_state;

	mutex_lock(&sgm->lock);
	old_state = sgm->state;
	mutex_unlock(&sgm->lock);

	return (old_state.chrg_type != new_state->chrg_type ||
		old_state.chrg_stat != new_state->chrg_stat     ||		
		old_state.online != new_state->online		    ||
		old_state.therm_stat != new_state->therm_stat	||		
		old_state.vsys_stat != new_state->vsys_stat 	||
		old_state.chrg_fault != new_state->chrg_fault	
		);
}
#endif

static bool sgm4154x_sub_dpdm_detect_is_done(struct sgm4154x_sub_device * sgm)
{
	u8 chrg_stat;
	int ret;

	ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_INPUT_DET, &chrg_stat);
	if(ret) {
		dev_err(sgm->dev, "Check DPDM detecte error\n");
	}
	

	return (chrg_stat&SGM4154x_SUB_DPDM_ONGOING)?true:false;
}

static void sgm4154x_sub_chg_type_hvdcp_work(struct work_struct *work)
{
	
	struct sgm4154x_sub_device * sgm =
		container_of(work, struct sgm4154x_sub_device, hvdcp_work.work);
		
	sgm4154x_sub_charging_set_hvdcp20(sgm,9);	
	pr_err("%s HVDCP",__func__);
	
}
static void charger_monitor_work_func(struct work_struct *work)
{
	int ret = 0;
	struct sgm4154x_sub_device * sgm = NULL;
	struct delayed_work *charge_monitor_work = NULL;
	//static u8 last_chg_method = 0;
	struct sgm4154x_sub_state state;

	charge_monitor_work = container_of(work, struct delayed_work, work);
	if(charge_monitor_work == NULL) {
		pr_err("Cann't get charge_monitor_work\n");
		return ;
	}
	sgm = container_of(charge_monitor_work, struct sgm4154x_sub_device, charge_monitor_work);
	if(sgm == NULL) {
		pr_err("Cann't get sgm \n");
		return ;
	}

	ret = sgm4154x_sub_get_state(sgm, &state);
	mutex_lock(&sgm->lock);
	sgm->state = state;
	mutex_unlock(&sgm->lock);

	if(!sgm->state.vbus_gd) {
		dev_err(sgm->dev, "charger_monitor_work_func Vbus not present, disable charge\n");
		sgm4154x_sub_dump_register(sgm->chg_dev);
		sgm4154x_sub_disable_charger(sgm);
		goto OUT;
	}
	if(!state.online)
	{
		dev_err(sgm->dev, "Vbus not online\n");		
		goto OUT;
	}
	
	sgm4154x_sub_dump_register(sgm->chg_dev);
	pr_err("%s\n",__func__);
OUT:	
	schedule_delayed_work(&sgm->charge_monitor_work, 10*HZ);
}

static void charger_detect_work_func(struct work_struct *work)
{
	struct delayed_work *charge_detect_delayed_work = NULL;
	struct sgm4154x_sub_device * sgm = NULL;
	//static int charge_type_old = 0;
#if (defined(__SGM41542_CHIP_ID__)|| defined(__SGM41516D_CHIP_ID__)|| defined(__SGM41543D_CHIP_ID__))
	int curr_in_limit = 0;
#endif
	struct sgm4154x_sub_state state;	
	int ret;
	
	charge_detect_delayed_work = container_of(work, struct delayed_work, work);
	if(charge_detect_delayed_work == NULL) {
		pr_err("Cann't get charge_detect_delayed_work\n");
		return ;
	}
	sgm = container_of(charge_detect_delayed_work, struct sgm4154x_sub_device, charge_detect_delayed_work);
	if(sgm == NULL) {
		pr_err("Cann't get sgm4154x_sub_device\n");
		return ;
	}

	if (!sgm->charger_wakelock->active)
		__pm_stay_awake(sgm->charger_wakelock);

	ret = sgm4154x_sub_get_state(sgm, &state);
	mutex_lock(&sgm->lock);
	sgm->state = state;	
	mutex_unlock(&sgm->lock);	
	
	if(!sgm->state.vbus_gd) {
		dev_err(sgm->dev, "charger_detect_work_func Vbus not present, disable charge\n");
		sgm4154x_sub_disable_charger(sgm);
		goto err;
	}
	if(!state.online)
	{
		dev_err(sgm->dev, "Vbus not online\n");		
		goto err;
	}

	if(!sgm4154x_sub_dpdm_detect_is_done(sgm)) {
		dev_err(sgm->dev, "DPDM detecte not done, disable charge\n");
		goto err;
	}
#if (defined(__SGM41542_CHIP_ID__)|| defined(__SGM41516D_CHIP_ID__)|| defined(__SGM41543D_CHIP_ID__))
	switch(sgm->state.chrg_type) {
		case SGM4154x_SUB_USB_SDP:
			pr_err("SGM4154x_SUB charger type: SDP\n");
			curr_in_limit = 500000;
			break;

		case SGM4154x_SUB_USB_CDP:
			pr_err("SGM4154x_SUB charger type: CDP\n");
			curr_in_limit = 1500000;
			break;

		case SGM4154x_SUB_USB_DCP:
			pr_err("SGM4154x_SUB charger type: DCP\n");
			sgm4154x_sub_set_dpdm(sgm, SGM_DP_DM_VOL_0P6, SGM_DP_DM_VOL_HIZ);
			schedule_delayed_work(&sgm->hvdcp_work, msecs_to_jiffies(1400));
			curr_in_limit = 2000000;
			break;

		case SGM4154x_SUB_UNKNOWN:
			pr_err("SGM4154x_SUB charger type: UNKNOWN\n");
			curr_in_limit = 500000;
			break;	

		default:
			pr_err("SGM4154x_SUB charger type: default\n");
			//curr_in_limit = 500000;
			//break;
			return;
	}

	//set charge parameters
	dev_err(sgm->dev, "Update: curr_in_limit = %d\n", curr_in_limit);
	sgm4154x_sub_set_input_curr_lim(sgm->chg_dev, curr_in_limit);
#endif
	//enable charge
	sgm4154x_sub_enable_charger(sgm);
	sgm4154x_sub_dump_register(sgm->chg_dev);
	
err:
	//release wakelock
	//power_supply_changed(sgm->charger);
	dev_err(sgm->dev, "Relax wakelock\n");
	__pm_relax(sgm->charger_wakelock);
	return;
}

static irqreturn_t sgm4154x_sub_irq_handler_thread(int irq, void *private)
{
	struct sgm4154x_sub_device *sgm = private;

	//lock wakelock
	//pr_err("%s entry\n",__func__);
    
	schedule_delayed_work(&sgm->charge_detect_delayed_work, 100);
	//power_supply_changed(sgm->charger);

	return IRQ_HANDLED;
}
static char *sgm4154x_sub_charger_supplied_to[] = {
    "sgm4154x_sub-charger",
};

static struct power_supply_desc sgm4154x_sub_power_supply_desc = {
	.name = "sgm4154x_sub-charger",
	.type = POWER_SUPPLY_TYPE_USB,
	//.usb_types = sgm4154x_sub_usb_type,
	//.num_usb_types = ARRAY_SIZE(sgm4154x_sub_usb_type),
	.properties = sgm4154x_sub_power_supply_props,
	.num_properties = ARRAY_SIZE(sgm4154x_sub_power_supply_props),
	.get_property = sgm4154x_sub_charger_get_property,
	.set_property = sgm4154x_sub_charger_set_property,
	.property_is_writeable = sgm4154x_sub_property_is_writeable,
};

static int sgm4154x_sub_power_supply_init(struct sgm4154x_sub_device *sgm,
							struct device *dev)
{
	struct power_supply_config psy_cfg = { .drv_data = sgm,
						.of_node = dev->of_node, };

	psy_cfg.supplied_to = sgm4154x_sub_charger_supplied_to;
	psy_cfg.num_supplicants = ARRAY_SIZE(sgm4154x_sub_charger_supplied_to);

	sgm->charger = devm_power_supply_register(sgm->dev,
						 &sgm4154x_sub_power_supply_desc,
						 &psy_cfg);
	if (IS_ERR(sgm->charger))
		return -EINVAL;
	
	return 0;
}

static int sgm4154x_sub_hw_init(struct sgm4154x_sub_device *sgm)
{
	int ret = 0;	
	struct power_supply_battery_info bat_info = { };	

	bat_info.constant_charge_current_max_ua =
			SGM4154x_SUB_ICHRG_I_DEF_uA;

	bat_info.constant_charge_voltage_max_uv =
			SGM4154x_SUB_VREG_V_DEF_uV;

	bat_info.precharge_current_ua =
			SGM4154x_SUB_PRECHRG_I_DEF_uA;

	bat_info.charge_term_current_ua =
			SGM4154x_SUB_TERMCHRG_I_DEF_uA;

	sgm->init_data.max_ichg =
			SGM4154x_SUB_ICHRG_I_MAX_uA;

	sgm->init_data.max_vreg =
			SGM4154x_SUB_VREG_V_MAX_uV;
			
	sgm4154x_sub_set_watchdog_timer(sgm,0);

	ret = sgm4154x_sub_set_ichrg_curr(s_chg_dev_otg,
				bat_info.constant_charge_current_max_ua);
	if (ret) {
		printk("xuchen sgm4154x_sub_set_ichrg_curr error\n");
		goto err_out;
	}

	ret = sgm4154x_sub_set_prechrg_curr(sgm, bat_info.precharge_current_ua);
	if (ret) {
		printk("xuchen sgm4154x_sub_set_prechrg_curr error\n");
		goto err_out;
	}

	ret = sgm4154x_sub_set_chrg_volt(s_chg_dev_otg,
				bat_info.constant_charge_voltage_max_uv);
	if (ret) {
		printk("xuchen sgm4154x_sub_set_chrg_volt error\n");
		goto err_out;
	}

	ret = sgm4154x_sub_set_term_curr(sgm, bat_info.charge_term_current_ua);
	if (ret) {
		printk("xuchen sgm4154x_sub_set_term_curr error\n");
		goto err_out;
	}

	/*ret = sgm4154x_sub_set_input_volt_lim(sgm, sgm->init_data.vlim);
	if (ret)
		goto err_out;*/

	ret = sgm4154x_sub_set_input_curr_lim(s_chg_dev_otg, sgm->init_data.ilim);
	if (ret) {
		printk("xuchen sgm4154x_sub_set_input_curr_lim error\n");
		goto err_out;
	}
	/* set ovp 6.5V */
	ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_6,
			SGM4154x_SUB_VAC_OVP_MASK, 0xC0);
	if (ret)
		goto err_out;	

	ret = sgm4154x_sub_set_recharge_volt(sgm, 200);//100~200mv
	if (ret) {
		printk("xuchen sgm4154x_sub_set_recharge_volt error\n");
		goto err_out;
	}
	
	dev_err(sgm->dev, "ichrg_curr:%d prechrg_curr:%d chrg_vol:%d"
		" term_curr:%d input_curr_lim:%d",
		bat_info.constant_charge_current_max_ua,
		bat_info.precharge_current_ua,
		bat_info.constant_charge_voltage_max_uv,
		bat_info.charge_term_current_ua,
		sgm->init_data.ilim);

	return 0;

err_out:
	return ret;

}

static int sgm4154x_sub_parse_dt(struct sgm4154x_sub_device *sgm)
{
	int ret;	
	int irq_gpio = 0, irqn = 0;	
	int chg_en_gpio = 0;	
	

	sgm->init_data.vlim = SGM4154x_SUB_VINDPM_DEF_uV;
	sgm->init_data.ilim = SGM4154x_SUB_IINDPM_DEF_uA;

	irq_gpio = of_get_named_gpio(sgm->dev->of_node, "sgm,irq-gpio", 0);
	if (!gpio_is_valid(irq_gpio))
	{
		dev_err(sgm->dev, "%s: %d gpio get failed\n", __func__, irq_gpio);
		return -EINVAL;
	}
	ret = gpio_request(irq_gpio, "sgm4154x_sub irq pin");
	if (ret) {
		dev_err(sgm->dev, "%s: %d gpio request failed\n", __func__, irq_gpio);
		return ret;
	}
	gpio_direction_input(irq_gpio);
	irqn = gpio_to_irq(irq_gpio);
	if (irqn < 0) {
		dev_err(sgm->dev, "%s:%d gpio_to_irq failed\n", __func__, irqn);
		return irqn;
	}
	sgm->client->irq = irqn;
	
	chg_en_gpio = of_get_named_gpio(sgm->dev->of_node, "sgm,chg-en-gpio", 0);
	if (!gpio_is_valid(chg_en_gpio))
	{
		dev_err(sgm->dev, "%s: %d gpio get failed\n", __func__, chg_en_gpio);
		return -EINVAL;
	}
	ret = gpio_request(chg_en_gpio, "sgm chg en pin");
	if (ret) {
		dev_err(sgm->dev, "%s: %d gpio request failed\n", __func__, chg_en_gpio);
		return ret;
	}
	gpio_direction_output(chg_en_gpio,1);//default enable charge
	return 0;
}

static int sgm4154x_sub_enable_vbus(struct regulator_dev *rdev)
{	
	int ret = 0;
	struct sgm4154x_sub_device *sgm = charger_get_data(s_chg_dev_otg);
	
	ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_1, SGM4154x_SUB_OTG_EN,
                     SGM4154x_SUB_OTG_EN);
	return ret;
}

static int sgm4154x_sub_disable_vbus(struct regulator_dev *rdev)
{
	int ret = 0;
	struct sgm4154x_sub_device *sgm = charger_get_data(s_chg_dev_otg);	

	ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_1, SGM4154x_SUB_OTG_EN,
                     0);

	return ret;
}

static int sgm4154x_sub_enable_otg(struct charger_device *chg_dev, bool en)
{
	int ret = 0;

	pr_info("%s en = %d\n", __func__, en);
	if (en) {
		ret = sgm4154x_sub_enable_vbus(NULL);
		__pm_stay_awake(otg_in_wakelock);
	} else {
		ret = sgm4154x_sub_disable_vbus(NULL);
		__pm_relax(otg_in_wakelock);
	}
	return ret;
}

#if 0
static int sgm4154x_sub_set_boost_voltage_limit(struct charger_device
		*chg_dev, u32 uV)
{	
	int ret = 0;
	char reg_val = -1;
	int i = 0;
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
	
	while(i<4){
		if (uV == BOOST_VOLT_LIMIT[i]){
			reg_val = i;
			break;
		}
		i++;
	}
	if (reg_val < 0)
		return reg_val;
	reg_val = reg_val << 4;
	ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_6,
				  SGM4154x_SUB_BOOSTV, reg_val);

	return ret;
}
#endif

static int sgm4154x_sub_set_boost_current_limit(struct charger_device *chg_dev, u32 uA)
{	
	int ret = 0;
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
	
	if (uA == BOOST_CURRENT_LIMIT[0]){
		ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_2, SGM4154x_SUB_BOOST_LIM,
                     0); 
	}
		
	else if (uA == BOOST_CURRENT_LIMIT[1]){
		ret = sgm4154x_sub_update_bits(sgm, SGM4154x_SUB_CHRG_CTRL_2, SGM4154x_SUB_BOOST_LIM,
                     BIT(7)); 
	}
	return ret;
}

static int sgm4154x_sub_enable_chg_type_det(struct charger_device *chg_dev)
{
	struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
	struct sgm4154x_sub_state state;
	int ret,charge_type;
	unsigned int dpdm_status = 0,bq_detect_count=0;
	unsigned int iidet_bit=1;
	
	charge_type = CHARGER_UNKNOWN;
	Charger_Detect_Init();
	ret = sgm4154x_sub_set_dpdm(sgm, SGM_DP_DM_VOL_HIZ, SGM_DP_DM_VOL_HIZ);	
	do{
		msleep(50);
		ret = sgm4154x_sub_get_state(sgm, &state);
		mutex_lock(&sgm->lock);
		sgm->state = state;	
		mutex_unlock(&sgm->lock);	
		dpdm_status = sgm4154x_sub_dpdm_detect_is_done(sgm);	
		bq_detect_count++;
		pr_info("%s: count_max=%d,iidet_bit=%d,dpdm_status =%d\n",
				__func__,bq_detect_count,iidet_bit,
				dpdm_status);
		if((bq_detect_count>5)||(sgm->state.vbus_gd && dpdm_status))
			iidet_bit = 0;
	}while (iidet_bit);
	
	if(!sgm->state.vbus_gd) {
		dev_err(sgm->dev, "sgm4154x_sub_enable_chg_type_det Vbus not present, disable charge\n");
	}
	if(!state.online)
	{
		dev_err(sgm->dev, "sgm4154x_sub_enable_chg_type_det Vbus not online\n");		
	}
	if(!dpdm_status) {
		dev_err(sgm->dev, "sgm4154x_sub_enable_chg_type_det DPDM detecte not done, disable charge\n");
	}
	
	switch(sgm->state.chrg_type) {
		case SGM4154x_SUB_USB_SDP:
			pr_err("SGM4154x_SUB charger type: SDP\n");
			charge_type = CHARGING_HOST;
			break;

		case SGM4154x_SUB_USB_CDP:
			pr_err("SGM4154x_SUB charger type: CDP\n");
			charge_type = CHARGING_HOST;
			break;

		case SGM4154x_SUB_USB_DCP:
		case SGM4154x_SUB_NON_STANDARD:
			pr_err("SGM4154x_SUB charger type: DCP\n");
			//sgm4154x_sub_set_dpdm(sgm, SGM_DP_DM_VOL_0P6, SGM_DP_DM_VOL_HIZ);
			//schedule_delayed_work(&sgm->hvdcp_work, msecs_to_jiffies(1400));
			charge_type = STANDARD_CHARGER;
			break;

		case SGM4154x_SUB_UNKNOWN:
			pr_err("SGM4154x_SUB charger type: UNKNOWN\n");
			break;	

		default:
		    charge_type = STANDARD_CHARGER;
			pr_err("SGM4154x_SUB charger type: default\n");
			break;
	}
	sgm4154x_sub_dump_register(sgm->chg_dev);
	//release wakelock
	Charger_Detect_Release();
	dev_err(sgm->dev, "Relax wakelock\n");
	__pm_relax(sgm->charger_wakelock);

	return charge_type;
}

static int sgm4154x_sub_do_event(struct charger_device *chg_dev, u32 event,
			    u32 args)
{	
	if (chg_dev == NULL)
		return -EINVAL;
	pr_info("%s: event = %d\n", __func__, event);

	switch (event) {
	case EVENT_EOC:
		charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_EOC);
		break;
	case EVENT_RECHARGE:
		charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_RECHG);
		break;
	default:
		break;
	}
	return 0;
}

static int sgm4154x_sub_is_chip_enabled(struct charger_device *chg_dev, bool *en)
{
        *en = sgm4154x_sub_hw_exist;
        return 0;
}

static int sgm4154x_sub_charger_is_enabled(struct charger_device *chg_dev, bool *en)
{
        int ret = 0;
		u8 chrg_stat = 0;
		
		struct sgm4154x_sub_device *sgm = charger_get_data(chg_dev);
		
        ret = sgm4154x_sub_read_reg(sgm, SGM4154x_SUB_CHRG_CTRL_1, &chrg_stat);
        if (ret < 0){
            return ret;
		}	
		chrg_stat = chrg_stat & SGM4154x_SUB_CHRG_EN;		
        *en = chrg_stat;
		pr_err("%s : chrg_stat =0x%x \n",__func__,chrg_stat);
		
        return 0;
}

	
static struct charger_ops sgm4154x_sub_chg_ops = {
	//.enable_hz = sgm4154x_sub_set_hiz_en,

	/* Normal charging */
	.dump_registers = sgm4154x_sub_dump_register,
	.enable = sgm4154x_sub_charging_switch,
	.is_enabled = sgm4154x_sub_charger_is_enabled,
	.get_charging_current = sgm4154x_sub_get_current,
	.set_charging_current = sgm4154x_sub_set_ichrg_curr,
	.get_input_current = sgm4154x_sub_get_input_curr_lim,
	.set_input_current = sgm4154x_sub_set_input_curr_lim,
	.get_constant_voltage = sgm4154x_sub_get_chrg_volt,
	.set_constant_voltage = sgm4154x_sub_set_chrg_volt,
	.get_min_charging_current = sgm4154x_sub_get_min_ichg,
	.kick_wdt = sgm4154x_sub_reset_watch_dog_timer,
	.set_mivr = sgm4154x_sub_set_input_volt_lim,
	.is_charging_done = sgm4154x_sub_get_charging_status,

	/* Safety timer */
	.enable_safety_timer = sgm4154x_sub_enable_safetytimer,
	.is_safety_timer_enabled = sgm4154x_sub_get_is_safetytimer_enable,

	.is_chip_enabled = sgm4154x_sub_is_chip_enabled,
	
	/* Power path */
	/*.enable_powerpath = sgm4154x_sub_enable_power_path, */
	/*.is_powerpath_enabled = sgm4154x_sub_get_is_power_path_enable, */


	/* OTG */
	.enable_otg = sgm4154x_sub_enable_otg,	
	.set_boost_current_limit = sgm4154x_sub_set_boost_current_limit,
	.event = sgm4154x_sub_do_event,
	
	/* PE+/PE+20 */
#if (defined(__SGM41542_CHIP_ID__)|| defined(__SGM41516D_CHIP_ID__)|| defined(__SGM41543D_CHIP_ID__))
	.send_ta_current_pattern = sgm4154x_sub_en_pe_current_partern,
#else
	.send_ta_current_pattern = NULL,
#endif
	.set_pe20_efficiency_table = NULL,
	.send_ta20_current_pattern = NULL,
//	.set_ta20_reset = NULL,
	.enable_cable_drop_comp = NULL,
	.get_ext_chgtyp = sgm4154x_sub_enable_chg_type_det,
	/* ADC */
    .get_adc = NULL,
	.get_vbus_adc = sgm4154x_sub_get_chrg_volt,
    .get_ibus_adc = sgm4154x_sub_get_ibus_adc,
    .get_ibat_adc = NULL,
    .get_tchg_adc = NULL,
    .get_zcv = NULL,
};

static int sgm4154x_sub_driver_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int ret = 0;
	struct device *dev = &client->dev;
	struct sgm4154x_sub_device *sgm;

    char *name = NULL;
	
	pr_info("[%s]\n", __func__);

	sgm = devm_kzalloc(dev, sizeof(*sgm), GFP_KERNEL);
	if (!sgm)
		return -ENOMEM;

	//client->addr = 0x1A;
	sgm->client = client;
	sgm->dev = dev;	
	
	mutex_init(&sgm->lock);
	mutex_init(&sgm->i2c_rw_lock);
	
	i2c_set_clientdata(client, sgm);
	
	ret = sgm4154x_sub_parse_dt(sgm);
	if (ret)
		return ret;
	
	ret = sgm4154x_sub_hw_chipid_detect(sgm);
	if (ret != SGM4154x_SUB_PN_ID){
		pr_err("[%s] device not found !!!\n", __func__);
		sgm4154x_sub_hw_exist = 0;
		return ret;
	}else
		sgm4154x_sub_hw_exist = 1;	
	
	name = devm_kasprintf(sgm->dev, GFP_KERNEL, "%s","sgm4154x_sub suspend wakelock");
	sgm->charger_wakelock =	wakeup_source_register(NULL,name);
	pr_err("xuchen sgm4154x_sub_driver_probe1 is ok\n");
	/* Register charger device */
	sgm->chg_dev = charger_device_register("secondary_chg",
						&client->dev, sgm,
						&sgm4154x_sub_chg_ops,
						&sgm4154x_sub_chg_props);
	if (IS_ERR_OR_NULL(sgm->chg_dev)) {
		pr_err("%s: register charger device  failed\n", __func__);
		ret = PTR_ERR(sgm->chg_dev);
		return ret;
	}
	pr_err("xuchen sgm4154x_sub_driver_probe2 is ok\n");
	/* otg regulator */
	s_chg_dev_otg=sgm->chg_dev;
		
	
	INIT_DELAYED_WORK(&sgm->charge_detect_delayed_work, charger_detect_work_func);
	INIT_DELAYED_WORK(&sgm->charge_monitor_work, charger_monitor_work_func);
	INIT_DELAYED_WORK(&sgm->hvdcp_work, sgm4154x_sub_chg_type_hvdcp_work);
	if (client->irq) {
		ret = devm_request_threaded_irq(dev, client->irq, NULL,
						sgm4154x_sub_irq_handler_thread,
						IRQF_TRIGGER_FALLING |
						IRQF_ONESHOT,
						dev_name(&client->dev), sgm);
		if (ret)
			return ret;
		enable_irq_wake(client->irq);
	}	
	pr_err("xuchen sgm4154x_sub_driver_probe3 is ok\n");
	ret = sgm4154x_sub_power_supply_init(sgm, dev);
	if (ret) {
		pr_err("Failed to register power supply\n");
		return ret;
	}
	pr_err("xuchen sgm4154x_sub_driver_probe4 is ok\n");
	ret = sgm4154x_sub_hw_init(sgm);
	if (ret) {
		dev_err(dev, "Cannot initialize the chip.\n");
		return ret;
	}
	
	pr_err("xuchen sgm4154x_sub_driver_probe5 is ok\n");
	//OTG setting
	//sgm4154x_sub_set_otg_voltage(s_chg_dev_otg, 5000000); //5V
	//sgm4154x_sub_set_otg_current(s_chg_dev_otg, 1200000); //1.2A

	//ret = sgm4154x_sub_vbus_regulator_register(sgm);
	
	schedule_delayed_work(&sgm->charge_monitor_work,100);
	otg_in_wakelock = wakeup_source_register(&client->dev,"sgm415xx_sub_otg_wakelock");
	pr_err("xuchen sgm4154x_sub_driver_probe is ok\n");
	return ret;

}

static int sgm4154x_sub_charger_remove(struct i2c_client *client)
{
    struct sgm4154x_sub_device *sgm = i2c_get_clientdata(client);

    cancel_delayed_work_sync(&sgm->charge_monitor_work);

    regulator_unregister(sgm->otg_rdev);

    power_supply_unregister(sgm->charger); 
	
	mutex_destroy(&sgm->lock);
    mutex_destroy(&sgm->i2c_rw_lock);       

    return 0;
}

static void sgm4154x_sub_charger_shutdown(struct i2c_client *client)
{
    int ret = 0;
	
	struct sgm4154x_sub_device *sgm = i2c_get_clientdata(client);
    ret = sgm4154x_sub_disable_charger(sgm);
    if (ret) {
        pr_err("Failed to disable charger, ret = %d\n", ret);
    }
    pr_info("sgm4154x_sub_charger_shutdown\n");
}

static const struct i2c_device_id sgm4154x_sub_i2c_ids[] = {
	{ "sgm41541", 0 },
	{ "sgm41542", 1 },
	{ "sgm41543", 2 },
	{ "sgm41543D", 3 },
	{ "sgm41513", 4 },
	{ "sgm41513A", 5 },
	{ "sgm41513D", 6 },
	{ "sgm41516", 7 },
	{ "sgm41516D", 8 },
	{},
};
MODULE_DEVICE_TABLE(i2c, sgm4154x_sub_i2c_ids);

static const struct of_device_id sgm4154x_sub_of_match[] = {
	{ .compatible = "sgm,sgm41541", },
	{ .compatible = "sgm,sgm41542", },
	{ .compatible = "sgm,sgm41543", },
	{ .compatible = "sgm,sgm41543D", },
	{ .compatible = "sgm,sgm41513", },
	{ .compatible = "sgm,sgm41513A", },
	{ .compatible = "sgm,sgm41513D", },
	{ .compatible = "sgm,sgm41516", },
	{ .compatible = "sgm,sgm41516D", },
	{ .compatible = "sgm,sgm4154x_sub", },
	
	{ },
};
MODULE_DEVICE_TABLE(of, sgm4154x_sub_of_match);


static struct i2c_driver sgm4154x_sub_driver = {
	.driver = {
		.name = "sgm4154x_sub-charger",
		.of_match_table = sgm4154x_sub_of_match,		
	},
	.probe = sgm4154x_sub_driver_probe,
	.remove = sgm4154x_sub_charger_remove,
	.shutdown = sgm4154x_sub_charger_shutdown,
	.id_table = sgm4154x_sub_i2c_ids,
};
//module_i2c_driver(sgm4154x_sub_driver);
static int __init sgm4154x_sub_init(void)
{

	if (i2c_add_driver(&sgm4154x_sub_driver) != 0)
		pr_info("Failed to register sgm4154x_sub i2c driver.\n");
	else
		pr_info("Success to register sgm4154x_sub i2c driver.\n");

	return 0;
}

static void __exit sgm4154x_sub_exit(void)
{
	i2c_del_driver(&sgm4154x_sub_driver);
}

module_init(sgm4154x_sub_init);
module_exit(sgm4154x_sub_exit);
MODULE_AUTHOR(" qhq <Allen_qin@sg-micro.com>");
MODULE_DESCRIPTION("sgm4154x_sub charger driver");
MODULE_LICENSE("GPL v2");
