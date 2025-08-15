/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 *
 * Filename: ap3220.c
 *
 * Summary:
 *	ap3220 sensor dirver.
 *
 * Modification History:
 * Date(dd/mm/yy)	 By		Summary
 * -------- -------- -------------------------------------------------------
 * 09/11/20 Leo First release for MT6771
 *
 *
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include "cust_alsps.h"
#include "ap3220.h"
#include "alsps.h"


/*-------------------------------flag---------------------------------------------*/
#define APS_TAG			"[ALS/PS] "
#define APS_TAG2		"[ALS/PS]ap3220: "

#define APS_FUN(f)			pr_info(APS_TAG"%s\n", __func__)
#define APS_ERR(fmt, args...)	pr_err(APS_TAG"%s %d : "fmt"\n", __func__, __LINE__, ##args)
#define APS_LOG(fmt, args...)	pr_info(APS_TAG fmt"\n", ##args)
#define APS_DBG(fmt, args...)	pr_debug(APS_TAG"%s %d : "fmt"\n", __func__, __LINE__, ##args)
#define APS_BDBG(fmt, args...)	printk(APS_TAG2 fmt"\n", ##args)

/******************************************************************************
 * extern functions  move to .h   for kernel standard style
*******************************************************************************/

#define I2C_FLAG_WRITE	0
#define I2C_FLAG_READ	1

/*---------------------------user define-------------------------------------------------*/
#define DELAYED_WORK 0	/*1*/
#define AP3220
#define DI_AUTO_CAL
#define DI_PS_CAL_THR 255
#define AP3220_DEV_NAME	 "AP3220"

#define DI_ALS_AUTO_GAIN
#ifdef DI_ALS_AUTO_GAIN
	#define MAXS_DATA 	65535
	#define THRS_HIGH 	3 * MAXS_DATA / 4
	#define THRS_LOWS 	2 * MAXS_DATA / 16
#endif

static struct i2c_client *ap3220_i2c_client;
static struct ap3220_priv *ap3220_obj;
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id ap3220_i2c_id[] = {{AP3220_DEV_NAME, 0}, {} };
struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;
static uint32_t ap3220_range[] = {65535, 16383, 4095, 1023};


/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/
static int ap3220_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int ap3220_i2c_remove(struct i2c_client *client);
static int ap3220_i2c_suspend(struct device *dev);
static int ap3220_i2c_resume(struct device *dev);
static int ap3220_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
static int ap3220_ps_get_data(int *value, int *status);
static int ap3220_als_get_lux(int *value, int *status);
static int ap3220_set_ALSGain(struct i2c_client *client, int val);
static int ap3220_read_als_raw(struct i2c_client *client, u16 *data);
/*----------------------------------------------------------------------------*/

//static struct wake_lock chrg_lock;
struct wakeup_source *chrg_lock;

/*----------------------------------------------------------------------------*/
enum {
	CMC_BIT_ALS	= 1,
	CMC_BIT_PS	= 2,
};
enum {
	TRACE_DEBUG = 0x1,
	TRACE_ALS = 0x2,
	TRACE_PS = 0x4,
};
/*----------------------------------------------------------------------------*/
struct ap3220_i2c_addr {	/*define a series of i2c slave address*/
	u8 write_addr;
	u8 ps_thd;	/*PS INT threshold*/
};
/*----------------------------------------------------------------------------*/
struct ap3220_priv {
	struct alsps_hw *hw;
	struct i2c_client *client;
#if DELAYED_WORK
	struct delayed_work eint_work;
#else
	struct work_struct eint_work;
#endif
	struct mutex lock;
	/*i2c address group*/
	struct ap3220_i2c_addr addr;
	struct device_node *irq_node;
	int irq;
	/*misc*/
	u16			als_modulus;
	atomic_t	i2c_retry;
	atomic_t	als_suspend;
	atomic_t	als_debounce;	/*debounce time after enabling als*/
	atomic_t	als_deb_on;	/*indicates if the debounce is on*/
	atomic_t	als_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_mask;		/*mask ps: always return far away*/
	atomic_t	ps_debounce;	/*debounce time after enabling ps*/
	atomic_t	ps_deb_on;		/*indicates if the debounce is on*/
	atomic_t	ps_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_suspend;
	atomic_t	trace;

	/*data*/
	u16		als;
	u16		bef_als;
	u16		ps;
	u8		_align;
	u8		als_gain;
	u16		als_level_num;
	u16		als_value_num;
	u32		als_level[C_CUST_ALS_LEVEL-1];
	u32		als_value[C_CUST_ALS_LEVEL];

	atomic_t	als_cmd_val;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_cmd_val;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_h;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_l;	/*the cmd value can't be read, stored in ram*/
	int ps_cali;
	int als_cali;

	ulong		enable;		/*enable mask*/
	ulong		pending_intr;	/*pending interrupt*/

	bool		als_auto_gain_trig;
	u64		als_auto_gain_trig_time;
	u64		als_auto_gain_msdelay;

