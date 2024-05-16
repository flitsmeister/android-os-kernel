/******************** (C) COPYRIGHT 2016 MEMSIC ********************
 *
 * File Name          : mxc6655.c
 * Description        : MXC6655 accelerometer sensor API
 *
 *******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
 * PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
 * AS A RESULT, MEMSIC SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH MEMSIC PARTS.
 *

 ******************************************************************************/


#include "sensors_io.h"
#include "accel.h"
#include "cust_acc.h"
#include "mxc6655.h"
#ifdef XUNHU_LPS_TEKHW_SUPPORT
#include <teksunhw.h>
#endif

#ifdef CONFIG_MID_ITEMS_SUPPORT
#include <mt-plat/items.h>
#endif


#define I2C_DRIVERID_MXC6655		120
#define SW_CALIBRATION
#define DRIVER_VERSION				"1.00.20132"

#define GSE_DEBUG_ON          		0
#define GSE_DEBUG_FUNC_ON     		0
/* Log define */
#define GSE_INFO(fmt, arg...)      	pr_warn("<<-GSE INFO->> "fmt"\n", ##arg)
#define GSE_ERR(fmt, arg...)          	pr_err("<<-GSE ERROR->> "fmt"\n", ##arg)
#define GSE_DEBUG(fmt, arg...)		do {\
						if (GSE_DEBUG_ON)\
							pr_warn("<<-GSE DEBUG->> [%d]"fmt"\n", __LINE__, ##arg);\
					} while (0)
#define GSE_DEBUG_FUNC()		do {\
						if (GSE_DEBUG_FUNC_ON)\
							pr_debug("<<-GSE FUNC->> Func:%s@Line:%d\n", __func__, __LINE__);\
					} while (0)

#define DRIVER_ATTR(_name, _mode, _show, _store) \
  	struct driver_attribute driver_attr_##_name = __ATTR(_name, _mode, \
  			_show, _store)

#define MXC6655_AXIS_X          	0
#define MXC6655_AXIS_Y          	1
#define MXC6655_AXIS_Z          	2
#define MXC6655_AXES_NUM        	3
#define MXC6655_DATA_LEN        	6
#define C_MAX_FIR_LENGTH 		(32)
#define  USE_DELAY
#define MXC6655_BUF_SIZE                256

#define GSENSOR_IOCTL_READ_OFFSET	\
	_IOR(GSENSOR, 0x04, struct GSENSOR_VECTOR3D)
#define GSENSOR_IOCTL_READ_GAIN	\
	_IOR(GSENSOR, 0x05, struct GSENSOR_VECTOR3D)

static s16 cali_sensor_data;
static struct acc_hw accel_cust;
static struct acc_hw *hw = &accel_cust;
#ifdef XUNHU_LPS_TEKHW_SUPPORT
extern int accel_compatible_flag;
#endif
#ifdef USE_DELAY
static int delay_state = 0;
#endif

static const struct i2c_device_id mxc6655_i2c_id[] = { { MXC6655_DEV_NAME, 0 }, { }, };
static int mxc6655_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int mxc6655_i2c_remove(struct i2c_client *client);

static int  mxc6655_local_init(void);
static int mxc6655_remove(void);
static int mxc6655_factory_set_cali(int32_t data[3]);

typedef enum {
	ADX_TRC_FILTER   = 0x01,
	ADX_TRC_RAWDATA  = 0x02,
	ADX_TRC_IOCTL	 = 0x04,
	ADX_TRC_CALI	 = 0X08,
	ADX_TRC_INFO	 = 0X10,
} ADX_TRC;

struct scale_factor{
	u8  whole;
	u8  fraction;
};

struct data_resolution {
	struct scale_factor scalefactor;
	int    sensitivity;
};


struct data_filter {
	s16 raw[C_MAX_FIR_LENGTH][MXC6655_AXES_NUM];
	int sum[MXC6655_AXES_NUM];
	int num;
	int idx;
};
static int mxc6655_init_flag = -1;
/*----------------------------------------------------------------------------*/
static struct acc_init_info mxc6655_init_info = {
		.name = "mxc6655",
		.init = mxc6655_local_init,
		.uninit = mxc6655_remove,
};
struct mxc6655_i2c_data {
		 struct i2c_client *client;
		 struct acc_hw *hw;
		 struct hwmsen_convert	 cvt;
		 atomic_t layout;
		 /*misc*/
		 struct data_resolution *reso;
		 atomic_t				 trace;
		 atomic_t				 suspend;
		 atomic_t				 selftest;
		 atomic_t				 filter;
		 s16					 cali_sw[MXC6655_AXES_NUM+1];

		 /*data*/
		 s8 					 offset[MXC6655_AXES_NUM+1];  /*+1: for 4-byte alignment*/
		 s16					 data[MXC6655_AXES_NUM+1];
		 bool                    first_boot;
#if defined(CONFIG_MXC6655_LOWPASS)
		 atomic_t				 firlen;
		 atomic_t				 fir_en;
		 struct data_filter 	 fir;
#endif
};

#ifdef CONFIG_OF
static const struct of_device_id accel_of_match[] = {
	{.compatible = "mediatek,gsensor_mxc6655"},
	{.compatible = "mediatek,gsensor"},
	{},
};
#endif

static struct i2c_driver mxc6655_i2c_driver = {
	 .driver = {
		// .owner		   = THIS_MODULE,
		 .name			 = MXC6655_DEV_NAME,
	   #ifdef CONFIG_OF
		.of_match_table = accel_of_match,
	   #endif
	 },
	 .probe 			 = mxc6655_i2c_probe,
	 .remove			 = mxc6655_i2c_remove,
	 .id_table = mxc6655_i2c_id,
};



struct i2c_client      			*mxc6655_i2c_client = NULL;
static struct mxc6655_i2c_data 		*obj_i2c_data = NULL;
static bool sensor_power = false;
static struct GSENSOR_VECTOR3D	gsensor_gain;
static struct mutex mxc6655_mutex;
static bool enable_status = false;

static struct data_resolution mxc6655_data_resolution[] = {
    {{ 0, 9}, 1024},   /*+/-2g  in 12-bit resolution:  0.9 mg/LSB*/
    {{ 1, 9}, 512},   /*+/-4g  in 12-bit resolution:  1.9 mg/LSB*/
    {{ 3, 9},  256},   /*+/-8g  in 12-bit resolution: 3.9 mg/LSB*/
};
static struct data_resolution mxc6655_offset_resolution = {{3, 9}, 256};

