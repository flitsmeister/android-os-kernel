/* 
 * ICM42607 sensor driver
 * Copyright (C) 2018 Invensense, Inc.
 *
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

#include <linux/ioctl.h>
#include <cust_gyro.h>
#include <gyroscope.h>
#include <sensors_io.h>
#include "gyroscope.h"
#include "../../drivers/misc/mediatek/sensors-1.0/accelerometer/icm42607_a/icm42607_register.h"
#include "../../drivers/misc/mediatek/sensors-1.0/accelerometer/icm42607_a/icm42607_share.h"
#include "icm42607_gyro.h"

#define ICM42607_GYRO_DEV_NAME "ICM42607_GYRO"
#define ICM_AD0_LOW                 1              //define for select i2c address 
#define MISC_DEVICE_FACTORY 0

#ifdef CONFIG_MID_ITEMS_SUPPORT
#include <mt-plat/items.h>
#endif
//#define DEBUG

static bool icm42607_gyro_first_enable = false;
/*
#define GYRO_FLAG               "[GRYO][ICM42607_G]"
#define GYRO_PR_ERR(fmt, args...)	pr_err(GYRO_FLAG fmt, ##args)
#if defined(ICM206XX_G_DEBUG)
#define GYRO_FUN(f)	pr_debug(GYRO_FLAG"%s\n", __func__)
#define GYRO_DBG(fmt, args...)	pr_debug(GYRO_FLAG fmt, ##args)
#define GYRO_LOG(fmt, args...) pr_info(GYRO_FLAG fmt, ##args)
#else
#define GYRO_FUN(f)
#define GYRO_DBG(fmt, args...)
#define GYRO_LOG(fmt, args...)
#endif
*/
//static int icm206xx_gyro_current_highest_samplerate = 0;
static int icm42607_gyro_discardcount = 0;

struct icm42607_gyro_i2c_data {
    struct i2c_client *client;
    struct gyro_hw *hw;
    struct hwmsen_convert cvt;
    /*misc*/
    atomic_t trace;
    atomic_t suspend;
    atomic_t selftest;
    atomic_t is_enabled;
    /*data*/
    s16 cali_sw[ICM42607_AXIS_NUM+1];
    s16 data[ICM42607_AXIS_NUM+1];
};

static int icm42607_gyro_init_flag =  -1;

static struct i2c_client *icm42607_gyro_i2c_client;
static struct icm42607_gyro_i2c_data *obj_i2c_data;

#ifdef ICM42607_SELFTEST
static char selftestRes[8] = { 0 };
#define SELF_TEST_GYR_BW_IND        BIT_GYRO_UI_LNM_BW_34HZ
#endif

/* +/-1000DPS as default */
static int g_icm42607_gyro_sensitivity = ICM42607_GYRO_DEFAULT_SENSITIVITY;

struct gyro_hw gyro_cust;
static struct gyro_hw *hw = &gyro_cust;

static int icm42607_gyro_local_init(struct platform_device *pdev);
static int icm42607_gyro_remove(void);
static int icm42607_gyro_get_data(int *x , int *y, int *z, int *status);
static int icm42607_gyro_enable_nodata(int en);
static int icm42607_gyro_set_delay(u64 ns);
static struct gyro_init_info icm42607_gyro_init_info = {
    .name = ICM42607_GYRO_DEV_NAME,
    .init = icm42607_gyro_local_init,
    .uninit = icm42607_gyro_remove,
};

static int icm42607_gyro_SetFullScale(struct i2c_client *client, u8 gyro_fsr)
{
    u8 databuf[2] = {0};
    int res = 0;

    res = icm42607_share_read_register(REG_GYRO_CONFIG0, databuf, 1);
    if (res < 0) {
        GYRO_PR_ERR("read fsr register err!\n");
        return ICM42607_ERR_BUS;
    }
    /* clear FSR bit */
    databuf[0] &= ~BIT_GYRO_FSR;
    databuf[0] |= gyro_fsr << SHIFT_GYRO_FS_SEL;
    g_icm42607_gyro_sensitivity = (ICM42607_GYRO_MAX_SENSITIVITY >> (3 - gyro_fsr)) + 1;
    res = icm42607_share_write_register(REG_GYRO_CONFIG0, databuf, 1);
    if (res < 0){
        GYRO_PR_ERR("write fsr register err!\n");
        return ICM42607_ERR_BUS;
    }
    return ICM42607_SUCCESS;
}

static int icm42607_gyro_SetFilter(struct i2c_client *client, u8 gyro_filter)
{
    u8 databuf[2] = {0};
    int res = 0;

    res = icm42607_share_read_register(REG_GYRO_CONFIG1, databuf, 1);
    if (res < 0) {
        GYRO_PR_ERR("read filter register err!\n");
        return ICM42607_ERR_BUS;
    }
    /* clear filter bit */
    databuf[0] &= ~BIT_GYRO_FILTER;
    databuf[0] |= gyro_filter;
    res = icm42607_share_write_register(REG_GYRO_CONFIG1, databuf, 1);
    if (res < 0) {
        GYRO_PR_ERR("write filter register err!\n");
        return ICM42607_ERR_BUS;
    }
    return ICM42607_SUCCESS;
}

#ifdef ICM42607_SELFTEST 
static int icm42607_gyro_ReadSensorDataDirect(struct i2c_client *client,
    s16 data[ICM42607_AXIS_NUM])
{
    char databuf[6];
    int i;
    int res = 0;

    if (client == NULL)
        return ICM42607_ERR_INVALID_PARAM;
    res = icm42607_share_read_register(REG_GYRO_DATA_X0_UI, databuf,
        ICM42607_DATA_LEN);
    if (res < 0) {
        GYRO_PR_ERR("read gyroscope data error\n");
        return ICM42607_ERR_BUS ;
    }
    /* convert 8-bit to 16-bit */
    for (i = 0; i < ICM42607_AXIS_NUM; i++)
        data[i] = be16_to_cpup((__be16 *) (&databuf[i*2]));
    return ICM42607_SUCCESS;
}
#else

#if MISC_DEVICE_FACTORY

static int icm42607_gyro_ReadSensorDataDirect(struct i2c_client *client,
    s16 data[ICM42607_AXIS_NUM])
{
    char databuf[6];
    int i;
    int res = 0;

    if (client == NULL)
        return ICM42607_ERR_INVALID_PARAM;
    res = icm42607_share_read_register(REG_GYRO_DATA_X0_UI, databuf,
        ICM42607_DATA_LEN);
    if (res < 0) {
        GYRO_PR_ERR("read gyroscope data error\n");
        return ICM42607_ERR_BUS ;
    }
    /* convert 8-bit to 16-bit */
    for (i = 0; i < ICM42607_AXIS_NUM; i++)
        data[i] = be16_to_cpup((__be16 *) (&databuf[i*2]));
    return ICM42607_SUCCESS;
}
	#endif 
