// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/types.h>
#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#endif
#include <mt-plat/mtk_boot.h>
#include <mt-plat/upmu_common.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include "eta6963.h"
//#include "charger_class.h"
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include "mtk_charger_intf.h"
/**********************************************************
 *
 *   [I2C Slave Setting]
 *
 *********************************************************/

#define GETARRAYNUM(array) (ARRAY_SIZE(array))
static struct wakeup_source *otg_in_wakelock;

static int eta6965_en_gpio;
/*eta6963 REG06 VREG[5:0]*/
static const unsigned int VBAT_CV_VTH[] = {
	3848000,3880000,3912000,3944000,
	3976000,4008000,4040000,4072000,
	4104000,4136000,4168000,4200000,
	4232000,4264000,4296000,4328000,
	4360000,4392000,4424000,4456000,
	4488000,4520000,4552000,4584000,
	4616000

};

/*ETA6369 REG00 IINLIM[5:0]*/
static const unsigned int INPUT_CS_VTH[] = {
	0,100,200,300,400,
	500,600,700,800,
	900,1000,1100,1200,
	1300,1400,1500,1600,
	1700,1800,1900,2000,
	2100,2200,2300,2400,
	2500,2600,2700,2800,2900,
	3000,3100,3200
};
/*ETA6369 REG04 ICHG[6:0]*/
static const unsigned int CS_VTH[] = {
	0,60,120,180,
	240,300,360,420,
	480,540,600,660,
	720,780,840,900,
	960,1020,1080,1140,
	1200,1260,1320,1380,
	1440,1500,1560,1620,
	1680,1740,1800,1860,
	1920,1980,2040,2100,
	2160,2220,2280,2340,
	2400,2460,2520,2580,
	2640,2700,2760,2820,
	2880,2940,3000
};


static const unsigned int VCDT_HV_VTH[] = {
	4200000, 4250000, 4300000, 4350000,
	4400000, 4450000, 4500000, 4550000,
	4600000, 6000000, 6500000, 7000000,
	7500000, 8500000, 9500000, 10500000

};


static const unsigned int VINDPM_REG[] = {
	3900, 4000, 4100, 4200, 4300, 4400,
	4500, 4600, 4700, 4800, 4900, 5000,
	5100, 5200, 5300, 5400, 5500, 5600,
	5700, 5800, 5900, 6000, 6100, 6200,
	6300, 6400
};

/* ETA6369 REG0A BOOST_LIM[2:0], mA */
static const unsigned int BOOST_CURRENT_LIMIT[] = {
	500, 1200
};

struct eta6963_info {
	struct charger_device *chg_dev;
	struct charger_properties chg_props;
	struct device *dev;
	const char *chg_dev_name;
	const char *eint_name;
	int irq;
	struct regulator_dev *otg_rdev;
};

//DEFINE_MUTEX(g_input_current_mutex);
static struct i2c_client *new_client;
static const struct i2c_device_id eta6963_i2c_id[] = { {"eta6963", 0}, {} };

static int eta6963_driver_probe(struct i2c_client *client,
				const struct i2c_device_id *id);

static unsigned int charging_parameter_to_value(const unsigned int
		*parameter, const unsigned int array_size,
		const unsigned int val)
{
	unsigned int i;

	pr_debug_ratelimited("array_size = %d\n", array_size);

	for (i = 0; i < array_size; i++) {
		if (val == *(parameter + i))
			return i;
	}

	pr_info("NO register value match\n");
	/* TODO: ASSERT(0);    // not find the value */
	return 0;
}

static unsigned int bmt_find_closest_level(const unsigned int *pList,
		unsigned int number,
		unsigned int level)
{
	unsigned int i;
	unsigned int max_value_in_last_element;

	if (pList[0] < pList[1])
		max_value_in_last_element = 1;
	else
		max_value_in_last_element = 0;

	if (max_value_in_last_element == 1) {
		for (i = (number - 1); i != 0;
		     i--) {	/* max value in the last element */
			if (pList[i] <= level) {
				pr_debug_ratelimited("zzf_%d<=%d, i=%d\n",
					pList[i], level, i);
				return pList[i];
			}
		}

		pr_info("Can't find closest level\n");
		return pList[0];
		/* return CHARGE_CURRENT_0_00_MA; */
	} else {
		/* max value in the first element */
		for (i = 0; i < number; i++) {
			if (pList[i] <= level)
				return pList[i];
		}

		pr_info("Can't find closest level\n");
		return pList[number - 1];
		/* return CHARGE_CURRENT_0_00_MA; */
	}
}


/**********************************************************
 *
 *   [Global Variable]
 *
 *********************************************************/
unsigned char eta6963_reg[eta6963_REG_NUM] = { 0 };

static DEFINE_MUTEX(eta6963_i2c_access);
static DEFINE_MUTEX(eta6963_access_lock);

int g_eta6963_hw_exist;

/**********************************************************
 *
 *   [I2C Function For Read/Write eta6963]
 *
 *********************************************************/