static int mxc6655_i2c_read_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
	u8 beg = addr;
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,	.flags = 0,
			.len = 1,	.buf = &beg
		},
		{
			.addr = client->addr,	.flags = I2C_M_RD,
			.len = len,	.buf = data,
		}
	};
	int err;

	if (!client)
		return -EINVAL;
	else if (len > C_I2C_FIFO_SIZE) {
		GSE_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	err = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
	if (err != 2) {
		GSE_ERR("i2c_transfer error: (%d %p %d) %d\n",
				addr, data, len, err);
		err = -EIO;
	} else {
		err = 0;
	}
	return err;

}
static void mxc6655_power(struct acc_hw *hw, unsigned int on)
{
	 static unsigned int power_on = 0;

	 power_on = on;
}

static int MXC6655_SetDataResolution(struct mxc6655_i2c_data *obj)
{
 	obj->reso = &mxc6655_data_resolution[2];
	return MXC6655_SUCCESS;
}
static int MXC6655_ReadData(struct i2c_client *client, s16 data[MXC6655_AXES_NUM])
{
#ifdef CONFIG_MXC6655_LOWPASS
	mxc6655_i2c_data *priv = i2c_get_clientdata(client);
#endif
	u8 addr = MXC6655_REG_X;
	u8 buf[MXC6655_DATA_LEN] = {0};
	int err = 0;

#ifdef USE_DELAY
	if(delay_state)
	{
	    GSE_INFO("sleep 300ms\n");
		msleep(300);
		delay_state = 0;
	}
#endif

	if(NULL == client)
	{
        	GSE_ERR("client is null\n");
		err = -EINVAL;
	}
	if((err = mxc6655_i2c_read_block(client, addr, buf, MXC6655_DATA_LEN))!=0)
	{
		GSE_ERR("error: %d\n", err);
	}
	else
	{
		data[MXC6655_AXIS_X] = (s16)(buf[0] << 8 | buf[1]) >> 4;
		data[MXC6655_AXIS_Y] = (s16)(buf[2] << 8 | buf[3]) >> 4;
		data[MXC6655_AXIS_Z] = (s16)(buf[4] << 8 | buf[5]) >> 4;
		//GSE_ERR("reg data x = %d %d y = %d %d z = %d %d\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
               // GSE_ERR("gsensor raw  x = %d, y = %d, z = %d\n", data[MXC6655_AXIS_X], data[MXC6655_AXIS_Y],data[MXC6655_AXIS_Z]);

#ifdef CONFIG_MXC6655_LOWPASS
		if(atomic_read(&priv->filter))
		{
			if(atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend))
			{
				int idx, firlen = atomic_read(&priv->firlen);
				if(priv->fir.num < firlen)
				{
					priv->fir.raw[priv->fir.num][MXC6655_AXIS_X] = data[MXC6655_AXIS_X];
					priv->fir.raw[priv->fir.num][MXC6655_AXIS_Y] = data[MXC6655_AXIS_Y];
					priv->fir.raw[priv->fir.num][MXC6655_AXIS_Z] = data[MXC6655_AXIS_Z];
					priv->fir.sum[MXC6655_AXIS_X] += data[MXC6655_AXIS_X];
					priv->fir.sum[MXC6655_AXIS_Y] += data[MXC6655_AXIS_Y];
					priv->fir.sum[MXC6655_AXIS_Z] += data[MXC6655_AXIS_Z];
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_DEBUG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n", priv->fir.num,
							priv->fir.raw[priv->fir.num][MXC6655_AXIS_X], priv->fir.raw[priv->fir.num][MXC6655_AXIS_Y], priv->fir.raw[priv->fir.num][MXC6655_AXIS_Z],
							priv->fir.sum[MXC6655_AXIS_X], priv->fir.sum[MXC6655_AXIS_Y], priv->fir.sum[MXC6655_AXIS_Z]);
					}
					priv->fir.num++;
					priv->fir.idx++;
				}
				else
				{
					idx = priv->fir.idx % firlen;
					priv->fir.sum[MXC6655_AXIS_X] -= priv->fir.raw[idx][MXC6655_AXIS_X];
					priv->fir.sum[MXC6655_AXIS_Y] -= priv->fir.raw[idx][MXC6655_AXIS_Y];
					priv->fir.sum[MXC6655_AXIS_Z] -= priv->fir.raw[idx][MXC6655_AXIS_Z];
					priv->fir.raw[idx][MXC6655_AXIS_X] = data[MXC6655_AXIS_X];
					priv->fir.raw[idx][MXC6655_AXIS_Y] = data[MXC6655_AXIS_Y];
					priv->fir.raw[idx][MXC6655_AXIS_Z] = data[MXC6655_AXIS_Z];
					priv->fir.sum[MXC6655_AXIS_X] += data[MXC6655_AXIS_X];
					priv->fir.sum[MXC6655_AXIS_Y] += data[MXC6655_AXIS_Y];
					priv->fir.sum[MXC6655_AXIS_Z] += data[MXC6655_AXIS_Z];
					priv->fir.idx++;
					data[MXC6655_AXIS_X] = priv->fir.sum[MXC6655_AXIS_X]/firlen;
					data[MXC6655_AXIS_Y] = priv->fir.sum[MXC6655_AXIS_Y]/firlen;
					data[MXC6655_AXIS_Z] = priv->fir.sum[MXC6655_AXIS_Z]/firlen;
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_DEBUG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d] : [%5d %5d %5d]\n", idx,
						priv->fir.raw[idx][MXC6655_AXIS_X], priv->fir.raw[idx][MXC6655_AXIS_Y], priv->fir.raw[idx][MXC6655_AXIS_Z],
						priv->fir.sum[MXC6655_AXIS_X], priv->fir.sum[MXC6655_AXIS_Y], priv->fir.sum[MXC6655_AXIS_Z],
						data[MXC6655_AXIS_X], data[MXC6655_AXIS_Y], data[MXC6655_AXIS_Z]);
					}
				}
			}
		}
#endif
	}
	return err;
}

/*
static int MXC6655_ReadOffset(struct i2c_client *client, s8 ofs[MXC6655_AXES_NUM])
{
	int err;
	err = 0;
#ifdef SW_CALIBRATION
	ofs[0]=ofs[1]=ofs[2]=0x0;
#endif
	return err;
}
*/