#endif 

static int icm42607_gyro_ReadSensorData(struct i2c_client *client,
    char *buf, int bufsize)
{
    char databuf[6];
//	char regbug[0x80];
    int  data[3];
	int  data_buff[3];
    int  i = 0;
    int res = 0;
	static int discard_count = 0;
    struct icm42607_gyro_i2c_data * obj = i2c_get_clientdata(client);

    if (client == NULL)
        return ICM42607_ERR_INVALID_PARAM;
    res = icm42607_share_read_register(REG_GYRO_DATA_X0_UI, databuf,
        ICM42607_DATA_LEN);
    if (res < 0) {
        GYRO_PR_ERR("read gyroscope data error\n");
        return ICM42607_ERR_BUS;
    }

	if (icm42607_gyro_first_enable){
		if(discard_count < icm42607_gyro_discardcount){ 
			discard_count ++;
			GYRO_PR_ERR("discard times %d\n",discard_count);
            return ICM42607_ERR_INVALID_PARAM;
        }
        else{
            discard_count =0;
            icm42607_gyro_first_enable = false;
        }
    }       
//	icm42607_share_read_register(0, regbug, 0x80);
//	for(i = 0; i <= 0x7f; i++)
//	GYRO_LOG("Gyro reg 0x%x : 0x%x\n", i, regbug[i]);
    /* convert 8-bit to 16-bit */
    for (i = 0; i < ICM42607_AXIS_NUM; i++) {
        obj->data[i] = be16_to_cpup((__be16 *) (&databuf[i*2]));
        /* add calibration value */
        // 1. Apply SENSITIVITY_SCALE_FACTOR calculated from SetFullScale 	(DPS)
		// 2. Translated it to MAX SENSITIVITY_SCALE_FACTOR			(DPS)
		data_buff[i] = obj->data[i] * ICM42607_GYRO_MAX_SENSITIVITY / g_icm42607_gyro_sensitivity;
    }
  
	#ifdef DEBUG

		GYRO_LOG("Gyro Data 1- %d %d %d\n", 
					data_buff[ICM42607_AXIS_X], 
					data_buff[ICM42607_AXIS_Y], 
					data_buff[ICM42607_AXIS_Z]);

		GYRO_LOG("offset - %d %d %d\n", 
					obj->cali_sw[ICM42607_AXIS_X], 
					obj->cali_sw[ICM42607_AXIS_Y], 
					obj->cali_sw[ICM42607_AXIS_Z]);
		
	    GYRO_LOG("Gyro Data 2- %d %d %d\n", 
					data_buff[ICM42607_AXIS_X], 
					data_buff[ICM42607_AXIS_Y], 
					data_buff[ICM42607_AXIS_Z]);

		GYRO_LOG("Gyro map x- %d %d \n", 
					obj->cvt.map[ICM42607_AXIS_X], 
					obj->cvt.sign[ICM42607_AXIS_X]);
		GYRO_LOG("Gyro map y- %d %d \n", 
					obj->cvt.map[ICM42607_AXIS_Y], 
					obj->cvt.sign[ICM42607_AXIS_Y]);	
		GYRO_LOG("Gyro map z- %d %d \n", 
					obj->cvt.map[ICM42607_AXIS_Z], 
					obj->cvt.sign[ICM42607_AXIS_Z]);	
	#endif
	/* Add Cali */
		//obj->data[i] += obj->cali_sw[i];
        data_buff[ICM42607_AXIS_X]+= obj->cali_sw[ICM42607_AXIS_X];
		data_buff[ICM42607_AXIS_Y]+= obj->cali_sw[ICM42607_AXIS_Y];
  		data_buff[ICM42607_AXIS_Z]+= obj->cali_sw[ICM42607_AXIS_Z];

		/* 3 . Orientation Translation Lower (sensor) --> Upper (Device) */
		data[obj->cvt.map[ICM42607_AXIS_X]] = obj->cvt.sign[ICM42607_AXIS_X] * data_buff[ICM42607_AXIS_X];
		data[obj->cvt.map[ICM42607_AXIS_Y]] = obj->cvt.sign[ICM42607_AXIS_Y] * data_buff[ICM42607_AXIS_Y];
		data[obj->cvt.map[ICM42607_AXIS_Z]] = obj->cvt.sign[ICM42607_AXIS_Z] * data_buff[ICM42607_AXIS_Z];
    	sprintf(buf, "%04x %04x %04x",
        data[ICM42607_AXIS_X],
        data[ICM42607_AXIS_Y],
        data[ICM42607_AXIS_Z]);
    if (atomic_read(&obj->trace)) {
        GYRO_LOG("Gyroscope data - %04x %04x %04x\n",
            data[ICM42607_AXIS_X],
            data[ICM42607_AXIS_Y],
            data[ICM42607_AXIS_Z]);
    }
    return ICM42607_SUCCESS;
}



static int icm42607_gyro_ResetCalibration(struct i2c_client *client)
{
    struct icm42607_gyro_i2c_data *obj = i2c_get_clientdata(client);

    memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
    return ICM42607_SUCCESS;
}

static int icm42607_gyro_ReadCalibration(struct i2c_client *client,
    struct SENSOR_DATA *sensor_data)
{
    struct icm42607_gyro_i2c_data *obj = i2c_get_clientdata(client);
    int cali[ICM42607_AXIS_NUM];
	
    cali[obj->cvt.map[ICM42607_AXIS_X]] = obj->cvt.sign[ICM42607_AXIS_X]*obj->cali_sw[ICM42607_AXIS_X];
    cali[obj->cvt.map[ICM42607_AXIS_Y]] = obj->cvt.sign[ICM42607_AXIS_Y]*obj->cali_sw[ICM42607_AXIS_Y];
    cali[obj->cvt.map[ICM42607_AXIS_Z]] = obj->cvt.sign[ICM42607_AXIS_Z]*obj->cali_sw[ICM42607_AXIS_Z];
	sensor_data->x = cali[ICM42607_AXIS_X] ;
	sensor_data->y = cali[ICM42607_AXIS_Y] ;
	sensor_data->z = cali[ICM42607_AXIS_Z] ;
    if (atomic_read(&obj->trace)) {
        GYRO_LOG("Gyro ReadCalibration:[sensor_data:%5d %5d %5d]\n",
            sensor_data->x, sensor_data->y, sensor_data->z);
        GYRO_LOG("Gyro ReadCalibration:[cali_sw:%5d %5d %5d]\n",
            obj->cali_sw[ICM42607_AXIS_X],
            obj->cali_sw[ICM42607_AXIS_Y],
            obj->cali_sw[ICM42607_AXIS_Z]);
    }
    return ICM42607_SUCCESS;
}