#ifdef CONFIG_MTK_I2C_EXTENSION
unsigned int eta6963_read_byte(unsigned char cmd,
			       unsigned char *returnData)
{
	char cmd_buf[1] = { 0x00 };
	char readData = 0;
	int ret = 0;

	mutex_lock(&eta6963_i2c_access);

	/* new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) |
	 * I2C_WR_FLAG;
	 */
	new_client->ext_flag =
		((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_WR_FLAG |
		I2C_DIRECTION_FLAG;

	cmd_buf[0] = cmd;
	ret = i2c_master_send(new_client, &cmd_buf[0], (1 << 8 | 1));
	if (ret < 0) {
		/* new_client->addr = new_client->addr & I2C_MASK_FLAG; */
		new_client->ext_flag = 0;
		mutex_unlock(&eta6963_i2c_access);

		return 0;
	}

	readData = cmd_buf[0];
	*returnData = readData;

	/* new_client->addr = new_client->addr & I2C_MASK_FLAG; */
	new_client->ext_flag = 0;
	mutex_unlock(&eta6963_i2c_access);

	return 1;
}

unsigned int eta6963_write_byte(unsigned char cmd,
				unsigned char writeData)
{
	char write_data[2] = { 0 };
	int ret = 0;

	mutex_lock(&eta6963_i2c_access);

	write_data[0] = cmd;
	write_data[1] = writeData;

	new_client->ext_flag = ((new_client->ext_flag) & I2C_MASK_FLAG) |
			       I2C_DIRECTION_FLAG;

	ret = i2c_master_send(new_client, write_data, 2);
	if (ret < 0) {
		new_client->ext_flag = 0;
		mutex_unlock(&eta6963_i2c_access);
		return 0;
	}

	new_client->ext_flag = 0;
	mutex_unlock(&eta6963_i2c_access);
	return 1;
}
#else
unsigned int eta6963_read_byte(unsigned char cmd,
			       unsigned char *returnData)
{
	unsigned char xfers = 2;
	int ret, retries = 1;

	mutex_lock(&eta6963_i2c_access);

	do {
		struct i2c_msg msgs[2] = {
			{
				.addr = new_client->addr,
				.flags = 0,
				.len = 1,
				.buf = &cmd,
			},
			{

				.addr = new_client->addr,
				.flags = I2C_M_RD,
				.len = 1,
				.buf = returnData,
			}
		};

		/*
		 * Avoid sending the segment addr to not upset non-compliant
		 * DDC monitors.
		 */
		ret = i2c_transfer(new_client->adapter, msgs, xfers);

		if (ret == -ENXIO) {
			pr_info("skipping non-existent adapter %s\n",
				new_client->adapter->name);
			break;
		}
	} while (ret != xfers && --retries);

	mutex_unlock(&eta6963_i2c_access);

	return ret == xfers ? 1 : -1;
}

unsigned int eta6963_write_byte(unsigned char cmd,
				unsigned char writeData)
{
	unsigned char xfers = 1;
	int ret, retries = 1;
	unsigned char buf[8];

	mutex_lock(&eta6963_i2c_access);

	buf[0] = cmd;
	memcpy(&buf[1], &writeData, 1);

	do {
		struct i2c_msg msgs[1] = {
			{
				.addr = new_client->addr,
				.flags = 0,
				.len = 1 + 1,
				.buf = buf,
			},
		};

		/*
		 * Avoid sending the segment addr to not upset non-compliant
		 * DDC monitors.
		 */
		ret = i2c_transfer(new_client->adapter, msgs, xfers);

		if (ret == -ENXIO) {
			pr_info("skipping non-existent adapter %s\n",
				new_client->adapter->name);
			break;
		}
	} while (ret != xfers && --retries);

	mutex_unlock(&eta6963_i2c_access);

	return ret == xfers ? 1 : -1;
}
#endif
/**********************************************************
 *
 *   [Read / Write Function]
 *
 *********************************************************/
unsigned int eta6963_read_interface(unsigned char RegNum,
				    unsigned char *val, unsigned char MASK,
				    unsigned char SHIFT)
{
	unsigned char eta6963_reg = 0;
	unsigned int ret = 0;

	ret = eta6963_read_byte(RegNum, &eta6963_reg);

	pr_debug_ratelimited("[%s] Reg[%x]=0x%x\n", __func__,
			     RegNum, eta6963_reg);

	eta6963_reg &= (MASK << SHIFT);
	*val = (eta6963_reg >> SHIFT);

	pr_debug_ratelimited("[%s] val=0x%x\n", __func__, *val);

	return ret;
}

unsigned int eta6963_config_interface(unsigned char RegNum,
				      unsigned char val, unsigned char MASK,
				      unsigned char SHIFT)
{
	unsigned char eta6963_reg = 0;
	unsigned char eta6963_reg_ori = 0;
	unsigned int ret = 0;

	mutex_lock(&eta6963_access_lock);
	ret = eta6963_read_byte(RegNum, &eta6963_reg);

	eta6963_reg_ori = eta6963_reg;
	eta6963_reg &= ~(MASK << SHIFT);
	eta6963_reg |= (val << SHIFT);

	ret = eta6963_write_byte(RegNum, eta6963_reg);
	mutex_unlock(&eta6963_access_lock);
	pr_debug_ratelimited("[%s] write Reg[%x]=0x%x from 0x%x\n", __func__,
			     RegNum,
			     eta6963_reg, eta6963_reg_ori);

	/* Check */
	/* eta6963_read_byte(RegNum, &eta6963_reg); */
	/* pr_info("[%s] Check Reg[%x]=0x%x\n", __func__,*/
	/* RegNum, eta6963_reg); */

	return ret;
}

/* write one register directly */
unsigned int eta6963_reg_config_interface(unsigned char RegNum,
		unsigned char val)
{
	unsigned int ret = 0;

	ret = eta6963_write_byte(RegNum, val);

	return ret;
}

/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/
/* CON0---------------------------------------------------- */
void eta6963_set_en_hiz(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON0),
				       (unsigned char) (val),
				       (unsigned char) (CON0_EN_HIZ_MASK),
				       (unsigned char) (CON0_EN_HIZ_SHIFT)
				      );
}

