/* 
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>

#include "cust_alsps.h"
#include "ltr559.h"
#include "alsps.h"

#define GN_MTK_BSP_PS_DYNAMIC_CALI
 //#define DELAYED_PS_CALI
//#define DEMO_BOARD

/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/
#define LTR559_DEV_NAME			"ltr559"

/*----------------------------------------------------------------------------*/
#define APS_TAG					"[ALS/PS] "
#define APS_FUN(f)              printk(KERN_INFO 	APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)   printk(KERN_ERR  	APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)   printk(KERN_NOTICE	APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)   printk(KERN_ERR 	APS_TAG fmt, ##args)

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id ltr559_i2c_id[] = {{LTR559_DEV_NAME,0},{}};
static unsigned long long int_top_time;
struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;

/*----------------------------------------------------------------------------*/
static int ltr559_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int ltr559_i2c_remove(struct i2c_client *client);
static int ltr559_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int ltr559_i2c_suspend(struct device *dev);
static int ltr559_i2c_resume(struct device *dev);

//static int ps_gainrange;
static int als_gainrange;

static int final_prox_val;
static int final_lux_val;

/*----------------------------------------------------------------------------*/
typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;

/*----------------------------------------------------------------------------*/
struct ltr559_priv {
	struct alsps_hw  *hw;
	struct i2c_client *client;
	struct work_struct	eint_work;
	struct delayed_work cali_ps_work;

	/*misc*/
	u16 		als_modulus;
	atomic_t	i2c_retry;
	atomic_t	als_suspend;
	atomic_t	als_debounce;	/*debounce time after enabling als*/
	atomic_t	als_deb_on; 	/*indicates if the debounce is on*/
	atomic_t	als_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_mask;		/*mask ps: always return far away*/
	atomic_t	ps_debounce;	/*debounce time after enabling ps*/
	atomic_t	ps_deb_on;		/*indicates if the debounce is on*/
	atomic_t	ps_deb_end; 	/*the jiffies representing the end of debounce*/
	atomic_t	ps_suspend;
	atomic_t 	trace;

#ifdef CONFIG_OF
	struct device_node *irq_node;
	int		irq;
#endif

	/*data*/
	u16			als;
	u16 		ps;
	u8			_align;
	u16			als_level_num;
	u16			als_value_num;
	u32			als_level[C_CUST_ALS_LEVEL-1];
	u32			als_value[C_CUST_ALS_LEVEL];
	int			ps_cali;
	
	atomic_t	als_cmd_val;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_cmd_val; 	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_high;	 /*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_low; 	/*the cmd value can't be read, stored in ram*/
	atomic_t	als_thd_val_high;	 /*the cmd value can't be read, stored in ram*/
	atomic_t	als_thd_val_low; 	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val;
	ulong		enable; 		/*enable mask*/
	ulong		pending_intr;	/*pending interrupt*/
};

struct PS_CALI_DATA_STRUCT
{
    int close;
    int far_away;
    int valid;
} ;

static struct PS_CALI_DATA_STRUCT ps_cali={0,0,0};
static int intr_flag_value = 0;

static struct ltr559_priv *ltr559_obj = NULL;
static struct i2c_client *ltr559_i2c_client = NULL;

static DEFINE_MUTEX(ltr559_mutex);

static int ltr559_local_init(void);
static int ltr559_remove(void);
static int ltr559_init_flag =  -1;

static int irq_enabled = 0;

static struct alsps_init_info ltr559_init_info = {
		.name = "ltr559",
		.init = ltr559_local_init,
		.uninit = ltr559_remove,
};

#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{.compatible = "mediatek,alsps"},
	{},
};
#endif

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops ltr559_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ltr559_i2c_suspend, ltr559_i2c_resume)
};
#endif

static struct i2c_driver ltr559_i2c_driver = {	
	.probe      = ltr559_i2c_probe,
	.remove     = ltr559_i2c_remove,
	.detect     = ltr559_i2c_detect,
	.id_table   = ltr559_i2c_id,
	.driver = {
		.name           = LTR559_DEV_NAME,
#ifdef CONFIG_OF
		.of_match_table = alsps_of_match,
#endif

#ifdef CONFIG_PM_SLEEP
	.pm = &ltr559_pm_ops,
#endif
	},
};

/*----------------------------------------------------------------------------*/
#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
static int ltr559_dynamic_calibrate(void);
static int dynamic_calibrate = 0;
#endif
/*-----------------------------------------------------------------------------*/

/* 
 * #########
 * ## I2C ##
 * #########
 */

// I2C Read
static int ltr559_i2c_read_reg(u8 regnum)
{
    u8 buffer[1],reg_value[1];
	int res = 0;

	mutex_lock(&ltr559_mutex);
	buffer[0]= regnum;
	res = i2c_master_send(ltr559_obj->client, buffer, 0x1);
	if(res <= 0)
	{	   
	    APS_ERR("read reg send res = %d\n",res);
	    mutex_unlock(&ltr559_mutex);
		return res;
	}
	res = i2c_master_recv(ltr559_obj->client, reg_value, 0x1);
	if(res <= 0)
	{
		APS_ERR("read reg recv res = %d\n",res);
		mutex_unlock(&ltr559_mutex);
		return res;
	}
	mutex_unlock(&ltr559_mutex);
	return reg_value[0];
}

// I2C Write
static int ltr559_i2c_write_reg(u8 regnum, u8 value)
{
	u8 databuf[2];    
	int res = 0;
   
	mutex_lock(&ltr559_mutex);
	databuf[0] = regnum;   
	databuf[1] = value;
	res = i2c_master_send(ltr559_obj->client, databuf, 0x2);
	mutex_unlock(&ltr559_mutex);

	if (res < 0)
	{
		APS_ERR("wirte reg send res = %d\n",res);
	   	return res;
	}		
	else
		return 0;
}

/*----------------------------------------------------------------------------*/
static void ltr559_power(struct alsps_hw *hw, unsigned int on) 
{
#ifdef DEMO_BOARD
	static unsigned int power_on = 0;	

	if(hw->power_id != POWER_NONE_MACRO)
	{
		if(power_on == on)
		{
			APS_LOG("ignore power control: %d\n", on);
		}
		else if(on)
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "ltr559")) 
			{
				APS_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "ltr559")) 
			{
				APS_ERR("power off fail!!\n");   
			}
		}
	}
	power_on = on;