	/*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend	early_drv;
#endif
};

static DEFINE_MUTEX(ap3220_mutex);

typedef enum {
	AP3220_REG_Sys = 0x00,
	AP3220_REG_IntStatus = 0x01,
	AP3220_REG_INTClearManner = 0x02,
	AP3220_REG_ALSDataLow = 0x0C,
	AP3220_REG_ALSDataHigh = 0x0D,
	AP3220_REG_PSDataLow = 0x0E,
	AP3220_REG_PSDataHigh = 0x0F,
	AP3220_REG_AlsConfig = 0x10,
	AP3220_REG_AlsCalibr = 0x19,
	AP3220_REG_AlsLowThr_L = 0x1A,
	AP3220_REG_AlsLowThr_H = 0x1B,
	AP3220_REG_AlsHighThr_L = 0x1C,
	AP3220_REG_AlsHighThr_H = 0x1D,
	AP3220_REG_PSConfig = 0x20,
	AP3220_REG_PSLEDCtrl = 0x21,
	AP3220_REG_PSINTForm = 0x22,
	AP3220_REG_PSMeanTime = 0x23,
	AP3220_REG_PSLEDWaitTime = 0x24,
	AP3220_REG_PSCalibL = 0x28,
	AP3220_REG_PSCalibH = 0x29,
	AP3220_REG_PSLowThr_L = 0x2A,
	AP3220_REG_PSLowThr_H = 0x2B,
	AP3220_REG_PSHighThr_L = 0x2C,
	AP3220_REG_PSHighThr_H = 0x2D,
} ap3220_regs_list;

typedef struct {
	u8 reg, val, flag;
} regs_def;

regs_def ap3220_regs[] = {
	{AP3220_REG_Sys, 0x00, 0},
	{AP3220_REG_IntStatus, 0x00, 0},
	{AP3220_REG_INTClearManner, 0x01, 1},
	{AP3220_REG_ALSDataLow, 0x00, 0},
	{AP3220_REG_ALSDataHigh, 0x00, 0},
	{AP3220_REG_PSDataLow, 0x00, 0},
	{AP3220_REG_PSDataHigh, 0x00, 0},
	{AP3220_REG_AlsConfig, 0x01, 1},
	{AP3220_REG_AlsCalibr, 0x40, 1}, // do not change!
	{AP3220_REG_AlsLowThr_L, 0x00, 0},
	{AP3220_REG_AlsLowThr_H, 0x00, 0},
	{AP3220_REG_AlsHighThr_L, 0x00, 0},
	{AP3220_REG_AlsHighThr_H, 0x00, 0},
	{AP3220_REG_PSConfig, 0x86, 1},
	{AP3220_REG_PSLEDCtrl, 0x13, 1},
	{AP3220_REG_PSINTForm, 0x00, 0},
	{AP3220_REG_PSMeanTime, 0x00, 1},
	{AP3220_REG_PSLEDWaitTime, 0x00, 0},
	{AP3220_REG_PSCalibL, 0x00, 0},
	{AP3220_REG_PSCalibH, 0x00, 0},
	{AP3220_REG_PSLowThr_L, 0x00, 0},
	{AP3220_REG_PSLowThr_H, 0x00, 0},
	{AP3220_REG_PSHighThr_L, 0x00, 0},
	{AP3220_REG_PSHighThr_H, 0x00, 0},
};


#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{.compatible = "mediatek,alsps"},
	{},
};
#endif

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops ap3220_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ap3220_i2c_suspend, ap3220_i2c_resume)
};
#endif

/*----------------------------------------------------------------------------*/
static struct i2c_driver ap3220_i2c_driver = {
	.probe		= ap3220_i2c_probe,
	.remove	= ap3220_i2c_remove,
	.detect = ap3220_i2c_detect,
	.id_table	= ap3220_i2c_id,
	.driver = {
/* .owner			= THIS_MODULE, */
		.name		= AP3220_DEV_NAME,
#ifdef CONFIG_OF
		.of_match_table = alsps_of_match,
#endif
#ifdef CONFIG_PM_SLEEP
		.pm 		= &ap3220_pm_ops,
#endif
	},
};

static int ap3220_local_init(void);
static int ap3220_remove(void);

static int ap3220_init_flag = -1; /* 0<==>OK -1 <==> fail */

static struct alsps_init_info ap3220_init_info = {
	.name = "ap3220",
	.init = ap3220_local_init,
	.uninit = ap3220_remove,
};

/*
 * #########
 * ## I2C ##
 * #########
 */
static int ap3220_i2c_read_block(struct i2c_client *client, u8 addr, u8 *data,
    u8 len)
{
	int err = 0;
	u8 beg = addr;
	struct i2c_msg msgs[2] = { {0}, {0} };

	mutex_lock(&ap3220_mutex);
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &beg;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = data;

	if (!client) {
		mutex_unlock(&ap3220_mutex);
		return -EINVAL;
	} else if (len > C_I2C_FIFO_SIZE) {
		mutex_unlock(&ap3220_mutex);
		APS_ERR("length %d exceeds %d", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	err = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));

	if (err != 2) {
		APS_ERR("i2c_transfer error: (%d %p %d) %d", addr, data, len, err);
		err = -EIO;
	} else
		err = 0;

	mutex_unlock(&ap3220_mutex);
	return err;

}

int ap3220_i2c_master_operate(struct i2c_client *client, char *buf, int count,
    int i2c_flag)
{
	int res = 0;
#ifndef CONFIG_MTK_I2C_EXTENSION
	struct i2c_msg msg[2];
#endif
	mutex_lock(&ap3220_mutex);

	switch (i2c_flag) {
	case I2C_FLAG_WRITE:
#ifdef CONFIG_MTK_I2C_EXTENSION
		client->addr &= I2C_MASK_FLAG;
		res = i2c_master_send(client, buf, count);
		client->addr &= I2C_MASK_FLAG;
#else
		res = i2c_master_send(client, buf, count);
#endif
		break;

	case I2C_FLAG_READ:
#ifdef CONFIG_MTK_I2C_EXTENSION
		client->addr &= I2C_MASK_FLAG;
		client->addr |= I2C_WR_FLAG;
		client->addr |= I2C_RS_FLAG;
		res = i2c_master_send(client, buf, (count << 8) | 1);
		client->addr &= I2C_MASK_FLAG;
#else
		msg[0].addr = client->addr;
		msg[0].flags = 0;
		msg[0].len = 1;
		msg[0].buf = buf;

		msg[1].addr = client->addr;
		msg[1].flags = I2C_M_RD;
		msg[1].len = count;
		msg[1].buf = buf;
		res = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
#endif
		break;

	default:
		APS_LOG("ap3220_i2c_master_operate i2c_flag command not support!");
		break;
	}

	if (res < 0)
		goto EXIT_ERR;

	mutex_unlock(&ap3220_mutex);
	return res;
EXIT_ERR:
	mutex_unlock(&ap3220_mutex);
	APS_ERR("ap3220_i2c_master_operate fail");
	return res;
}


static int ap3220_read_reg(struct i2c_client *client, char reg)
{
	int ret = 0;
	char tmp[1];
	tmp[0] = reg;
	mutex_lock(&ap3220_obj->lock);

	ret = i2c_master_send(client, tmp, 0x01);
	if (ret <= 0) {
		APS_ERR("ap3220_read_reg 1 ret=%x", ret);
		goto EXIT_ERR;
	}
	ret = i2c_master_recv(client, tmp, 0x01);
	if (ret <= 0) {
		APS_ERR("ap3220_read_reg 2 ret=%d", ret);
		goto EXIT_ERR;
	}

	mutex_unlock(&ap3220_obj->lock);
	return tmp[0];

EXIT_ERR:
		APS_ERR("ap3220_read_reg fail");
	mutex_unlock(&ap3220_obj->lock);
		return ret;
}

static int ap3220_read_data(struct i2c_client *client,
			u32 reg, uint8_t mask, uint8_t shift)
{
	uint8_t val=0;

	val = ap3220_read_reg(client, reg);

	return (int)((val & mask) >> shift);
}

static int ap3220_write_reg(struct i2c_client *client,
		char reg, u8 val)
{
	int ret = 0x00;
	char tmp[2];

	mutex_lock(&ap3220_obj->lock);

	tmp[0] = reg;
	tmp[1] = val;
	ret = i2c_master_send(client, tmp, 0x02);
	if (ret <= 0) {
		APS_ERR("ap3220_write_reg ret=%d", ret);
		goto EXIT_ERR;
	}

	mutex_unlock(&ap3220_obj->lock);
	return ret;

EXIT_ERR:
	APS_ERR("ap3220_write_reg fail");
	mutex_unlock(&ap3220_obj->lock);
	return ret;
}

static int ap3220_write_data(struct i2c_client *client,
		u32 reg, uint8_t val, uint8_t mask, uint8_t shift)
{
	int ret = 0;
	u8 tmp;

	tmp = ap3220_read_reg(client, reg);
	tmp &= ~mask;
	tmp |= (val << shift);
	ret = ap3220_write_reg(client, reg, tmp);

	return ret;
}


/*----------------------------------------------------------------------------*/
int ap3220_get_addr(struct alsps_hw *hw, struct ap3220_i2c_addr *addr)
{
	if (!hw || !addr)
		return -EFAULT;

	addr->write_addr = hw->i2c_addr[0];
	return 0;
}

#ifdef DI_ALS_AUTO_GAIN
static bool ap3220_als_auto_gain(struct i2c_client *client, int alsraw)
{
	struct ap3220_priv *obj = i2c_get_clientdata(client);

	if ((alsraw >= THRS_HIGH) && (obj->als_gain > 0x00)) {
		obj->als_auto_gain_trig_time = ktime_get_ns();
		obj->als_gain = obj->als_gain -1;
		APS_DBG("auto als gain: %d", obj->als_gain);
		ap3220_set_ALSGain(client, obj->als_gain);
	} else if ((alsraw <= THRS_LOWS) && (obj->als_gain < 0x03)) {
		obj->als_auto_gain_trig_time = ktime_get_ns();
		obj->als_gain = obj->als_gain + 1;
		APS_DBG("auto als gain: %d", obj->als_gain);
		ap3220_set_ALSGain(client, obj->als_gain);
	} else {
		return false;
	}

	return true;
}
#endif

/*----------------------------------------------------------------------------*/
static int ap3220_enable_als(struct i2c_client *client, int enable)
{
	struct ap3220_priv *obj = i2c_get_clientdata(client);
	int res = 0;

	if (client == NULL) {
		APS_DBG("CLIENT CANN'T EQUAL NULL");
		return -1;
	}

	if (enable) {
		res = ap3220_write_data(client, AP3220_REG_Sys, 1, 0x01, 0);
		if (res <= 0)
			goto EXIT_ERR;
		atomic_set(&obj->als_deb_on, 1);
		atomic_set(&obj->als_deb_end, jiffies+atomic_read(&obj->als_debounce)/(1000/HZ));
		APS_DBG("ALS enable");
	#if DELAYED_WORK
		schedule_delayed_work(&obj->eint_work, 113*HZ/1000);
	#endif
	} else {
		res = ap3220_write_data(client, AP3220_REG_Sys, 0, 0x01, 0);
		if (res <= 0)
			goto EXIT_ERR;

		atomic_set(&obj->als_deb_on, 0);
		APS_DBG("ALS disable");
	}
	mdelay(10);
	return 0;

EXIT_ERR:
	APS_ERR("Enable als fail");
	return res;
}

/*----------------------------------------------------------------------------*/
static int ap3220_enable_ps(struct i2c_client *client, int enable)
{
	struct ap3220_priv *obj = i2c_get_clientdata(client);
	int res = 0;

	if (client == NULL) {
		APS_DBG("CLIENT CANN'T EQUAL NULL");
		return -1;
	}

	if (enable) {
		res = ap3220_write_data(client, AP3220_REG_Sys, 1, 0x02, 1);
		if (res <= 0)
			goto EXIT_ERR;

		mdelay(10);

		if (0 == obj->hw->polling_mode_ps) {
		#if defined(CONFIG_OF)
			enable_irq(obj->irq);
		//	__pm_stay_awake(chrg_lock);
		#else
			mt_eint_unmask(CUST_EINT_ALS_NUM);
		#endif
		} else {
		//	__pm_stay_awake(chrg_lock);
			atomic_set(&obj->ps_deb_on, 1);
			atomic_set(&obj->ps_deb_end, jiffies+atomic_read(&obj->ps_debounce)/(1000/HZ));
		}

	#if DELAYED_WORK
		schedule_delayed_work(&obj->eint_work, 113*HZ/1000);
	#endif
		APS_DBG("PS enable");
	} else {
		res = ap3220_write_data(client, AP3220_REG_Sys, 0, 0x02, 1);
		if (res <= 0)
			goto EXIT_ERR;
		mdelay(10);

		obj->als_auto_gain_trig = false;
		obj->als_auto_gain_trig_time = 0;
		obj->bef_als = 0;
		atomic_set(&obj->ps_deb_on, 0);
		APS_DBG("PS disable");

		if (0 == obj->hw->polling_mode_ps) {
		#if (!DELAYED_WORK)
			cancel_work_sync(&obj->eint_work);
		#endif
		#if defined(CONFIG_OF)
			disable_irq_nosync(obj->irq);
		//	__pm_relax(chrg_lock);
		#else
			mt_eint_mask(CUST_EINT_ALS_NUM);
		#endif
		} //else
		//	__pm_relax(chrg_lock);
			//wake_unlock(&chrg_lock);
		ap3220_write_data(obj->client, AP3220_REG_IntStatus, 1, 0x02, 1);  // clear ps int flag
	}

	return 0;

EXIT_ERR:
	APS_ERR("Enable ps fail");
	return res;
}
/*----------------------------------------------------------------------------*/
static int ap3220_check_and_clear_intr(struct i2c_client *client)
{
	int ret = 0;

	ret = ap3220_write_data(client, AP3220_REG_IntStatus, 0x03, 0x03, 0);

	if (ret <= 0) {
		APS_ERR("ap3220_write_reg ret=%d", ret);
	}

	return ret;
}

/* als */
static int ap3220_set_ALSGain(struct i2c_client *client, int val)
{
	struct ap3220_priv *obj = i2c_get_clientdata(client);
	int err;

	val &= 0x03;
	obj->als_gain = (uint8_t)val;
	err = ap3220_write_data(client, AP3220_REG_AlsConfig, val, 0x30, 4);

	return err;
}

static int ap3220_get_ALSGain(struct i2c_client *client)
{
	uint8_t val=0;

	val = ap3220_read_data(client, AP3220_REG_AlsConfig, 0x30, 4);

	return (int)val;
}

static int ap3220_set_althres(struct i2c_client *client, int val)
{
	int lsb, msb, err;

	msb = val >> 8;
	lsb = val & 0xff;

	err = ap3220_write_reg(client, 0x1A, lsb);
	err = ap3220_write_reg(client, 0x1B, msb);
	if (err <= 0)
		return err;

	return err;
}

static int ap3220_get_althres(struct i2c_client *client)
{
	u8 val[2] = {0};

	ap3220_i2c_read_block(client, 0x1A, val, 2);

	return (val[1] << 8) | val[0];
}

static int ap3220_set_ahthres(struct i2c_client *client, int val)
{
	int lsb, msb, err;

	msb = val >> 8;
	lsb = val & 0xff;

	err = ap3220_write_reg(client, 0x1C, lsb);
	err = ap3220_write_reg(client, 0x1D, msb);

	if (err <= 0)
		return err;

	return err;
}

static int ap3220_get_ahthres(struct i2c_client *client)
{
	u8 val[2] = {0};

	ap3220_i2c_read_block(client, 0x1C, val, 2);

	return (val[1] << 8) | val[0];
}

/* ps */
static int ap3220_set_plthres(struct i2c_client *client, int val)
{
	int lsb, msb, err;

	lsb = val & 0x03;
	msb = val >> 2;

	err = ap3220_write_reg(client, 0x2A, lsb);
	err = ap3220_write_reg(client, 0x2B, msb);

	if (err <= 0)
		return err;

	return err;
}

static int ap3220_get_plthres(struct i2c_client *client)
{
	u8 val[2] = {0};

	ap3220_i2c_read_block(client, 0x2A, val, 2);

	return (val[1] << 2) | (val[0] & 0x03);
}

static int ap3220_set_phthres(struct i2c_client *client, int val)
{
	int lsb, msb, err;

	lsb = val & 0x03;
	msb = val >> 2;

	err = ap3220_write_reg(client, 0x2C, lsb);
	err = ap3220_write_reg(client, 0x2D, msb);

	if (err <= 0)
		return err;

	return err;
}

static int ap3220_get_phthres(struct i2c_client *client)
{
	u8 val[2] = {0};

	ap3220_i2c_read_block(client, 0x2C, val, 2);

	return (val[1] << 2) | (val[0] & 0x03);
}

static int ap3220_set_pcrosstalk(struct i2c_client *client, int val)
{
	int lsb, msb, err;

	lsb = val & 0x01;
	msb = val >> 1;

	err = ap3220_write_reg(client, 0x28, lsb);
	err = ap3220_write_reg(client, 0x29, msb);

	if (err <= 0)
		return err;

	return err;
}

static unsigned int ap3220_get_pcrosstalk(struct i2c_client *client)
{
	u8 val[2] = {0};

	ap3220_i2c_read_block(client, 0x28, val, 2);

	return (val[1] << 1) | (val[0] & 0x01);
}

#if 0
static int ap3220_set_PSTtime(struct i2c_client *client, int val)
{
	int re_val, err;

#ifdef AP3220
/*val=0x00~0x3F*/
	re_val = val&0x3F;
	err = ap3220_write_reg(client, 0x25, 0xFF, 0x00, re_val);
#else
/*val=0x00~0xF*/
	re_val = ap3220_read_reg(client, 0x20, 0xFF, 0x00);
	re_val = (re_val&0xF)|(val << 4);
#endif

	return err;
}


/*val=0x00~0x03*/
static int ap3220_set_PSgain(struct i2c_client *client, int val)
{
	int re_val, err;

#ifdef AP3220
	re_val = val << 2;
	err = ap3220_write_reg(client, 0x20, 0xFF, 0x00, re_val);
#else
	re_val = ap3220_read_reg(client, 0x20, 0xFF, 0x00);
	re_val = (re_val&0xF3)|(val << 2);
#endif
	return err;
}


static int ap3220_set_PSpulse(struct i2c_client *client, int val)
{
	int re_val, err;

#ifdef AP3220
	re_val = ap3220_read_reg(client, 0x21, 0xFF, 0x00);
	re_val = ((re_val&0xFC)|val);


	err = ap3220_write_reg(client, 0x21, 0xFF, 0x00, re_val);
#else
/*val=0x00~0x03*/
	re_val = ap3220_read_reg(client, 0x21, 0xFF, 0x00);
	re_val = (re_val&0xCF)|(val<<4);


	err = ap3220_write_reg(client, 0x21, 0xFF, 0x00, re_val);
#endif

	return err;
}

static int ap3220_set_meantime(struct i2c_client *client, int val)
{
	int re_val, err;
/*val=0x00~0x03*/
	re_val = val&0x3;
	err = ap3220_write_reg(client, 0x23,
		0xFF, 0x00, re_val);

	return err;
}
#endif

/*----------------------------------------------------------------------------*/

void ap3220_eint_func(void)
{
	struct ap3220_priv *obj = ap3220_obj;
	if (unlikely(obj == NULL)) {
		APS_ERR("%s--%d ap3220_obj is NULL!");
		return;
	}

	APS_DBG("%s start", __func__);


#if DELAYED_WORK
	schedule_delayed_work(&obj->eint_work, 113*HZ/1000);
#else
	schedule_work(&obj->eint_work);
#endif
}

#if defined(CONFIG_OF)
static irqreturn_t ap3220_eint_handler(int irq, void *desc)
{
	struct ap3220_priv *obj = ap3220_obj;
	if (unlikely(obj == NULL)) {
		APS_ERR("%s--%d ap3220_obj is NULL!");
		return IRQ_HANDLED;
	}

	APS_LOG("%s start", __func__);


	disable_irq_nosync(obj->irq);
	ap3220_eint_func();

	return IRQ_HANDLED;
}

#endif

/*----------------------------------------------------------------------------*/
/* This function depends the real hw setting, customers should modify it. 2012/5/10 YC. */
int ap3220_setup_eint(struct i2c_client *client)
{

	APS_FUN();

	/* eint request */
	if (ap3220_obj->irq_node) {
		ap3220_obj->irq = irq_of_parse_and_map(ap3220_obj->irq_node, 0);
		APS_LOG("ap3220_obj->irq = %d", ap3220_obj->irq);
		if (!ap3220_obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!");
			return -EINVAL;
		}
		if (request_irq(ap3220_obj->irq, ap3220_eint_handler, IRQ_TYPE_LEVEL_LOW|IRQF_ONESHOT, "ALS-eint", ap3220_obj)) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!");
			return -EINVAL;
		}
		disable_irq_nosync(ap3220_obj->irq);
	} else {
		APS_ERR("null irq node!!");
		return -EINVAL;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int ap3220_init_client(struct i2c_client *client)
{
	struct ap3220_priv *obj = i2c_get_clientdata(client);
	int res = 0;
	int i;

	res = ap3220_write_reg(client, AP3220_REG_Sys, 0x04); // SW rest
	if (res <= 0)
		return res;
	else
		mdelay(30);

	for (i = 0; i < ARRAY_SIZE(ap3220_regs); i++) {
		if (ap3220_regs[i].flag == 1) 	{
			res = ap3220_write_reg(client, ap3220_regs[i].reg, ap3220_regs[i].val);
			if (res <= 0) {
				APS_ERR("i2c_master_send function err");
			}
		}
	}

	ap3220_set_plthres(obj->client, atomic_read(&obj->ps_thd_val_l));
	ap3220_set_phthres(obj->client, atomic_read(&obj->ps_thd_val_h));

	return res;
}

/******************************************************************************
 * Function Configuration
******************************************************************************/
int ap3220_read_als_lux(struct i2c_client *client, u16 *data)
{
	struct ap3220_priv *obj = i2c_get_clientdata(client);
	u64 now_time;
	u16 alsraw;
	u64 alscali=0;
	u32 ap3220fixp=10000, datatmp=0;

	now_time = 0;
	if (client == NULL) {
		APS_DBG("CLIENT CANN'T EQUAL NULL");
		return -1;
	}

#ifdef DI_ALS_AUTO_GAIN
	if (obj->als_auto_gain_trig) {
		now_time = ktime_get_ns();
		if ((now_time - obj->als_auto_gain_trig_time) < obj->als_auto_gain_msdelay * 1000000) {
			*data = obj->bef_als;
			goto exit_als_auto_gain;
		}
	}
#endif

	obj->als_gain = ap3220_get_ALSGain(client);
	ap3220_read_als_raw(obj->client, &alsraw);
	APS_DBG("raw %d, als gain: %d", alsraw, obj->als_gain);

#ifdef DI_ALS_AUTO_GAIN
	if (ap3220_als_auto_gain(client, alsraw)) {
		*data = obj->bef_als;
		goto exit_als_auto_gain;
	}
#endif

	datatmp = (ap3220_range[obj->als_gain] * ap3220fixp)/65535;
	datatmp = ((uint32_t)alsraw * datatmp)/ap3220fixp;
	*data = (uint16_t)datatmp;
	alscali = (u64)*data * ((u64)obj->als_cali);
	*data = (u16)(alscali/100);
	APS_DBG("%s lux %d, als raw %d", __func__, *data, alsraw);
	obj->bef_als = *data;

exit_als_auto_gain:

	return 0;
}

static int ap3220_read_als_raw(struct i2c_client *client, u16 *data)
{
	u8 val[2] = {0};

	if (client == NULL) {
		APS_DBG("CLIENT CANN'T EQUAL NULL");
		return -1;
	}

	ap3220_i2c_read_block(client, AP3220_REG_ALSDataLow, val, 2);

	*data = val[0] | (val[1]<<8);

	return 0;
}


/*----------------------------------------------------------------------------*/
#if 0
static int ap3220_get_als_value(struct ap3220_priv *obj, u16 als)
{
	int idx;
	int invalid = 0;
	for (idx = 0; idx < obj->als_level_num; idx++)
		if (als < obj->hw->als_level[idx])
			break;


	if (idx >= obj->als_value_num) {
		APS_ERR("exceed range\n");
		idx = obj->als_value_num - 1;
	}

	if (1 == atomic_read(&obj->als_deb_on)) {
		unsigned long endt = atomic_read(&obj->als_deb_end);
		if (time_after(jiffies, endt))
			atomic_set(&obj->als_deb_on, 0);


		if (1 == atomic_read(&obj->als_deb_on))
			invalid = 1;

	}

	if (!invalid) {
		if (atomic_read(&obj->trace) & TRACE_DEBUG)
			APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);

		return obj->hw->als_value[idx];
	} else{
		APS_ERR("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);
		return -1;
	}
}
#endif
/*----------------------------------------------------------------------------*/
int ap3220_read_ps_raw(struct i2c_client *client, u16 *data)
{
	/*struct ap3220_priv *obj = i2c_get_clientdata(client); */
	u8 val[2]={0};

	if (client == NULL) {
		APS_DBG("CLIENT CANN'T EQUAL NULL");
		return -1;
	}

	ap3220_i2c_read_block(client, AP3220_REG_PSDataLow, val, 2);

	val[0] = val[0] & 0x0f;
	val[1] = val[1] & 0x3f;

	*data = val[0] | (val[1] << 4);

	return 0;
}
/*----------------------------------------------------------------------------*/
/*
	for ap3220_get_ps_value:
	return 1 = object close,
	return 0 = object far away.
*/
static int ap3220_get_ps_value(struct ap3220_priv *obj, u16 ps)
{
	int val;
	int invalid = 0;

	if (ps > atomic_read(&obj->ps_thd_val_h))
		val = 1;	/*close*/
	else if (ps < atomic_read(&obj->ps_thd_val_l))
		val = 0;	/*far away*/

	if (atomic_read(&obj->ps_suspend))
		invalid = 1;
	else if (1 == atomic_read(&obj->ps_deb_on)) {
		unsigned long endt = atomic_read(&obj->ps_deb_end);
		if (time_after(jiffies, endt))
			atomic_set(&obj->ps_deb_on, 0);


		if (1 == atomic_read(&obj->ps_deb_on))
			invalid = 1;

	}

	if (!invalid) {
		if (atomic_read(&obj->trace) & TRACE_DEBUG)
			APS_DBG("PS: %05d => %05d", ps, val);

		return val;
	} else
		return -1;

}

static int ap3220_get_obj(struct i2c_client *client)
{

	if (client == NULL) {
		APS_DBG("CLIENT CANN'T EQUAL NULL");
		return -1;
	}

	return (int)ap3220_read_data(client, 0x0e, 0x80, 7);
}

#ifdef DI_AUTO_CAL
u8 ap3220_Calibration_Flag = 0;
int ap3220_Calibration(struct i2c_client *client)
{
	int err;
	int i = 0;
	u16 ps_data = 0;
	/* struct i2c_client *client = (struct i2c_client*)file->private_data; */
	struct ap3220_priv *obj = i2c_get_clientdata(client);

	APS_DBG("AP3220 C_F =%d", ap3220_Calibration_Flag);

	if (ap3220_Calibration_Flag == 0) {
		for (i = 0; i < 5; i++) {
			err = ap3220_read_ps_raw(obj->client, &obj->ps);
			if (i == 0)
				continue;

			if (err != 0)
				goto err_out;

			if ((obj->ps) > DI_PS_CAL_THR) {
				ap3220_Calibration_Flag = 0;
				goto err_out;
			} else
				ps_data += obj->ps;

			msleep(100);
		}
		ap3220_Calibration_Flag = 1;

		ps_data = ps_data/4;

		APS_DBG("AP3220 ps_data =%d", ps_data);

		ap3220_set_pcrosstalk(obj->client, ps_data);
	}
	return 1;
err_out:
	APS_ERR("ap3220_read_ps_raw fail");
	return -1;
}
#endif

/*----------------------------------------------------------------------------*/
static void ap3220_eint_work(struct work_struct *work)
{
	struct ap3220_priv *obj = (struct ap3220_priv *)container_of(work, struct ap3220_priv, eint_work);
	int err, int_stat;
	//hwm_sensor_data sensor_data;

	int_stat = ap3220_read_data(obj->client, AP3220_REG_IntStatus, 0x03, 0x00);
	APS_DBG("Int flag: 0x%02x", int_stat);

	if ((int_stat < 0) || int_stat & 0x20)
		APS_ERR("ap3220_eint_work check intrs: %d", err);
	else if (int_stat & 0x01) {
		/* ALS interrupt. User should add their code here if they want to handle ALS Int. */
	} else if (int_stat & 0x02) {
		/* ap3220_read_ps_raw(obj->client, &obj->ps); */
		err = ps_report_interrupt_data(ap3220_get_obj(obj->client));
		if (err < 0)
			APS_ERR("call hwmsen_get_interrupt_data fail = %d", err);

		ap3220_write_data(obj->client, AP3220_REG_IntStatus, 1, 0x02, 1);  // clear ps int flag
	}
#if defined(CONFIG_OF)
	enable_irq(obj->irq);
#else
	mt_eint_unmask(CUST_EINT_ALS_NUM);
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t ps_raw_show(struct device_driver *ddri, char *buf)
{
	struct ap3220_priv *obj = ap3220_obj;
	int err = 0;
	u16 ps = -1;

	if (obj == NULL) {
		APS_ERR("ap3220_obj is null");
		return -1;
	}

	err = ap3220_read_ps_raw(obj->client, &ps);
	if (err < 0) {
		APS_ERR("ap3220_read_ps_raw failed");
		return -1;
	}
	return sprintf(buf, "ps raw = %d\n", ps);
}
static ssize_t ps_obj_show(struct device_driver *ddri, char *buf)
{
	struct ap3220_priv *obj = ap3220_obj;
	int err = 0;
	int value = -1;
	int status = -1;

	if (obj == NULL) {
		APS_ERR("ap3220_obj is null");
		return -1;
	}

	err = ap3220_ps_get_data(&value, &status);
	if (err < 0) {
		APS_ERR("ap3220_ps_get_data failed");
		return -1;
	}
	return sprintf(buf, "ps data = %d,,status=%d\n", value, status);
}

static ssize_t ps_cali_show(struct device_driver *ddri, char *buf)
{
	struct ap3220_priv *obj = ap3220_obj;
    int val;

    val = ap3220_get_pcrosstalk(obj->client);
    return sprintf(buf, "%d\n", val);
}

static ssize_t ps_cali_store(struct device_driver *ddri, const char *buf, size_t count)
{
	struct ap3220_priv *obj = ap3220_obj;
	unsigned long val;
	int ret;

	//if (strict_strtoul(buf, 10, &val) < 0)
	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	ap3220_enable_ps(obj->client, 1);
	msleep(100);
	ret = ap3220_Calibration(obj->client);
	if (ret < 0)
		return ret;

	return count;
}

static ssize_t ps_lth_show(struct device_driver *ddri, char *buf)
{
    int val;
    struct ap3220_priv *obj = ap3220_obj;

    val = ap3220_get_plthres(obj->client);
    return sprintf(buf, "%d\n", val);
}

static ssize_t ps_lth_store(struct device_driver *ddri, const char *buf, size_t count)
{
	struct ap3220_priv *obj = ap3220_obj;
	unsigned long val;
	int ret;

	//if (strict_strtoul(buf, 10, &val) < 0)
	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	ret = ap3220_set_plthres(obj->client,val);
	if (ret < 0)
		return ret;

	return count;
}

static ssize_t ps_hth_show(struct device_driver *ddri, char *buf)
{
    int val;
    struct ap3220_priv *obj = ap3220_obj;

    val = ap3220_get_phthres(obj->client);
    return sprintf(buf, "%d\n", val);
}

static ssize_t ps_hth_store(struct device_driver *ddri, const char *buf, size_t count)
{
	struct ap3220_priv *obj = ap3220_obj;
	unsigned long val;
	int ret;

	//if (strict_strtoul(buf, 10, &val) < 0)
	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	ret = ap3220_set_phthres(obj->client,val);
	if (ret < 0)
		return ret;

	return count;
}

static ssize_t als_lth_show(struct device_driver *ddri, char *buf)
{
    unsigned int val;
    struct ap3220_priv *obj = ap3220_obj;

    val = ap3220_get_althres(obj->client);
    return sprintf(buf, "%d\n", val);
}

static ssize_t als_lth_store(struct device_driver *ddri, const char *buf, size_t count)
{
	struct ap3220_priv *obj = ap3220_obj;
	unsigned long val;
	int ret;

	//if (strict_strtoul(buf, 10, &val) < 0)
	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	ret = ap3220_set_althres(obj->client,val);
	if (ret < 0)
		return ret;

	return count;
}

static ssize_t als_hth_show(struct device_driver *ddri, char *buf)
{
    unsigned int val;
    struct ap3220_priv *obj = ap3220_obj;

    val = ap3220_get_ahthres(obj->client);
    return sprintf(buf, "%d\n", val);
}

static ssize_t als_hth_store(struct device_driver *ddri, const char *buf, size_t count)
{
	struct ap3220_priv *obj = ap3220_obj;
	unsigned long val;
	int ret;

	//if (strict_strtoul(buf, 10, &val) < 0)
	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	ret = ap3220_set_ahthres(obj->client,val);
	if (ret < 0)
		return ret;

	return count;
}

static ssize_t als_raw_show(struct device_driver *ddri, char *buf)
{
	struct ap3220_priv *obj = ap3220_obj;
	int err = 0;
	u16 als = -1;

	if (obj == NULL) {
		APS_ERR("ap3220_obj is null");
		return -1;
	}

	err = ap3220_read_als_raw(obj->client, &als);
	if (err < 0) {
		APS_ERR("ap3220_read_als_raw failed");
		return -1;
	}
	return sprintf(buf, "als raw dat= %d\n", als);
}

static ssize_t als_lux_show(struct device_driver *ddri, char *buf)
{
	struct ap3220_priv *obj = ap3220_obj;
	int err = 0;
	int value = -1;
	int status = -1;

	if (obj == NULL) {
		APS_ERR("ap3220_obj is null");
		return -1;
	}

	err = ap3220_als_get_lux(&value, &status);
	if (err < 0) {
		APS_ERR("ap3220_als_get_lux failed");
		return -1;
	}
	return sprintf(buf, "als dat= %d,,status=%d\n", value, status);
}

static ssize_t als_cali_show(struct device_driver *ddri, char *buf)
{
    struct ap3220_priv *obj = ap3220_obj;

    return sprintf(buf, "%d\n", obj->als_cali);
}

static ssize_t als_cali_store(struct device_driver *ddri, const char *buf, size_t count)
{
	struct ap3220_priv *obj = ap3220_obj;
	unsigned long val;

	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	obj->als_cali = val;

	return count;
}

static ssize_t trace_show(struct device_driver *ddri, char *buf)
{
	struct ap3220_priv *obj = ap3220_obj;
	int ret = 0;

	if (obj == NULL) {
		APS_ERR("ap3220_obj is null");
		return -1;
	}
	ret = sprintf(buf, "obj->trace = %d,\n", atomic_read(&obj->trace));

	return ret;
}

static ssize_t trace_store(struct device_driver *ddri, const char *buf, size_t count)
{
	int val = 0;
	int ret = 0;

	if (!ap3220_obj) {
		APS_ERR("ap3220_obj is null!!");
		return -1;
	}
	ret = sscanf(buf, "0x%x", &val);
	if (ret == 1)
		atomic_set(&ap3220_obj->trace, val);

	return count;
}

static ssize_t status_show(struct device_driver *ddri, char *buf)
{
	struct ap3220_priv *obj = ap3220_obj;
	int ret = 0;
	int i = 0;

	if (obj == NULL) {
		APS_ERR("ap3220_obj is null");
		return -1;
	}
	ret += sprintf(buf+ret, "obj->irq = %d,\n", obj->irq);
#ifdef CUST_EINT_ALS_NUM
	ret += sprintf(buf+ret, "CUST_EINT_ALS_NUM = %d,\n", CUST_EINT_ALS_NUM);
#endif
	ret += sprintf(buf+ret, "als_level:");
	for (i = 0; i < sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]); i++)
		ret += sprintf(buf+ret, "%d, ", obj->hw->als_level[i]);

	ret += sprintf(buf+ret, "\n als_value:");
	for (i = 0; i < sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]); i++)
		ret += sprintf(buf+ret, "%d, ", obj->hw->als_value[i]);


	ret += sprintf(buf+ret, "\n ps_thd_val_h= %d,,ps_thd_val_l=%d, i2c_num = %d\n",
		atomic_read(&obj->ps_thd_val_h), atomic_read(&obj->ps_thd_val_l), obj->hw->i2c_num);
	return ret;
}