static int icm42607_gyro_write_rel_calibration(struct icm42607_gyro_i2c_data *obj, int dat[ICM_GYRO_AXES_NUM])
{
   	obj->cali_sw[ICM42607_AXIS_X] = obj->cvt.sign[ICM42607_AXIS_X]*dat[obj->cvt.map[ICM42607_AXIS_X]];
    obj->cali_sw[ICM42607_AXIS_Y] = obj->cvt.sign[ICM42607_AXIS_Y]*dat[obj->cvt.map[ICM42607_AXIS_Y]];
	obj->cali_sw[ICM42607_AXIS_Z] = obj->cvt.sign[ICM42607_AXIS_Z]*dat[obj->cvt.map[ICM42607_AXIS_Z]];
#ifdef DEBUG
    GYRO_LOG("test (%5d, %5d, %5d))\n",obj->cali_sw[ICM42607_AXIS_X],obj->cali_sw[ICM42607_AXIS_Y],obj->cali_sw[ICM42607_AXIS_Z]);		
#endif
    return 0;
}

static int icm42607_gyro_WriteCalibration(struct i2c_client *client,
    struct SENSOR_DATA *sensor_data)
{
    struct icm42607_gyro_i2c_data *obj = i2c_get_clientdata(client);
    int cali[ICM42607_AXIS_NUM];
	
    cali[obj->cvt.map[ICM42607_AXIS_X]] = obj->cvt.sign[ICM42607_AXIS_X]*obj->cali_sw[ICM42607_AXIS_X];
	cali[obj->cvt.map[ICM42607_AXIS_Y]] = obj->cvt.sign[ICM42607_AXIS_Y]*obj->cali_sw[ICM42607_AXIS_Y];
	cali[obj->cvt.map[ICM42607_AXIS_Z]] = obj->cvt.sign[ICM42607_AXIS_Z]*obj->cali_sw[ICM42607_AXIS_Z];
	cali[ICM42607_AXIS_X] += sensor_data ->x;
	cali[ICM42607_AXIS_Y] += sensor_data ->y;
	cali[ICM42607_AXIS_Z] += sensor_data ->z;
#ifdef DEBUG
	GYRO_LOG("write gyro calibration data  (%5d, %5d, %5d)-->(%5d, %5d, %5d)\n",
			sensor_data ->x, sensor_data ->y, sensor_data ->z,
			cali[ICM42607_AXIS_X],cali[ICM42607_AXIS_Y],cali[ICM42607_AXIS_Z]);
#endif
	return icm42607_gyro_write_rel_calibration(obj, cali);
}

#ifdef ICM42607_SELFTEST
static int  icm42607_gyro_InitSelfTest(struct i2c_client * client)
{
    int res = 0;

    /* softreset */
    res = icm42607_share_ChipSoftReset();
    if (res != ICM42607_SUCCESS)
        return res;
    /* set power mode */
    icm42607_share_set_sensor_power_mode(ICM42607_SENSOR_TYPE_GYRO,
        BIT_GYRO_MODE_LNM);
    /* setpowermode(true) --> exit sleep */
    res = icm42607_share_SetPowerMode(ICM42607_SENSOR_TYPE_GYRO, true);
    if (res != ICM42607_SUCCESS)
        return res;
	/* fsr : ICM42607_GYRO_RANGE_250DPS */
    res = icm42607_gyro_SetFullScale(client, ICM42607_GYRO_RANGE_250DPS);
    if (res != ICM42607_SUCCESS)
        return res;
    /* filter : SELF_TEST_GYR_BW_IND */
    res = icm42607_gyro_SetFilter(client, SELF_TEST_GYR_BW_IND);
    if (res != ICM42607_SUCCESS)
        return res;
    /* odr : 800hz */
    res = icm42607_share_SetSampleRate(ICM42607_SENSOR_TYPE_GYRO,
        1250000, true);
    if (res != ICM42607_SUCCESS)
        return res;
    /* set enable sensor */
    res = icm42607_share_EnableSensor(ICM42607_SENSOR_TYPE_GYRO, true,&icm42607_gyro_first_enable);
    if (res != ICM42607_SUCCESS)
        return res;
    /* delay for selftest */
    mdelay(SELF_TEST_READ_INTERVAL_MS);
    return res;
}

static int icm42607_gyro_CalcAvgWithSamples(struct i2c_client *client,
    int avg[3], int count)
{
    int res = 0;
    int i, nAxis;
    s16 sensor_data[ICM42607_AXIS_NUM];
    s32 sum[ICM42607_AXIS_NUM] = {0,};

    for (i = 0; i < count; i++) {
        res = icm42607_gyro_ReadSensorDataDirect(client, sensor_data);
        if (res) {
            GYRO_PR_ERR("read data fail: %d\n", res);
            return ICM42607_ERR_STATUS;
        }
        for (nAxis = 0; nAxis < ICM42607_AXIS_NUM; nAxis++)
            sum[nAxis] += sensor_data[nAxis];
        /* data register updated @1khz */
        mdelay(1);
    }
    for (nAxis = 0; nAxis < ICM42607_AXIS_NUM; nAxis++)
        avg[nAxis] = (int)(sum[nAxis] / count) * SELF_TEST_PRECISION;
    return ICM42607_SUCCESS;
}