#endif
}
/********************************************************************/
/* 
 * ###############
 * ## PS CONFIG ##
 * ###############

 */
static int ltr559_ps_set_thres(void)
{
	int res;
	u8 databuf[2];
	
	struct i2c_client *client = ltr559_obj->client;
	struct ltr559_priv *obj = ltr559_obj;

	APS_FUN();

	APS_DBG("ps_cali.valid: %d\n", ps_cali.valid);

	if(1 == ps_cali.valid)
	{
		databuf[0] = LTR559_PS_THRES_LOW_0; 
		databuf[1] = (u8)(ps_cali.far_away & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{			
			goto EXIT_ERR;
		}
		databuf[0] = LTR559_PS_THRES_LOW_1; 
		databuf[1] = (u8)((ps_cali.far_away & 0xFF00) >> 8);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR559_PS_THRES_UP_0;	
		databuf[1] = (u8)(ps_cali.close & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR559_PS_THRES_UP_1;	
		databuf[1] = (u8)((ps_cali.close & 0xFF00) >> 8);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
	}
	else
	{
		databuf[0] = LTR559_PS_THRES_LOW_0; 
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR559_PS_THRES_LOW_1; 
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low )>> 8) & 0x00FF);
		
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR559_PS_THRES_UP_0;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR559_PS_THRES_UP_1;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high) >> 8) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}	
	}

	res = 1;
	return res;
	
EXIT_ERR:
	APS_ERR("set thres: %d\n", res);
	return res;
}

static int ltr559_ps_enable(struct i2c_client *client, int enable)
{
	u8 regdata;
	int err;

	APS_LOG("ltr559_ps_enable() ...start!\n");

	regdata = ltr559_i2c_read_reg(LTR559_PS_CONTR);
	if (enable != 0) {
		APS_LOG("PS: enable ps only \n");
		regdata |= 0x03;
	}
	else {
		APS_LOG("PS: disable ps only \n");
		regdata &= 0xfc;
	}

	err = ltr559_i2c_write_reg(LTR559_PS_CONTR, regdata);
	if (err < 0)
	{
		APS_ERR("PS: enable ps err: %d en: %d \n", err, enable);
		return err;
	}
	mdelay(WAKEUP_DELAY);
	ltr559_i2c_read_reg(LTR559_PS_CONTR);

	if (0 == ltr559_obj->hw->polling_mode_ps && enable != 0)
	{
#ifndef DELAYED_PS_CALI
#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
		err = ltr559_dynamic_calibrate();
		if (err < 0)
		{
			APS_LOG("ltr559_dynamic_calibrate() failed\n");			
		}
#endif
		ltr559_ps_set_thres();
#else
		cancel_delayed_work(&ltr559_obj->cali_ps_work);
		schedule_delayed_work(&ltr559_obj->cali_ps_work, msecs_to_jiffies(200));
#endif
	}
	else if (0 == ltr559_obj->hw->polling_mode_ps && enable == 0)
	{
		//cancel_work_sync(&ltr559_obj->eint_work);
	}

	if ((irq_enabled == 1) && (enable != 0))
	{
		irq_enabled = 2;
	}

	return err;
}

/********************************************************************/
static int ltr559_ps_read(struct i2c_client *client, u16 *data)
{
	int psval_lo, psval_hi, psdata;

	psval_lo = ltr559_i2c_read_reg(LTR559_PS_DATA_0);
	APS_DBG("ps_rawdata_psval_lo = %d\n", psval_lo);
	if (psval_lo < 0){	    
	    APS_DBG("psval_lo error\n");
		psdata = psval_lo;
		goto out;
	}

	psval_hi = ltr559_i2c_read_reg(LTR559_PS_DATA_1);
    APS_DBG("ps_rawdata_psval_hi = %d\n", psval_hi);
	if (psval_hi < 0){
	    APS_DBG("psval_hi error\n");
		psdata = psval_hi;
		goto out;
	}
	
	psdata = ((psval_hi & 7)* 256) + psval_lo;
	*data = psdata;
    APS_DBG("ltr559_ps_read: ps_rawdata = %d\n", psdata);
    
out:
	final_prox_val = psdata;	
	return psdata;
}

#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
static int ltr559_dynamic_calibrate(void)
{
	//int ret = 0;
	int i = 0;
	int data;
	int data_total = 0;
	//ssize_t len = 0;
	int noise = 0;
	int count = 5;
	//int max = 0;
	int ps_thd_val_low, ps_thd_val_high;
	struct ltr559_priv *obj = ltr559_obj;

	if (!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		//len = sprintf(buf, "ltr559_obj is null\n");
		return -1;
	}	

	for (i = 0; i < count; i++) {
		// wait for ps value be stable
		msleep(120);

		data = ltr559_ps_read(ltr559_obj->client, &ltr559_obj->ps);
		if (data < 0) {
			i--;
			continue;
		}

		if (data & 0x8000) {
			break;
		}		

		data_total += data;
	}

	noise = data_total / count;
	dynamic_calibrate = noise;

	if (noise < 100) {
		ps_thd_val_high = noise + 100;
		ps_thd_val_low  = noise + 50;
	}
	else if (noise < 200) {
		ps_thd_val_high = noise + 150;
		ps_thd_val_low  = noise + 60;
	}
	else if (noise < 300) {
		ps_thd_val_high = noise + 150;
		ps_thd_val_low  = noise + 60;
	}
	else if (noise < 400) {
		ps_thd_val_high = noise + 150;
		ps_thd_val_low  = noise + 60;
	}
	else if (noise < 600) {
		ps_thd_val_high = noise + 180;
		ps_thd_val_low  = noise + 90;
	}
	else if (noise < 1000) {
		ps_thd_val_high = noise + 300;
		ps_thd_val_low  = noise + 180;
	}
	else if (noise < 1250) {
		ps_thd_val_high = noise + 400;
		ps_thd_val_low  = noise + 300;
	}
	else {
		ps_thd_val_high = 1600;
		ps_thd_val_low  = 1400;
		APS_ERR("dynamic calibrate fails!!\n");
	}

	atomic_set(&obj->ps_thd_val_high, ps_thd_val_high);
	atomic_set(&obj->ps_thd_val_low, ps_thd_val_low);
	
	APS_LOG("%s:noise = %d\n", __func__, noise);
	APS_LOG("%s:obj->ps_thd_val_high = %d\n", __func__, ps_thd_val_high);
	APS_LOG("%s:obj->ps_thd_val_low = %d\n", __func__, ps_thd_val_low);

	return 0;
}
#endif
/********************************************************************/
/* 
 * ################
 * ## ALS CONFIG ##
 * ################
 */