static ssize_t em_show(struct device_driver *ddri, char *buf)
{
	struct ap3220_priv *obj = ap3220_obj;
	int i = 0;
	u8 databuf[2] = {0};
	int tmp_index = 0;

	for (i = 0; i < ARRAY_SIZE(ap3220_regs); i++) {
		databuf[0] = ap3220_read_reg(obj->client, ap3220_regs[i].reg);
		tmp_index += sprintf(buf + tmp_index, "Reg:[0x%02x] Val:[0x%02x]\n",
		        ap3220_regs[i].reg, databuf[0]);
		APS_ERR("Reg:[0x%02x] Val:[0x%02x]", ap3220_regs[i].reg, databuf[0]);
	}

	return tmp_index;
}

static ssize_t em_store(struct device_driver *ddri, const char *buf, size_t count)
{

	int addr, val;
	int ret = 0;

	if (!ap3220_obj) {
		APS_ERR("ap3220_obj is null!!");
		return -1;
	}

	ret = sscanf(buf, "%x %x", &addr, &val);

	APS_LOG("Reg[%x].Write [%x]", addr, val);

	ret = ap3220_write_reg(ap3220_obj->client, addr, val);

	return count;
}
static DRIVER_ATTR_RW(em);
static DRIVER_ATTR_RO(ps_raw);
static DRIVER_ATTR_RO(ps_obj);
static DRIVER_ATTR_RW(ps_cali);
static DRIVER_ATTR_RW(ps_hth);
static DRIVER_ATTR_RW(ps_lth);
static DRIVER_ATTR_RO(als_raw);
static DRIVER_ATTR_RO(als_lux);
static DRIVER_ATTR_RW(als_cali);
static DRIVER_ATTR_RW(als_hth);
static DRIVER_ATTR_RW(als_lth);
static DRIVER_ATTR_RO(status);
static DRIVER_ATTR_RW(trace);