static bool icm42607_gyro_DoSelfTest(struct i2c_client *client)
{
    int res = 0;
    int i;
    int gyro_ST_on[ICM42607_AXIS_NUM], gyro_ST_off[ICM42607_AXIS_NUM];
    /* index of otp_lookup_tbl */
    u8  st_code[ICM42607_AXIS_NUM];
    u16 st_otp[ICM42607_AXIS_NUM];
    bool otp_value_has_zero = false;
    bool test_result = true;
    u8 databuf[2] = {0};
    int st_res;
    int retry;

    /* acquire OTP value from OTP lookup table */
    res = icm42607_share_read_blkreg(BIT_BLK_SEL_3, REG_XG_ST_DATA, &(st_code[0]), 3);
    if (res) {
        GYRO_PR_ERR("read data fail: %d\n", res);
        return false;
    }
    GYRO_LOG("st_code: %02x, %02x, %02x\n", st_code[0], st_code[1], st_code[2]);

    /* lookup OTP value with st_code */
    for (i = 0; i < ICM42607_AXIS_NUM; i++) {
        if (st_code[i] != 0)
            st_otp[i] = st_otp_lookup_tbl[ st_code[i] - 1];
        else {
            st_otp[i] = 0;
            otp_value_has_zero = true;
        }
    }
    /* read sensor data and calculate average values from it */
    for (retry = 0 ; retry < RETRY_CNT_SELF_TEST ; retry++ ) {
        /* read 200 samples with selftest off */
        res = icm42607_gyro_CalcAvgWithSamples(client, gyro_ST_off,
            SELF_TEST_SAMPLE_NB);
        if (res) {
            GYRO_PR_ERR("read data fail: %d\n", res);
            return false;
        }
        /* set selftest on */
        databuf[0] = BIT_GYRO_ST_EN;
        res = icm42607_share_write_blkreg(BIT_BLK_SEL_1, REG_SELF_TEST_CONFIG, databuf, 1);
        if (res) {
            GYRO_PR_ERR("enable st gyro fail: %d\n", res);
            return false;
        }
        /* wait 20ms for oscillations to stabilize */
        mdelay(20);
        /* Read 200 Samples with selftest on */
        res = icm42607_gyro_CalcAvgWithSamples(client, gyro_ST_on,
            SELF_TEST_SAMPLE_NB);
        if (res) {
            GYRO_PR_ERR("read data fail: %d\n", res);
            return false;
        }
        /* set selftest off */
        databuf[0] = 0x00;
        res = icm42607_share_write_blkreg(BIT_BLK_SEL_1, REG_SELF_TEST_CONFIG, databuf, 1);
        if (res) {
            GYRO_PR_ERR("disable st gyro fail: %d\n", res);
            return false;
        }
        /* wait 20ms for oscillations to stabilize */
        mdelay(20);
        /* compare calculated value with OTP value to judge success or fail */
        if (!otp_value_has_zero) {
            /* criteria a */
            for (i = 0; i < ICM42607_AXIS_NUM; i++) {
                st_res = gyro_ST_on[i] - gyro_ST_off[i];
                if (st_res <= st_otp[i] * SELF_TEST_GYR_SHIFT_DELTA) {
                    GYRO_LOG("error gyro[%d] : st_res = %d, st_otp = %d\n",
                        i, st_res, st_otp[i]);
                    test_result = false;
                }
            }
        } else {
            /* criteria b */
            for (i = 0; i < ICM42607_AXIS_NUM; i++) {
                st_res = abs(gyro_ST_on[i] - gyro_ST_off[i]);
                if (st_res < SELF_TEST_MIN_GYR) {
                    GYRO_LOG("error gyro[%d] : st_res = %d, min = %d\n",
                        i, st_res, SELF_TEST_MIN_GYR);
                    test_result = false;
                }
            }
        }
        if (test_result) {
            /* criteria c */
            for (i = 0; i < ICM42607_AXIS_NUM; i++) {
                if (abs(gyro_ST_off[i]) > SELF_TEST_MAX_GYR_OFFSET) {
                    GYRO_LOG("error gyro[%d] = %d, max = %d\n",
                        i, abs(gyro_ST_off[i]), SELF_TEST_MAX_GYR_OFFSET);
                    test_result = false;
                }
            }
        }
        if (test_result)
            break;
    }
    return test_result;
}
#endif

static int icm42607_gyro_init_client(struct i2c_client *client, bool enable)
{
    int res = 0;

	char strbuf[ICM42607_BUFSIZE];
	res = icm42607_share_ReadChipInfo(strbuf, ICM42607_BUFSIZE);
	if (res != ICM42607_SUCCESS)
		return res;
	GYRO_LOG("%s \n",strbuf);
    /* xxit sleep mode */
    res = icm42607_share_SetPowerMode(ICM42607_SENSOR_TYPE_GYRO, true);
    if (res != ICM42607_SUCCESS)
        return res;
    /* set fsr +/-1000 dps as default */
    res = icm42607_gyro_SetFullScale(client, ICM42607_GYRO_RANGE_1000DPS);
    if (res != ICM42607_SUCCESS)
        return res;
    /* set power mode */
    icm42607_share_set_sensor_power_mode(ICM42607_SENSOR_TYPE_GYRO,
        BIT_GYRO_MODE_LNM);
    /* set filter BIT_ACCEL_UI_LNM_BW_2_FIR as default */
    res = icm42607_gyro_SetFilter(client, BIT_GYRO_UI_LNM_BW_180HZ);
    if (res != ICM42607_SUCCESS)
        return res;
    /* set 5ms(200hz) sample rate */
    res = icm42607_share_SetSampleRate(ICM42607_SENSOR_TYPE_GYRO,
        5000000, false);
    if (res != ICM42607_SUCCESS)
        return res;
    /* disable sensor - standby mode for gyroscope */
    res = icm42607_share_EnableSensor(ICM42607_SENSOR_TYPE_GYRO, enable,&icm42607_gyro_first_enable );
    if (res != ICM42607_SUCCESS)
        return res;
    /* set power mode - sleep or normal */
    res = icm42607_share_SetPowerMode(ICM42607_SENSOR_TYPE_GYRO, enable);
    if (res != ICM42607_SUCCESS)
        return res;
    GYRO_LOG("icm42607_gyro_init_client OK!\n");
    return ICM42607_SUCCESS;
}

struct gyro_hw *get_cust_gyro(void)
{
    return &gyro_cust;
}

static void icm42607_gyro_power(struct gyro_hw *hw, unsigned int on)
{
    /* nothing to do here, because the power of sensor is always on */
}