void eta6963_set_iinlim(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON0),
				       (unsigned char) (val),
				       (unsigned char) (CON0_IINLIM_MASK),
				       (unsigned char) (CON0_IINLIM_SHIFT)
				      );
}

void eta6963_set_stat_ctrl(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON0),
				   (unsigned char) (val),
				   (unsigned char) (CON0_STAT_IMON_CTRL_MASK),
				   (unsigned char) (CON0_STAT_IMON_CTRL_SHIFT)
				   );
}

/* CON1---------------------------------------------------- */

void eta6963_set_reg_rst(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON11),
				       (unsigned char) (val),
				       (unsigned char) (CON11_REG_RST_MASK),
				       (unsigned char) (CON11_REG_RST_SHIFT)
				      );
}

void eta6963_set_pfm(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_PFM_MASK),
				       (unsigned char) (CON1_PFM_SHIFT)
				      );
}

void eta6963_set_wdt_rst(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_WDT_RST_MASK),
				       (unsigned char) (CON1_WDT_RST_SHIFT)
				      );
}

void eta6963_set_otg_config(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_OTG_CONFIG_MASK),
				       (unsigned char) (CON1_OTG_CONFIG_SHIFT)
				      );
}

unsigned int eta6963_get_otg_config(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta6963_read_interface((unsigned char) (eta6963_CON1),
				     (&val),
				     (unsigned char) (CON1_OTG_CONFIG_MASK),
				     (unsigned char) (CON1_OTG_CONFIG_SHIFT)
				    );
	return val;
}

void eta6963_set_chg_config(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_CHG_CONFIG_MASK),
				       (unsigned char) (CON1_CHG_CONFIG_SHIFT)
				      );
}

unsigned int eta6963_get_chg_config(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta6963_read_interface((unsigned char) (eta6963_CON1),
				     (&val),
				     (unsigned char) (CON1_CHG_CONFIG_MASK),
				     (unsigned char) (CON1_CHG_CONFIG_SHIFT)
				    );
	return val;
}
void eta6963_set_sys_min(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_SYS_MIN_MASK),
				       (unsigned char) (CON1_SYS_MIN_SHIFT)
				      );
}

void eta6963_set_batlowv(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_MIN_VBAT_SEL_MASK),
				       (unsigned char) (CON1_MIN_VBAT_SEL_SHIFT)
				      );
}



/* CON2---------------------------------------------------- */
void eta6963_set_rdson(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON2),
				       (unsigned char) (val),
				       (unsigned char) (CON2_Q1_FULLON_MASK),
				       (unsigned char) (CON2_Q1_FULLON_SHIFT)
				      );
}

void eta6963_set_boost_lim(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON2),
				       (unsigned char) (val),
				       (unsigned char) (CON2_BOOST_LIM_MASK),
				       (unsigned char) (CON2_BOOST_LIM_SHIFT)
				      );
}

void eta6963_set_ichg(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON2),
				       (unsigned char) (val),
				       (unsigned char) (CON2_ICHG_MASK),
				       (unsigned char) (CON2_ICHG_SHIFT)
				      );
}

#ifdef FIXME //this function does not exist on eta6963
void eta6963_set_force_20pct(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON2),
				       (unsigned char) (val),
				       (unsigned char) (CON2_FORCE_20PCT_MASK),
				       (unsigned char) (CON2_FORCE_20PCT_SHIFT)
				      );
}
#endif
/* CON3---------------------------------------------------- */

void eta6963_set_iprechg(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON3),
				       (unsigned char) (val),
				       (unsigned char) (CON3_IPRECHG_MASK),
				       (unsigned char) (CON3_IPRECHG_SHIFT)
				      );
}

void eta6963_set_iterm(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON3),
				       (unsigned char) (val),
				       (unsigned char) (CON3_ITERM_MASK),
				       (unsigned char) (CON3_ITERM_SHIFT)
				      );
}

/* CON4---------------------------------------------------- */

void eta6963_set_vreg(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON4),
				       (unsigned char) (val),
				       (unsigned char) (CON4_VREG_MASK),
				       (unsigned char) (CON4_VREG_SHIFT)
				      );
}

void eta6963_set_topoff_timer(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON4),
				       (unsigned char) (val),
				       (unsigned char) (CON4_TOPOFF_TIMER_MASK),
				       (unsigned char) (CON4_TOPOFF_TIMER_SHIFT)
				      );

}


void eta6963_set_vrechg(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON4),
				       (unsigned char) (val),
				       (unsigned char) (CON4_VRECHG_MASK),
				       (unsigned char) (CON4_VRECHG_SHIFT)
				      );
}

/* CON5---------------------------------------------------- */

void eta6963_set_en_term(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON5),
				       (unsigned char) (val),
				       (unsigned char) (CON5_EN_TERM_MASK),
				       (unsigned char) (CON5_EN_TERM_SHIFT)
				      );
}



void eta6963_set_watchdog(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON5),
				       (unsigned char) (val),
				       (unsigned char) (CON5_WATCHDOG_MASK),
				       (unsigned char) (CON5_WATCHDOG_SHIFT)
				      );
}

void eta6963_set_en_timer(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON5),
				       (unsigned char) (val),
				       (unsigned char) (CON5_EN_TIMER_MASK),
				       (unsigned char) (CON5_EN_TIMER_SHIFT)
				      );
}