static int MXC6655_ResetCalibration(struct i2c_client *client)
{
	struct mxc6655_i2c_data *obj = i2c_get_clientdata(client);

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int MXC6655_ReadCalibration(struct i2c_client *client, int dat[MXC6655_AXES_NUM])
{
	struct mxc6655_i2c_data *obj = i2c_get_clientdata(client);

	dat[obj->cvt.map[MXC6655_AXIS_X]] = obj->cvt.sign[MXC6655_AXIS_X] * obj->cali_sw[MXC6655_AXIS_X];
	dat[obj->cvt.map[MXC6655_AXIS_Y]] = obj->cvt.sign[MXC6655_AXIS_Y] * obj->cali_sw[MXC6655_AXIS_Y];
	dat[obj->cvt.map[MXC6655_AXIS_Z]] = obj->cvt.sign[MXC6655_AXIS_Z] * obj->cali_sw[MXC6655_AXIS_Z];

	return 0;
}

static int MXC6655_WriteCalibration(struct i2c_client *client, int dat[MXC6655_AXES_NUM])
{
	struct mxc6655_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	/* int cali[MXC6655_AXES_NUM]; */

	if (!obj || !dat) {
		err = -EINVAL;
	} else {
		s16 cali[MXC6655_AXES_NUM];

		cali[obj->cvt.map[MXC6655_AXIS_X]] = obj->cvt.sign[MXC6655_AXIS_X] * obj->cali_sw[MXC6655_AXIS_X];
		cali[obj->cvt.map[MXC6655_AXIS_Y]] = obj->cvt.sign[MXC6655_AXIS_Y] * obj->cali_sw[MXC6655_AXIS_Y];
		cali[obj->cvt.map[MXC6655_AXIS_Z]] = obj->cvt.sign[MXC6655_AXIS_Z] * obj->cali_sw[MXC6655_AXIS_Z];
		cali[MXC6655_AXIS_X] += dat[MXC6655_AXIS_X];
		cali[MXC6655_AXIS_Y] += dat[MXC6655_AXIS_Y];
		cali[MXC6655_AXIS_Z] += dat[MXC6655_AXIS_Z];

		obj->cali_sw[MXC6655_AXIS_X] += obj->cvt.sign[MXC6655_AXIS_X] * dat[obj->cvt.map[MXC6655_AXIS_X]];
		obj->cali_sw[MXC6655_AXIS_Y] += obj->cvt.sign[MXC6655_AXIS_Y] * dat[obj->cvt.map[MXC6655_AXIS_Y]];
		obj->cali_sw[MXC6655_AXIS_Z] += 0;//obj->cvt.sign[MXC6655_AXIS_Z] * dat[obj->cvt.map[MXC6655_AXIS_Z]];
	}

	return err;
}
static int MXC6655_CheckDeviceID(struct i2c_client *client)
{
	u8 databuf[10];
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);
	databuf[0] = MXC6655_REG_CHIP_ID;
	msleep(12);
	res = mxc6655_i2c_read_block(client,MXC6655_REG_CHIP_ID,databuf,0x01);
	if (res)
	{
		GSE_ERR("MXC6655 Device ID read faild\n");
		return MXC6655_ERR_I2C;
	}

       GSE_ERR("MXC6655 Device ID before is %x\n", databuf[0]);
	databuf[0]= (databuf[0]&0x3f);
       GSE_ERR("MXC6655 Device ID is %x\n", databuf[0]);

	if((databuf[0]!= MXC6655_ID_1) && (databuf[0] != MXC6655_ID_2))
	{
		return MXC6655_ERR_IDENTIFICATION;
	}

	GSE_INFO("MXC6655_CheckDeviceID %d done!\n ", databuf[0]);

	return MXC6655_SUCCESS;
}


static int MXC6655_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2] = {0};
	int res = 0, i=0;

	if(enable == 1)
	{
		databuf[1] = MXC6655_AWAKE;
	}
	else
	{
		databuf[1] = MXC6655_SLEEP;
	}
	msleep(MXC6655_STABLE_DELAY);
	databuf[0] = MXC6655_REG_CTRL;
	while (i++ < 3)
	{
		res = i2c_master_send(client, databuf, 0x2);
		msleep(5);
		if(res > 0)
			break;
	}

	if(res <= 0)
	{
		GSE_ERR("memsic set power mode failed!\n");
		return MXC6655_ERR_I2C;
	}
	sensor_power = enable;
#ifdef USE_DELAY
	delay_state = enable;
#else
	msleep(300);
#endif
	return MXC6655_SUCCESS;
}
static int MXC6655_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
	struct mxc6655_i2c_data *obj = i2c_get_clientdata(client);
	u8 databuf[10];
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);
	databuf[0] = MXC6655_REG_CTRL;
	databuf[1] = MXC6655_RANGE_8G;

	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("set power mode failed!\n");
		return MXC6655_ERR_I2C;
	}

	return MXC6655_SetDataResolution(obj);
}
static int MXC6655_SetBWRate(struct i2c_client *client, u8 bwrate)
{
	u8 databuf[10];

	memset(databuf, 0, sizeof(u8)*10);

	return MXC6655_SUCCESS;
}

static int mxc6655_init_client(struct i2c_client *client, int reset_cali)
{
	 struct mxc6655_i2c_data *obj = i2c_get_clientdata(client);
	 int res = 0;

	 GSE_DEBUG_FUNC();
	 res = MXC6655_SetPowerMode(client, true);
	 if(res != MXC6655_SUCCESS)
	 {
		return res;
	 }
	 res = MXC6655_CheckDeviceID(client);
	 if(res != MXC6655_SUCCESS)
	 {
	 	 GSE_ERR("MXC6655 check device id failed\n");
	 	 return res;
	 }

	res = MXC6655_SetBWRate(client, MXC6655_BW_50HZ);
	if(res != MXC6655_SUCCESS )
	{
		GSE_ERR("MXC6655 Set BWRate failed\n");
		return res;
	}

	res = MXC6655_SetDataFormat(client, MXC6655_RANGE_8G);
	if(res != MXC6655_SUCCESS)
	{
		return res;
	}

	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;

	if(0 != reset_cali)
	{
	 	/*reset calibration only in power on*/
		 res = MXC6655_ResetCalibration(client);
		 if(res != MXC6655_SUCCESS)
		 {
			 return res;
		 }
	}
	GSE_INFO("mxc6655_init_client OK!\n");
#ifdef CONFIG_MXC6655_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));
#endif
	MXC6655_SetPowerMode(client, 0);
	msleep(20);

	return MXC6655_SUCCESS;
}

static int MXC6655_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
	u8 databuf[10];

	memset(databuf, 0, sizeof(u8)*10);

	if((NULL == buf)||(bufsize<=30))
	{
		return -1;
	}

	if(NULL == client)
	{
		*buf = 0;
		return -2;
	}

	sprintf(buf, "MXC6655 Chip");
	return 0;
}

