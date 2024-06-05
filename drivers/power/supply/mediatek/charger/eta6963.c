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
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#include <linux/gpio.h>
#include <linux/of_gpio.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#endif
#include <mt-plat/mtk_boot.h>
#include <mt-plat/upmu_common.h>
#include "eta6963.h"

#include "mtk_charger_intf.h"

static int eta696x_load_status = 0;

/**********************************************************
 *
 *   [I2C Slave Setting]
 *
 *********************************************************/

#define GETARRAYNUM(array) (ARRAY_SIZE(array))

/*eta696x REG06 VREG[5:0]*/
const unsigned int VBAT_CV_VTH[] = {
	3848000, 3880000, 3912000, 3944000,
	3976000, 4008000, 4040000, 4072000,
	4104000, 4136000, 4168000, 4200000,
	4232000, 4264000, 4296000, 4328000,
	4360000, 4392000, 4424000, 4456000,
	4488000, 4520000, 4552000, 4584000,
	4616000

};

/*eta696x REG02 ICHG[5:0]*/
const unsigned int CS_VTH[] = {
	0, 6000, 12000, 18000, 24000,
	30000, 36000, 42000, 48000, 54000,
	60000, 66000, 72000, 78000, 84000,
	90000, 96000, 102000, 108000, 114000,
	120000, 126000, 132000, 138000, 144000,
	150000, 156000, 162000, 168000, 174000,
	180000, 186000, 192000, 198000, 204000,
	210000, 216000, 222000, 228000, 234000,
	240000, 246000, 252000, 258000, 264000,
	270000, 276000, 282000, 288000, 294000,
	300000, 306000, 312000, 318000, 324000
};

/*eta696x REG00 IINLIM[4:0]*/
const unsigned int INPUT_CS_VTH[] = {
	0, 10000, 20000, 30000, 40000,
	50000, 60000, 70000, 80000,
	90000, 100000, 110000, 120000,
	130000, 140000, 150000, 160000,
	170000, 180000, 190000, 200000,
	210000, 220000, 230000, 250000,
	260000, 270000, 280000, 290000,
	300000, 310000, 320000
};


const unsigned int VCDT_HV_VTH[] = {
	4200000, 4250000, 4300000, 4350000,
	4400000, 4450000, 4500000, 4550000,
	4600000, 6000000, 6500000, 7000000,
	7500000, 8500000, 9500000, 10500000

};


const unsigned int VINDPM_REG[] = {
	3900, 4000, 4100, 4200, 4300, 4400,
	4500, 4600, 4700, 4800, 4900, 5000,
	5100, 5200, 5300, 5400, 5500, 5600,
	5700, 5800, 5900, 6000, 6100, 6200,
	6300, 6400
};

/*eta696x REG0A BOOST_LIM[2:0], mA */
const unsigned int BOOST_CURRENT_LIMIT[] = {
	500, 1200
};

struct eta696x_info {
	struct charger_device *chg_dev;
	struct charger_properties chg_props;
	struct power_supply *psy;
	struct power_supply_desc psy_desc;
	struct device *dev;
	const char *chg_dev_name;
	const char *eint_name;
	int irq;
	struct regulator_dev *otg_rdev;
	int psy_usb_type;
	int chg_en;
	bool pwr_rdy;
	atomic_t usb_connected;
	struct mutex bc12_access_lock;
	enum charger_type chg_type;
	struct pinctrl *pinctrl;
	struct pinctrl_state *psc_chg_en_low;
	struct pinctrl_state *psc_chg_en_high;
} *g_eta696x_info;

static const struct charger_properties eta696x_chg_props = {
	.alias_name = "eta696x-user",
};

//DEFINE_MUTEX(g_input_current_mutex);
static struct i2c_client *new_client;
static const struct i2c_device_id eta696x_i2c_id[] = { {"eta696x", 0}, {} };

static int eta696x_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id);

unsigned int charging_value_to_parameter(const unsigned int
		*parameter, const unsigned int array_size,
		const unsigned int val)
{
	if (val < array_size)
		return parameter[val];

	pr_info("Can't find the parameter\n");
	return parameter[0];

}

unsigned int charging_parameter_to_value(const unsigned int
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
unsigned char eta696x_reg[eta696x_REG_NUM] = { 0 };

static DEFINE_MUTEX(eta696x_i2c_access);
static DEFINE_MUTEX(eta696x_access_lock);

int g_eta696x_hw_exist;

//from kernel-4.19/drivers/power/supply/mediatek/battery/mtk_battery.c
extern int g_eta696x_bat_exist; //leewin add

/**********************************************************
 *
 *   [I2C Function For Read/Write eta696x]
 *
 *********************************************************/
#ifdef CONFIG_MTK_I2C_EXTENSION
unsigned int eta696x_read_byte(unsigned char cmd,
			       unsigned char *returnData)
{
	char cmd_buf[1] = { 0x00 };
	char readData = 0;
	int ret = 0;

	mutex_lock(&eta696x_i2c_access);

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
		mutex_unlock(&eta696x_i2c_access);

		return 0;
	}

	readData = cmd_buf[0];
	*returnData = readData;

	/* new_client->addr = new_client->addr & I2C_MASK_FLAG; */
	new_client->ext_flag = 0;
	mutex_unlock(&eta696x_i2c_access);

	return 1;
}