static struct driver_attribute *ap3220_attr_list[] = {
	&driver_attr_em,
	&driver_attr_ps_raw,
	&driver_attr_ps_obj,
	&driver_attr_ps_cali,
	&driver_attr_ps_hth,
	&driver_attr_ps_lth,
	&driver_attr_als_raw,
	&driver_attr_als_lux,
	&driver_attr_als_cali,
	&driver_attr_als_hth,
	&driver_attr_als_lth,
	&driver_attr_status,
	&driver_attr_trace,
};

static int ap3220_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(ap3220_attr_list)/sizeof(ap3220_attr_list[0]));
	if (driver == NULL)
		return -EINVAL;


	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, ap3220_attr_list[idx]);
		if (err != 0) {
			APS_ERR("driver_create_file (%s) = %d", ap3220_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}

/*----------------------------------------------------------------------------*/
static int ap3220_delete_attr(struct device_driver *driver)
{
	int idx , err = 0;
	int num = (int)(sizeof(ap3220_attr_list)/sizeof(ap3220_attr_list[0]));

	if (!driver)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, ap3220_attr_list[idx]);


	return err;
}

/******************************************************************************
 * Function Configuration
******************************************************************************/
static int ap3220_open(struct inode *inode, struct file *file)
{
	file->private_data = ap3220_i2c_client;

	if (!file->private_data) {
		APS_ERR("null pointer!!");
		return -EINVAL;
	}

	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int ap3220_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
static long ap3220_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	struct ap3220_priv *obj = i2c_get_clientdata(client);
	long err = 0;
	void __user *ptr = (void __user *) arg;
	int dat;
	uint32_t enable;

	switch (cmd) {
	case ALSPS_SET_PS_MODE:
			if (copy_from_user(&enable, ptr, sizeof(enable))) {
				err = -EFAULT;
				goto err_out;
			}
			if (enable) {
				err = ap3220_enable_ps(obj->client, 1);
				if (err != 0) {
					APS_ERR("enable ps fail: %ld", err);
					goto err_out;
				}
				msleep(100);

				set_bit(CMC_BIT_PS, &obj->enable);

			} else{
				err = ap3220_enable_ps(obj->client, 0);
				if (err != 0) {
					APS_ERR("disable ps fail: %ld", err);
					goto err_out;
				}


				clear_bit(CMC_BIT_PS, &obj->enable);

			}
			break;
/*
	case ALSPS_GET_PS_MODE:

			enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);

			if (copy_to_user(ptr, &enable, sizeof(enable))) {
				err = -EFAULT;
				goto err_out;
			}
			break;

	case ALSPS_GET_PS_DATA:
			err = ap3220_read_ps_raw(obj->client, &obj->ps);
			if (err != 0)
				goto err_out;


			dat = ap3220_get_ps_value(obj, obj->ps);
			if (copy_to_user(ptr, &dat, sizeof(dat))) {
				err = -EFAULT;
				goto err_out;
			}
			break;
*/
	case ALSPS_GET_PS_RAW_DATA:
			err = ap3220_read_ps_raw(obj->client, &obj->ps);
			if (err != 0)
				goto err_out;


			dat = obj->ps;
			if (copy_to_user(ptr, &dat, sizeof(dat))) {
				err = -EFAULT;
				goto err_out;
			}
			break;

	case ALSPS_SET_ALS_MODE:
			if (copy_from_user(&enable, ptr, sizeof(enable))) {
				err = -EFAULT;
				goto err_out;
			}
			if (enable) {
				err = ap3220_enable_als(obj->client, 1);
				if (err != 0) {
					APS_ERR("enable als fail: %ld", err);
					goto err_out;
				}

				set_bit(CMC_BIT_ALS, &obj->enable);

			} else{
				err = ap3220_enable_als(obj->client, 0);
				if (err != 0) {
					APS_ERR("disable als fail: %ld", err);
					goto err_out;
				}

				clear_bit(CMC_BIT_ALS, &obj->enable);

			}
			break;
/*
	case ALSPS_GET_ALS_MODE:

			enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);

			if (copy_to_user(ptr, &enable, sizeof(enable)))	{
				err = -EFAULT;
				goto err_out;
			}
			break;

	case ALSPS_GET_ALS_DATA:
			err = ap3220_read_als_lux(obj->client, &obj->als);
			if (err != 0)
				goto err_out;

			dat = obj->als;
			if (copy_to_user(ptr, &dat, sizeof(dat))) {
				err = -EFAULT;
				goto err_out;
			}
			break;
*/
	case ALSPS_GET_ALS_RAW_DATA:
			err = ap3220_read_als_raw(obj->client, &obj->als);
			if (err != 0)
				goto err_out;


			dat = obj->als;
			if (copy_to_user(ptr, &dat, sizeof(dat))) {
				err = -EFAULT;
				goto err_out;
			}
			break;

	default:
			APS_ERR("%s not supported = 0x%04x", __func__, cmd);
			err = -ENOIOCTLCMD;
			break;
	}

err_out:
	return err;
}
/*----------------------------------------------------------------------------*/
static const struct file_operations ap3220_fops = {
/* .owner = THIS_MODULE, */
	.open = ap3220_open,
	.release = ap3220_release,
	.unlocked_ioctl = ap3220_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice ap3220_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &ap3220_fops,
};
/*----------------------------------------------------------------------------*/
static int ap3220_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, AP3220_DEV_NAME);
	return 0;
}