static int ltr559_als_enable(struct i2c_client *client, int enable)
{
	int err = 0;
	u8 regdata = 0;
	
	regdata = ltr559_i2c_read_reg(LTR559_ALS_CONTR);
	if (enable != 0) {
		APS_LOG("ALS(1): enable als only \n");
		regdata |= 0x01;
	}
	else {
		APS_LOG("ALS(1): disable als only \n");
		regdata &= 0xfe;
	}

	err = ltr559_i2c_write_reg(LTR559_ALS_CONTR, regdata);
	if (err < 0)
	{
		APS_ERR("ALS: enable als err: %d en: %d \n", err, enable);
		return err;
	}

	mdelay(WAKEUP_DELAY);

	return 0;
}

static int ltr559_als_read(struct i2c_client *client, u16* data)
{
	int alsval_ch0_lo, alsval_ch0_hi, alsval_ch0;
	int alsval_ch1_lo, alsval_ch1_hi, alsval_ch1;
	int luxdata_int;
	int ratio;

	alsval_ch1_lo = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH1_0);
	alsval_ch1_hi = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH1_1);
	alsval_ch1 = (alsval_ch1_hi * 256) + alsval_ch1_lo;
	APS_DBG("alsval_ch1_lo = %d,alsval_ch1_hi=%d,alsval_ch1=%d\n",alsval_ch1_lo,alsval_ch1_hi,alsval_ch1);

	alsval_ch0_lo = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH0_0);
	alsval_ch0_hi = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH0_1);
	alsval_ch0 = (alsval_ch0_hi * 256) + alsval_ch0_lo;
	APS_DBG("alsval_ch0_lo = %d,alsval_ch0_hi=%d,alsval_ch0=%d\n",alsval_ch0_lo,alsval_ch0_hi,alsval_ch0);

    if((alsval_ch1==0)||(alsval_ch0==0))
    {
        luxdata_int = 0;
        goto out;
    }

	ratio = (alsval_ch1 * 100) / (alsval_ch0 + alsval_ch1);
	APS_DBG("ratio = %d  gainrange = %d\n", ratio, als_gainrange);
	if (ratio < 45) {
		luxdata_int = (((17743 * alsval_ch0) + (11059 * alsval_ch1)) / als_gainrange) / 1000;
	}
	else if ((ratio < 64) && (ratio >= 45)) {
		luxdata_int = (((42785 * alsval_ch0) - (19548 * alsval_ch1)) / als_gainrange) / 1000;
	}
	else if ((ratio < 85) && (ratio >= 64)) {
		luxdata_int = (((5926 * alsval_ch0) + (1185 * alsval_ch1)) / als_gainrange) / 1000;
	}
	else {
		luxdata_int = 0;
	}
	
	APS_DBG("ltr559_als_read: als_value_lux = %d\n", luxdata_int);
out:
	*data = luxdata_int;
	final_lux_val = luxdata_int;
	return luxdata_int;
}
/********************************************************************/
static int ltr559_get_ps_value(struct ltr559_priv *obj, u16 ps)
{
	int val;// , mask = atomic_read(&obj->ps_mask);
	int invalid = 0;

	static int val_temp = 1;
	if((ps > atomic_read(&obj->ps_thd_val_high)))
	{
		val = 0;  /*close*/
		val_temp = 0;
		intr_flag_value = 1;
	}
	else if((ps < atomic_read(&obj->ps_thd_val_low)))
	{
		val = 1;  /*far away*/
		val_temp = 1;
		intr_flag_value = 0;
	}
	else
		val = val_temp;			
	
	if(atomic_read(&obj->ps_suspend))
	{
		invalid = 1;
	}
	else if(1 == atomic_read(&obj->ps_deb_on))
	{
		unsigned long endt = atomic_read(&obj->ps_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->ps_deb_on, 0);
		}
		
		if (1 == atomic_read(&obj->ps_deb_on))
		{
			invalid = 1;
		}
	}
	else if (obj->als > 50000)
	{
		//invalid = 1;
		APS_DBG("ligh too high will result to failt proximiy\n");
		return 1;  /*far away*/
	}

	if(!invalid)
	{
		APS_DBG("PS:  %05d => %05d\n", ps, val);
		return val;
	}	
	else
	{
		return -1;
	}	
}
/********************************************************************/
static int ltr559_get_als_value(struct ltr559_priv *obj, u16 als)
{
	int idx;
	int invalid = 0;
	APS_DBG("als  = %d\n",als); 
	for(idx = 0; idx < obj->als_level_num; idx++)
	{
		if(als < obj->hw->als_level[idx])
		{
			break;
		}
	}
	
	if(idx >= obj->als_value_num)
	{
		APS_ERR("exceed range\n"); 
		idx = obj->als_value_num - 1;
	}
	
	if(1 == atomic_read(&obj->als_deb_on))
	{
		unsigned long endt = atomic_read(&obj->als_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->als_deb_on, 0);
		}
		
		if(1 == atomic_read(&obj->als_deb_on))
		{
			invalid = 1;
		}
	}

	if(!invalid)
	{
		APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);	
		return obj->hw->als_value[idx];
	}
	else
	{
		APS_ERR("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);    
		return -1;
	}
}
/*-------------------------------attribute file for debugging----------------------------------*/