unsigned int eta696x_write_byte(unsigned char cmd,
				unsigned char writeData)
{
	char write_data[2] = { 0 };
	int ret = 0;

	mutex_lock(&eta696x_i2c_access);

	write_data[0] = cmd;
	write_data[1] = writeData;

	new_client->ext_flag = ((new_client->ext_flag) & I2C_MASK_FLAG) |
			       I2C_DIRECTION_FLAG;

	ret = i2c_master_send(new_client, write_data, 2);
	if (ret < 0) {
		new_client->ext_flag = 0;
		mutex_unlock(&eta696x_i2c_access);
		return 0;
	}

	new_client->ext_flag = 0;
	mutex_unlock(&eta696x_i2c_access);
	return 1;
}
#else
unsigned int eta696x_read_byte(unsigned char cmd,
			       unsigned char *returnData)
{
	unsigned char xfers = 2;
	int ret, retries = 1;

	mutex_lock(&eta696x_i2c_access);

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

	mutex_unlock(&eta696x_i2c_access);

	return ret == xfers ? 1 : -1;
}

unsigned int eta696x_write_byte(unsigned char cmd,
				unsigned char writeData)
{
	unsigned char xfers = 1;
	int ret, retries = 1;
	unsigned char buf[8];

	mutex_lock(&eta696x_i2c_access);

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

	mutex_unlock(&eta696x_i2c_access);

	return ret == xfers ? 1 : -1;
}
#endif
/**********************************************************
 *
 *   [Read / Write Function]
 *
 *********************************************************/
unsigned int eta696x_read_interface(unsigned char RegNum,
				    unsigned char *val, unsigned char MASK,
				    unsigned char SHIFT)
{
	unsigned char eta696x_reg = 0;
	unsigned int ret = 0;

	ret = eta696x_read_byte(RegNum, &eta696x_reg);

	pr_debug_ratelimited("[%s] Reg[%x]=0x%x\n", __func__,
			     RegNum, eta696x_reg);

	eta696x_reg &= (MASK << SHIFT);
	*val = (eta696x_reg >> SHIFT);

	pr_debug_ratelimited("[%s] val=0x%x\n", __func__, *val);

	return ret;
}

unsigned int eta696x_config_interface(unsigned char RegNum,
				      unsigned char val, unsigned char MASK,
				      unsigned char SHIFT)
{
	unsigned char eta696x_reg = 0;
	unsigned char eta696x_reg_ori = 0;
	unsigned int ret = 0;

	mutex_lock(&eta696x_access_lock);
	ret = eta696x_read_byte(RegNum, &eta696x_reg);

	eta696x_reg_ori = eta696x_reg;
	eta696x_reg &= ~(MASK << SHIFT);
	eta696x_reg |= (val << SHIFT);

	ret = eta696x_write_byte(RegNum, eta696x_reg);
	mutex_unlock(&eta696x_access_lock);
	pr_debug_ratelimited("[%s] write Reg[%x]=0x%x from 0x%x\n", __func__,
			     RegNum,
			     eta696x_reg, eta696x_reg_ori);

	/* Check */
	/* eta696x_read_byte(RegNum, &eta696x_reg); */
	/* pr_info("[%s] Check Reg[%x]=0x%x\n", __func__,*/
	/* RegNum, eta696x_reg); */

	return ret;
}

/* write one register directly */
unsigned int eta696x_reg_config_interface(unsigned char RegNum,
		unsigned char val)
{
	unsigned int ret = 0;

	ret = eta696x_write_byte(RegNum, val);

	return ret;
}

/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/
/* CON0---------------------------------------------------- */
void eta696x_set_en_hiz(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON0),
				       (unsigned char) (val),
				       (unsigned char) (CON0_EN_HIZ_MASK),
				       (unsigned char) (CON0_EN_HIZ_SHIFT)
				      );
}

void eta696x_set_iinlim(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON0),
				       (unsigned char) (val),
				       (unsigned char) (CON0_IINLIM_MASK),
				       (unsigned char) (CON0_IINLIM_SHIFT)
				      );
}

void eta696x_set_stat_ctrl(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON0),
				   (unsigned char) (val),
				   (unsigned char) (CON0_STAT_IMON_CTRL_MASK),
				   (unsigned char) (CON0_STAT_IMON_CTRL_SHIFT)
				   );
}

/* CON1---------------------------------------------------- */

void eta696x_set_reg_rst(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON11),
				       (unsigned char) (val),
				       (unsigned char) (CON11_REG_RST_MASK),
				       (unsigned char) (CON11_REG_RST_SHIFT)
				      );
}

void eta696x_set_pfm(unsigned int val)
{
	unsigned int ret = 0;
	unsigned char otg_en=0;

	ret = eta696x_read_interface((unsigned char) (eta696x_CON1),
				     (&otg_en),
				     (unsigned char) (CON1_OTG_CONFIG_MASK),
				     (unsigned char) (CON1_OTG_CONFIG_SHIFT)
				    );

	if (otg_en==1) //PFM_DIS£½1 only for OTG
	{
		ret = eta696x_config_interface((unsigned char) (eta696x_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_PFM_MASK),
				       (unsigned char) (CON1_PFM_SHIFT)
				      );
	}
	else
	{
		ret = eta696x_config_interface((unsigned char) (eta696x_CON1),
				       (unsigned char) (0),
				       (unsigned char) (CON1_PFM_MASK),
				       (unsigned char) (CON1_PFM_SHIFT)
				      );	
	}
}

void eta696x_set_wdt_rst(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_WDT_RST_MASK),
				       (unsigned char) (CON1_WDT_RST_SHIFT)
				      );
}

void eta696x_set_otg_config(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_OTG_CONFIG_MASK),
				       (unsigned char) (CON1_OTG_CONFIG_SHIFT)
				      );
}

unsigned int eta696x_get_otg_config(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta696x_read_interface((unsigned char) (eta696x_CON1),
				     (&val),
				     (unsigned char) (CON1_OTG_CONFIG_MASK),
				     (unsigned char) (CON1_OTG_CONFIG_SHIFT)
				    );
	return val;
}