static int ap3220_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ap3220_priv *obj = i2c_get_clientdata(client);
	int err = 0;

	/* APS_FUN(); */
	if (!obj) {
		APS_ERR("null pointer!!\n");
		return 0;
	}

	atomic_set(&obj->als_suspend, 1);
	err = ap3220_enable_als(obj->client, 0);
	err = ap3220_enable_ps(obj->client, 0);
	if (err)
		APS_ERR("disable als/ps fail: %d\n", err);
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ap3220_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ap3220_priv *obj = i2c_get_clientdata(client);
	int err = 0;

	/* APS_FUN(); */
	if (!obj) {
		APS_ERR("null pointer!!\n");
		return 0;
	}

	atomic_set(&obj->als_suspend, 0);
	if (test_bit(CMC_BIT_ALS, &obj->enable)) {
		err = ap3220_enable_als(obj->client, 1);
		if (err)
			APS_ERR("enable als fail: %d\n", err);

	}

	if (test_bit(CMC_BIT_PS, &obj->enable)) {
		err = ap3220_enable_ps(obj->client, 1);
		if (err)
			APS_ERR("enable ps fail: %d\n", err);

	}
	return 0;
}

static int ap3220_als_open_report_data(int open)
{

	return 0;
}

static int ap3220_als_enable_nodata(int en)
{
	struct ap3220_priv *obj = ap3220_obj;
	int value = 0;
	int err = 0;

	if (obj == NULL) {
		APS_ERR("ap3220_obj is null");
		return -1;
	}

	value = en;
	if (value) {
		err = ap3220_enable_als(obj->client, 1);
		if (err != 0) {
			APS_ERR("enable als fail: %d", err);
			return -1;
		}
		set_bit(CMC_BIT_ALS, &obj->enable);
	} else {
		err = ap3220_enable_als(obj->client, 0);
		if (err != 0) {
			APS_ERR("disable als fail: %d", err);
			return -1;
		}
		clear_bit(CMC_BIT_ALS, &obj->enable);
	}
	return 0;
}