static int MXC6655_ReadSensorDataFactory(struct i2c_client *client, char *buf, int bufsize)
{
	struct mxc6655_i2c_data *obj = (struct mxc6655_i2c_data*)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[MXC6655_AXES_NUM] = {0};
	int res = 0;

	GSE_DEBUG_FUNC();
	memset(databuf, 0, sizeof(u8)*10);

	if(NULL == buf)
	{
		GSE_ERR("mxc6655 buf is null !!!\n");
		return -1;
	}
	if(NULL == client)
	{
		*buf = 0;
		GSE_ERR("mxc6655 client is null !!!\n");
		return MXC6655_ERR_STATUS;
	}

	if (atomic_read(&obj->suspend)&& !enable_status)
	{
		GSE_ERR("mxc6655 sensor in suspend read not data!\n");
		return MXC6655_ERR_GETGSENSORDATA;
	}

	if((res = MXC6655_ReadData(client, obj->data)))
	{
		GSE_ERR("mxc6655 I2C error: ret value=%d", res);
		return MXC6655_ERR_I2C;
	}
	else
	{
		obj->data[MXC6655_AXIS_X] += obj->cali_sw[MXC6655_AXIS_X];
		obj->data[MXC6655_AXIS_Y] += obj->cali_sw[MXC6655_AXIS_Y];
		obj->data[MXC6655_AXIS_Z] += obj->cali_sw[MXC6655_AXIS_Z];

		/*remap coordinate*/
		acc[obj->cvt.map[MXC6655_AXIS_X]] = obj->cvt.sign[MXC6655_AXIS_X]*obj->data[MXC6655_AXIS_X];
		acc[obj->cvt.map[MXC6655_AXIS_Y]] = obj->cvt.sign[MXC6655_AXIS_Y]*obj->data[MXC6655_AXIS_Y];
		acc[obj->cvt.map[MXC6655_AXIS_Z]] = obj->cvt.sign[MXC6655_AXIS_Z]*obj->data[MXC6655_AXIS_Z];

		//Out put the mg
		acc[MXC6655_AXIS_X] = acc[MXC6655_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[MXC6655_AXIS_Y] = acc[MXC6655_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[MXC6655_AXIS_Z] = acc[MXC6655_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		sprintf(buf, "%04x %04x %04x",acc[MXC6655_AXIS_X],acc[MXC6655_AXIS_Y],cali_sensor_data);

	}
	return res;
}



static int MXC6655_ReadSensorData(struct i2c_client *client, int *buf, int bufsize)
{
	struct mxc6655_i2c_data *obj = (struct mxc6655_i2c_data*)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[MXC6655_AXES_NUM] = {0};
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);

	if(NULL == buf)
	{
		GSE_ERR("buf is null !!!\n");
		return -1;
	}
	if(NULL == client)
	{
		*buf = 0;
		GSE_ERR("client is null !!!\n");
		return MXC6655_ERR_STATUS;
	}

	if (atomic_read(&obj->suspend) && !enable_status)
	{
		GSE_DEBUG("sensor in suspend read not data!\n");
		return MXC6655_ERR_GETGSENSORDATA;
	}


	if((res = MXC6655_ReadData(client, obj->data)))
	{
		GSE_ERR("I2C error: ret value=%d", res);
		return MXC6655_ERR_I2C;
	}
	else
	{
		obj->data[MXC6655_AXIS_X] += obj->cali_sw[MXC6655_AXIS_X];
		obj->data[MXC6655_AXIS_Y] += obj->cali_sw[MXC6655_AXIS_Y];
		obj->data[MXC6655_AXIS_Z] += obj->cali_sw[MXC6655_AXIS_Z];

		/*remap coordinate*/
		acc[obj->cvt.map[MXC6655_AXIS_X]] = obj->cvt.sign[MXC6655_AXIS_X]*obj->data[MXC6655_AXIS_X];
		acc[obj->cvt.map[MXC6655_AXIS_Y]] = obj->cvt.sign[MXC6655_AXIS_Y]*obj->data[MXC6655_AXIS_Y];
		acc[obj->cvt.map[MXC6655_AXIS_Z]] = obj->cvt.sign[MXC6655_AXIS_Z]*obj->data[MXC6655_AXIS_Z];

		//Out put the mg
		acc[MXC6655_AXIS_X] = acc[MXC6655_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[MXC6655_AXIS_Y] = acc[MXC6655_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[MXC6655_AXIS_Z] = acc[MXC6655_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		buf[0] = acc[MXC6655_AXIS_X];
		buf[1] = acc[MXC6655_AXIS_Y];
		buf[2] = acc[MXC6655_AXIS_Z];

	}

	return res;
}

static int MXC6655_ReadRawData(struct i2c_client *client, char *buf)
{
	struct mxc6655_i2c_data *obj = (struct mxc6655_i2c_data*)i2c_get_clientdata(client);
	int res = 0;

	if (!buf || !client)
	{
        GSE_ERR(" buf or client is null !!\n");
		return EINVAL;
	}

	if((res = MXC6655_ReadData(client, obj->data)))
	{
		GSE_ERR("I2C error: ret value=%d\n", res);
		return EIO;
	}
	else
	{
		buf[0] = (int)obj->data[MXC6655_AXIS_X];
		buf[1] = (int)obj->data[MXC6655_AXIS_Y];
		buf[2] = (int)obj->data[MXC6655_AXIS_Z];
	}

	return 0;
}


static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = mxc6655_i2c_client;
	char strbuf[MXC6655_BUFSIZE];
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	MXC6655_ReadChipInfo(client, strbuf, MXC6655_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", (char*)strbuf);
}

static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = mxc6655_i2c_client;
	int strbuf[MXC6655_BUFSIZE] = {0};

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	MXC6655_ReadSensorData(client, strbuf, MXC6655_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", (char*)strbuf);
}

static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = mxc6655_i2c_client;
	struct mxc6655_i2c_data *obj;
	int err, len, mul;
	int tmp[MXC6655_AXES_NUM];

	len = 0;

	if (client == NULL) {
		return 0;
	}

	obj = i2c_get_clientdata(client);

	err = MXC6655_ReadCalibration(client, tmp);
	if (!err) {
		mul = obj->reso->sensitivity/mxc6655_offset_resolution.sensitivity;
		len += snprintf(buf+len, PAGE_SIZE-len, "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n",
			mul,
			obj->offset[MXC6655_AXIS_X], obj->offset[MXC6655_AXIS_Y], obj->offset[MXC6655_AXIS_Z],
			obj->offset[MXC6655_AXIS_X], obj->offset[MXC6655_AXIS_Y], obj->offset[MXC6655_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
			obj->cali_sw[MXC6655_AXIS_X], obj->cali_sw[MXC6655_AXIS_Y], obj->cali_sw[MXC6655_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len, "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n",
			obj->offset[MXC6655_AXIS_X] * mul + obj->cali_sw[MXC6655_AXIS_X],
			obj->offset[MXC6655_AXIS_Y] * mul + obj->cali_sw[MXC6655_AXIS_Y],
			obj->offset[MXC6655_AXIS_Z] * mul + obj->cali_sw[MXC6655_AXIS_Z],
			tmp[MXC6655_AXIS_X], tmp[MXC6655_AXIS_Y], tmp[MXC6655_AXIS_Z]);

		return len;
	} else
		return -EINVAL;
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = mxc6655_i2c_client;
	int err, x, y, z;
	int dat[MXC6655_AXES_NUM];

	if (!strncmp(buf, "rst", 3)) {
		err = MXC6655_ResetCalibration(client);
		if (err)
			GSE_INFO("reset offset err = %d\n", err);
	} else if (sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z) == 3) {
		dat[MXC6655_AXIS_X] = x;
		dat[MXC6655_AXIS_Y] = y;
		dat[MXC6655_AXIS_Z] = z;
		err = MXC6655_WriteCalibration(client, dat);
		if (err)
			GSE_INFO("write calibration err = %d\n", err);
	} else
		GSE_INFO("invalid format\n");

	return count;
}

static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_MXC6655_LOWPASS
	struct i2c_client *client = mxc6655_i2c_client;
	struct mxc6655_i2c_data *obj = i2c_get_clientdata(client);
	if(atomic_read(&obj->firlen))
	{
	 	int idx, len = atomic_read(&obj->firlen);
	 	GSE_DEBUG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for(idx = 0; idx < len; idx++)
		{
			GSE_INFO("[%5d %5d %5d]\n", obj->fir.raw[idx][MXC6655_AXIS_X], obj->fir.raw[idx][MXC6655_AXIS_Y], obj->fir.raw[idx][MXC6655_AXIS_Z]);
		}

		GSE_INFO("sum = [%5d %5d %5d]\n", obj->fir.sum[MXC6655_AXIS_X], obj->fir.sum[MXC6655_AXIS_Y], obj->fir.sum[MXC6655_AXIS_Z]);
		GSE_INFO("avg = [%5d %5d %5d]\n", obj->fir.sum[MXC6655_AXIS_X]/len, obj->fir.sum[MXC6655_AXIS_Y]/len, obj->fir.sum[MXC6655_AXIS_Z]/len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device_driver *ddri, const char *buf, size_t count)
{
#ifdef CONFIG_MXC6655_LOWPASS
	struct i2c_client *client = mxc6655_i2c_client;
	struct mxc6655_i2c_data *obj = i2c_get_clientdata(client);
	int firlen;

	if(1 != sscanf(buf, "%d", &firlen))
	{
		GSE_ERR("invallid format\n");
	}
	else if(firlen > C_MAX_FIR_LENGTH)
	{
		GSE_ERR("exceeds maximum filter length\n");
	}
	else
	{
		atomic_set(&obj->firlen, firlen);
		if(NULL == firlen)
		{
			atomic_set(&obj->fir_en, 0);
		}
		else
		{
			memset(&obj->fir, 0x00, sizeof(obj->fir));
			atomic_set(&obj->fir_en, 1);
		}
	}
#endif
	return 0;
}
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct mxc6655_i2c_data *obj = obj_i2c_data;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct mxc6655_i2c_data *obj = obj_i2c_data;
	int trace;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&obj->trace, trace);
	}
	else
	{
		GSE_ERR("invalid content: '%s', length = %d\n", buf, (int)count);
	}
	return count;
}
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct mxc6655_i2c_data *obj = obj_i2c_data;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if(obj->hw)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d %d (%d %d)\n",
	            obj->hw->i2c_num, obj->hw->direction, obj->hw->power_id, obj->hw->power_vol);
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_power_status_value(struct device_driver *ddri, char *buf)
{

	if(sensor_power)
		GSE_INFO("G sensor is in work mode, sensor_power = %d\n", sensor_power);
	else
		GSE_INFO("G sensor is in standby mode, sensor_power = %d\n", sensor_power);

	return snprintf(buf, PAGE_SIZE, "%x\n", sensor_power);
}
static ssize_t show_layout_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = mxc6655_i2c_client;
	struct mxc6655_i2c_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n",
		data->hw->direction,atomic_read(&data->layout),	data->cvt.sign[0], data->cvt.sign[1],
		data->cvt.sign[2],data->cvt.map[0], data->cvt.map[1], data->cvt.map[2]);
}
/*----------------------------------------------------------------------------*/
static ssize_t store_layout_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = mxc6655_i2c_client;
	struct mxc6655_i2c_data *data = i2c_get_clientdata(client);
	int layout = 0;

	if(1 == sscanf(buf, "%d", &layout))
	{
		atomic_set(&data->layout, layout);
		if(!hwmsen_get_convert(layout, &data->cvt))
		{
			GSE_ERR("HWMSEN_GET_CONVERT function error!\r\n");
		}
		else if(!hwmsen_get_convert(data->hw->direction, &data->cvt))
		{
			GSE_ERR("invalid layout: %d, restore to %d\n", layout, data->hw->direction);
		}
		else
		{
			GSE_ERR("invalid layout: (%d, %d)\n", layout, data->hw->direction);
			hwmsen_get_convert(0, &data->cvt);
		}
	}
	else
	{
		GSE_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}