void eta696x_set_chg_config(unsigned int val)
{
	unsigned int ret = 0;
	ret = eta696x_config_interface((unsigned char) (eta696x_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_CHG_CONFIG_MASK),
				       (unsigned char) (CON1_CHG_CONFIG_SHIFT)
				      );

}


void eta696x_set_sys_min(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_SYS_MIN_MASK),
				       (unsigned char) (CON1_SYS_MIN_SHIFT)
				      );
}

void eta696x_set_batlowv(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON1),
				       (unsigned char) (val),
				       (unsigned char) (CON1_MIN_VBAT_SEL_MASK),
				       (unsigned char) (CON1_MIN_VBAT_SEL_SHIFT)
				      );
}



/* CON2---------------------------------------------------- */
void eta696x_set_rdson(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON2),
				       (unsigned char) (val),
				       (unsigned char) (CON2_Q1_FULLON_MASK),
				       (unsigned char) (CON2_Q1_FULLON_SHIFT)
				      );
}

void eta696x_set_boost_lim(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON2),
				       (unsigned char) (val),
				       (unsigned char) (CON2_BOOST_LIM_MASK),
				       (unsigned char) (CON2_BOOST_LIM_SHIFT)
				      );
}

void eta696x_set_ichg(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON2),
				       (unsigned char) (val),
				       (unsigned char) (CON2_ICHG_MASK),
				       (unsigned char) (CON2_ICHG_SHIFT)
				      );
}

#ifdef FIXME //this function does not exist on eta696x
void eta696x_set_force_20pct(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON2),
				       (unsigned char) (val),
				       (unsigned char) (CON2_FORCE_20PCT_MASK),
				       (unsigned char) (CON2_FORCE_20PCT_SHIFT)
				      );
}
#endif
/* CON3---------------------------------------------------- */

void eta696x_set_iprechg(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON3),
				       (unsigned char) (val),
				       (unsigned char) (CON3_IPRECHG_MASK),
				       (unsigned char) (CON3_IPRECHG_SHIFT)
				      );
}

void eta696x_set_iterm(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON3),
				       (unsigned char) (val),
				       (unsigned char) (CON3_ITERM_MASK),
				       (unsigned char) (CON3_ITERM_SHIFT)
				      );
}

/* CON4---------------------------------------------------- */

void eta696x_set_vreg(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON4),
				       (unsigned char) (val),
				       (unsigned char) (CON4_VREG_MASK),
				       (unsigned char) (CON4_VREG_SHIFT)
				      );
}

void eta696x_set_topoff_timer(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON4),
				       (unsigned char) (val),
				       (unsigned char) (CON4_TOPOFF_TIMER_MASK),
				       (unsigned char) (CON4_TOPOFF_TIMER_SHIFT)
				      );

}


void eta696x_set_vrechg(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON4),
				       (unsigned char) (val),
				       (unsigned char) (CON4_VRECHG_MASK),
				       (unsigned char) (CON4_VRECHG_SHIFT)
				      );
}

/* CON5---------------------------------------------------- */

void eta696x_set_en_term(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON5),
				       (unsigned char) (val),
				       (unsigned char) (CON5_EN_TERM_MASK),
				       (unsigned char) (CON5_EN_TERM_SHIFT)
				      );
}



void eta696x_set_watchdog(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON5),
				       (unsigned char) (val),
				       (unsigned char) (CON5_WATCHDOG_MASK),
				       (unsigned char) (CON5_WATCHDOG_SHIFT)
				      );
}

void eta696x_set_en_timer(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON5),
				       (unsigned char) (val),
				       (unsigned char) (CON5_EN_TIMER_MASK),
				       (unsigned char) (CON5_EN_TIMER_SHIFT)
				      );
}

void eta696x_set_chg_timer(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON5),
				       (unsigned char) (val),
				       (unsigned char) (CON5_CHG_TIMER_MASK),
				       (unsigned char) (CON5_CHG_TIMER_SHIFT)
				      );
}

/* CON6---------------------------------------------------- */

void eta696x_set_treg(unsigned int val)
{
#ifdef FIXME
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON6),
				       (unsigned char) (val),
				       (unsigned char) (CON6_BOOSTV_MASK),
				       (unsigned char) (CON6_BOOSTV_SHIFT)
				      );
#endif
}

void eta696x_set_vindpm(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON6),
				       (unsigned char) (val),
				       (unsigned char) (CON6_VINDPM_MASK),
				       (unsigned char) (CON6_VINDPM_SHIFT)
				      );
}


void eta696x_set_ovp(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON6),
				       (unsigned char) (val),
				       (unsigned char) (CON6_OVP_MASK),
				       (unsigned char) (CON6_OVP_SHIFT)
				      );

}

void eta696x_set_boostv(unsigned int val)
{

	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON6),
				       (unsigned char) (val),
				       (unsigned char) (CON6_BOOSTV_MASK),
				       (unsigned char) (CON6_BOOSTV_SHIFT)
				      );
}



/* CON7---------------------------------------------------- */
/*unsigned int eta696x_get_bc12_status(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta696x_read_interface((unsigned char) (eta696x_CON7),
				     (&val),
				     (unsigned char) (CON7_FORCE_DPDM_MASK),
				     (unsigned char) (CON7_FORCE_DPDM_SHIFT)
				    );
	return val;
}*/

void eta696x_set_bc12_enable(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON7),
				       (unsigned char) (val),
				       (unsigned char) (CON7_FORCE_DPDM_MASK),
				       (unsigned char) (CON7_FORCE_DPDM_SHIFT)
				      );
}