static int ap3220_als_set_delay(u64 ns)
{
	struct ap3220_priv *obj = ap3220_obj;
	uint8_t psmeantime;
	uint8_t mean[4]={13, 25, 38, 50};
	uint8_t waittime;

	//polling periode ns minumum is 113 dont delow it.

	psmeantime = ap3220_read_data(obj->client, AP3220_REG_PSMeanTime, 0x03, 0);
	waittime = ap3220_read_data(obj->client, AP3220_REG_PSLEDWaitTime, 0x1f, 0);
	obj->als_auto_gain_msdelay = (113 + (mean[psmeantime] * waittime)) * 2;
	APS_DBG("als_auto_gain_msdelay: %lu", obj->als_auto_gain_msdelay);

	return 0;
}

static int ap3220_als_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return ap3220_als_set_delay(samplingPeriodNs);
}

static int ap3220_als_flush(void)
{
	return als_flush_report();
}

static int ap3220_als_get_lux(int *value, int *status)
{
	struct ap3220_priv *obj = ap3220_obj;
	int err = 0;

	if (obj == NULL) {
		APS_ERR("ap3220_obj is null");
		return -1;
	}

	ap3220_read_als_lux(obj->client, &obj->als);

	*value = (int)obj->als;
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return err;
}

static int ap3220_ps_open_report_data(int open)
{
	return 0;
}