static DRIVER_ATTR(chipinfo,	S_IWUSR | S_IRUGO, show_chipinfo_value,		NULL);
static DRIVER_ATTR(sensordata,	S_IWUSR | S_IRUGO, show_sensordata_value,	NULL);
static DRIVER_ATTR(cali,	S_IWUSR | S_IRUGO, show_cali_value,		store_cali_value);
static DRIVER_ATTR(firlen,	S_IWUSR | S_IRUGO, show_firlen_value,		store_firlen_value);
static DRIVER_ATTR(trace,	S_IWUSR | S_IRUGO, show_trace_value,		store_trace_value);
static DRIVER_ATTR(layout, 	S_IRUGO | S_IWUSR, show_layout_value,		store_layout_value);
static DRIVER_ATTR(status, 	S_IRUGO, show_status_value,			NULL);
static DRIVER_ATTR(powerstatus, S_IRUGO, show_power_status_value,		NULL);

static struct driver_attribute *mxc6655_attr_list[] = {
	&driver_attr_chipinfo,     /*chip information*/
	&driver_attr_sensordata,   /*dump sensor data*/
	&driver_attr_cali,         /*show calibration data*/
	&driver_attr_firlen,	   /*filter length: 0: disable, others: enable*/
	&driver_attr_trace,		   /*trace log*/
	&driver_attr_layout,
	&driver_attr_status,
	&driver_attr_powerstatus,
};
static int mxc6655_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(mxc6655_attr_list)/sizeof(mxc6655_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, mxc6655_attr_list[idx])))
		{
			GSE_ERR("driver_create_file (%s) = %d\n", mxc6655_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}
static int mxc6655_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(mxc6655_attr_list)/sizeof(mxc6655_attr_list[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}


	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, mxc6655_attr_list[idx]);
	}

	return err;
}
/******************************************************************************
 * Function Configuration
******************************************************************************/
static int mxc6655_open(struct inode *inode, struct file *file)
{
	 file->private_data = mxc6655_i2c_client;

	 if(file->private_data == NULL)
	 {
		 GSE_ERR("null mxc6655!!\n");
		 return -EINVAL;
	 }
	 return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int mxc6655_release(struct inode *inode, struct file *file)
{
	 file->private_data = NULL;
	 return 0;
}

static long mxc6655_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct mxc6655_i2c_data *obj = (struct mxc6655_i2c_data*)i2c_get_clientdata(client);
	char strbuf[MXC6655_BUFSIZE] = {0};
	void __user *data;
	struct SENSOR_DATA sensor_data;
	int err = 0;
	int cali[3];

	if(_IOC_DIR(cmd) & _IOC_READ)
	{
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	}
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
	{
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}

	if(err)
	{
		GSE_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}
	switch (cmd) {
		case GSENSOR_IOCTL_INIT:
			MXC6655_SetPowerMode(client, true);
			break;

		case GSENSOR_IOCTL_READ_CHIPINFO:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}

			MXC6655_ReadChipInfo(client, strbuf, MXC6655_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;
			}
			break;

		case GSENSOR_IOCTL_READ_SENSORDATA:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}
			if(!sensor_power)
			{
				MXC6655_SetPowerMode(client,true);
			}
			MXC6655_ReadSensorDataFactory(client, strbuf, MXC6655_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;
			}
			break;
		case GSENSOR_IOCTL_READ_GAIN:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}

			if(copy_to_user(data, &gsensor_gain, sizeof(struct GSENSOR_VECTOR3D)))
			{
				err = -EFAULT;
				break;
			}
			break;
		case GSENSOR_IOCTL_READ_RAW_DATA:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}
			MXC6655_ReadRawData(client, strbuf);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;
			}
			break;
		case GSENSOR_IOCTL_SET_CALI:
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}
			if(copy_from_user(&sensor_data, data, sizeof(sensor_data)))
			{
				err = -EFAULT;
				break;
			}
			if(atomic_read(&obj->suspend))
			{
				GSE_ERR("Perform calibration in suspend state!!\n");
				err = -EINVAL;
			}
			else
			{
				cali[MXC6655_AXIS_X] = sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[MXC6655_AXIS_Y] = sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[MXC6655_AXIS_Z] = sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				err = MXC6655_WriteCalibration(client, cali);
			}
			break;
		case GSENSOR_IOCTL_CLR_CALI:
			err = MXC6655_ResetCalibration(client);
			break;
		case GSENSOR_IOCTL_GET_CALI:
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;
			}
			if((err = MXC6655_ReadCalibration(client, cali)))
			{
				break;
			}

			sensor_data.x = cali[MXC6655_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.y = cali[MXC6655_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.z = cali[MXC6655_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			if(copy_to_user(data, &sensor_data, sizeof(sensor_data)))
			{
				err = -EFAULT;
				break;
			}
			break;
		default:
			GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
			err = -ENOIOCTLCMD;
			break;
	}

	return err;
}

static struct file_operations mxc6655_fops = {
		 .owner = THIS_MODULE,
		 .open = mxc6655_open,
		 .release = mxc6655_release,
		 .unlocked_ioctl = mxc6655_unlocked_ioctl,
};

static struct miscdevice mxc6655_device = {
		 .minor = MISC_DYNAMIC_MINOR,
		 .name = "mxc6655x",
		 .fops = &mxc6655_fops,
};


// if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL
static int mxc6655_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}