void eta696x_set_tmr2x_en(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON7),
					(unsigned char) (val),
					(unsigned char) (CON7_TMR2X_EN_MASK),
					(unsigned char) (CON7_TMR2X_EN_SHIFT)
					);
}

void eta696x_set_batfet_disable(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON7),
				(unsigned char) (val),
				(unsigned char) (CON7_BATFET_Disable_MASK),
				(unsigned char) (CON7_BATFET_Disable_SHIFT)
				);
}


void eta696x_set_batfet_delay(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON7),
				       (unsigned char) (val),
				       (unsigned char) (CON7_BATFET_DLY_MASK),
				       (unsigned char) (CON7_BATFET_DLY_SHIFT)
				      );
}

void eta696x_set_batfet_reset_enable(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON7),
				(unsigned char) (val),
				(unsigned char) (CON7_BATFET_RST_EN_MASK),
				(unsigned char) (CON7_BATFET_RST_EN_SHIFT)
				);
}


/* CON8---------------------------------------------------- */

unsigned int eta696x_get_system_status(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta696x_read_interface((unsigned char) (eta696x_CON8),
				     (&val), (unsigned char) (0xFF),
				     (unsigned char) (0x0)
				    );
	return val;
}

unsigned int eta696x_get_vbus_stat(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta696x_read_interface((unsigned char) (eta696x_CON8),
				     (&val),
				     (unsigned char) (CON8_VBUS_STAT_MASK),
				     (unsigned char) (CON8_VBUS_STAT_SHIFT)
				    );
	return val;
}

unsigned int eta696x_get_chrg_stat(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta696x_read_interface((unsigned char) (eta696x_CON8),
				     (&val),
				     (unsigned char) (CON8_CHRG_STAT_MASK),
				     (unsigned char) (CON8_CHRG_STAT_SHIFT)
				    );
	return val;
}

unsigned int eta696x_get_vsys_stat(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta696x_read_interface((unsigned char) (eta696x_CON8),
				     (&val),
				     (unsigned char) (CON8_VSYS_STAT_MASK),
				     (unsigned char) (CON8_VSYS_STAT_SHIFT)
				    );
	return val;
}

unsigned int eta696x_get_pg_stat(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta696x_read_interface((unsigned char) (eta696x_CON8),
				     (&val),
				     (unsigned char) (CON8_PG_STAT_MASK),
				     (unsigned char) (CON8_PG_STAT_SHIFT)
				    );
	return val;
}

/*CON10----------------------------------------------------------*/