static int ap3220_ps_enable_nodata(int en)
{
	struct ap3220_priv *obj = ap3220_obj;
	int value = 0;
	int err = 0;
	int ps_value = -1;
	int ps_status = 0;

	if (obj == NULL) {
		APS_ERR("ap3220_obj is null");
		return -1;
	}
	value = en;
	if (value) {
		err = ap3220_enable_ps(obj->client, 1);
		if (err != 0) {
			APS_ERR("enable ps fail: %d", err);
			return -1;
		}
		set_bit(CMC_BIT_PS, &obj->enable);

#ifdef DI_AUTO_CAL
		ap3220_Calibration(obj->client);
#endif
		err = ap3220_ps_get_data(&ps_value, &ps_status);
		if ((err == 0) && (ps_value > 0))
			ps_report_interrupt_data(ps_value);

	} else{
		err = ap3220_enable_ps(obj->client, 0);
		if (err != 0) {
			APS_ERR("disable ps fail: %d", err);
			return -1;
		}

		clear_bit(CMC_BIT_PS, &obj->enable);
	}

	return 0;
}

static int ap3220_ps_set_delay(u64 ns)
{
	return 0;
}

static int ap3220_ps_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return 0;
}

static int ap3220_ps_flush(void)
{
	return ps_flush_report();
}

static int ap3220_ps_get_data(int *value, int *status)
{
	struct ap3220_priv *obj = ap3220_obj;
	int err = 0;

	if (obj == NULL) {
		APS_ERR("ap3220_obj is null");
		return -1;
	}

	err = ap3220_read_ps_raw(obj->client, &obj->ps);
	if (err != 0) {
		APS_ERR("ap3220_read_ps_raw failed err=%d", err);
		*value = -1;
		return err;
	}
	*value = ap3220_get_ps_value(obj, obj->ps);
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return 0;
}


static int ap3220_als_factory_enable_sensor(bool enable_disable, int64_t sample_periods_ms)
{
	int err = 0;

	err = ap3220_als_enable_nodata(enable_disable ? 1 : 0);
	if (err) {
		APS_ERR("%s:%s failed", __func__, enable_disable ? "enable" : "disable");
		return -1;
	}
	err = ap3220_als_batch(0, sample_periods_ms * 1000000, 0);
	if (err) {
		APS_ERR("%s set_batch failed", __func__);
		return -1;
	}
	return 0;
}
static int ap3220_als_factory_get_data(int32_t *data)
{
	int status;

	return ap3220_als_get_lux(data, &status);
}
static int ap3220_als_factory_get_raw_data(int32_t *data)
{
	int err = 0;
	struct ap3220_priv *obj = ap3220_obj;

	if (!obj) {
		APS_ERR("obj is null!!");
		return -1;
	}

	err = ap3220_read_als_raw(obj->client, &obj->als);
	if (err) {
		APS_ERR("%s failed");
		return -1;
	}
	*data = obj->als;

	return 0;
}
static int ap3220_als_factory_enable_calibration(void)
{
	return 0;
}
static int ap3220_als_factory_clear_cali(void)
{
	return 0;
}
static int ap3220_als_factory_set_cali(int32_t offset)
{
	return 0;
}
static int ap3220_als_factory_get_cali(int32_t *offset)
{
	return 0;
}
static int ap3220_ps_factory_enable_sensor(bool enable_disable, int64_t sample_periods_ms)
{
	int err = 0;

	err = ap3220_ps_enable_nodata(enable_disable ? 1 : 0);
	if (err) {
		APS_ERR("%s:%s failed", enable_disable ? "enable" : "disable");
		return -1;
	}
	err = ap3220_ps_batch(0, sample_periods_ms * 1000000, 0);
	if (err) {
		APS_ERR("%s set_batch failed");
		return -1;
	}
	return err;
}
static int ap3220_ps_factory_get_data(int32_t *data)
{
	int err = 0, status = 0;

	err = ap3220_ps_get_data(data, &status);
	if (err < 0)
		return -1;
	return 0;
}
static int ap3220_ps_factory_get_raw_data(int32_t *data)
{
	int err = 0;
	struct ap3220_priv *obj = ap3220_obj;

	err = ap3220_read_ps_raw(obj->client, &obj->ps);
	if (err) {
		APS_ERR("%s failed");
		return -1;
	}
	*data = ap3220_obj->ps;
	return 0;
}
static int ap3220_ps_factory_enable_calibration(void)
{
	return 0;
}
static int ap3220_ps_factory_clear_cali(void)
{
	struct ap3220_priv *obj = ap3220_obj;

	obj->ps_cali = 0;
	return 0;
}
static int ap3220_ps_factory_set_cali(int32_t offset)
{
	struct ap3220_priv *obj = ap3220_obj;

	obj->ps_cali = offset;
	return 0;
}
static int ap3220_ps_factory_get_cali(int32_t *offset)
{
	struct ap3220_priv *obj = ap3220_obj;

	*offset = obj->ps_cali;
	return 0;
}
static int ap3220_ps_factory_set_threshold(int32_t threshold[2])
{
	int err = 0;
	struct ap3220_priv *obj = ap3220_obj;

	APS_LOG("%s set threshold high: 0x%x, low: 0x%x", __func__, threshold[0], threshold[1]);
	atomic_set(&obj->ps_thd_val_h, (threshold[0] + obj->ps_cali));
	atomic_set(&obj->ps_thd_val_l, (threshold[1] + obj->ps_cali));

	err = ap3220_set_plthres(obj->client, atomic_read(&obj->ps_thd_val_l));

	if (err < 0) {
		APS_ERR("set_psensor_threshold_l fail");
		return -1;
	}

	err = ap3220_set_phthres(obj->client, atomic_read(&obj->ps_thd_val_h));

	if (err < 0) {
		APS_ERR("set_psensor_threshold_h fail");
		return -1;
	}
	return 0;
}
static int ap3220_ps_factory_get_threshold(int32_t threshold[2])
{
	struct ap3220_priv *obj = ap3220_obj;

	threshold[0] = atomic_read(&obj->ps_thd_val_h) - obj->ps_cali;
	threshold[1] = atomic_read(&obj->ps_thd_val_l) - obj->ps_cali;
	return 0;
}


static struct alsps_factory_fops ap3220_factory_fops = {
	.als_enable_sensor = ap3220_als_factory_enable_sensor,
	.als_get_data = ap3220_als_factory_get_data,
	.als_get_raw_data = ap3220_als_factory_get_raw_data,
	.als_enable_calibration = ap3220_als_factory_enable_calibration,
	.als_clear_cali = ap3220_als_factory_clear_cali,
	.als_set_cali = ap3220_als_factory_set_cali,
	.als_get_cali = ap3220_als_factory_get_cali,