static ssize_t chipinfo_show(struct device_driver *ddri, char *buf)
{
    char strbuf[ICM42607_BUFSIZE];

    icm42607_share_ReadChipInfo(strbuf, ICM42607_BUFSIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t sensordata_show(struct device_driver *ddri, char *buf)
{
    char strbuf[ICM42607_BUFSIZE];
    struct i2c_client *client = icm42607_gyro_i2c_client;

    if (NULL == client) {
        GYRO_PR_ERR("i2c client is null!!\n");
        return 0;
    }
    icm42607_gyro_ReadSensorData(client, strbuf, ICM42607_BUFSIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t trace_show(struct device_driver *ddri, char *buf)
{
    ssize_t res = 0;
    struct icm42607_gyro_i2c_data *obj = obj_i2c_data;

    if (obj == NULL) {
        GYRO_PR_ERR("i2c_data obj is null!!\n");
        return 0;
    }
    res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
    return res;
}

static ssize_t trace_store(struct device_driver *ddri,
    const char *buf, size_t count)
{
    int trace;
    struct icm42607_gyro_i2c_data *obj = obj_i2c_data;

    if (obj == NULL) {
        GYRO_PR_ERR("i2c_data obj is null!!\n");
        return 0;
    }
    if (0 == kstrtoint(buf, 16, &trace))
        atomic_set(&obj->trace, trace);
    else
        GYRO_PR_ERR("invalid content: '%s', length = %zu\n", buf, count);
    return count;
}

static ssize_t status_show(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;
    struct icm42607_gyro_i2c_data *obj = obj_i2c_data;

    if (obj == NULL) {
        GYRO_PR_ERR("i2c_data obj is null!!\n");
        return 0;
    }
    if (obj->hw)
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d %d (%d %d)\n",
            obj->hw->i2c_num,
            obj->hw->direction,
            obj->hw->power_id,
            obj->hw->power_vol);
    else
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
    return len;
}

static ssize_t orientation_show(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;
    struct icm42607_gyro_i2c_data *obj = obj_i2c_data;

    if (obj == NULL) {
        GYRO_PR_ERR("i2c_data obj is null!!\n");
        return 0;
    }
    GYRO_LOG("[%s] default direction: %d\n", __func__, obj->hw->direction);
    len = snprintf(buf, PAGE_SIZE, "default direction = %d\n",
        obj->hw->direction);
    return len;
}

static ssize_t orientation_store(struct device_driver *ddri,
    const char *buf, size_t tCount)
{
    int nDirection = 0;
    int res = 0;
    struct icm42607_gyro_i2c_data   *obj = obj_i2c_data;

    if (obj == NULL) {
        GYRO_PR_ERR("i2c_data obj is null!!\n");
        return 0;
    }
    res = kstrtoint(buf, 10, &nDirection);
    if (res != 0) {
        if (hwmsen_get_convert(nDirection, &obj->cvt))
            GYRO_PR_ERR("ERR: fail to set direction\n");
    }
    GYRO_LOG("[%s] set direction: %d\n", __func__, nDirection);
    return tCount;
}

#ifdef ICM42607_SELFTEST
static ssize_t selftest_show(struct device_driver *ddri, char *buf)
{
    struct i2c_client *client = icm42607_gyro_i2c_client;

    if (NULL == client) {
        GYRO_PR_ERR("i2c client is null!!\n");
        return 0;
    }
    return snprintf(buf, 8, "%s\n", selftestRes);
}

static ssize_t selftest_store(struct device_driver *ddri,
    const char *buf, size_t count)
{
    struct i2c_client *client = icm42607_gyro_i2c_client;
    int num;
    int res = 0;

    /* check parameter values to run selftest */
    res = kstrtoint(buf, 10, &num);
    if (res != 0) {
        GYRO_PR_ERR("parse number fail\n");
        return count;
    } else if (num == 0) {
        GYRO_PR_ERR("invalid data count\n");
        return count;
    }
    /* run selftest */
    res = icm42607_gyro_InitSelfTest(client);
    if (icm42607_gyro_DoSelfTest(client) == true) {
        strcpy(selftestRes, "y");
        GYRO_LOG("GYRO SELFTEST : PASS\n");
    } else {
        strcpy(selftestRes, "n");
        GYRO_LOG("GYRO SELFTEST : FAIL\n");
    }
    /* selftest is considered to be called only in factory mode
    in general mode, the condition before selftest will not be recovered
    and sensor will not be in sleep mode */
    res = icm42607_gyro_init_client(client, true);
    return count;
}
#endif

static DRIVER_ATTR_RO(chipinfo);
static DRIVER_ATTR_RO(sensordata);
static DRIVER_ATTR_RW(trace);
static DRIVER_ATTR_RO(status);
static DRIVER_ATTR_RW(orientation);
#ifdef ICM42607_SELFTEST
static DRIVER_ATTR_RW(selftest);
#endif

static struct driver_attribute *icm42607_gyro_attr_list[] = {
    /* chip information - whoami */
    &driver_attr_chipinfo,
    /* dump sensor data */
    &driver_attr_sensordata,
    /* trace log */
    &driver_attr_trace,
    /* chip status */
    &driver_attr_status,
    /* chip orientation information */
    &driver_attr_orientation,
#ifdef ICM42607_SELFTEST
    /* run selftest when store, report selftest result when show */
    &driver_attr_selftest,
#endif
};

static int icm42607_gyro_create_attr(struct device_driver *driver)
{
    int idx;
    int res = 0;
    int num = (int)(sizeof(icm42607_gyro_attr_list)/
        sizeof(icm42607_gyro_attr_list[0]));

    if (driver == NULL)
        return -EINVAL;
    for (idx = 0; idx < num; idx++) {
        res = driver_create_file(driver, icm42607_gyro_attr_list[idx]);
        if (0 != res) {
            GYRO_PR_ERR("driver_create_file (%s) = %d\n",
                icm42607_gyro_attr_list[idx]->attr.name, res);
            break;
        }
    }
    return res;
}

static int icm42607_gyro_delete_attr(struct device_driver *driver)
{
    int idx;
    int res = 0;
    int num = (int)(sizeof(icm42607_gyro_attr_list)/
        sizeof(icm42607_gyro_attr_list[0]));

    if (driver == NULL)
        return -EINVAL;
    for (idx = 0; idx < num; idx++)
        driver_remove_file(driver, icm42607_gyro_attr_list[idx]);
    return res;
}

/*=======================================================================================*/
/* Misc - Factory Mode (IOCTL) Device Driver Section					 */
/*=======================================================================================*/
#if MISC_DEVICE_FACTORY

static int icm42607_gyro_ReadRawData(struct i2c_client * client, char * buf)
{
    int res = 0;
    s16 data[ICM42607_AXIS_NUM] = { 0, 0, 0 };

    res = icm42607_gyro_ReadSensorDataDirect(client, data);
    if (res < 0) {
        GYRO_PR_ERR("read gyroscope raw data  error\n");
        return ICM42607_ERR_BUS;
    }
    /* sensor raw data direct read from sensor register
    no orientation translation, no unit translation */
    sprintf(buf, "%04x %04x %04x",
        data[ICM42607_AXIS_X],
        data[ICM42607_AXIS_Y],
        data[ICM42607_AXIS_Z]);
    return ICM42607_SUCCESS;
}

static int icm42607_gyro_open(struct inode *inode, struct file *file)
{
    file->private_data = icm42607_gyro_i2c_client;

    if (file->private_data == NULL) {
        GYRO_PR_ERR("null pointer!!\n");
        return -EINVAL;
    }
    return nonseekable_open(inode, file);
}

static int icm42607_gyro_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}

static long icm42607_gyro_unlocked_ioctl(struct file *file,
    unsigned int cmd, unsigned long arg)
{
    struct i2c_client *client = (struct i2c_client *)file->private_data;
    char strbuf[ICM42607_BUFSIZE] = {0};
    void __user *data;
    long res = 0;
    int copy_cnt = 0;
    struct SENSOR_DATA sensor_data;
    int smtRes = 0;

    if (_IOC_DIR(cmd) & _IOC_READ)
        res = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        res = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (res) {
        GYRO_PR_ERR("access error: %08X, (%2d, %2d)\n",
            cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
        return -EFAULT;
    }
    switch (cmd) {
    case GYROSCOPE_IOCTL_INIT:
        icm42607_gyro_init_client(client, true);
        break;
    case GYROSCOPE_IOCTL_SMT_DATA:
        data = (void __user *) arg;
        if (data == NULL) {
            res = -EINVAL;
            break;
        }
        GYRO_LOG("ioctl smtRes: %d!\n", smtRes);
        copy_cnt = copy_to_user(data, &smtRes,  sizeof(smtRes));
        if (copy_cnt) {
            res = -EFAULT;
            GYRO_PR_ERR("copy gyro data to user failed!\n");
        }
        GYRO_LOG("copy gyro data to user OK: %d!\n", copy_cnt);
        break;
    case GYROSCOPE_IOCTL_READ_SENSORDATA:
        data = (void __user *) arg;
        if (data == NULL) {
            res = -EINVAL;
            break;
        }
        icm42607_gyro_ReadSensorData(client, strbuf, ICM42607_BUFSIZE);
        if (copy_to_user(data, strbuf, sizeof(strbuf))) {
            res = -EFAULT;
            break;
        }
        break;
    case GYROSCOPE_IOCTL_SET_CALI:
        data = (void __user *)arg;
        if (data == NULL) {
            res = -EINVAL;
            break;
        }
        if (copy_from_user(&sensor_data, data, sizeof(sensor_data)))
            res = -EFAULT;
        else {
            GYRO_LOG("gyro set cali:[%5d %5d %5d]\n",
                sensor_data.x, sensor_data.y, sensor_data.z);
            res = icm42607_gyro_WriteCalibration(client, &sensor_data);
        }
        break;
    case GYROSCOPE_IOCTL_GET_CALI:
        data = (void __user *)arg;
        if (data == NULL) {
            res = -EINVAL;
            break;
        }
        res = icm42607_gyro_ReadCalibration(client, &sensor_data);
        if (copy_to_user(data, &sensor_data, sizeof(sensor_data))) {
            res = -EFAULT;
            break;
        }
        break;
    case GYROSCOPE_IOCTL_CLR_CALI:
        res = icm42607_gyro_ResetCalibration(client);
        break;
    case GYROSCOPE_IOCTL_READ_SENSORDATA_RAW:
        data = (void __user *)arg;
        if (data == NULL) {
            res = -EINVAL;
            break;
        }
        icm42607_gyro_ReadRawData(client, strbuf);
        if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
            res = -EFAULT;
            break;
        }
        break;
    default:
        GYRO_PR_ERR("unknown IOCTL: 0x%08x\n", cmd);
        res = -ENOIOCTLCMD;
    }
    return res;
}

#ifdef CONFIG_COMPAT
static long icm42607_gyro_compat_ioctl(struct file *file,
    unsigned int cmd, unsigned long arg)
{
    long res = 0;
    void __user *arg32 = compat_ptr(arg);

    if(!file->f_op || !file->f_op->unlocked_ioctl){
        GYRO_PR_ERR("compat_ion_ioctl file has no f_op\n");
        GYRO_PR_ERR("or no f_op->unlocked_ioctl.\n");
        return -ENOTTY;
    }
    switch (cmd) {
    case COMPAT_GYROSCOPE_IOCTL_INIT:
    case COMPAT_GYROSCOPE_IOCTL_SMT_DATA:
    case COMPAT_GYROSCOPE_IOCTL_READ_SENSORDATA:
    case COMPAT_GYROSCOPE_IOCTL_SET_CALI:
    case COMPAT_GYROSCOPE_IOCTL_GET_CALI:
    case COMPAT_GYROSCOPE_IOCTL_CLR_CALI:
    case COMPAT_GYROSCOPE_IOCTL_READ_SENSORDATA_RAW:
    case COMPAT_GYROSCOPE_IOCTL_READ_TEMPERATURE:
    case COMPAT_GYROSCOPE_IOCTL_GET_POWER_STATUS:
        if (arg32 == NULL) {
            GYRO_PR_ERR("invalid argument.");
            return -EINVAL;
        }
        res = file->f_op->unlocked_ioctl(file, cmd, (unsigned long)arg32);
        break;
    default:
        GYRO_PR_ERR("%s not supported = 0x%04x\n", __func__, cmd);
        res = -ENOIOCTLCMD;
        break;
    }
    return res;
}
#endif

static const struct file_operations icm42607_gyro_fops = {
    .open = icm42607_gyro_open,
    .release = icm42607_gyro_release,
    .unlocked_ioctl = icm42607_gyro_unlocked_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = icm42607_gyro_compat_ioctl,
#endif
};

static struct miscdevice icm42607_gyro_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "gyroscope",
    .fops = &icm42607_gyro_fops,
};
#else 

/************************* For MTK New factory mode ************************************/
static int icm42607_gyro_factory_do_self_test(void)
{
        return 0;
}

static int icm42607_gyro_factory_get_cali(int32_t data[3])
{
        int err;
		struct icm42607_gyro_i2c_data *priv = obj_i2c_data;
        struct SENSOR_DATA  sensor_data;
		GYRO_FUN();
	
        err = icm42607_gyro_ReadCalibration(priv->client, &sensor_data);
        if (err) {
                GYRO_LOG("icm42607_gyro_ReadCalibration failed!\n");
                return -1;
        }
        data[0] = sensor_data.x ;
        data[1] = sensor_data.y ;
        data[2] = sensor_data.z ;
        return 0;
}

static int icm42607_gyro_factory_set_cali(int32_t data[3])
{
        int err = 0;
		struct icm42607_gyro_i2c_data *priv = obj_i2c_data;
		struct SENSOR_DATA  sensor_data;
        GYRO_FUN();
        GYRO_LOG("gyro set cali:[%5d %5d %5d]\n", data[0], data[1], data[2]);
        sensor_data.x = data[0] ;
        sensor_data.y = data[1] ;
        sensor_data.z = data[2] ;	
        err = icm42607_gyro_WriteCalibration(priv->client, &sensor_data);
        if (err) {
                GYRO_LOG("406xx_gyro_WriteCalibration failed!\n");
                return -1;
        }
        return 0;
}

static int icm42607_gyro_factory_enable_calibration(void)
{
        return 0;
}
static int icm42607_gyro_factory_clear_cali(void)
{
        int err = 0;
        struct icm42607_gyro_i2c_data *priv = obj_i2c_data;
		GYRO_FUN();
        err = icm42607_gyro_ResetCalibration(priv->client);
        if (err) {
                GYRO_LOG("icm42607_gyro_factory_clear_cali failed!\n");
                return -1;
        }
        return 0;
}

static int icm42607_gyro_factory_get_raw_data(int32_t data[3])
{
        
        GYRO_LOG("do not support raw data now!\n");
        return 0;
}

static int icm42607_gyro_factory_get_data(int32_t data[3], int *status)
{
        int res;	
        res =  icm42607_gyro_get_data(&data[0], &data[1], &data[2], status);
		GYRO_LOG("%s  %d %d %d!\n", __func__,data[0], data[1], data[2]);
		return res;

}

static int icm42607_gyro_factory_enable_sensor(bool enabledisable, int64_t sample_periods_ms)
{
        int err;

        err = icm42607_gyro_enable_nodata(enabledisable == true ? 1 : 0);
        if (err) {
                GYRO_LOG("%s enable gyro sensor failed!\n", __func__);
                return -1;
        }
        return 0;
}

static struct gyro_factory_fops icm42607_gyro_factory_fops = {
        .enable_sensor =icm42607_gyro_factory_enable_sensor,
        .get_data = icm42607_gyro_factory_get_data,
        .get_raw_data =icm42607_gyro_factory_get_raw_data,
        .enable_calibration = icm42607_gyro_factory_enable_calibration,
        .clear_cali = icm42607_gyro_factory_clear_cali,
        .set_cali = icm42607_gyro_factory_set_cali,
        .get_cali = icm42607_gyro_factory_get_cali,
        .do_self_test = icm42607_gyro_factory_do_self_test,
};

static struct gyro_factory_public icm42607_gyro_factory_device = {
        .gain = 1,
        .sensitivity = 1,
        .fops = &icm42607_gyro_factory_fops,
};


#endif

static int icm42607_gyro_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{

    return icm42607_gyro_set_delay((u64)samplingPeriodNs);
}

static int icm42607_gyro_flush(void)
{
    return gyro_flush_report();
}

static int icm42607_gyro_open_report_data(int open)
{
    /* nothing to do here for 406xx */
    return 0;
}

static int icm42607_gyro_enable_nodata(int en)
{
    /* if use this type of enable , 
    gsensor only enabled but not report inputEvent to HAL */
    int res = 0;
    bool power = false;
    struct icm42607_gyro_i2c_data *obj = obj_i2c_data;

    if (1 == en) {
        power = true;
		res = icm42607_share_EnableSensor(ICM42607_SENSOR_TYPE_GYRO, power, &icm42607_gyro_first_enable);
		res |= icm42607_share_SetPowerMode(ICM42607_SENSOR_TYPE_GYRO, power);
    } else {
        power = false;
        res = icm42607_share_SetSampleRate(ICM42607_SENSOR_TYPE_GYRO,
            0, false);
        res |= icm42607_share_EnableSensor(ICM42607_SENSOR_TYPE_GYRO, power,&icm42607_gyro_first_enable);
        res |= icm42607_share_SetPowerMode(ICM42607_SENSOR_TYPE_GYRO, power);
    }
    if (res != ICM42607_SUCCESS) {
        GYRO_PR_ERR("icm42607_gyro_SetPowerMode fail!\n");
        return -1;
    }
    atomic_set(&obj->is_enabled, en);
    GYRO_LOG("icm42607_gyro_enable_nodata OK!\n");
    return 0;
}

static int icm42607_gyro_set_delay(u64 ns)
{
    /* power mode setting in case of set_delay
    is called before enable_nodata */
    if(icm42607_share_get_sensor_power(ICM42607_SENSOR_TYPE_GYRO) == false)
        icm42607_share_SetPowerMode(ICM42607_SENSOR_TYPE_GYRO, true);
    icm42607_share_SetSampleRate(ICM42607_SENSOR_TYPE_GYRO, ns, false);

	icm42607_gyro_discardcount = 80 * 1000 *1000 / (unsigned int)(ns);
	if (icm42607_gyro_discardcount <=2)
    	icm42607_gyro_discardcount = 2;

    GYRO_LOG("%s  discardcount num %d\n", __func__, icm42607_gyro_discardcount);
    return 0;
}

static int icm42607_gyro_get_data(int *x , int *y, int *z, int *status)
{
    char buff[ICM42607_BUFSIZE];
    int res = 0;

    /* dps */
    icm42607_gyro_ReadSensorData(obj_i2c_data->client, buff, ICM42607_BUFSIZE);

	 if (res < 0) {
     	GYRO_PR_ERR("read gyro data not ready\n");
        return ICM42607_ERR_BUS;
    }

    res = sscanf(buff, "%x %x %x", x, y, z);
    *status = SENSOR_STATUS_ACCURACY_MEDIUM;
    return 0;
}

static const struct i2c_device_id icm42607_gyro_i2c_id[] =
    {{ICM42607_GYRO_DEV_NAME, 0}, {} };

#ifdef CONFIG_OF
static const struct of_device_id gyro_of_match[] = {
    {.compatible = "mediatek,gyro"},
    {},
};
#endif

/*****************************************************************************
 *** FUNCTION
 *****************************************************************************/
/**
* calculate icm206xx direction according to bma direction
*  add by mid
*/
#ifdef CONFIG_MID_ITEMS_SUPPORT
int icm42607g_direction_calculate(int bma)
{
    int direction = 0;
    switch(bma){
        case 0:
            direction = 0;
            break;
        case 1:
            direction = 1;
            break;
        case 2:
            direction = 2;
            break;
        case 3:
            direction = 3;
            break;
        case 4:
            direction = 6;
            break;
        case 5:
            direction = 5;   //fix
            break;
        case 6:
            direction = 7;
            break;
        case 7:
            direction = 4;
            break;
        default:
            break;
    }
    return direction;
}
#endif
/************************************************************************/

static int icm42607_gyro_i2c_probe(struct i2c_client *client,
    const struct i2c_device_id *id)
{
    struct i2c_client *new_client;
    struct icm42607_gyro_i2c_data *obj;
    struct gyro_control_path ctl = {0};
    struct gyro_data_path data = {0};
    int res = 0;
#ifdef CONFIG_MID_ITEMS_SUPPORT
	int orient=0;
#endif

    obj = kzalloc(sizeof(*obj), GFP_KERNEL);
    if (!obj) {
        res = -ENOMEM;
        goto exit;
    }
	res = get_gyro_dts_func(client->dev.of_node, hw);
        if (res < 0) {
                GYRO_PR_ERR("get gyro dts info fail\n");
                goto exit;
        }
    memset(obj, 0, sizeof(struct icm42607_gyro_i2c_data));
    /* hwmsen_get_convert() depend on the direction value */
    obj->hw = hw;
#ifdef CONFIG_MID_ITEMS_SUPPORT
	orient = item_integer("sensor.accelerometer.bma.orientation",0);
	orient = orient < 0 ? 0 : orient;
	obj->hw->direction = icm42607g_direction_calculate(orient);
	printk("icm42607_gyro_direction bma=%d,icm42607=%d\n",orient,obj->hw->direction);
#endif
    res = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
    if (res) {
        GYRO_PR_ERR("invalid direction: %d\n", obj->hw->direction);
        goto exit;
    }
    GYRO_LOG("direction: %d\n", obj->hw->direction);
	if (0 != obj->hw->addr) {
	#if ICM_AD0_LOW
		client->addr =  0xD0 >> 1;
	#else 
	    client->addr =  0xD2 >> 1;
	#endif
        GYRO_LOG("gyro_use_i2c_addr: %x\n", client->addr);
    }
    obj_i2c_data = obj;
    obj->client = client;
    new_client = obj->client;
    i2c_set_clientdata(new_client, obj);
    atomic_set(&obj->trace, 0);
    atomic_set(&obj->suspend, 0);
    atomic_set(&obj->is_enabled, 0);
    icm42607_gyro_i2c_client = new_client;
    res = icm42607_gyro_init_client(new_client, false);
    if (res)
        goto exit_init_failed;
    /* misc_register() for factory mode, engineer mode and so on */
#if  MISC_DEVICE_FACTORY

    res = misc_register(&icm42607_gyro_device);
    if (res) {
        GYRO_PR_ERR("icm42607_gyro_device misc register failed!\n");
        goto exit_misc_device_register_failed;
    }
#else
	res = gyro_factory_device_register(&icm42607_gyro_factory_device);

        if (res) {
                GYRO_LOG("icm42607_gyro_factory_device register failed!\n");
                goto exit_misc_device_register_failed;
        }

#endif
    /* create platform_driver attribute */
    res = icm42607_gyro_create_attr(
        &(icm42607_gyro_init_info.platform_diver_addr->driver));
    if (res) {
        GYRO_PR_ERR("icm42607_g create attribute err = %d\n", res);
        goto exit_create_attr_failed;
    }
    /* fill the gyro_control_path */
    ctl.is_use_common_factory = false;
    ctl.is_report_input_direct = false;
    ctl.is_support_batch = obj->hw->is_batch_supported;
    ctl.open_report_data = icm42607_gyro_open_report_data;
    ctl.enable_nodata = icm42607_gyro_enable_nodata;
    ctl.set_delay  = icm42607_gyro_set_delay;
	ctl.batch = icm42607_gyro_batch;
    ctl.flush = icm42607_gyro_flush;
    /* register the gyro_control_path */
    res = gyro_register_control_path(&ctl);
    if (res) {
        GYRO_PR_ERR("register gyro control path err\n");
        goto exit_kfree;
    }
    /* fill the gyro_data_path */
    data.get_data = icm42607_gyro_get_data;
    data.vender_div = DEGREE_TO_RAD;
    /* register the gyro_data_path */
    res = gyro_register_data_path(&data);
    if (res) {
        GYRO_PR_ERR("register gyro_data_path fail = %d\n", res);
        goto exit_kfree;
    }
    icm42607_gyro_init_flag = 0;
    GYRO_LOG("%s: OK\n", __func__);
    return 0;

exit_create_attr_failed:
#if MISC_DEVICE_FACTORY
    misc_deregister(&icm42607_gyro_device);
#else
	gyro_factory_device_deregister(&icm42607_gyro_factory_device);
#endif 
exit_misc_device_register_failed:
exit_init_failed:
exit_kfree:
exit:
    kfree(obj);
    obj = NULL;
    icm42607_gyro_init_flag =  -1;
    GYRO_PR_ERR("%s: err = %d\n", __func__, res);
    return res;
}

static int icm42607_gyro_i2c_remove(struct i2c_client *client)
{
    int res = 0;

    res = icm42607_gyro_delete_attr(
        &(icm42607_gyro_init_info.platform_diver_addr->driver));
		
#if MISC_DEVICE_FACTORY
	misc_deregister(&icm42607_gyro_device);
#else
	gyro_factory_device_deregister(&icm42607_gyro_factory_device);
#endif 

    icm42607_gyro_i2c_client = NULL;
    i2c_unregister_device(client);
    kfree(i2c_get_clientdata(client));
    return 0;
}

static int icm42607_gyro_i2c_detect(struct i2c_client *client,
    struct i2c_board_info *info)
{
    strcpy(info->type, ICM42607_GYRO_DEV_NAME);
    return 0;
}

#ifdef CONFIG_PM_SLEEP
static int icm42607_gyro_i2c_suspend(struct device *dev)
{
    int res = 0;
	struct i2c_client *client = to_i2c_client(dev);	
    struct icm42607_gyro_i2c_data *obj = i2c_get_clientdata(client);


        if (obj == NULL) {
            GYRO_PR_ERR("null pointer!!\n");
            return -EINVAL;
        }
        atomic_set(&obj->suspend, 1);
        res = icm42607_share_SetPowerMode(ICM42607_SENSOR_TYPE_GYRO, false);
        if (res < 0) {
            GYRO_PR_ERR("write power control fail!\n");
            return res;
        }
    
    icm42607_gyro_power(obj->hw, 0);
    GYRO_LOG("icm42607_gyro suspend ok\n");
    return res;
}

static int icm42607_gyro_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
    struct icm42607_gyro_i2c_data *obj = i2c_get_clientdata(client);
    int res = 0;

    if (obj == NULL) {
        GYRO_PR_ERR("null pointer!!\n");
        return -EINVAL;
    }
    icm42607_gyro_power(obj->hw, 1);
    if(atomic_read(&obj->is_enabled) == 1) {
        res = icm42607_share_SetPowerMode(ICM42607_SENSOR_TYPE_GYRO, true);
    }
    if (res) {
        GYRO_PR_ERR("resume client fail!!\n");
        return res;
    }
    atomic_set(&obj->suspend, 0);
    GYRO_LOG("icm42607_gyro resume ok\n");
    return 0;
}