void eta6963_set_chg_timer(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON5),
				       (unsigned char) (val),
				       (unsigned char) (CON5_CHG_TIMER_MASK),
				       (unsigned char) (CON5_CHG_TIMER_SHIFT)
				      );
}

/* CON6---------------------------------------------------- */

void eta6963_set_treg(unsigned int val)
{
#ifdef FIXME
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON6),
				       (unsigned char) (val),
				       (unsigned char) (CON6_BOOSTV_MASK),
				       (unsigned char) (CON6_BOOSTV_SHIFT)
				      );
#endif
}

void eta6963_set_vindpm(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON6),
				       (unsigned char) (val),
				       (unsigned char) (CON6_VINDPM_MASK),
				       (unsigned char) (CON6_VINDPM_SHIFT)
				      );
}


void eta6963_set_ovp(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON6),
				       (unsigned char) (val),
				       (unsigned char) (CON6_OVP_MASK),
				       (unsigned char) (CON6_OVP_SHIFT)
				      );

}

void eta6963_set_boostv(unsigned int val)
{

	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON6),
				       (unsigned char) (val),
				       (unsigned char) (CON6_BOOSTV_MASK),
				       (unsigned char) (CON6_BOOSTV_SHIFT)
				      );
}



/* CON7---------------------------------------------------- */

void eta6963_set_tmr2x_en(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON7),
					(unsigned char) (val),
					(unsigned char) (CON7_TMR2X_EN_MASK),
					(unsigned char) (CON7_TMR2X_EN_SHIFT)
					);
}

void eta6963_set_batfet_disable(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON7),
				(unsigned char) (val),
				(unsigned char) (CON7_BATFET_Disable_MASK),
				(unsigned char) (CON7_BATFET_Disable_SHIFT)
				);
}


void eta6963_set_batfet_delay(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON7),
				       (unsigned char) (val),
				       (unsigned char) (CON7_BATFET_DLY_MASK),
				       (unsigned char) (CON7_BATFET_DLY_SHIFT)
				      );
}

void eta6963_set_batfet_reset_enable(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON7),
				(unsigned char) (val),
				(unsigned char) (CON7_BATFET_RST_EN_MASK),
				(unsigned char) (CON7_BATFET_RST_EN_SHIFT)
				);
}

void eta6963_set_geita_vset(unsigned int val)
{
	unsigned int ret = 0;
	ret = eta6963_config_interface((unsigned char) (eta6963_CON7),
				       (unsigned char) (val),
				       (unsigned char) (CON7_JEITA_VSET_MASK),
				       (unsigned char) (CON7_JEITA_VSET_SHIFT)
				      );
}

/* CON8---------------------------------------------------- */

unsigned int eta6963_get_system_status(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta6963_read_interface((unsigned char) (eta6963_CON8),
				     (&val), (unsigned char) (0xFF),
				     (unsigned char) (0x0)
				    );
	return val;
}

unsigned int eta6963_get_vbus_stat(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta6963_read_interface((unsigned char) (eta6963_CON8),
				     (&val),
				     (unsigned char) (CON8_VBUS_STAT_MASK),
				     (unsigned char) (CON8_VBUS_STAT_SHIFT)
				    );
	return val;
}

static int eta6963_get_chrg_volt(struct charger_device *chg_dev,unsigned int *volt)
{
	int ret;
	u8 vreg_val;
	
	ret = eta6963_read_interface((unsigned char) (eta6963_CON4),
				        (&vreg_val),
				       (unsigned char) (CON4_VREG_MASK),
				       (unsigned char) (CON4_VREG_SHIFT)
				      );
					  
	if (15 == vreg_val)
		*volt = 4352000; //default
	else if (vreg_val < 25)	
		*volt = vreg_val*ETA6963_VREG_V_STEP_uV + ETA6963_VREG_V_MIN_uV;	
	printk("eta6963_get_chrg_volt volt = %dmV\n",volt);
	return 0;
}

unsigned int eta6963_get_chrg_stat(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta6963_read_interface((unsigned char) (eta6963_CON8),
				     (&val),
				     (unsigned char) (CON8_CHRG_STAT_MASK),
				     (unsigned char) (CON8_CHRG_STAT_SHIFT)
				    );
	return val;
}

unsigned int eta6963_get_vsys_stat(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta6963_read_interface((unsigned char) (eta6963_CON8),
				     (&val),
				     (unsigned char) (CON8_VSYS_STAT_MASK),
				     (unsigned char) (CON8_VSYS_STAT_SHIFT)
				    );
	return val;
}

unsigned int eta6963_get_pg_stat(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta6963_read_interface((unsigned char) (eta6963_CON8),
				     (&val),
				     (unsigned char) (CON8_PG_STAT_MASK),
				     (unsigned char) (CON8_PG_STAT_SHIFT)
				    );
	return val;
}

/*CON10----------------------------------------------------------*/

void eta6963_set_int_mask(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta6963_config_interface((unsigned char) (eta6963_CON10),
				       (unsigned char) (val),
				       (unsigned char) (CON10_INT_MASK_MASK),
				       (unsigned char) (CON10_INT_MASK_SHIFT)
				      );
}