/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t config_show(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	
	res = snprintf(buf, PAGE_SIZE, "(%d %d %d %d %d)\n", 
		atomic_read(&ltr559_obj->i2c_retry), atomic_read(&ltr559_obj->als_debounce), 
		atomic_read(&ltr559_obj->ps_mask), atomic_read(&ltr559_obj->ps_thd_val), atomic_read(&ltr559_obj->ps_debounce));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t config_store(struct device_driver *ddri, const char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, thres;
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	
	if(5 == sscanf(buf, "%d %d %d %d %d", &retry, &als_deb, &mask, &thres, &ps_deb))
	{ 
		atomic_set(&ltr559_obj->i2c_retry, retry);
		atomic_set(&ltr559_obj->als_debounce, als_deb);
		atomic_set(&ltr559_obj->ps_mask, mask);
		atomic_set(&ltr559_obj->ps_thd_val, thres);        
		atomic_set(&ltr559_obj->ps_debounce, ps_deb);
	}
	else
	{
		APS_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t trace_show(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&ltr559_obj->trace));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t trace_store(struct device_driver *ddri, const char *buf, size_t count)
{
    int trace;
    if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&ltr559_obj->trace, trace);
	}
	else 
	{
		APS_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t als_show(struct device_driver *ddri, char *buf)
{
	int res;
		
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	res = ltr559_als_read(ltr559_obj->client, &ltr559_obj->als);
	return snprintf(buf, PAGE_SIZE, "0x%04X(%d)\n", res, res);
	
}
/*----------------------------------------------------------------------------*/
static ssize_t ps_show(struct device_driver *ddri, char *buf)
{
	int  res;
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	res = ltr559_ps_read(ltr559_obj->client, &ltr559_obj->ps);
    return snprintf(buf, PAGE_SIZE, "0x%04X(%d)\n", res, res);
}
/*----------------------------------------------------------------------------*/
static ssize_t reg_show(struct device_driver *ddri, char *buf)
{
	int i,len=0;
	int reg[]={0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,
		0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x97,0x98,0x99,0x9a,0x9e};
	for(i=0;i<27;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "reg:0x%04X value: 0x%04X\n", reg[i],ltr559_i2c_read_reg(reg[i]));	
	}
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t send_show(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t send_store(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr, cmd;
	u8 dat;

	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	else if(2 != sscanf(buf, "%x %x", &addr, &cmd))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	dat = (u8)cmd;
	//****************************
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t recv_show(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t recv_store(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr;
	
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	else if(1 != sscanf(buf, "%x", &addr))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	//****************************
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t status_show(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	
	if(ltr559_obj->hw)
	{	
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n", 
			ltr559_obj->hw->i2c_num, ltr559_obj->hw->power_id, ltr559_obj->hw->power_vol);		
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}


	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&ltr559_obj->als_suspend), atomic_read(&ltr559_obj->ps_suspend));

	return len;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct ltr559_priv *obj, const char* buf, size_t count, u32 data[], int len)
{
	int idx = 0;
	char *cur = (char*)buf, *end = (char*)(buf+count);

	while(idx < len)
	{
		while((cur < end) && IS_SPACE(*cur))
		{
			cur++;        
		}

		if(1 != sscanf(cur, "%d", &data[idx]))
		{
			break;
		}

		idx++; 
		while((cur < end) && !IS_SPACE(*cur))
		{
			cur++;
		}
	}
	return idx;
}
/*----------------------------------------------------------------------------*/
static ssize_t alslv_show(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < ltr559_obj->als_level_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", ltr559_obj->hw->als_level[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t alslv_store(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(ltr559_obj->als_level, ltr559_obj->hw->als_level, sizeof(ltr559_obj->als_level));
	}
	else if(ltr559_obj->als_level_num != read_int_from_buf(ltr559_obj, buf, count, 
			ltr559_obj->hw->als_level, ltr559_obj->als_level_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t alsval_show(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < ltr559_obj->als_value_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", ltr559_obj->hw->als_value[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t alsval_store(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(ltr559_obj->als_value, ltr559_obj->hw->als_value, sizeof(ltr559_obj->als_value));
	}
	else if(ltr559_obj->als_value_num != read_int_from_buf(ltr559_obj, buf, count, 
			ltr559_obj->hw->als_value, ltr559_obj->als_value_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}
/*---------------------------------------------------------------------------------------*/
static DRIVER_ATTR_RO(als);
static DRIVER_ATTR_RO(ps);
static DRIVER_ATTR_RW(config);
static DRIVER_ATTR_RW(alslv);
static DRIVER_ATTR_RW(alsval);
static DRIVER_ATTR_RW(trace);
static DRIVER_ATTR_RO(status);
static DRIVER_ATTR_RW(send);
static DRIVER_ATTR_RW(recv);
static DRIVER_ATTR_RO(reg);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *ltr559_attr_list[] = {
    &driver_attr_als,
    &driver_attr_ps,    
    &driver_attr_trace,        /*trace log*/
    &driver_attr_config,
    &driver_attr_alslv,
    &driver_attr_alsval,
    &driver_attr_status,
    &driver_attr_send,
    &driver_attr_recv,
    &driver_attr_reg,
};

/*----------------------------------------------------------------------------*/
static int ltr559_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(ltr559_attr_list)/sizeof(ltr559_attr_list[0]));

	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, ltr559_attr_list[idx])))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", ltr559_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int ltr559_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(ltr559_attr_list)/sizeof(ltr559_attr_list[0]));
	
	if (!driver)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, ltr559_attr_list[idx]);
	}
	
	return err;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------interrupt functions--------------------------------*/