	.ps_enable_sensor = ap3220_ps_factory_enable_sensor,
	.ps_get_data = ap3220_ps_factory_get_data,
	.ps_get_raw_data = ap3220_ps_factory_get_raw_data,
	.ps_enable_calibration = ap3220_ps_factory_enable_calibration,
	.ps_clear_cali = ap3220_ps_factory_clear_cali,
	.ps_set_cali = ap3220_ps_factory_set_cali,
	.ps_get_cali = ap3220_ps_factory_get_cali,
	.ps_set_threshold = ap3220_ps_factory_set_threshold,
	.ps_get_threshold = ap3220_ps_factory_get_threshold,
};


static struct alsps_factory_public ap3220_factory_device = {
	.gain = 1,
	.sensitivity = 1,
	.fops = &ap3220_factory_fops,
};

/*----------------------------------------------------------------------------*/
static int ap3220_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ap3220_priv *obj = NULL;
	struct als_control_path als_ctl = {0};
	struct als_data_path als_data = {0};
	struct ps_control_path ps_ctl = {0};
	struct ps_data_path ps_data = {0};
	int err = 0;
	const char *name = "mediatek,alsps";
	struct device_node *node = NULL;
	APS_BDBG("%s ap3220_i2c_probe start");

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (obj == NULL) {
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	ap3220_obj = obj;

	mutex_init(&obj->lock);
	printk(" alsps node is %s\n",client->dev.of_node->name);
	node = of_find_compatible_node(NULL, NULL, name);
	client->dev.of_node = node;
	err = get_alsps_dts_func(client->dev.of_node, hw);
	if (err < 0) {
		APS_ERR("get customization info from dts failed");
		return -EFAULT;
	}

	obj->hw = hw;
	ap3220_get_addr(obj->hw, &obj->addr);
	client->addr = hw->i2c_addr[0];
#if DELAYED_WORK
	INIT_DELAYED_WORK(&obj->eint_work, ap3220_eint_work);
#else
	INIT_WORK(&obj->eint_work, ap3220_eint_work);
#endif
	obj->client = client;
	i2c_set_clientdata(client, obj);

	atomic_set(&obj->als_debounce, 300);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 300);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->als_cmd_val, 0xDF);
	atomic_set(&obj->ps_cmd_val, 0xC1);
	atomic_set(&obj->ps_thd_val_h, obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_l, obj->hw->ps_threshold_low);
	atomic_set(&obj->trace, 0);

	obj->enable = 0;
	obj->irq_node = client->dev.of_node;
	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);
	obj->als_modulus = (400*100*40)/(1*1500);
	obj->als_auto_gain_trig = false;
	obj->als_auto_gain_trig_time = 0;
	obj->bef_als = 0;
	obj->als_cali = 100;

	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);

	ap3220_i2c_client = client;

	err = ap3220_init_client(client);
	if (err <= 0) {
		APS_ERR("ap3220_init_client() ERROR: %d !", err);
		goto exit_init_failed;
	}
	APS_LOG("ap3220_init_client() OK!");

#ifdef DI_AUTO_CAL
	ap3220_enable_ps(client, 1);
	msleep(100);
	ap3220_Calibration(client);
	ap3220_enable_ps(client, 0);
#endif

	APS_BDBG("%s DI_AUTO_CAL");

	err = alsps_factory_device_register(&ap3220_factory_device);
	if (err != 0) {
		APS_ERR("ap3220_device register failed");
		goto exit_misc_device_register_failed;
	}

	err = ap3220_create_attr(&ap3220_init_info.platform_diver_addr->driver);
	if (err != 0) {
		APS_ERR("create attribute err = %d", err);
		goto exit_create_attr_failed;
	}

	APS_ERR("polling_mode_ps = %d", obj->hw->polling_mode_ps);
	if (1 == obj->hw->polling_mode_ps) {
		ps_ctl.is_report_input_direct = false;
		ps_ctl.is_polling_mode = true;
		chrg_lock = wakeup_source_register(&client->dev, "ap3220_wake_lock");
	} else{
		ps_ctl.is_report_input_direct = true;
		ps_ctl.is_polling_mode = false;

		err = ap3220_setup_eint(client);
		if (err != 0) {
			APS_ERR("setup eint: %d", err);
			goto exit_create_attr_failed;
		}
		err = ap3220_check_and_clear_intr(client);
		if (err < 0)
			APS_ERR("check/clear intr: %d", err);

	}

	ps_ctl.open_report_data = ap3220_ps_open_report_data;
	ps_ctl.enable_nodata = ap3220_ps_enable_nodata;
	ps_ctl.set_delay = ap3220_ps_set_delay;
    ps_ctl.batch = ap3220_ps_batch;
	ps_ctl.flush = ap3220_ps_flush;
	//ps_ctl.is_support_batch = obj->hw->is_batch_supported_ps;
	ps_ctl.is_support_batch = false;
	ps_ctl.is_use_common_factory = false;

	err = ps_register_control_path(&ps_ctl);
	if (err != 0) {
		APS_ERR("ps_register_control_path fail = %d", err);
		goto exit_create_attr_failed;
	}

	ps_data.get_data = ap3220_ps_get_data;
	ps_data.vender_div = 1;

	err = ps_register_data_path(&ps_data);

	if (err != 0) {
		APS_ERR("ps_register_data_path fail = %d", err);
		goto exit_create_attr_failed;
	}

	als_ctl.open_report_data = ap3220_als_open_report_data;
	als_ctl.enable_nodata = ap3220_als_enable_nodata;
	als_ctl.set_delay = ap3220_als_set_delay;
	als_ctl.batch = ap3220_als_batch;
	als_ctl.flush = ap3220_als_flush;
	//als_ctl.is_support_batch = obj->hw->is_batch_supported_als;
	als_ctl.is_support_batch = false;
	als_ctl.is_use_common_factory = false;
	als_ctl.is_report_input_direct = false;
	als_ctl.is_polling_mode = true;

	err = als_register_control_path(&als_ctl);
	if (err != 0) {
		APS_ERR("als_register_control_path fail = %d", err);
		goto exit_create_attr_failed;
	}

	als_data.get_data = ap3220_als_get_lux;
	als_data.vender_div = 1;

	err = als_register_data_path(&als_data);

	if (err != 0) {
		APS_ERR("als_register_data_path fail = %d", err);
		goto exit_create_attr_failed;
	}

	APS_LOG("als_register_data_path OK.%s:", __func__);

	ap3220_init_flag = 0;
	APS_LOG("%s: OK", __func__);
	return 0;

exit_create_attr_failed:
	misc_deregister(&ap3220_device);
exit_misc_device_register_failed:
exit_init_failed:
	mutex_destroy(&ap3220_obj->lock);
	kfree(obj);
exit:
	ap3220_i2c_client = NULL;
	ap3220_init_flag = -1;
	APS_ERR("%s: err = %d", err);
	return err;
}
/*----------------------------------------------------------------------------*/
static int ap3220_i2c_remove(struct i2c_client *client)
{
	APS_FUN();

	ap3220_delete_attr(&ap3220_init_info.platform_diver_addr->driver);

	misc_deregister(&ap3220_device);

	if (1 == ap3220_obj->hw->polling_mode_ps)
		wakeup_source_unregister(chrg_lock);


	ap3220_i2c_client = NULL;
	i2c_unregister_device(client);
	mutex_destroy(&ap3220_obj->lock);
	kfree(i2c_get_clientdata(client));

	return 0;
}
/*----------------------------------------------------------------------------*/
static int ap3220_local_init(void)
{
	APS_FUN();

	if (i2c_add_driver(&ap3220_i2c_driver)) {
		APS_ERR("add driver error");
		return -1;
	}
	if (-1 == ap3220_init_flag) {
		APS_ERR("add driver--ap3220_init_flag check error");
		return -1;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int ap3220_remove(void)
{
	APS_FUN();

	i2c_del_driver(&ap3220_i2c_driver);
	ap3220_init_flag = -1;

	return 0;
}
/*----------------------------------------------------------------------------*/
static int __init ap3220_init(void)
{
	APS_FUN();

	alsps_driver_add(&ap3220_init_info);

	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit ap3220_exit(void)
{
	APS_FUN();
}

module_init(ap3220_init);
module_exit(ap3220_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Leo Tsai");
MODULE_DESCRIPTION("AP3220 driver");
MODULE_LICENSE("GPL");