#if 0
static void cali_acc_frm_nv(void);
static void cali_acc_frm_nv()
{
    int cali_buff[3];
    int cali_err = 0;
    int ps_cali_offset=2690;

    memset(cali_buff, 0, sizeof(cali_buff));

    cali_err = get_calidata_frm_nvfile(&cali_buff[0], ps_cali_offset);
    if (cali_err < 0)
    {        
        GSE_ERR("cali_acc_frm_nv:read nvdata failed!"); 
        return;    
    }

    GSE_DEBUG("cali_acc_frm_nv is cali_buff[0] = %d, cali_buff[1] = %d, cali_buff[2] = %d",
        cali_buff[0], cali_buff[1], cali_buff[2]);

    mxc6655_factory_set_cali(cali_buff);
}
#endif
// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL

static int mxc6655_enable_nodata(int en)
{
	int res =0;
	bool power = false;

	if(1==en)
	{
		power = true;
	}
	if(0==en)
	{
		power = false;
	}
#if 0
	if(obj_i2c_data->first_boot)
	{
		GSE_DEBUG("first_boot is true");
        cali_acc_frm_nv();
		obj_i2c_data->first_boot = false;	
	}
#endif
	res = MXC6655_SetPowerMode(obj_i2c_data->client, power);
	if(res != MXC6655_SUCCESS)
	{
		GSE_ERR("MXC6655_SetPowerMode fail!\n");
		return -1;
	}
	GSE_DEBUG("MXC6655_enable_nodata OK en = %d sensor_power = %d\n", en, sensor_power);
	enable_status = en;
	return 0;
}

static int mxc6655_set_delay(u64 ns)
{
	return 0;
}

static int gsensor_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	int value = 0;

	value = (int)samplingPeriodNs/1000/1000;

	return mxc6655_set_delay(samplingPeriodNs);
}

static int gsensor_flush(void)
{
	return acc_flush_report();
}

static int mxc6655_get_data(int* x ,int* y,int* z, int* status)
{
	int buff[MXC6655_BUFSIZE] = {0};
	MXC6655_ReadSensorData(obj_i2c_data->client, buff, MXC6655_BUFSIZE);
	*x = buff[0];
	*y = buff[1];
	*z = buff[2];//cali_sensor_data;
	//printk( "mxc6655_get_data entry");
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return 0;
}