#ifndef CUSTOM_KERNEL_SENSORHUB
static int ltr559_check_and_clear_intr(struct i2c_client *client) 
{
	int res,intp,intl;
	u8 buffer[2];	
	u8 temp;

	APS_FUN();
		
	//if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1) /*skip if no interrupt*/	
	//	  return 0;
	
	buffer[0] = LTR559_ALS_PS_STATUS;
	res = i2c_master_send(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = i2c_master_recv(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	temp = buffer[0];
	res = 1;
	intp = 0;
	intl = 0;
	if(0 != (buffer[0] & 0x02))
	{
		res = 0;
		intp = 1;
	}
	if(0 != (buffer[0] & 0x08))
	{
		res = 0;
		intl = 1;		
	}
	
	if(0 == res)
	{
		if((1 == intp) && (0 == intl))
		{
			buffer[1] = buffer[0] & 0xfD;			
		}
		else if((0 == intp) && (1 == intl))
		{
			buffer[1] = buffer[0] & 0xf7;
		}
		else
		{
			buffer[1] = buffer[0] & 0xf5;
		}
		buffer[0] = LTR559_ALS_PS_STATUS	;
		res = i2c_master_send(client, buffer, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		else
		{
			res = 0;
		}
	}
	else
		return 0;
	
EXIT_ERR:
	APS_ERR("ltr559_check_and_clear_intr fail\n");
	return 1;
}

static int ltr559_check_intr(struct i2c_client *client) 
{
	int res,intp,intl;
	u8 buffer[2];

	APS_FUN();

	//if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1) /*skip if no interrupt*/  
	//    return 0;

	buffer[0] = LTR559_ALS_PS_STATUS;
	res = i2c_master_send(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = i2c_master_recv(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	
	res = 0;
	intp = 0;
	intl = 0;
	if (0 != (buffer[0] & 0x02))
	{
		res = 1;	//PS int
		intp = 1;
	}
	if (0 != (buffer[0] & 0x08))
	{
		res = 2;	//ALS int
		intl = 1;		
	}
	if ((intp == 1) && (intl == 1))
	{
		res = 4;	//ALS & PS int		
	}

	return res;

EXIT_ERR:
	APS_ERR("ltr559_check_intr fail\n");
	return 0;
}

static int ltr559_clear_intr(struct i2c_client *client) 
{
	int res;
	u8 buffer[2];

	APS_FUN();
	
	buffer[0] = LTR559_ALS_PS_STATUS;
	res = i2c_master_send(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = i2c_master_recv(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	APS_DBG("buffer[0] = %d \n",buffer[0]);
	buffer[1] = buffer[0] & 0xF5;
	buffer[0] = LTR559_ALS_PS_STATUS	;

	res = i2c_master_send(client, buffer, 0x2);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	else
	{
		res = 0;
	}

	return res;

EXIT_ERR:
	APS_ERR("ltr559_clear_intr fail\n");
	return 1;
}
#endif //#ifndef CUSTOM_KERNEL_SENSORHUB

static void ltr559_cali_ps_work(struct work_struct *work)
{
	int err = 0;

#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
	err = ltr559_dynamic_calibrate();
	if (err < 0)
	{
		APS_LOG("ltr559_dynamic_calibrate() failed\n");
	}
#endif
	ltr559_ps_set_thres();
}
/*----------------------------------------------------------------------------*/
static void ltr559_eint_work(struct work_struct *work)
{
	struct ltr559_priv *obj = (struct ltr559_priv *)container_of(work, struct ltr559_priv, eint_work);
	u8 databuf[2];
	int res = 0;
	int err;
	int value = 1;
	
	err = ltr559_check_intr(obj->client);
	if (err < 0) {
		goto EXIT_INTR;
	}
	else
	{
		//get raw data
		obj->ps = ltr559_ps_read(obj->client, &obj->ps);
		if (obj->ps < 0)
		{
			err = -1;
			goto EXIT_INTR;
		}
				
		APS_DBG("ltr559_eint_work: rawdata ps=%d!\n",obj->ps);
		value = ltr559_get_ps_value(obj, obj->ps);
		APS_DBG("intr_flag_value=%d\n",intr_flag_value);
		if(intr_flag_value){
			databuf[0] = LTR559_PS_THRES_LOW_0;	
			databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_INTR;
			}
			databuf[0] = LTR559_PS_THRES_LOW_1;	
			databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_low)) & 0xFF00) >> 8);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_INTR;
			}
			databuf[0] = LTR559_PS_THRES_UP_0;	
			databuf[1] = (u8)(0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_INTR;
			}
			databuf[0] = LTR559_PS_THRES_UP_1; 
			databuf[1] = (u8)((0xFF00) >> 8);;
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_INTR;
			}
		}
		else{	
#ifndef DELAYED_PS_CALI
#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
			if(obj->ps > 20 && obj->ps < (dynamic_calibrate - 50)){ 
        		if(obj->ps < 100){			
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+100);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+50);
        		}else if(obj->ps < 200){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+60);
        		}else if(obj->ps < 300){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+60);
        		}else if(obj->ps < 400){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+60);
        		}else if(obj->ps < 600){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+180);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+90);
        		}else if(obj->ps < 1000){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+300);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+180);	
        		}else if(obj->ps < 1250){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+400);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+300);
        		}
        		else{
        			atomic_set(&obj->ps_thd_val_high,  1400);
        			atomic_set(&obj->ps_thd_val_low, 1000);        			
        		}
        		
        		dynamic_calibrate = obj->ps;
        	}	        
#endif        	
			databuf[0] = LTR559_PS_THRES_LOW_0;	
			databuf[1] = (u8)(0 & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_INTR;
			}
			databuf[0] = LTR559_PS_THRES_LOW_1;	
			databuf[1] = (u8)((0 & 0xFF00) >> 8);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_INTR;
			}
			databuf[0] = LTR559_PS_THRES_UP_0;	
			databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_INTR;
			}
			databuf[0] = LTR559_PS_THRES_UP_1; 
			databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_high)) & 0xFF00) >> 8);;
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_INTR;
			}
#else
			cancel_delayed_work(&ltr559_obj->cali_ps_work);
			schedule_delayed_work(&ltr559_obj->cali_ps_work, msecs_to_jiffies(2000));
#endif
		}
		//let up layer to know
		res = ps_report_interrupt_data(value);
	}

EXIT_INTR:
	ltr559_clear_intr(obj->client);
#ifdef CONFIG_OF
	enable_irq(obj->irq);
#endif
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static void ltr559_eint_func(void)
{
	struct ltr559_priv *obj = ltr559_obj;
	if(!obj)
	{
		return;
	}	
	int_top_time = sched_clock();
	schedule_work(&obj->eint_work);
}

#ifdef CONFIG_OF
static irqreturn_t ltr559_eint_handler(int irq, void *desc)
{
	if (irq_enabled == 2)
	{
		disable_irq_nosync(ltr559_obj->irq);
		ltr559_eint_func();
	}
	
	return IRQ_HANDLED;
}
#endif
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
int ltr559_setup_eint(struct i2c_client *client)
{
	int ret;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_cfg;
	u32 ints[2] = { 0, 0 };	

	APS_FUN();

	/* gpio setting */
	pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		APS_ERR("Cannot find alsps pinctrl!\n");
	}
	
	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		APS_ERR("Cannot find alsps pinctrl pin_cfg!\n");
	}
	
	/* eint request */
	if (ltr559_obj->irq_node) {
		of_property_read_u32_array(ltr559_obj->irq_node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_request(ints[0], "p-sensor");
		gpio_set_debounce(ints[0], ints[1]);
		pinctrl_select_state(pinctrl, pins_cfg);
		APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);

		ltr559_obj->irq = irq_of_parse_and_map(ltr559_obj->irq_node, 0);
		APS_LOG("ltr559_obj->irq = %d\n", ltr559_obj->irq);
		if (!ltr559_obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}
		//APS_ERR("irq to gpio = %d \n", irq_to_gpio(ltr559_obj->irq));
		if (request_irq(ltr559_obj->irq, ltr559_eint_handler, IRQF_TRIGGER_NONE, "ALS-eint", NULL)) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
		enable_irq_wake(ltr559_obj->irq);
		//enable_irq(ltr559_obj->irq);
		irq_enabled = 1;
	}
	else {
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}

	return 0;
}