void eta696x_set_int_mask(unsigned int val)
{
	unsigned int ret = 0;

	ret = eta696x_config_interface((unsigned char) (eta696x_CON10),
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
static int eta696x_dump_register(struct charger_device *chg_dev)
{

	unsigned char i = 0;
	unsigned int ret = 0;

	pr_debug("[eta696x] ");
	for (i = 0; i < eta696x_REG_NUM; i++) {
		ret = eta696x_read_byte(i, &eta696x_reg[i]);
		if (ret == 0) {
			pr_info("[eta696x] i2c transfor error\n");
			return 1;
		}
		pr_debug("[0x%x]=0x%x ", i, eta696x_reg[i]);
	}
	pr_debug("\n");
	return 0;
}


/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/
static void eta696x_hw_component_detect(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = eta696x_read_interface(0x0B, &val, 0xFF, 0x0);

	if (val == 0)
		g_eta696x_hw_exist = 0;
	else
		g_eta696x_hw_exist = 1;

	pr_info("[%s] exist=%d, Reg[0x0B]=0x%x\n", __func__,
		g_eta696x_hw_exist, val);
}


static int eta696x_enable_charging(struct charger_device *chg_dev,
				   bool en)
{
	int status = 0;
	int ret = 0;
	//struct eta696x_info * info = g_eta696x_info;

        if(!g_eta696x_bat_exist){ //lewin add
		//g_eta696x_bat_exist = 1;
		en = 0;
		pr_info("%s bat donot exist\n", __func__);
	}

	printk("enable state : %d\n", en);
	if (en) {
		/* eta696x_config_interface(eta696x_CON3, 0x1, 0x1, 4); */
		/* enable charging */
		eta696x_set_en_hiz(0x0);
		eta696x_set_chg_config(en);
		//ret = pinctrl_select_state(info->pinctrl, info->psc_chg_en_low);
		msleep(500);
		if (ret){
			pr_err("mycat Error pinctrl_select_state low");
		}
		else{
			pr_err("mycat Ok pinctrl_select_state low");
		}
	} else {
		/* eta696x_config_interface(eta696x_CON3, 0x0, 0x1, 4); */
		/* enable charging */
		eta696x_set_chg_config(en);
		pr_info("[charging_enable] under test mode: disable charging\n");
		//ret = pinctrl_select_state(info->pinctrl, info->psc_chg_en_high);
		//if (ret){
		//	pr_err("mycat Error pinctrl_select_state high");
		//}
		//else{
		//	pr_err("mycat Ok pinctrl_select_state high");
		//}
		/*eta696x_set_en_hiz(0x1);*/
	}

	return status;
}

static int eta696x_get_current(struct charger_device *chg_dev,
			       u32 *ichg)
{
	unsigned int ret_val = 0;
#ifdef FIXME
	unsigned char ret_force_20pct = 0;

	/* Get current level */
	eta696x_read_interface(eta696x_CON2, &ret_val, CON2_ICHG_MASK,
			       CON2_ICHG_SHIFT);

	/* Get Force 20% option */
	eta696x_read_interface(eta696x_CON2, &ret_force_20pct,
			       CON2_FORCE_20PCT_MASK,
			       CON2_FORCE_20PCT_SHIFT);

	/* Parsing */
	ret_val = (ret_val * 64) + 512;

#endif
printk("%s: ret_val = %d\n", __func__, ret_val);
	return ret_val;
}

static int eta696x_set_current(struct charger_device *chg_dev,
			       u32 current_value)
{
	unsigned int status = true;
	unsigned int set_chr_current;
	unsigned int array_size;
	unsigned int register_value;

	/*u8 vbus_stat;
	vbus_stat = eta696x_get_vbus_stat();
	switch(vbus_stat){
		case 0:
			current_value = 0;
			break;
		case 1:
			current_value = 500000;
			
			break;
		case 2:
			current_value = 1500000;
			break;
		case 3:
			current_value = 3200000;
			break;
		case 5:
			current_value = 500000;
			break;
		default:
			current_value = 500000;
			break;
	}*/

	pr_debug("&&&& charge_current_value = %d\n", current_value);
	current_value /= 10;
	array_size = GETARRAYNUM(CS_VTH);
	set_chr_current = bmt_find_closest_level(CS_VTH, array_size,
			  current_value);
	register_value = charging_parameter_to_value(CS_VTH, array_size,
			 set_chr_current);

	pr_debug("&&&& %s register_value = %d\n", __func__,
		register_value);
	eta696x_set_ichg(register_value);

	return status;
}

static int eta696x_get_input_current(struct charger_device *chg_dev,
				     u32 *aicr)
{
	int ret = 0;
#ifdef FIXME
	unsigned char val = 0;

	eta696x_read_interface(eta696x_CON0, &val, CON0_IINLIM_MASK,
			       CON0_IINLIM_SHIFT);
	ret = (int)val;
	*aicr = INPUT_CS_VTH[val];
#endif
	return ret;
}


static int eta696x_set_input_current(struct charger_device *chg_dev,
				     u32 current_value)
{
	unsigned int status = true;
	unsigned int set_chr_current;
	unsigned int array_size;
	unsigned int register_value;
//	dump_stack();
	current_value /= 10;
	pr_info("&&&& current_value = %d\n", current_value);
	array_size = GETARRAYNUM(INPUT_CS_VTH);
	set_chr_current = bmt_find_closest_level(INPUT_CS_VTH, array_size,
			  current_value);
	register_value = charging_parameter_to_value(INPUT_CS_VTH, array_size,
			 set_chr_current);
	pr_info("&&&& %s register_value = %d\n", __func__,
		register_value);

        if(0 == register_value){ //fix vsys fall down //leewin add
               return status;
        }

	if(!g_eta696x_bat_exist){ //lewin add
	       //g_eta696x_bat_exist = 1;
	       register_value = 15; //1.6A
	       pr_info("%s bat donot exist\n", __func__);
	}
	eta696x_set_iinlim(register_value);

	return status;
}

static int eta696x_set_cv_voltage(struct charger_device *chg_dev,
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
	eta696x_set_vreg(register_value);
	pr_info("&&&& cv reg value = %d\n", register_value);

	return status;
}

static int eta696x_reset_watch_dog_timer(struct charger_device
		*chg_dev)
{
	unsigned int status = true;

	pr_info("charging_reset_watch_dog_timer\n");

	eta696x_set_wdt_rst(0x1);	/* Kick watchdog */
	eta696x_set_watchdog(0x3);	/* WDT 160s */

	return status;
}


static int eta696x_set_vindpm_voltage(struct charger_device *chg_dev,
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
	/*eta696x_set_en_hiz(en);*/

	return status;
}

static int eta696x_get_charging_status(struct charger_device *chg_dev,
				       bool *is_done)
{
	unsigned int status = true;
	unsigned int ret_val;

	ret_val = eta696x_get_chrg_stat();

	if (ret_val == 0x3)
		*is_done = true;
	else
		*is_done = false;

	return status;
}

static int eta696x_enable_otg(struct charger_device *chg_dev, bool en)
{
	int ret = 0;

	pr_info("%s en = %d\n", __func__, en);
	if (en) {
		eta696x_set_chg_config(0);
		eta696x_set_otg_config(1);
		//eta696x_set_watchdog(0x3);	/* WDT 160s */
	} else {
		eta696x_set_otg_config(0);
		eta696x_set_chg_config(1);
	}
	return ret;
}

static int eta696x_set_boost_current_limit(struct charger_device
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
	eta696x_set_boost_lim(boost_reg);

	return ret;
}

static int eta696x_enable_safetytimer(struct charger_device *chg_dev,
				      bool en)
{
	int status = 0;

	if (en)
		eta696x_set_en_timer(0x1);
	else
		eta696x_set_en_timer(0x0);
	return status;
}

static int eta696x_get_is_safetytimer_enable(struct charger_device
		*chg_dev, bool *en)
{
	unsigned char val = 0;

	eta696x_read_interface(eta696x_CON5, &val, CON5_EN_TIMER_MASK,
			       CON5_EN_TIMER_SHIFT);
	*en = (bool)val;
	return val;
}
#if 0
static int eta696x_inform_psy_changed(struct eta696x_info *chg_data)
{
	int ret = 0;
	union power_supply_propval propval;

	pr_info("%s: pwr_rdy = %d, type = %d\n", __func__,	chg_data->pwr_rdy, chg_data->chg_type);

	/* Get chg type det power supply */
	if (!chg_data->psy)
		chg_data->psy = power_supply_get_by_name("charger");
	if (!chg_data->psy) {
		dev_notice(chg_data->dev, "%s: get power supply failed\n",
			__func__);
		return -EINVAL;
	}

	/* Inform chg det power supply */
	propval.intval = chg_data->pwr_rdy;
	ret = power_supply_set_property(chg_data->psy, POWER_SUPPLY_PROP_ONLINE,
		&propval);
	if (ret < 0)
		pr_info("%s: psy online failed, ret = %d\n",
			__func__, ret);

	propval.intval = chg_data->chg_type;
	ret = power_supply_set_property(chg_data->psy,
		POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);
	if (ret < 0)
		pr_info("%s: psy type failed, ret = %d\n",
			__func__, ret);

	return ret;
}
#endif
/*
static int eta696x_enable_bc12_detect(struct charger_device *chg_dev, bool en)
{
	int ret = 0;
	//printk("huangxuan %s %d\n", __func__, __LINE__);
	eta696x_set_bc12_enable(en);
	return ret;
}
*/
static int eta696x_enable_chg_type_det(struct charger_device *chg_dev)
{
	int ret = 0;
	int pg_stat = 0;
	unsigned int bq_detect_count = 0;
	u8 val = 0;
	int i = 0;

	eta696x_dump_register(chg_dev);
	Charger_Detect_Init();
	ret = CHARGER_UNKNOWN;
	do{
		msleep(50);
		pg_stat = eta696x_get_pg_stat();
		if (pg_stat == 1) {
			bq_detect_count = 0 ;
			pr_err("%s: eta696x_get_pg_stat is good!\r\n",__func__);
		} else {
			bq_detect_count++;
		}
		pr_err("%s: count_max=%d,pg_stat=%d\n",__func__, bq_detect_count, pg_stat);
		if(bq_detect_count > 10) {
			bq_detect_count = 0;
			pr_err("%s: eta696x_get_pg_stat power not good!\r\n",__func__);
			Charger_Detect_Release();
			ret = CHARGER_UNKNOWN;
			return ret;
		}
	}while (bq_detect_count);

	ret = eta696x_get_vbus_stat();
	for (i; i <= 10; i++)
	{
		if (ret > 0){
 			break;
		} else {
			msleep(300);
			ret = eta696x_get_vbus_stat();
			pr_err("%s: read 0x0b   ret = %d [0x0B] = 0x%X\n", __func__, ret, val);
		}
 	}
 
	switch (ret) {
	case ETA696x_VBUS_TYPE_SDP:
		ret = STANDARD_HOST;
		pr_err("%s: charge type = STANDARD_HOST ! \n", __func__);
		break;
	case ETA696x_VBUS_TYPE_CDP:
		ret = CHARGING_HOST;
		pr_err("%s: charge type = CHARGING_HOST ! \n", __func__);
		break;
	case ETA696x_VBUS_TYPE_NON_STD:
	case ETA696x_VBUS_TYPE_DCP:
		ret = STANDARD_CHARGER;
		pr_err("%s: charge type = STANDARD_CHARGER ! \n", __func__);
		break;
	case ETA696x_VBUS_TYPE_HVDCP:
	case ETA696x_VBUS_TYPE_UNKNOWN:
		pr_err("%s: case = ETA696x_VBUS_TYPE_UNKNOWN ! \n", __func__);
		// 0 =< vbat < 3.0v
		ret = STANDARD_HOST;
		pr_info("%s charge type = STANDARD_HOST\n", __func__);
		break;
	case ETA696x_VBUS_TYPE_NONE:
	default:
		ret = CHARGER_UNKNOWN;
		pr_err("%s: charge type = CHARGER_UNKNOWN ! \n", __func__);
		break;
	}
	pr_err("%s: charge type ret = %d \n", __func__, ret);
	Charger_Detect_Release();
	eta696x_dump_register(chg_dev);

	return ret;
}

static unsigned int charging_hw_init(void)
{
	unsigned int status = 0;

	eta696x_set_en_hiz(0x0);
	eta696x_set_vindpm(0x3);	/* VIN DPM check 4.6V */
	eta696x_set_wdt_rst(0x1);	/* Kick watchdog */
	eta696x_set_sys_min(0x5);	/* Minimum system voltage 3.5V */
	eta696x_set_iprechg(0x8);	/* Precharge current 540mA */
	eta696x_set_iterm(0x0);	/* Termination current 120mA */
	//eta696x_set_iterm(0x2);	/* Termination current 180mA */
	//eta696x_set_iterm(0x6);	/* Termination current 420mA */
	eta696x_set_vreg(0xb);	/* VREG 4.2V */
	eta696x_set_pfm(0x0);//enable pfm
	eta696x_set_rdson(0x0);     /*close rdson*/
	eta696x_set_batlowv(0x1);	/* BATLOWV 3.0V */
	eta696x_set_vrechg(0x1);	/* 240mv */
	eta696x_set_en_term(0x1);	/* Enable termination */
	eta696x_set_watchdog(0x0);	/* WDT disable */
	eta696x_set_en_timer(0x0);	/* Enable charge timer */
	eta696x_set_int_mask(0x0);	/* Disable fault interrupt */
	eta696x_set_iinlim(0xf);        /* input current 1.6A */
	eta696x_set_ichg(0x4); //0x00---> pogo4.7v ok //0x0d--> pogo5v ok //leewin add
	pr_info("%s: hw_init down!\n", __func__);
	return status;
}

static int eta696x_parse_dt(struct eta696x_info *info,
			    struct device *dev)
{
	struct device_node *np = dev->of_node;
	//int eta696x_en_pin = 0;
	//int ret = 0;
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
		info->chg_props.alias_name = "eta696x";
		pr_info("%s: no alias name\n", __func__);
	}
	
	/*
	info->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(info->pinctrl)) {
		ret = PTR_ERR(info->pinctrl);
		pr_err("mycat Cannot find eta696x info->pinctrl! ret=%d\n", ret);
	}

	info->psc_chg_en_high = pinctrl_lookup_state(info->pinctrl,"psc_chg_en_high");
	if (IS_ERR(info->psc_chg_en_high))
	{
		ret = PTR_ERR(info->psc_chg_en_high);
		pr_err("mycat info->psc_chg_en_high ret = %d\n",ret);
	}

	info->psc_chg_en_low = pinctrl_lookup_state(info->pinctrl,"psc_chg_en_low");
	if (IS_ERR(info->psc_chg_en_low))
	{
		ret = PTR_ERR(info->psc_chg_en_low);
		pr_err("mycat info->psc_chg_en_low ret = %d\n",ret);
	}
	*/
	
	/*
	 * eta696x_en_pin = of_get_named_gpio(np,"gpio_eta696x_en",0);
	 * if(eta696x_en_pin < 0){
	 * pr_info("%s: no eta696x_en_pin\n", __func__);
	 * return -ENODATA;
	 * }
	 * gpio_request(eta696x_en_pin,"eta696x_en_pin");
	 * gpio_direction_output(eta696x_en_pin,0);
	 * gpio_set_value(eta696x_en_pin,0);
	 */
	/*
	 * if (of_property_read_string(np, "eint_name", &info->eint_name) < 0) {
	 * info->eint_name = "chr_stat";
	 * pr_debug("%s: no eint name\n", __func__);
	 * }
	 */
	return 0;
}

static int eta696x_do_event(struct charger_device *chg_dev, u32 event,
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

static int eta696x_enable_vbus(struct regulator_dev *rdev)
{
	eta696x_set_chg_config(0);
	eta696x_set_otg_config(1);

	return 0;
}

static int eta696x_disable_vbus(struct regulator_dev *rdev)
{
	eta696x_set_otg_config(0);
	eta696x_set_chg_config(1);

	return 0;
}

static int eta696x_is_enabled_vbus(struct regulator_dev *rdev)
{
	return eta696x_get_otg_config();
}

static const struct regulator_ops eta696x_vbus_ops = {
	.enable = eta696x_enable_vbus,
	.disable = eta696x_disable_vbus,
	.is_enabled = eta696x_is_enabled_vbus,
};

/*static const struct regulator_desc eta696x_otg_rdesc = {
	.of_match = "usb-otg-vbus",
	.name = "usb-otg-vbus",
	.ops = &eta696x_vbus_ops,
	.owner = THIS_MODULE,
	.type = REGULATOR_VOLTAGE,
	.fixed_uV = 5000000,
	.n_voltages = 1,
};*/

static unsigned char first_flag =1;

static int eta696x_plug_out(struct charger_device *chg_dev)
{
		first_flag =1;
		printk("%s:set first_flag=%d\n", __func__,first_flag);
		return 0;
}

static int eta696x_plug_in(struct charger_device *chg_dev)
{
	printk("%s:do nothing\n", __func__);
	return 0;
}

static struct charger_ops eta696x_chg_ops = {
#ifdef FIXME
	.enable_hz = eta696x_enable_hz,
#endif

	/* Normal charging */
	.plug_out = eta696x_plug_out,
	.plug_in = eta696x_plug_in,
	.dump_registers = eta696x_dump_register,
	.enable = eta696x_enable_charging,
	.get_charging_current = eta696x_get_current,
	.set_charging_current = eta696x_set_current,
	.get_input_current = eta696x_get_input_current,
	.set_input_current = eta696x_set_input_current,
	/*.get_constant_voltage = eta696x_get_battery_voreg,*/
	.set_constant_voltage = eta696x_set_cv_voltage,
	.kick_wdt = eta696x_reset_watch_dog_timer,
	.set_mivr = eta696x_set_vindpm_voltage,
	.is_charging_done = eta696x_get_charging_status,
	/* Safety timer */
	.enable_safety_timer = eta696x_enable_safetytimer,
	.is_safety_timer_enabled = eta696x_get_is_safetytimer_enable,


	/* Power path */
	/*.enable_powerpath = eta696x_enable_power_path, */
	/*.is_powerpath_enabled = eta696x_get_is_power_path_enable, */
	
	/* Charger type detection */
	.get_ext_chgtyp = eta696x_enable_chg_type_det,
//	.enable_bc12_detect = eta696x_enable_bc12_detect,


	/* OTG */
	.enable_otg = eta696x_enable_otg,
	.set_boost_current_limit = eta696x_set_boost_current_limit,
	.event = eta696x_do_event,
};

static int eta696x_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct eta696x_info *info = NULL;
	
	info = g_eta696x_info;
	new_client = client;

	eta696x_hw_component_detect();
	charging_hw_init();

	eta696x_dump_register(info->chg_dev);

	eta696x_load_status = 1;

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id eta696x_of_match[] = {
	{.compatible = "mediatek,eta696x"},
	{},
};
#else
static struct i2c_board_info i2c_eta696x __initdata = {
	I2C_BOARD_INFO("eta696x", (eta696x_SLAVE_ADDR_WRITE >> 1))
};
#endif

static struct i2c_driver eta696x_driver = {
	.driver = {
		.name = "eta696x",
#ifdef CONFIG_OF
		.of_match_table = eta696x_of_match,
#endif
	},
	.probe = eta696x_i2c_probe,
	.id_table = eta696x_i2c_id,
};

/**********************************************************
 *
 *   [platform_driver API]
 *
 *********************************************************/
unsigned char g_reg_value_eta696x;
static ssize_t eta696x_access_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	pr_info("[%s] 0x%x\n", __func__, g_reg_value_eta696x);
	return sprintf(buf, "%u\n", g_reg_value_eta696x);
}

static ssize_t eta696x_access_store(struct device *dev,
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
			"[%s] write eta696x reg 0x%x with value 0x%x !\n",
			__func__,
			(unsigned int) reg_address, reg_value);
			ret = eta696x_config_interface(reg_address,
				reg_value, 0xFF, 0x0);
		} else {
			ret = eta696x_read_interface(reg_address,
					     &g_reg_value_eta696x, 0xFF, 0x0);
			pr_info(
			"[%s] read eta696x reg 0x%x with value 0x%x !\n",
			__func__,
			(unsigned int) reg_address, g_reg_value_eta696x);
			pr_info(
			"[%s] use \"cat eta696x_access\" to get value\n",
			__func__);
		}
	}
	return size;
}