static const struct dev_pm_ops icm42607_gyro_pm_ops = {
        SET_SYSTEM_SLEEP_PM_OPS(icm42607_gyro_i2c_suspend, icm42607_gyro_i2c_resume)
};
#endif

static struct i2c_driver icm42607_gyro_i2c_driver = {
    .driver = {
        .name = ICM42607_GYRO_DEV_NAME,
#ifdef CONFIG_OF
        .of_match_table = gyro_of_match,
#endif
#ifdef CONFIG_PM_SLEEP
    	.pm = &icm42607_gyro_pm_ops,
#endif 
    },
    .probe = icm42607_gyro_i2c_probe,
    .remove = icm42607_gyro_i2c_remove,
    .detect = icm42607_gyro_i2c_detect,
   
    .id_table = icm42607_gyro_i2c_id,
};

static int icm42607_gyro_remove(void)
{
    icm42607_gyro_power(hw, 0);
    i2c_del_driver(&icm42607_gyro_i2c_driver);
    return 0;
}

static int icm42607_gyro_local_init(struct platform_device *pdev)
{
    icm42607_gyro_power(hw, 1);
    if (i2c_add_driver(&icm42607_gyro_i2c_driver)) {
        GYRO_PR_ERR("add driver error\n");
        return -1;
    }
    if (-1 == icm42607_gyro_init_flag)
        return -1;
    return 0;
}

static int __init icm42607_gyro_init(void)
{
    GYRO_LOG("%s: OK\n", __func__);
    gyro_driver_add(&icm42607_gyro_init_info);
    return 0;
}

static void __exit icm42607_gyro_exit(void)
{
    /* nothing to do here */
}

module_init(icm42607_gyro_init);
module_exit(icm42607_gyro_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("icm42607 gyroscope driver");