/*--------------------------------------------------------------------------------*/
static int ltr559_init_client(void)
{
	int res;
	int init_als_gain;
	u8 databuf[2];

	struct i2c_client *client = ltr559_obj->client;

	struct ltr559_priv *obj = ltr559_obj;

	mdelay(PON_DELAY);	

	res = ltr559_i2c_write_reg(LTR559_PS_N_PULSES, 4);
	if (res<0)
	{
		APS_LOG("ltr559 set ps pulse error\n");
		goto EXIT_ERR;
	}

	res = ltr559_i2c_write_reg(LTR559_PS_LED, 0x7F);
	if (res<0)
	{
		APS_LOG("ltr559 set ps led error\n");
		goto EXIT_ERR;
	}

	/*for interrup work mode support */
	if (0 == obj->hw->polling_mode_ps)
	{
		APS_LOG("eint enable");
		
		databuf[0] = LTR559_INTERRUPT;
		databuf[1] = 0x01;
		res = i2c_master_send(client, databuf, 0x2);
		if (res <= 0)
		{
			goto EXIT_ERR;			
		}

		databuf[0] = LTR559_INTERRUPT_PERSIST;
		databuf[1] = 0x20;
		res = i2c_master_send(client, databuf, 0x2);
		if (res <= 0)
		{
			goto EXIT_ERR;
		}
	}

	res = ltr559_ps_enable(client, 1);
	if (res < 0)
	{
		APS_ERR("enable ps fail: %d\n", res);
		goto EXIT_ERR;
	}

	// Enable ALS to Full Range at startup
	als_gainrange = ALS_RANGE_64K;
	init_als_gain = als_gainrange;
	APS_ERR("ALS sensor gainrange %d!\n", init_als_gain);

	switch (init_als_gain)
	{
	case ALS_RANGE_64K:
		res = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_Range1);
		break;

	case ALS_RANGE_32K:
		res = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_Range2);
		break;

	case ALS_RANGE_16K:
		res = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_Range3);
		break;

	case ALS_RANGE_8K:
		res = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_Range4);
		break;

	case ALS_RANGE_1300:
		res = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_Range5);
		break;

	case ALS_RANGE_600:
		res = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_Range6);
		break;

	default:
		res = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_Range1);
		break;
	}
	
	res = ltr559_als_enable(client, 1);
	if (res < 0)
	{
		APS_ERR("enable als fail: %d\n", res);
		goto EXIT_ERR;
	}

	if ((res = ltr559_setup_eint(client)) != 0)
	{
		APS_ERR("setup eint: %d\n", res);
		goto EXIT_ERR;
	}

	if ((res = ltr559_check_and_clear_intr(client)))
	{
		APS_ERR("check/clear intr: %d\n", res);
		goto EXIT_ERR;
	}

	return 0;

EXIT_ERR:
	APS_ERR("init dev: %d\n", res);
	return 1;
}
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
// if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL
static int als_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}

// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL
static int als_enable_nodata(int en)
{
	int res = 0;
	APS_LOG("ltr559_obj als enable value = %d\n", en);

	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return -1;
	}

	mutex_lock(&ltr559_mutex);
	if (en)
		set_bit(CMC_BIT_ALS, &ltr559_obj->enable);
	else
		clear_bit(CMC_BIT_ALS, &ltr559_obj->enable);
	mutex_unlock(&ltr559_mutex);
#if 0	
	if((en == 0) && test_bit(CMC_BIT_PS, &obj->enable))
		return 0;
#endif
	res = ltr559_als_enable(ltr559_obj->client, en);
	if (res) {
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}
	return 0;
}

static int als_set_delay(u64 ns)
{
	// Do nothing
	return 0;
}

static int als_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return als_set_delay(samplingPeriodNs);
}

static int als_flush(void)
{
	return als_flush_report();
}

static int als_get_data(int* value, int* status)
{
	int err = 0;
	
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return -1;
	}

	ltr559_obj->als = ltr559_als_read(ltr559_obj->client, &ltr559_obj->als);
	if (ltr559_obj->als < 0)
		err = -1;
	else {
		*value = ltr559_get_als_value(ltr559_obj, ltr559_obj->als);
		if (*value < 0)
			err = -1;
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}

	return err;
}

// if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL
static int ps_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}

// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL
static int ps_enable_nodata(int en)
{
	int res = 0;
	APS_LOG("ltr559_obj ps enable value = %d\n", en);

	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return -1;
	}
	
	mutex_lock(&ltr559_mutex);
	if (en)
		set_bit(CMC_BIT_PS, &ltr559_obj->enable);
	else
		clear_bit(CMC_BIT_PS, &ltr559_obj->enable);
	mutex_unlock(&ltr559_mutex);

	res = ltr559_ps_enable(ltr559_obj->client, en);
	if (res < 0) {
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}
	return 0;
}

static int ps_set_delay(u64 ns)
{
	// Do nothing
	return 0;
}

static int ps_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return 0;
}

static int ps_flush(void)
{
	return ps_flush_report();
}