/*******add factory function***********/
static int mxc6655_factory_enable_sensor(bool enabledisable, int64_t sample_periods_ms)
{
	int en = (true == enabledisable ? 1 : 0);

	if (mxc6655_enable_nodata(en))
	{
		GSE_DEBUG("enable sensor failed");
		return -1;
	}

	return 0;
}

static int mxc6655_factory_get_data(int32_t data[3], int *status)
{
	return mxc6655_get_data(&data[0], &data[1], &data[2], status);

}

static int mxc6655_factory_get_raw_data(int32_t data[3])
{

//	MXC6655_ReadRawData(mxc6655_i2c_client, data);
	struct i2c_client *client = mxc6655_i2c_client;
	char buff[3]= {0};

	printk( "mxc6655_factory_get_raw_data entry");	
	MXC6655_ReadRawData(client, buff);

    data[0] = (int32_t)buff[0];
    data[1] = (int32_t)buff[1];
    data[2] = (int32_t)buff[2];

	return 0;
}
static int mxc6655_factory_enable_calibration(void)
{
	int i; 
	int sample_no=15;
	struct i2c_client *client = mxc6655_i2c_client;
	struct mxc6655_i2c_data *obj;
	s16 buff[3] = {0};
	int axis = 0;
	s16 data[3] = {0};
	int32_t acc_ave[3] = {0};

	obj = i2c_get_clientdata(client);
	for (i = 0; i < sample_no; i++)
	{
		msleep(10);

		MXC6655_ReadData(mxc6655_i2c_client, buff);
		//MXC6655_ReadRawData(mxc6655_i2c_client, buff);

		data[0] = buff[0];
		data[1] = buff[1];
		data[2] = buff[2];
		   
		printk("mxc6655_factory_enable_calibration raw: %d %d %d",data[0],data[1],data[2]);

		acc_ave[0] += data[0];
		acc_ave[1] += data[1];
		acc_ave[2] += data[2];
	}

	for (i = 0; i < 3; i++)
	{
		acc_ave[i] =(acc_ave[i] + sample_no/2) / sample_no;
	}

	for (axis = 0;axis< 3; axis++)
	{  
		if(axis<=1)
		obj->cali_sw[axis] = -acc_ave[axis];
   		else
   		{
	   		obj->cali_sw[2] = 0;//-(256-acc_ave[2]);
   		}   
	}

    printk( "mxc6655_factory_enable_calibration offset: %d %d %d",obj->cali_sw[0],obj->cali_sw[1],obj->cali_sw[2]);
	
	
	return 0;
}
static int mxc6655_factory_clear_cali(void)
{
	int err = 0;

	err = MXC6655_ResetCalibration(mxc6655_i2c_client);
	if (err) {
		return -1;
	}
	return 0;
}
static int mxc6655_factory_set_cali(int32_t data[3])
{
	struct i2c_client *client = mxc6655_i2c_client;
	struct mxc6655_i2c_data *obj;	
	int i = 0;
	obj = i2c_get_clientdata(client);

	GSE_DEBUG("ori0 x:%d, y:%d, z:%d", data[0], data[1], data[2]);
	for(i = 0; i < 3; i++)
	{
		if((data[i]&0x8000) == 0x8000)//负数
		{
			data[i] = data[i] - 65536;
		}
	}

	GSE_DEBUG("ori x:%d, y:%d, z:%d", data[0], data[1], data[2]);

	obj->cali_sw[0]  = data[0] * obj->reso->sensitivity / GRAVITY_EARTH_1000;
	obj->cali_sw[1]  = data[1] * obj->reso->sensitivity / GRAVITY_EARTH_1000;
    obj->cali_sw[2]  = data[2] * obj->reso->sensitivity / GRAVITY_EARTH_1000;

	GSE_DEBUG("new x:%d, y:%d, z:%d", obj->cali_sw[0], obj->cali_sw[1], obj->cali_sw[2]);

#if 0
	int err = 0;
	int cali[3] = {0};
	if (atomic_read(&obj_i2c_data->suspend))
	{
		GSE_ERR("Perform calibration in suspend state!!\n");
		err = -EINVAL;
	}
	else
	{
		cali[MXC6655_AXIS_X] = data[0] * obj_i2c_data->reso->sensitivity / GRAVITY_EARTH_1000;
		cali[MXC6655_AXIS_Y] = data[1] * obj_i2c_data->reso->sensitivity / GRAVITY_EARTH_1000;
		cali[MXC6655_AXIS_Z] = data[2] * obj_i2c_data->reso->sensitivity / GRAVITY_EARTH_1000;
		
		err = MXC6655_WriteCalibration(mxc6655_i2c_client, cali);
		if (err)
		{
			return -1;
		}
	}
#endif
	return 0;
}
static int mxc6655_factory_get_cali(int32_t data[3])
{
	
	struct i2c_client *client = mxc6655_i2c_client;
	struct mxc6655_i2c_data *obj;
	obj = i2c_get_clientdata(client);
	
	GSE_DEBUG("orig x:%d, y:%d, z:%d", obj->cali_sw[0], obj->cali_sw[1], obj->cali_sw[2]);
	data[0] = obj->cali_sw[0] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
	data[1] = obj->cali_sw[1] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
	data[2] = obj->cali_sw[2] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
	
	GSE_DEBUG("new x:%d, y:%d, z:%d", data[0], data[1], data[2]);

	return 0;
}

static int mxc6655_factory_do_self_test(void)
{
	GSE_DEBUG("entry mxc6655_factory_do_self_test\n");
    return 0;
}

static struct accel_factory_fops mxc6655_factory_fops = {
	.enable_sensor = mxc6655_factory_enable_sensor,
	.get_data = mxc6655_factory_get_data,
	.get_raw_data = mxc6655_factory_get_raw_data,
	.enable_calibration = mxc6655_factory_enable_calibration,
	.clear_cali =mxc6655_factory_clear_cali,
	.set_cali = mxc6655_factory_set_cali,
	.get_cali = mxc6655_factory_get_cali,
	.do_self_test = mxc6655_factory_do_self_test,
};

static struct accel_factory_public mxc6655_factory_device =
{
    .gain = 1,
    .sensitivity = 1,
    .fops = &mxc6655_factory_fops,
};