static DEVICE_ATTR_RW(eta696x_access);


static int eta696x_platform_probe(struct platform_device *pdev)
{
	int ret_device_file = 0;
	
	int ret = 0;

	struct eta696x_info *info = NULL;
	//struct power_supply_desc *charger_desc;
	//struct power_supply_config charger_cfg = {};
	struct regulator_config config = { };

	printk("[eta696x_platform_probe]\n");
	
	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	
	g_eta696x_info = info;
	
	mutex_init(&info->bc12_access_lock);
	
	ret = eta696x_parse_dt(info, &pdev->dev);
	if (ret < 0)
		return ret;
	
	
	/* i2c registeration using DTS instead of boardinfo*/
#ifdef CONFIG_OF
	printk("[eta696x_init] init start with i2c DTS");
#else
	printk("[eta696x_init] init start. ch=%d\n", eta696x_BUSNUM);
	i2c_register_board_info(eta696x_BUSNUM, &i2c_eta696x, 1);
#endif
	if (i2c_add_driver(&eta696x_driver) != 0) {
		printk(
			    "[eta696x_init] failed to register eta696x i2c driver.\n");
	} else {
		printk(
			    "[eta696x_init] Success to register eta696x i2c driver.\n");
	}

	if (eta696x_load_status == 0) {
		printk("[fan]Failed to register eta696x i2c driver.\n");
		i2c_del_driver(&eta696x_driver);
		return -1;
	}
	
	

	//info = g_eta696x_info;
	info->dev = &pdev->dev;
	platform_set_drvdata(pdev, info);
	
	/* Register charger device */
	info->chg_dev = charger_device_register(info->chg_dev_name,
		&pdev->dev, info, &eta696x_chg_ops, &eta696x_chg_props);
	if (IS_ERR_OR_NULL(info->chg_dev)) {
		printk("%s: register charger device failed\n", __func__);
		ret = PTR_ERR(info->chg_dev);
		return ret;
	}
	
	#if 0
	/* power supply register */
	memcpy(&info->psy_desc,
		&eta696x_charger_desc, sizeof(info->psy_desc));
	info->psy_desc.name = dev_name(&pdev->dev);

	charger_cfg.drv_data = info;
	charger_cfg.of_node = pdev->dev.of_node;
	//charger_cfg.supplied_to = eta696x_charger_supplied_to;
	//charger_cfg.num_supplicants = ARRAY_SIZE(eta696x_charger_supplied_to);
	info->psy = devm_power_supply_register(&pdev->dev,
					&info->psy_desc, &charger_cfg);
	if (IS_ERR(info->psy)) {
		printk("%s: Failed to register power supply: %d\n", __func__,ret);
		ret = PTR_ERR(info->psy);
		return ret;
	}
	#endif
	
	/* otg regulator */
	config.dev = &pdev->dev;
	config.driver_data = info;
	//info->otg_rdev = devm_regulator_register(&pdev->dev, &eta696x_otg_rdesc, &config);
	if (IS_ERR(info->otg_rdev)) {
		ret = PTR_ERR(info->otg_rdev);
		pr_info("%s: register otg regulator failed (%d)\n", __func__, ret);
		return ret;
	}
	
	pr_info("******** %s!! ********\n", __func__);

	ret_device_file = device_create_file(&(pdev->dev),
					     &dev_attr_eta696x_access);
	 
	atomic_set(&info->usb_connected, 0);

	return 0;
}