static int ps_get_data(int* value, int* status)
{
    int err = 0;

    if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return -1;
	}
    
	ltr559_obj->ps = ltr559_ps_read(ltr559_obj->client, &ltr559_obj->ps);
	if (ltr559_obj->ps < 0)
		err = -1;
	else {
		*value = ltr559_get_ps_value(ltr559_obj, ltr559_obj->ps);
		if (*value < 0)
			err = -1;
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}
    
	return err;
}
/*-----------------------------------------------------------------------------------*/
static int ltr559_als_factory_enable_sensor(bool enable_disable,
					     int64_t sample_periods_ms)
{
	int err = 0;

	err = als_enable_nodata(enable_disable ? 1 : 0);
	if (err) {
		APS_ERR("%s:%s failed\n", __func__,
			   enable_disable ? "enable" : "disable");
		return -1;
	}
	err = als_batch(0, sample_periods_ms * 1000000, 0);
	if (err) {
		APS_ERR("%s set_batch failed\n", __func__);
		return -1;
	}
	return 0;
}
static int ltr559_als_factory_get_data(int32_t *data)
{
	int status;

	return als_get_data(data, &status);
}
static int ltr559_als_factory_get_raw_data(int32_t *data)
{
	int err = 0;
	struct ltr559_priv *obj = ltr559_obj;

	if (!obj) {
		APS_ERR("obj is null!!\n");
		return -1;
	}

	err = ltr559_als_read(obj->client, &obj->als);
	if (err < 0) {
		APS_ERR("%s failed\n", __func__);
		return -1;
	}
	*data = ltr559_obj->als;

	return 0;
}
static int ltr559_als_factory_enable_calibration(void)
{
	return 0;
}
static int ltr559_als_factory_clear_cali(void)
{
	return 0;
}
static int ltr559_als_factory_set_cali(int32_t offset)
{
	return 0;
}
static int ltr559_als_factory_get_cali(int32_t *offset)
{
	return 0;
}
static int ltr559_ps_factory_enable_sensor(bool enable_disable,
					    int64_t sample_periods_ms)
{
	int err = 0;

	err = ps_enable_nodata(enable_disable ? 1 : 0);
	if (err) {
		APS_ERR("%s:%s failed\n", __func__,
			   enable_disable ? "enable" : "disable");
		return -1;
	}
	err = ps_batch(0, sample_periods_ms * 1000000, 0);
	if (err) {
		APS_ERR("%s set_batch failed\n", __func__);
		return -1;
	}
	return err;
}
static int ltr559_ps_factory_get_data(int32_t *data)
{
	int err = 0, status = 0;

	err = ps_get_data(data, &status);
	if (err < 0)
		return -1;
	return 0;
}
static int ltr559_ps_factory_get_raw_data(int32_t *data)
{
	int err = 0;
	struct ltr559_priv *obj = ltr559_obj;

	err = ltr559_ps_read(obj->client, &obj->ps);
	if (err < 0) {
		APS_ERR("%s failed\n", __func__);
		return -1;
	}
	*data = ltr559_obj->ps;
	return 0;
}
static int ltr559_ps_factory_enable_calibration(void)
{
	return 0;
}
static int ltr559_ps_factory_clear_cali(void)
{
	struct ltr559_priv *obj = ltr559_obj;

	obj->ps_cali = 0;
	return 0;
}
static int ltr559_ps_factory_set_cali(int32_t offset)
{
	struct ltr559_priv *obj = ltr559_obj;

	obj->ps_cali = offset;
	return 0;
}
static int ltr559_ps_factory_get_cali(int32_t *offset)
{
	struct ltr559_priv *obj = ltr559_obj;

	*offset = obj->ps_cali;
	return 0;
}
static int ltr559_ps_factory_set_threshold(int32_t threshold[2])
{
	int err = 0;
	struct ltr559_priv *obj = ltr559_obj;

	APS_LOG("%s set threshold high: 0x%x, low: 0x%x\n", __func__,
		 threshold[0], threshold[1]);
	atomic_set(&obj->ps_thd_val_high, (threshold[0] + obj->ps_cali));
	atomic_set(&obj->ps_thd_val_low, (threshold[1] + obj->ps_cali));
	err = ltr559_ps_set_thres();

	if (err <= 0) {
		APS_ERR("set_psensor_threshold fail\n");
		return -1;
	}
	return 0;
}
static int ltr559_ps_factory_get_threshold(int32_t threshold[2])
{
	struct ltr559_priv *obj = ltr559_obj;

	threshold[0] = atomic_read(&obj->ps_thd_val_high) - obj->ps_cali;
	threshold[1] = atomic_read(&obj->ps_thd_val_low) - obj->ps_cali;
	return 0;
}

static struct alsps_factory_fops ltr559_factory_fops = {
	.als_enable_sensor = ltr559_als_factory_enable_sensor,
	.als_get_data = ltr559_als_factory_get_data,
	.als_get_raw_data = ltr559_als_factory_get_raw_data,
	.als_enable_calibration = ltr559_als_factory_enable_calibration,
	.als_clear_cali = ltr559_als_factory_clear_cali,
	.als_set_cali = ltr559_als_factory_set_cali,
	.als_get_cali = ltr559_als_factory_get_cali,

	.ps_enable_sensor = ltr559_ps_factory_enable_sensor,
	.ps_get_data = ltr559_ps_factory_get_data,
	.ps_get_raw_data = ltr559_ps_factory_get_raw_data,
	.ps_enable_calibration = ltr559_ps_factory_enable_calibration,
	.ps_clear_cali = ltr559_ps_factory_clear_cali,
	.ps_set_cali = ltr559_ps_factory_set_cali,
	.ps_get_cali = ltr559_ps_factory_get_cali,
	.ps_set_threshold = ltr559_ps_factory_set_threshold,
	.ps_get_threshold = ltr559_ps_factory_get_threshold,
};

static struct alsps_factory_public ltr559_factory_device = {
	.gain = 1,
	.sensitivity = 1,
	.fops = &ltr559_factory_fops,
};