/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/
static int eta6963_dump_register(struct charger_device *chg_dev)
{

	unsigned char i = 0;
	unsigned int ret = 0;

	pr_info("[%s]: ", __func__);
	for (i = 0; i < eta6963_REG_NUM; i++) {
		ret = eta6963_read_byte(i, &eta6963_reg[i]);
		if (ret == 0) {
			pr_info("[eta6963] i2c transfor error\n");
			return 1;
		}
		pr_info("[eta6963][0x%x]=0x%x ", i, eta6963_reg[i]);
	}
	pr_info("%s en_pin = %d\n", __func__, gpio_get_value(eta6965_en_gpio));
	pr_debug("\n");
	return 0;
}


/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/
static void eta6963_hw_component_detect(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta6963_read_interface(0x0B, &val, 0xFF, 0x0);

	if (val == 0)
		g_eta6963_hw_exist = 0;
	else
		g_eta6963_hw_exist = 1;

	pr_info("[%s] exist=%d, Reg[0x0B]=0x%x\n", __func__,
		g_eta6963_hw_exist, val);
}


static int eta6963_enable_charging(struct charger_device *chg_dev,
				   bool en)
{
	int status = 0;

	pr_info("enable state : %d\n", en);
	if (en) {
		/* eta6963_config_interface(eta6963_CON3, 0x1, 0x1, 4); */
		/* enable charging */
		eta6963_set_en_hiz(0x0);
		eta6963_set_chg_config(en);
		eta6963_set_geita_vset(en);
	} else {
		/* eta6963_config_interface(eta6963_CON3, 0x0, 0x1, 4); */
		/* enable charging */
		eta6963_set_chg_config(en);
		pr_info("[charging_enable] under test mode: disable charging\n");

		/*eta6963_set_en_hiz(0x1);*/
	}

	return status;
}


static int eta6963_charger_is_enabled(struct charger_device *chg_dev, bool *en)
{
        int ret = 0;
        ret = eta6963_get_chg_config();
        if (ret < 0)
                return ret;
        *en = (ret) ? true : false;
        return 0;
}
static int eta6963_get_current(struct charger_device *chg_dev,
			       u32 *ichg)
{
	unsigned char ret_val = 0;

	/* Get current level */
	eta6963_read_interface((unsigned char) (eta6963_CON2),
				     (&ret_val),
				     (unsigned char) (CON2_ICHG_MASK),
				     (unsigned char) (CON2_ICHG_SHIFT)
				    );
	/* Parsing */
	//ret_val = (ret_val * 64) + 512;
	//ret_val = ret_val * ETA6963_ICHRG_I_STEP_uA;

	//ret_val = 2000;
	
	*ichg = (ret_val * 64) + 512;
	printk("eta6963_get_current ichg = %d\n", *ichg);
	
	return ret_val;
}

static int eta6963_get_ibus_adc(struct charger_device *chg_dev,
			       u32 *ichg)
{
	unsigned char ret_val = 0;
	unsigned int curr;
	/* Get current level */
	eta6963_read_interface((unsigned char) (eta6963_CON2),
				     (&ret_val),
				     (unsigned char) (CON2_ICHG_MASK),
				     (unsigned char) (CON2_ICHG_SHIFT)
				    );
#if 0					
	if (ret_val <= 0x8)
		curr = ret_val * 5000;
	else if (ret_val <= 0xF)
		curr = 40000 + (ret_val - 0x8) * 10000;
	else if (ret_val <= 0x17)
		curr = 110000 + (ret_val - 0xF) * 20000;
	else if (ret_val <= 0x20)
		curr = 270000 + (ret_val - 0x17) * 30000;
	else if (ret_val <= 0x30)
		curr = 540000 + (ret_val - 0x20) * 60000;
	else if (ret_val <= 0x3C)
		curr = 1500000 + (ret_val - 0x30) * 120000;
	else
		curr = 3000000;
#endif
	curr = ret_val * ETA6963_ICHRG_I_STEP_uA;
	*ichg = curr; 
	printk("eta6963_get_ibus_adc ichg = %d\n", *ichg);
	
	return ret_val;
}

static int eta6963_set_current(struct charger_device *chg_dev,
			       u32 current_value)
{
	unsigned int status = true;
	unsigned int set_chr_current;
	unsigned int array_size;
	unsigned int register_value;

	pr_info("&&&& charge_current_value = %d\n", current_value);
	current_value /= 1000;
	array_size = GETARRAYNUM(CS_VTH);
	set_chr_current = bmt_find_closest_level(CS_VTH, array_size,
			  current_value);
	register_value = charging_parameter_to_value(CS_VTH, array_size,
			 set_chr_current);
	//pr_info("&&&& charge_register_value = %d\n",register_value);
	pr_info("&&&& %s register_value = %d\n", __func__,
		register_value);
	eta6963_set_ichg(register_value);

	return status;
}

static int eta6963_get_input_current(struct charger_device *chg_dev,
				     u32 *aicr)
{
	int ret = 0;
#ifdef FIXME
	unsigned char val = 0;

	eta6963_read_interface(eta6963_CON0, &val, CON0_IINLIM_MASK,
			       CON0_IINLIM_SHIFT);
	ret = (int)val;
	*aicr = INPUT_CS_VTH[val];
#endif
	return ret;
}


static int eta6963_set_input_current(struct charger_device *chg_dev,
				     u32 current_value)
{
	unsigned int status = true;
	unsigned int set_chr_current;
	unsigned int array_size;
	unsigned int register_value;

	current_value /= 1000;
	current_value=current_value-100;
	pr_info("&&&& current_value = %d\n", current_value);
	array_size = GETARRAYNUM(INPUT_CS_VTH);
	set_chr_current = bmt_find_closest_level(INPUT_CS_VTH, array_size,
			  current_value);
	register_value = charging_parameter_to_value(INPUT_CS_VTH, array_size,
			 set_chr_current);
	pr_info("&&&& %s register_value = 0x%02x\n", __func__,
		register_value);
	// eta6963_set_iinlim(register_value);
	eta6963_set_iinlim(0x17);

	return status;
}