#ifdef CONFIG_MID_ITEMS_SUPPORT
int mxc6655_direction_calculate(int bma)
{
    int mc3 = 0;
    switch(bma){
        case 0:
            mc3 = 2;
            break;
        case 1:
            mc3 = 3;
            break;
        case 2:
            mc3 = 0;
            break;
        case 3:
            mc3 = 1;
            break;
        case 4:
            mc3 = 4; //6
            break;
        case 5:
            mc3 = 5; //7
            break;
        case 6:
            mc3 = 6;//4
            break;
        case 7:
            mc3 = 7;//5
            break;
        default:
            break;
    }
    return mc3;
}
#endif

/*----------------------------------------------------------------------------*/
static int mxc6655_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct mxc6655_i2c_data *obj;

	int err = 0;
	
#ifdef CONFIG_MID_ITEMS_SUPPORT
	int orient = 0;
#endif	

	struct acc_control_path ctl={0};
	struct acc_data_path data={0};
	GSE_DEBUG_FUNC();
	GSE_INFO("driver version = %s\n",DRIVER_VERSION);
	
	err = get_accel_dts_func(client->dev.of_node, hw);
	if (err < 0) {
	
		goto exit;
	}

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}

	memset(obj, 0, sizeof(struct mxc6655_i2c_data));

	obj->hw = hw;
    //obj->hw->direction = 4;
#ifdef CONFIG_MID_ITEMS_SUPPORT
	orient = item_integer("sensor.accelerometer.bma.orientation", 0);
 	orient = orient < 0 ? 0 : orient;
	hw->direction = mxc6655_direction_calculate(orient); 
    printk("MXC6655: direction bma=%d, direction=%d\n", orient, hw->direction);
	//printk("mxw===mc3: direction obj->hw->direction=%d\n", obj->hw->direction);
#endif
	if((err = hwmsen_get_convert(obj->hw->direction, &obj->cvt)))
	{
		GSE_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit;
	}
    GSE_ERR(" direction=%d\n", obj->hw->direction);
	obj_i2c_data = obj;
	client->addr=0x15;
	
	obj->client = client;
	new_client = obj->client;
	i2c_set_clientdata(new_client,obj);

	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);


#ifdef CONFIG_MXC6655_LOWPASS
	if(obj->hw->firlen > C_MAX_FIR_LENGTH)
	{
		atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
	}
	else
	{
		atomic_set(&obj->firlen, obj->hw->firlen);
	}

	if(atomic_read(&obj->firlen) > 0)
	{
		atomic_set(&obj->fir_en, 1);
	}

#endif
	mxc6655_i2c_client = new_client;
	
	if((err = mxc6655_init_client(new_client, 1)))
	{
		goto exit_init_failed;
	}
#ifdef XUNHU_LPS_TEKHW_SUPPORT
	accel_compatible_flag=1;
#endif
	if((err = misc_register(&mxc6655_device)))
	{
		GSE_ERR("mxc6655_device register failed\n");
		printk("mxc6655_device register failed\n");
		goto exit_misc_device_register_failed;
	}
/* factory */
	err = accel_factory_device_register(&mxc6655_factory_device);
	if (err) {
		GSE_ERR("acc_factory register failed.\n");
		goto exit_misc_device_register_failed;
	}
/***end factory********/	
	if((err = mxc6655_create_attr(&mxc6655_init_info.platform_diver_addr->driver)))
	{
		GSE_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	ctl.open_report_data = mxc6655_open_report_data;
	ctl.enable_nodata = mxc6655_enable_nodata;
	ctl.set_delay  = mxc6655_set_delay;
	
	ctl.batch = gsensor_batch;
	ctl.flush = gsensor_flush;
	ctl.is_report_input_direct = false;
#ifdef CUSTOM_KERNEL_SENSORHUB
	ctl.is_support_batch = obj->hw.is_batch_supported;
#else
	ctl.is_support_batch = false;
#endif

	err = acc_register_control_path(&ctl);
	if(err)
	{
		GSE_ERR("register acc control path err\n");
		goto exit_create_attr_failed;
	}

	data.get_data = mxc6655_get_data;
	data.vender_div = 1000;
	err = acc_register_data_path(&data);
	if(err)
	{
		GSE_ERR("register acc data path err\n");
		goto exit_create_attr_failed;
	}
	mxc6655_init_flag = 0;
	GSE_INFO("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
	misc_deregister(&mxc6655_device);
exit_misc_device_register_failed:
exit_init_failed:
	kfree(obj);
exit:
	GSE_ERR("%s: err = %d\n", __func__, err);
	mxc6655_init_flag = -1;
	return err;
}

static int mxc6655_i2c_remove(struct i2c_client *client)
{
	 int err = 0;

	 if((err = mxc6655_delete_attr(&mxc6655_init_info.platform_diver_addr->driver)))
	 {
		 GSE_ERR("mxc6655_delete_attr fail: %d\n", err);
	 }

	if(err) {
		misc_deregister(&mxc6655_device);
	}

	accel_factory_device_deregister(&mxc6655_factory_device);

	 //if((err = hwmsen_detach(ID_ACCELEROMETER)))

	 mxc6655_i2c_client = NULL;
	 i2c_unregister_device(client);
	 accel_factory_device_deregister(&mxc6655_factory_device);
	 kfree(i2c_get_clientdata(client));
	 return 0;
}


static int mxc6655_local_init(void)
{
	GSE_DEBUG_FUNC();
	mxc6655_power(hw, 1);

	if(i2c_add_driver(&mxc6655_i2c_driver))
	{
		 GSE_ERR("add driver error\n");
		 return -1;
	}
	if(-1 == mxc6655_init_flag)
	{
		GSE_ERR("mxc6655_local_init failed mxc6655_init_flag=%d\n",mxc6655_init_flag);
	   	return -1;
	}
	return 0;
}
static int mxc6655_remove(void)
{
	 GSE_DEBUG_FUNC();
	 mxc6655_power(hw, 0);
	 i2c_del_driver(&mxc6655_i2c_driver);
	 return 0;
}

static int __init mxc6655_driver_init(void)
{
	
	acc_driver_add(&mxc6655_init_info);

	mutex_init(&mxc6655_mutex);

	return 0;
}

static void __exit mxc6655_driver_exit(void)
{
	GSE_DEBUG_FUNC();
	mutex_destroy(&mxc6655_mutex);
}

module_init(mxc6655_driver_init);
module_exit(mxc6655_driver_exit);


MODULE_AUTHOR("Lyon Miao<xlmiao@memsic.com>");
MODULE_DESCRIPTION("MEMSIC MXC6655 Accelerometer Sensor Driver");
MODULE_LICENSE("GPL");