static int eta696x_platform_remove(struct platform_device *pdev)
{
	struct eta696x_info *chg_data = platform_get_drvdata(pdev);
	
	if (chg_data) {
		mutex_destroy(&chg_data->bc12_access_lock);
	}
	return 0;
}

static const struct of_device_id mt_ofid_table[] = {
	{ .compatible = "mediatek,eta696x_chg_driver", },
	{ },
};
MODULE_DEVICE_TABLE(of, mt_ofid_table);

static const struct platform_device_id mt_id_table[] = {
	{ "eta696x_chg_driver", 0},
	{ },
};
MODULE_DEVICE_TABLE(platform, mt_id_table);

static struct platform_driver eta696x_chg_driver = {
	.driver = {
		   .name = "eta696x_chg_driver",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(mt_ofid_table),
	 },
	.probe = eta696x_platform_probe,
	.remove = eta696x_platform_remove,
	.id_table = mt_id_table,
};

static int __init eta696x_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&eta696x_chg_driver);
	if (ret) {
		pr_info("****[%s] Unable to register driver (%d)\n", __func__,
			ret);
		return ret;
	}

	return 0;
}

static void __exit eta696x_exit(void)
{
	i2c_del_driver(&eta696x_driver);
}
module_init(eta696x_init);
module_exit(eta696x_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C eta696x Driver");