static int eta6963_set_cv_voltage(struct charger_device *chg_dev,
				  u32 cv)
{
	unsigned int status = true;
	unsigned int array_size;
	unsigned int set_cv_voltage;
	unsigned short register_value;

	array_size = GETARRAYNUM(VBAT_CV_VTH);
	set_cv_voltage = bmt_find_closest_level(VBAT_CV_VTH, array_size, cv);
	register_value = charging_parameter_to_value(VBAT_CV_VTH, array_size,
			 set_cv_voltage);
	eta6963_set_vreg(register_value);
	pr_info("&&&& cv reg value = %d\n", register_value);

	return status;
}

static int eta6963_reset_watch_dog_timer(struct charger_device
		*chg_dev)
{
	unsigned int status = true;

	pr_info("charging_reset_watch_dog_timer\n");

	eta6963_set_wdt_rst(0x1);	/* Kick watchdog */
	eta6963_set_watchdog(0x3);	/* WDT 160s */

	return status;
}


static int eta6963_set_vindpm_voltage(struct charger_device *chg_dev,
				      u32 vindpm)
{
	int status = 0;
	unsigned int array_size;

	vindpm /= 1000;
	array_size = ARRAY_SIZE(VINDPM_REG);
	vindpm = bmt_find_closest_level(VINDPM_REG, array_size, vindpm);
	vindpm = charging_parameter_to_value(VINDPM_REG, array_size, vindpm);

	pr_info("%s vindpm =%d\r\n", __func__, vindpm);

	//	charging_set_vindpm(vindpm);
	/*eta6963_set_en_hiz(en);*/

	return status;
}

static int eta6963_get_charging_status(struct charger_device *chg_dev,
				       bool *is_done)
{
	unsigned int status = true;
	unsigned int ret_val;

	ret_val = eta6963_get_chrg_stat();

	if (ret_val == 0x3)
		*is_done = true;
	else
		*is_done = false;

	return status;
}

static int eta6963_enable_otg(struct charger_device *chg_dev, bool en)
{
	int ret = 0;

	pr_info("%s en = %d\n", __func__, en);
	if (en) {
		eta6963_set_chg_config(0);
		eta6963_set_otg_config(1);
		eta6963_set_watchdog(0x3);	/* WDT 160s */
		__pm_stay_awake(otg_in_wakelock);
	} else {
		eta6963_set_otg_config(0);
		eta6963_set_chg_config(1);
		__pm_relax(otg_in_wakelock);
	}
	return ret;
}

static int eta6963_set_boost_current_limit(struct charger_device
		*chg_dev, u32 uA)
{
	int ret = 0;
	u32 array_size = 0;
	u32 boost_ilimit = 0;
	u8 boost_reg = 0;

	uA /= 1000;
	array_size = ARRAY_SIZE(BOOST_CURRENT_LIMIT);
	boost_ilimit = bmt_find_closest_level(BOOST_CURRENT_LIMIT, array_size,
					      uA);
	boost_reg = charging_parameter_to_value(BOOST_CURRENT_LIMIT,
						array_size, boost_ilimit);
	eta6963_set_boost_lim(boost_reg);

	return ret;
}

static int eta6963_enable_safetytimer(struct charger_device *chg_dev,
				      bool en)
{
	int status = 0;

	if (en)
		eta6963_set_en_timer(0x1);
	else
		eta6963_set_en_timer(0x0);
	return status;
}

static int eta6963_get_is_safetytimer_enable(struct charger_device
		*chg_dev, bool *en)
{
	unsigned char val = 0;

	eta6963_read_interface(eta6963_CON5, &val, CON5_EN_TIMER_MASK,
			       CON5_EN_TIMER_SHIFT);
	*en = (bool)val;
	return val;
}


static unsigned int charging_hw_init(void)
{
	unsigned int status = 0;

	eta6963_set_en_hiz(0x0);
	eta6963_set_vindpm(0x6);	/* VIN DPM check 4.6V */
	eta6963_set_wdt_rst(0x1);	/* Kick watchdog */
	eta6963_set_sys_min(0x5);	/* Minimum system voltage 3.5V */
	eta6963_set_iprechg(0x8);	/* Precharge current 540mA */
	eta6963_set_iterm(0x2);	/* Termination current 180mA */
	eta6963_set_vreg(0x11);	/* VREG 4.4V */
	eta6963_set_pfm(0x0);//disable pfm
	eta6963_set_rdson(0x0);     /*close rdson*/
	eta6963_set_batlowv(0x1);	/* BATLOWV 3.0V */
	eta6963_set_vrechg(0x0);	/* VRECHG 0.1V (4.108V) */
	eta6963_set_en_term(0x1);	/* Enable termination */
	eta6963_set_watchdog(0x0);	/* WDT disable */
	eta6963_set_en_timer(0x0);	/* Enable charge timer */
	eta6963_set_int_mask(0x0);	/* Disable fault interrupt */
	eta6963_set_ovp(0x3);
	mdelay(2000);
	eta6963_set_iinlim(0x17);
	
	pr_info("%s: hw_init down!\n", __func__);
	return status;
}

static int eta6963_parse_dt(struct eta6963_info *info,
			    struct device *dev)
{
	struct device_node *np = dev->of_node;
	int eta6963_en_pin = 0;