/*-----------------------------------i2c operations----------------------------------*/
static int ltr559_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ltr559_priv *obj = NULL;
	struct als_control_path als_ctl={0};
	struct als_data_path als_data={0};
	struct ps_control_path ps_ctl={0};
	struct ps_data_path ps_data={0};
	int err = 0;

	APS_FUN();

	err = get_alsps_dts_func(client->dev.of_node, hw);
	if (err < 0) {
		APS_ERR("get customization info from dts failed\n");
		return -EFAULT;
	}
		
	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	ltr559_obj = obj;
	
	obj->hw = hw;
	INIT_WORK(&obj->eint_work, ltr559_eint_work);
	INIT_DELAYED_WORK(&obj->cali_ps_work, ltr559_cali_ps_work);

	client->addr = 0x23;
	obj->client = client;
	i2c_set_clientdata(client, obj);	
	
	/*-----------------------------value need to be confirmed-----------------------------------------*/
	atomic_set(&obj->als_debounce, 300);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 300);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	//atomic_set(&obj->als_cmd_val, 0xDF);
	//atomic_set(&obj->ps_cmd_val,  0xC1);
	atomic_set(&obj->ps_thd_val_high,  obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_low,  obj->hw->ps_threshold_low);
	atomic_set(&obj->ps_thd_val,  obj->hw->ps_threshold);
	atomic_set(&obj->als_thd_val_high,  obj->hw->als_threshold_high);
	atomic_set(&obj->als_thd_val_low,  obj->hw->als_threshold_low);
	obj->irq_node = client->dev.of_node;
	
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->ps_cali = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);
	obj->als_modulus = (400*100)/(16*150);//(1/Gain)*(400/Tine), this value is fix after init ATIME and CONTROL register value
										//(400)/16*2.72 here is amplify *100	
	/*-----------------------------value need to be confirmed-----------------------------------------*/
	
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	set_bit(CMC_BIT_ALS, &obj->enable);
	set_bit(CMC_BIT_PS, &obj->enable);

	APS_LOG("ltr559_init_client() start...!\n");
	ltr559_i2c_client = client;
	err = ltr559_init_client();
	if(err)
	{
		goto exit_init_failed;
	}
	APS_LOG("ltr559_init_client() OK!\n");
	
	//err = misc_register(&ltr559_device);
	err = alsps_factory_device_register(&ltr559_factory_device);
	if(err)
	{
		APS_ERR("ltr559_factory_device register failed\n");
		goto exit_factory_device_register_failed;
	}

    als_ctl.is_use_common_factory =false;
	ps_ctl.is_use_common_factory = false;
	
	/*------------------------ltr559 attribute file for debug--------------------------------------*/
	//err = ltr559_create_attr(&(ltr559_init_info.platform_diver_addr->driver));
	err = ltr559_create_attr(&(ltr559_i2c_driver.driver));
	if(err)
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
	/*------------------------ltr559 attribute file for debug--------------------------------------*/
	
	als_ctl.open_report_data= als_open_report_data;
	als_ctl.enable_nodata = als_enable_nodata;
	als_ctl.set_delay  = als_set_delay;
	als_ctl.batch = als_batch;
	als_ctl.flush = als_flush;
	als_ctl.is_report_input_direct = false;
	als_ctl.is_support_batch = false;
 	
	err = als_register_control_path(&als_ctl);
	if(err)
	{
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	als_data.get_data = als_get_data;
	als_data.vender_div = 100;
	err = als_register_data_path(&als_data);	
	if(err)
	{
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}
	
	ps_ctl.open_report_data= ps_open_report_data;
	ps_ctl.enable_nodata = ps_enable_nodata;
	ps_ctl.set_delay  = ps_set_delay;
	ps_ctl.batch = ps_batch;
	ps_ctl.flush = ps_flush;
	ps_ctl.is_report_input_direct = false;
	ps_ctl.is_support_batch = false;
	ps_ctl.is_polling_mode = hw->polling_mode_ps;
		
	err = ps_register_control_path(&ps_ctl);
	if(err)
	{
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	ps_data.get_data = ps_get_data;
	ps_data.vender_div = 100;
	err = ps_register_data_path(&ps_data);	
	if(err)
	{
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}	

	ltr559_init_flag =0;
	APS_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
exit_sensor_obj_attach_fail:
	alsps_factory_device_deregister(&ltr559_factory_device);
exit_factory_device_register_failed:
exit_init_failed:
	kfree(obj);
exit:
	ltr559_i2c_client = NULL;           
	APS_ERR("%s: err = %d\n", __func__, err);
	ltr559_init_flag =-1;
	return err;
}

static int ltr559_i2c_remove(struct i2c_client *client)
{
	int err;

	//err = ltr579_delete_attr(&(ltr579_init_info.platform_diver_addr->driver));
	err = ltr559_delete_attr(&(ltr559_i2c_driver.driver));
	if(err)
	{
		APS_ERR("ltr559_delete_attr fail: %d\n", err);
	}

	alsps_factory_device_deregister(&ltr559_factory_device);
		
	ltr559_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}

static int ltr559_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, LTR559_DEV_NAME);
	return 0;
}

static int ltr559_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ltr559_priv *obj = i2c_get_clientdata(client);    
	int err;
	APS_FUN();    

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
		
	atomic_set(&obj->als_suspend, 1);
	err = ltr559_als_enable(obj->client, 0);
	if(err < 0)
	{
		APS_ERR("disable als: %d\n", err);
		return err;
	}

	atomic_set(&obj->ps_suspend, 1);
	err = ltr559_ps_enable(obj->client, 0);
	if(err < 0)
	{
		APS_ERR("disable ps:  %d\n", err);
		return err;
	}
		
	ltr559_power(obj->hw, 0);
	
	return 0;
}

static int ltr559_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ltr559_priv *obj = i2c_get_clientdata(client);        
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	ltr559_power(obj->hw, 1);

	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
		err = ltr559_als_enable(obj->client, 1);
	    if (err < 0)
		{
			APS_ERR("enable als fail: %d\n", err);        
		}
	}
	atomic_set(&obj->ps_suspend, 0);
	if(test_bit(CMC_BIT_PS,  &obj->enable))
	{
		err = ltr559_ps_enable(obj->client, 1);
	    if (err < 0)
		{
			APS_ERR("enable ps fail: %d\n", err);                
		}
	}

	return 0;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int ltr559_remove(void)
{
	APS_FUN();

	ltr559_power(hw, 0);	
	i2c_del_driver(&ltr559_i2c_driver);

	return 0;
}
/*----------------------------------------------------------------------------*/
static int  ltr559_local_init(void)
{
	APS_FUN();

	ltr559_power(hw, 1);
	
	if(i2c_add_driver(&ltr559_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	}

	if(-1 == ltr559_init_flag)
	{
	   return -1;
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int __init ltr559_init(void)
{    
	alsps_driver_add(&ltr559_init_info);
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit ltr559_exit(void)
{
	APS_FUN();
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
module_init(ltr559_init);
module_exit(ltr559_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Liteon");
MODULE_DESCRIPTION("LTR-559ALSPS Driver");
MODULE_LICENSE("GPL");