	pr_info("%s\n", __func__);
	if (!np) {
		pr_info("%s: no of node\n", __func__);
		return -ENODEV;
	}

	if (of_property_read_string(np, "charger_name",
				    &info->chg_dev_name) < 0) {
		info->chg_dev_name = "primary_chg";
		pr_info("%s: no charger name\n", __func__);
	}

	if (of_property_read_string(np, "alias_name",
				    &(info->chg_props.alias_name)) < 0) {
		info->chg_props.alias_name = "eta6963";
		pr_info("%s: no alias name\n", __func__);
	}

 	eta6963_en_pin = of_get_named_gpio(np,"eta,chg-en-gpio",0);
	if(eta6963_en_pin < 0){
		pr_info("%s: no eta6963_en_pin\n", __func__);
		return -ENODATA;
	}
	
	gpio_request(eta6963_en_pin,"eta6963_en_pin");
	gpio_direction_output(eta6963_en_pin,0);
	gpio_set_value(eta6963_en_pin,0); 

	eta6965_en_gpio = eta6963_en_pin;
	pr_info("%s end %d, %d\n", __func__, eta6965_en_gpio, gpio_get_value(eta6965_en_gpio));

	return 0;
}

static int eta6963_do_event(struct charger_device *chg_dev, u32 event,
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

static int eta6963_enable_chg_type_det(struct charger_device *chg_dev)
{
	int ret = 0;
	int pg_stat = 0;
	unsigned int usb_status = 0,bq_detect_count=0;
	u8 val = 0;
	int i=0;

	pr_err("%s enter!!!\n",__func__);
    eta6963_dump_register(chg_dev);
	Charger_Detect_Init();
	ret = CHARGER_UNKNOWN;
	do{
		msleep(50);
		pg_stat = eta6963_get_pg_stat();
            if (pg_stat==1) {
               bq_detect_count = 0 ;
			   pr_err("%s: eta6963_get_pg_stat is good!\r\n",__func__);
            } else {
               bq_detect_count++;
            }
		pr_err("%s: count_max=%d,pg_stat=%d\n",__func__,bq_detect_count,pg_stat);
		if(bq_detect_count>10) {
			bq_detect_count = 0;
			pr_err("%s: eta6963_get_pg_stat power not good!\r\n",__func__);
			Charger_Detect_Release();
			ret = CHARGER_UNKNOWN;
			return ret;
		}
	}while (bq_detect_count);

	ret=eta6963_get_vbus_stat();
	
	for (i;i<=10;i++)
	{
	if (ret>0){
		break;
	} else {
	msleep(300);
	ret=eta6963_get_vbus_stat();
	pr_err("%s: read 0x0b   ret = %d [0x0B] = 0x%X\n", __func__, ret, val);
	}
	}
	pr_err("joyar   %s: ret=%d  !! \n",__func__, ret);
    switch (ret) {
    case ETA6963_VBUS_TYPE_SDP:
    case ETA6963_VBUS_TYPE_CDP:
		ret = CHARGING_HOST;
		pr_err("%s: charge type = CHARGING_HOST ! \n", __func__);
        break;
	case ETA6963_VBUS_TYPE_NON_STD:
    case ETA6963_VBUS_TYPE_DCP:
    case ETA6963_VBUS_TYPE_HVDCP:
    case ETA6963_VBUS_TYPE_UNKNOWN:
		ret = STANDARD_CHARGER;
		pr_err("%s: charge type = STANDARD_CHARGER ! \n", __func__);
        break;
	case ETA6963_VBUS_TYPE_NONE:
    default:
		ret = CHARGER_UNKNOWN;
		pr_err("%s: charge type = CHARGER_UNKNOWN ! \n", __func__);
        break;
    }
	pr_err("%s: charge type ret = %d usb_status = %d\n", __func__, ret,usb_status);
	Charger_Detect_Release();
	return ret;
}


static int eta6963_is_chip_enabled(struct charger_device *chg_dev, bool *en)
{
        *en = g_eta6963_hw_exist;
        return 0;
}
static struct charger_ops eta6963_chg_ops = {
#ifdef FIXME
	.enable_hz = eta6963_enable_hz,
#endif

	/* Normal charging */
	.dump_registers = eta6963_dump_register,
	.enable = eta6963_enable_charging,
	.is_enabled = eta6963_charger_is_enabled,
	.get_charging_current = eta6963_get_current,
	.set_charging_current = eta6963_set_current,
	.get_input_current = eta6963_get_input_current,
	.set_input_current = eta6963_set_input_current,
	/*.get_constant_voltage = eta6963_get_battery_voreg,*/
	.set_constant_voltage = eta6963_set_cv_voltage,
	.kick_wdt = eta6963_reset_watch_dog_timer,
	.set_mivr = eta6963_set_vindpm_voltage,
	.is_charging_done = eta6963_get_charging_status,

	/* Safety timer */
	.enable_safety_timer = eta6963_enable_safetytimer,
	.is_safety_timer_enabled = eta6963_get_is_safetytimer_enable,


	/* Power path */
	/*.enable_powerpath = eta6963_enable_power_path, */
	/*.is_powerpath_enabled = eta6963_get_is_power_path_enable, */

	.get_ext_chgtyp = eta6963_enable_chg_type_det,
	
	/* OTG */
	.enable_otg = eta6963_enable_otg,
	.set_boost_current_limit = eta6963_set_boost_current_limit,
	.event = eta6963_do_event,
	.is_chip_enabled = eta6963_is_chip_enabled,
	
       /* ADC */
    .get_adc = NULL,
    .get_vbus_adc = eta6963_get_chrg_volt,
    .get_ibus_adc = eta6963_get_ibus_adc,
    .get_ibat_adc = NULL,
    .get_tchg_adc = NULL,
    .get_zcv = NULL,

};

static int eta6963_driver_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int ret = 0;
	struct eta6963_info *info = NULL;

	pr_info("[%s]\n", __func__);

	info = devm_kzalloc(&client->dev, sizeof(struct eta6963_info),
			    GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	new_client = client;
	info->dev = &client->dev;

	ret = eta6963_parse_dt(info, &client->dev);
	if (ret < 0)
		return ret;

	eta6963_hw_component_detect();
	charging_hw_init();

	/* Register charger device */
	info->chg_dev = charger_device_register(info->chg_dev_name,
						&client->dev, info,
						&eta6963_chg_ops,
						&info->chg_props);
	if (IS_ERR_OR_NULL(info->chg_dev)) {
		pr_info("%s: register charger device  failed\n", __func__);
		ret = PTR_ERR(info->chg_dev);
		return ret;
	}

	otg_in_wakelock = wakeup_source_register(&client->dev,"eta6963_otg_wakelock");
	eta6963_dump_register(info->chg_dev);

	return 0;
}

/**********************************************************
 *
 *   [platform_driver API]
 *
 *********************************************************/
unsigned char g_reg_value_eta6963;
static ssize_t eta6963_access_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	pr_info("[%s] 0x%x\n", __func__, g_reg_value_eta6963);
	return sprintf(buf, "%u\n", g_reg_value_eta6963);
}

static ssize_t eta6963_access_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL, *addr, *val;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;

	pr_info("[%s]\n", __func__);

	if (buf != NULL && size != 0) {
		pr_info("[%s] buf is %s and size is %zu\n", __func__, buf,
			size);

		pvalue = (char *)buf;
		if (size > 3) {
			addr = strsep(&pvalue, " ");
			ret = kstrtou32(addr, 16,
				(unsigned int *)&reg_address);
		} else
			ret = kstrtou32(pvalue, 16,
				(unsigned int *)&reg_address);

		if (size > 3) {
			val = strsep(&pvalue, " ");
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);
			pr_info(
			"[%s] write eta6963 reg 0x%x with value 0x%x !\n",
			__func__,
			(unsigned int) reg_address, reg_value);
			ret = eta6963_config_interface(reg_address,
				reg_value, 0xFF, 0x0);
		} else {
			ret = eta6963_read_interface(reg_address,
					     &g_reg_value_eta6963, 0xFF, 0x0);
			pr_info(
			"[%s] read eta6963 reg 0x%x with value 0x%x !\n",
			__func__,
			(unsigned int) reg_address, g_reg_value_eta6963);
			pr_info(
			"[%s] use \"cat eta6963_access\" to get value\n",
			__func__);
		}
	}
	return size;
}

static DEVICE_ATTR_RW(eta6963_access);

static int eta6963_user_space_probe(struct platform_device *dev)
{
	int ret_device_file = 0;

	pr_info("******** %s!! ********\n", __func__);

	ret_device_file = device_create_file(&(dev->dev),
					     &dev_attr_eta6963_access);

	return 0;
}

struct platform_device eta6963_user_space_device = {
	.name = "eta6963-user",
	.id = -1,
};

static struct platform_driver eta6963_user_space_driver = {
	.probe = eta6963_user_space_probe,
	.driver = {
		.name = "eta6963-user",
	},
};

#ifdef CONFIG_OF
static const struct of_device_id eta6963_of_match[] = {
	{.compatible = "mediatek,eta6963"},
	{},
};
#else
static struct i2c_board_info i2c_eta6963 __initdata = {
	I2C_BOARD_INFO("eta6963", (eta6963_SLAVE_ADDR_WRITE >> 1))
};
#endif

static struct i2c_driver eta6963_driver = {
	.driver = {
		.name = "eta6963",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = eta6963_of_match,
#endif
	},
	.probe = eta6963_driver_probe,
	.id_table = eta6963_i2c_id,
};

static int __init eta6963_init(void)
{
	int ret = 0;

	/* i2c registration using DTS instead of boardinfo*/
#ifdef CONFIG_OF
	pr_info("[%s] init start with i2c DTS", __func__);
#else
	pr_info("[%s] init start. ch=%d\n", __func__, eta6963_BUSNUM);
	i2c_register_board_info(eta6963_BUSNUM, &i2c_eta6963, 1);
#endif
	if (i2c_add_driver(&eta6963_driver) != 0) {
		pr_info(
			"[%s] failed to register eta6963 i2c driver.\n",
			__func__);
	} else {
		pr_info(
			"[%s] Success to register eta6963 i2c driver.\n",
			__func__);
	}

	/* eta6963 user space access interface */
	ret = platform_device_register(&eta6963_user_space_device);
	if (ret) {
		pr_info("****[%s] Unable to device register(%d)\n", __func__,
			ret);
		return ret;
	}
	ret = platform_driver_register(&eta6963_user_space_driver);
	if (ret) {
		pr_info("****[%s] Unable to register driver (%d)\n", __func__,
			ret);
		return ret;
	}

	return 0;
}

static void __exit eta6963_exit(void)
{
	i2c_del_driver(&eta6963_driver);
}
module_init(eta6963_init);
module_exit(eta6963_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C eta6963 Driver");
MODULE_AUTHOR("will cai <will.cai@mediatek.com>");
