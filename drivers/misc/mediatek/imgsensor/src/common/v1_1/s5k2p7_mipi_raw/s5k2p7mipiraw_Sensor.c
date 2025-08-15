// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 s5k2p7mipi_Sensor.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 *
 *----------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "s5k2p7mipiraw_Sensor.h"


#define PFX "S5K2P7"
#define LOG_INF_NEW(format, args...)    pr_debug(PFX "[%s] " format, __func__, ##args)
#define LOG_INF printk

#define LOG_1 LOG_INF("S5K2P7,MIPI 4LANE\n")
#define SENSORDB LOG_INF

static DEFINE_SPINLOCK(imgsensor_drv_lock);
static bool bIsLongExposure = KAL_FALSE;

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = S5K2P7_SENSOR_ID,

	.checksum_value = 0x44724ea1,		/* checksum value for Camera Auto Test */

	.pre = {
		.pclk = 560000000,      /*record different mode's pclk*/
		.linelength = 5120,	/*record different mode's linelength*/
		.framelength = 3610,    /*record different mode's framelength*/
		.startx = 0,	/*record different mode's startx of grabwindow*/
		.starty = 0,	/*record different mode's starty of grabwindow*/

		/*record different mode's width of grabwindow*/
		.grabwindow_width = 2320,
		/*record different mode's height of grabwindow*/
		.grabwindow_height = 1744,

		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount
		 * by different scenario
		 */
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
		.mipi_pixel_rate = 590000000,
	},
	.cap = {
		.pclk = 560000000,
		.linelength = 5120,
		.framelength = 3610,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4640,
		.grabwindow_height = 3488,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 590000000,
	},
#if 0
	.cap1 = {
		.pclk = 960000000,	/* record different mode's pclk  */
		.linelength = 10160, /* record different mode's linelength  */
		.framelength = 3148, /* record different mode's framelength */
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4032,
		/* record different mode's height of grabwindow */
		.grabwindow_height = 2272,
		.mipi_data_lp2hs_settle_dc = 85,	/* unit , ns */
		.mipi_pixel_rate = 550400000,
		.max_framerate = 300
	},
#endif
	.normal_video = {
		.pclk = 560000000,
		.linelength = 5120,
		.framelength = 3610,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4640,
		.grabwindow_height = 3488,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 590000000,
	},
	.hs_video = {
		.pclk = 560000000,
		.linelength = 5120,
		.framelength = 3610,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2320,
		.grabwindow_height = 1744,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 595200000,

	},
	.slim_video = {
		.pclk = 560000000,	/*record different mode's pclk*/
		.linelength = 5120,	/*record different mode's linelength*/
		.framelength = 3610,	/*record different mode's framelength*/
		.startx = 0,	/*record different mode's startx of grabwindow*/
		.starty = 0,	/*record different mode's starty of grabwindow*/

		/*record different mode's width of grabwindow*/
		.grabwindow_width = 2320,
		.grabwindow_height = 1744,
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 595200000,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.custom1 = {
		.pclk = 560000000,	/* record different mode's pclk */
		.linelength = 5120,
		.framelength = 3610,
		.startx = 0,					/* record different mode's startx of grabwindow */
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width = 2320,
		.grabwindow_height = 1744,
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 595200000,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.custom2 = {
		.pclk = 560000000,	/* record different mode's pclk */
		.linelength = 5120,
		.framelength = 3610,
		.startx = 0,					/* record different mode's startx of grabwindow */
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width = 2320,
		.grabwindow_height = 1744,
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 595200000,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},

	.margin = 5,                    /*sensor framelength & shutter margin*/
	.min_shutter = 4,               /*min shutter*/

	/*max framelength by sensor register's limitation*/
	.max_frame_length = 0xffff,
	/*shutter delay frame for AE cycle, 2 frame*/
	.ae_shut_delay_frame = 0,
	/*sensor gain delay frame for AE cycle,2 frame*/
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,    /*isp gain delay frame for AE cycle*/
		.frame_time_delay_frame = 2,	/* The delay frame of setting frame length  */
	.ihdr_support = 0,	            /*1, support; 0,not support*/
	.ihdr_le_firstline = 0,         /*1,le first ; 0, se first*/
	.sensor_mode_num = 7,	        /*support sensor mode num*/

	.cap_delay_frame = 2,           /*enter capture delay frame num*/
	.pre_delay_frame = 2,           /*enter preview delay frame num*/
	.video_delay_frame = 3,         /*enter video delay frame num*/
	.hs_video_delay_frame = 3,   /*enter high speed video  delay frame num*/
	.slim_video_delay_frame = 3, /*enter slim video delay frame num*/
	.custom1_delay_frame = 2,
	.custom2_delay_frame = 2,

	.isp_driving_current = ISP_DRIVING_2MA,     /*mclk driving current*/

	/*sensor_interface_type*/
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	/*0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2*/
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	/*0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL*/
	.mipi_settle_delay_mode = 1,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gb,
	.mclk = 24,         /*mclk value, suggest 24 or 26 for 24Mhz or 26Mhz*/
	.mipi_lane_num = SENSOR_MIPI_4_LANE,

	/*record sensor support all write id addr*/
	.i2c_addr_table = {0x20, 0x21, 0x5a, 0xff},
	.i2c_speed = 385,
};


static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,		/*mirrorflip information*/

	/*IMGSENSOR_MODE enum value,record current sensor mode*/
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3D0,			/*current shutter*/
	.gain = 0x100,				/*current gain*/
	.dummy_pixel = 0,			/*current dummypixel*/
	.dummy_line = 0,			/*current dummyline*/

	/*full size current fps : 24fps for PIP, 30fps for Normal or ZSD*/
	.current_fps = 0,
	/*auto flicker enable: KAL_FALSE for disable auto flicker*/
	.autoflicker_en = KAL_FALSE,
	/*test pattern mode or not. KAL_FALSE for in test pattern mode*/
	.test_pattern = KAL_FALSE,
	/*current scenario id*/
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	/*sensor need support LE, SE with HDR feature*/
	.ihdr_mode = KAL_FALSE,

	.i2c_write_id = 0x5A,  /*record current sensor's i2c write id*/
};

/* Sensor output window information*/
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] = 
{   { 4640, 3488, 0, 0, 4640, 3488, 2320,  1744, 0000, 0000, 2320, 1744, 0, 0, 2320, 1744}, /*Preview*/
	{ 4640, 3488, 0, 0, 4640, 3488, 4640,  3488, 0000, 0000, 4640, 3488, 0, 0, 4640, 3488}, /*capture*/
	{ 4640, 3488, 0, 0, 4640, 3488, 4640,  3488, 0000, 0000, 4640, 3488, 0, 0, 4640, 3488}, /*video*/
	{ 4640, 3488, 0, 0, 4640, 3488, 2320,  1744, 0000, 0000, 2320, 1744, 0, 0, 2320, 1744}, /*hight speed video*/
	{ 4640, 3488, 0, 0, 4640, 3488, 2320,  1744, 0000, 0000, 2320, 1744, 0, 0, 2320, 1744}, /*slim video*/	
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info =
{
    .i4OffsetX = 0,
    .i4OffsetY = 0,
    .i4PitchX = 0,
    .i4PitchY = 0,
    .i4PairNum = 0,
    .i4SubBlkW = 0,
    .i4SubBlkH = 0,
    .i4PosL = {{0, 0} },
    .i4PosR = {{0, 0} },
    .i4BlockNumX = 0,
    .i4BlockNumY = 0,
    .i4LeFirst = 0,
    .i4Crop = {
        {0, 0}, {0, 0}, {0, 376}, {0, 0}, {0, 0},
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
    },
	.iMirrorFlip = 0,
};


/* Preview mode setting */
static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3] = {
	{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
	0x00, 0x2B, 0x0910, 0x06D0, 0x01, 0x00, 0x0000, 0x0000,
	0x02, 0x30, 0x00B4, 0x0360, 0x03, 0x00, 0x0000, 0x0000},
	/* Video mode setting */
	{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
	0x00, 0x2B, 0x1220, 0x0DA0, 0x01, 0x00, 0x0000, 0x0000,
	0x02, 0x30, 0x00B4, 0x0360, 0x03, 0x00, 0x0000, 0x0000},
	/* Capture mode setting */
	{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
	0x00, 0x2B, 0x1220, 0x0DA0, 0x01, 0x00, 0x0000, 0x0000,
	0x02, 0x30, 0x00B4, 0x0360, 0x03, 0x00, 0x0000, 0x0000}
	};
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
/*extern void kdSetI2CSpeed(u16 i2cSpeed);*/
/*extern bool read_2l9_eeprom( kal_uint16 addr, BYTE* data, kal_uint32 size);*/

static kal_uint16 read_cmos_sensor(kal_uint16 addr)
{
    kal_uint16 get_byte = 0;
    char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };

   // kdSetI2CSpeed(imgsensor_info.i2c_speed);
    iReadRegI2C(pu_send_cmd , 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);
	//iReadRegI2CTiming(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id,imgsensor_info.i2c_speed);
    return get_byte;
}

static kal_uint16 read_cmos_sensor_8(kal_uint32 addr)
{
    kal_uint16 get_byte = 0;
    char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

   // kdSetI2CSpeed(imgsensor_info.i2c_speed);
    iReadRegI2C(pu_send_cmd, 2, (u8 *)&get_byte, 1, imgsensor.i2c_write_id);
	//iReadRegI2CTiming(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id,imgsensor_info.i2c_speed);
    return get_byte;
}

static void write_cmos_sensor_8(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

    //kdSetI2CSpeed(imgsensor_info.i2c_speed);
    //iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
	iWriteRegI2CTiming(pu_send_cmd, 3, imgsensor.i2c_write_id,imgsensor_info.i2c_speed);
}

static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) , (char)(para >> 8), (char)(para & 0xFF)};

    //kdSetI2CSpeed(imgsensor_info.i2c_speed);
    iWriteRegI2C(pusendcmd , 4, imgsensor.i2c_write_id);
	//iWriteRegI2CTiming(pusendcmd, 4, imgsensor.i2c_write_id,imgsensor_info.i2c_speed);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d\n", imgsensor.dummy_line, imgsensor.dummy_pixel);

	 write_cmos_sensor(0x0340, imgsensor.frame_length);
	 write_cmos_sensor(0x0342, imgsensor.line_length);
}


static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;

	LOG_INF("framerate = %d, min framelength should enable(%d) \n", framerate, min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
	{
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}	/*	set_max_framerate  */

static kal_uint32 streaming_control(kal_bool enable)
{
	int timeout = (10000 / imgsensor.current_fps) + 1;
	int i = 0;
	int framecnt = 0;

	LOG_INF("streaming_enable(0= Sw Standby,1= streaming): %d\n", enable);
	if (enable) {
		write_cmos_sensor_8(0x0100, 0X01);
		mDELAY(10);
	} else {
		write_cmos_sensor_8(0x0100, 0x00);
		for (i = 0; i < timeout; i++) {
			mDELAY(5);
			framecnt = read_cmos_sensor_8(0x0005);
			if ( framecnt == 0xFF) {
				LOG_INF(" Stream Off OK at i=%d.\n", i);
				return ERROR_NONE;
			}
		}
		LOG_INF("Stream Off Fail! framecnt= %d.\n", framecnt);
	}
	return ERROR_NONE;
}

static void write_shutter(kal_uint32 shutter)
{
	kal_uint16 realtime_fps = 0;

	 //0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure
	 //AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here.

	// OV Recommend Solution
	// if shutter bigger than frame_length, should extend frame length first
	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);
	else {
		// Extend frame length
	        write_cmos_sensor(0x0340, imgsensor.frame_length);
	}
		} else {
		/* Extend frame length*/
		write_cmos_sensor(0x0340, imgsensor.frame_length);

	}
	
	if(shutter > 65530) {  //linetime=10160/960000000<< maxshutter=3023622-line=32s
		/*enter long exposure mode */
		kal_uint32 exposure_time;
		kal_uint16 new_framelength;
		kal_uint16 long_shutter;
		kal_uint16 shift = 0;
		kal_uint16 pow_shift = 0;
		LOG_INF("enter long exposure mode\n");
		LOG_INF("Calc long exposure  +\n");
		
		bIsLongExposure = KAL_TRUE;
		
		//exposure_time = shutter*imgsensor_info.cap.linelength/960000;//ms
		exposure_time = shutter/100*1016/960;//ms
		if(exposure_time <= 1000) {
			shift = 0x0100;
			pow_shift = 2;
		} else if (exposure_time > 1000 && exposure_time <= 2000) {
			shift = 0x0200;
			pow_shift = 4;
		} else if (exposure_time > 2000 && exposure_time <= 4000) {
			shift = 0x0300;
			pow_shift = 8;
		} else if (exposure_time > 4000 && exposure_time <= 8000) {
			shift = 0x0400;
			pow_shift = 16;
		} else if (exposure_time > 8000 && exposure_time <= 16000) {
			shift = 0x0500;
			pow_shift = 32;
		} else if (exposure_time > 16000) {
			shift = 0x0600;
			pow_shift = 64;
		}

		//long_shutter = (exposure_time*pclk-256)/lineleght/64 = (shutter*linelength-256)/linelength/pow_shift
        long_shutter = shutter/pow_shift;
		new_framelength = long_shutter+7;
        LOG_INF("Calc long exposure_time=%dms,long_shutter=%d, framelength=%d.\n", exposure_time,long_shutter, new_framelength);
		LOG_INF("Calc long exposure  -\n");

		/*streaming_control(KAL_FALSE);*/
		
		write_cmos_sensor_8(0x0100, 0x00); /*stream off*/
		write_cmos_sensor(0x0342, imgsensor_info.cap.linelength);
		write_cmos_sensor(0x0340, new_framelength);
		write_cmos_sensor(0x0202, long_shutter);
		write_cmos_sensor(0x0702, shift);
		write_cmos_sensor(0x0704, shift);
		/*streaming_control(KAL_TRUE);*/
		
		write_cmos_sensor_8(0x0100, 0x01);  /*stream on*/
		
		LOG_INF("long exposure  stream on-\n");
	} else {

	// Update Shutter
	    write_cmos_sensor(0x0202, shutter);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
	}
	//LOG_INF("frame_length = %d ", frame_length);

}	/*	write_shutter  */



/*************************************************************************
* FUNCTION
*	set_shutter
*
* DESCRIPTION
*	This function set e-shutter of sensor to change exposure time.
*
* PARAMETERS
*	iShutter : exposured lines
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;
	//kal_uint16 realtime_fps = 0;
	/* kal_uint32 frame_length = 0; */
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

		write_shutter(shutter);
}	/*	set_shutter */

/*	write_shutter  */
static void set_shutter_frame_length(kal_uint16 shutter, kal_uint16 frame_length)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	spin_lock(&imgsensor_drv_lock);
	/* Change frame time */
	if (frame_length > 1)
		dummy_line = frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + dummy_line;

	/*  */


	//spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
		/* Extend frame length */
		write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
	}

	/* Update Shutter */
	write_cmos_sensor(0X0202, shutter & 0xFFFF);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);

}

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0000;

    reg_gain = gain/2;

	return (kal_uint16)reg_gain;
}

/*************************************************************************
* FUNCTION
*	set_gain
*
* DESCRIPTION
*	This function is to set global gain to sensor.
*
* PARAMETERS
*	iGain : sensor global gain(base: 0x40)
*
* RETURNS
*	the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{


	kal_uint16 reg_gain;

	LOG_INF("set_gain %d\n", gain);
	if (gain < BASEGAIN || gain > 16 * BASEGAIN) {
		LOG_INF("Error gain setting");
		if (gain < BASEGAIN)
			gain = BASEGAIN;
		else if (gain > 16 * BASEGAIN)
			gain = 16 * BASEGAIN;
	}

    reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor(0x0204,reg_gain);
	return gain;
}	/*	set_gain  */
#if 0
/* ihdr_write_shutter_gain not support for s5k3l6 */
static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
	LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n", le, se, gain);
	if (imgsensor.ihdr_en) {

		spin_lock(&imgsensor_drv_lock);
			if (le > imgsensor.min_frame_length - imgsensor_info.margin)
				imgsensor.frame_length = le + imgsensor_info.margin;
			else
				imgsensor.frame_length = imgsensor.min_frame_length;
			if (imgsensor.frame_length > imgsensor_info.max_frame_length)
				imgsensor.frame_length = imgsensor_info.max_frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (le < imgsensor_info.min_shutter) le = imgsensor_info.min_shutter;
			if (se < imgsensor_info.min_shutter) se = imgsensor_info.min_shutter;


		/* Extend frame length first */
    /*  */

		set_gain(gain);
	}

}
#endif


static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d\n", image_mirror);

	/********************************************************
	   *
	   *   0x3820[2] ISP Vertical flip
	   *   0x3820[1] Sensor Vertical flip
	   *
	   *   0x3821[2] ISP Horizontal mirror
	   *   0x3821[1] Sensor Horizontal mirror
	   *
	   *   ISP and Sensor flip or mirror register bit should be the same!!
	   *
	   ********************************************************/
//	spin_lock(&imgsensor_drv_lock);
   // imgsensor.mirror = image_mirror;
    //spin_unlock(&imgsensor_drv_lock);
	switch (image_mirror) {
		case IMAGE_NORMAL:

	        write_cmos_sensor_8(0x0101,0x00);   /* Gr*/
	        break;

	    case IMAGE_H_MIRROR:

	        write_cmos_sensor_8(0x0101,0x01);
	        break;

	    case IMAGE_V_MIRROR:

	        write_cmos_sensor_8(0x0101,0x02);
	        break;

	    case IMAGE_HV_MIRROR:

	        write_cmos_sensor_8(0x0101,0x03);/*Gb*/
			break;
		default:
			LOG_INF("Error image_mirror setting\n");
	}

}

/*************************************************************************
* FUNCTION
*	night_mode
*
* DESCRIPTION
*	This function night mode of sensor.
*
* PARAMETERS
*	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
#if 0
static void night_mode(kal_bool enable)
{
/*No Need to implement this function*/
}	/*	night_mode	*/
#endif

static void sensor_init(void)
	{
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x0100, 0x0000);

	write_cmos_sensor(0x6214, 0x7971);
	write_cmos_sensor(0x6218, 0x7150);

	// Global
	write_cmos_sensor(0xF51A, 0x0076);
	write_cmos_sensor(0x3256, 0x0001);
	write_cmos_sensor(0x3274, 0x00E2);
	write_cmos_sensor(0x35AA, 0x000F);
	write_cmos_sensor(0x35CE, 0x0011);
	write_cmos_sensor(0x3276, 0x00B9);
	write_cmos_sensor(0x35AC, 0x000F);
	write_cmos_sensor(0x35D0, 0x0011);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x1C50);
	write_cmos_sensor(0x6F12, 0x1600);
	write_cmos_sensor(0x602A, 0x3684);
	write_cmos_sensor(0x6F12, 0xFFFF);
	write_cmos_sensor(0x6F12, 0xFFFF);
	write_cmos_sensor(0x602A, 0x368C);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x3092, 0x0200);
	write_cmos_sensor(0x3088, 0x0101);
	write_cmos_sensor(0x602A, 0x1B30);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x1B32);
	write_cmos_sensor(0x6F12, 0x0140);
	write_cmos_sensor(0x602A, 0x1B34);
	write_cmos_sensor(0x6F12, 0x0140);
	write_cmos_sensor(0x602A, 0x1B36);
	write_cmos_sensor(0x6F12, 0x64C6);
	write_cmos_sensor(0x602A, 0x1B38);
	write_cmos_sensor(0x6F12, 0x64A6);
	write_cmos_sensor(0x602A, 0x1B3A);
	write_cmos_sensor(0x6F12, 0xF442);
	write_cmos_sensor(0x3816, 0x0180);
	write_cmos_sensor(0x3818, 0x0180);
	write_cmos_sensor(0x37DE, 0x1304);
	write_cmos_sensor(0xF47C, 0x0019);   

	}	/*	sensor_init  */	/*	sensor_init  */


static void preview_setting(void)
{
	write_cmos_sensor(0xf440, 0x402F);
	write_cmos_sensor(0x37F6, 0x0011);
	write_cmos_sensor(0x319A, 0x0000);
	write_cmos_sensor(0x319C, 0x0130);
	write_cmos_sensor(0x3056, 0x0100);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x1266);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1268);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1BB0);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x0B0E, 0x0100);
	write_cmos_sensor(0x30D4, 0x0001);
	write_cmos_sensor(0x30D8, 0x0000);
	write_cmos_sensor(0xB138, 0x0000);
	write_cmos_sensor(0x31B0, 0x0004);
	write_cmos_sensor(0x31D2, 0x0100);
	write_cmos_sensor(0x0340, 0x0E1A);
	write_cmos_sensor(0x0342, 0x1400);
	write_cmos_sensor(0x0344, 0x0008);
	write_cmos_sensor(0x0346, 0x0000);
	write_cmos_sensor(0x0348, 0x1227);
	write_cmos_sensor(0x034A, 0x0D9F);
	write_cmos_sensor(0x034C, 0x0910);
	write_cmos_sensor(0x034E, 0x06D0);
	write_cmos_sensor(0x0900, 0x0112);
	write_cmos_sensor(0x0380, 0x0001);
	write_cmos_sensor(0x0382, 0x0001);
	write_cmos_sensor(0x0384, 0x0001);
	write_cmos_sensor(0x0386, 0x0003);
	write_cmos_sensor(0x0400, 0x0001);
	write_cmos_sensor(0x0404, 0x0020);
	write_cmos_sensor(0x0408, 0x0000);
	write_cmos_sensor(0x040A, 0x0000);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x0300, 0x0003);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0304, 0x0006);
	write_cmos_sensor(0x0306, 0x0069);
	write_cmos_sensor(0x030C, 0x0004);
	write_cmos_sensor(0x030E, 0x007C);
	write_cmos_sensor(0x300A, 0x0000);
	write_cmos_sensor(0x0200, 0x0200);
	write_cmos_sensor(0x0202, 0x0E00);
	write_cmos_sensor(0x0204, 0x0020);
	write_cmos_sensor(0x37C0, 0x0002);
	write_cmos_sensor(0x37C2, 0x0103);
	write_cmos_sensor(0x3004, 0x0003);
	write_cmos_sensor(0x0114, 0x0300);
	write_cmos_sensor(0x304C, 0x0300);
	write_cmos_sensor(0x602A, 0x13A4);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x6F12, 0x0003);
	
	}	/*	preview_setting  */


static void capture_setting(kal_uint16 currefps)
	{
	
	write_cmos_sensor(0xf440, 0x402F);
	write_cmos_sensor(0x37F6, 0x0001);
	write_cmos_sensor(0x319A, 0x0100);
	write_cmos_sensor(0x319C, 0x0130);
	write_cmos_sensor(0x3056, 0x0100);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x1266);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1268);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1BB0);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x0B0E, 0x0100);
	write_cmos_sensor(0x30D4, 0x0001);
	write_cmos_sensor(0x30D8, 0x0100);
	write_cmos_sensor(0xB138, 0x0000);
	write_cmos_sensor(0x31B0, 0x0008);
	write_cmos_sensor(0x31D2, 0x0100);
	write_cmos_sensor(0x0340, 0x0E1A);
	write_cmos_sensor(0x0342, 0x1400);
	write_cmos_sensor(0x0344, 0x0008);
	write_cmos_sensor(0x0346, 0x0000);
	write_cmos_sensor(0x0348, 0x1227);
	write_cmos_sensor(0x034A, 0x0D9F);
	write_cmos_sensor(0x034C, 0x1220);
	write_cmos_sensor(0x034E, 0x0DA0);
	write_cmos_sensor(0x0900, 0x0011);
	write_cmos_sensor(0x0380, 0x0001);
	write_cmos_sensor(0x0382, 0x0001);
	write_cmos_sensor(0x0384, 0x0001);
	write_cmos_sensor(0x0386, 0x0001);
	write_cmos_sensor(0x0400, 0x0000);
	write_cmos_sensor(0x0404, 0x0010);
	write_cmos_sensor(0x0408, 0x0000);
	write_cmos_sensor(0x040A, 0x0000);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x0300, 0x0003);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0304, 0x0006);
	write_cmos_sensor(0x0306, 0x0069);
	write_cmos_sensor(0x030C, 0x0004);
	write_cmos_sensor(0x030E, 0x007C);
	write_cmos_sensor(0x300A, 0x0000);
	write_cmos_sensor(0x0200, 0x0200);
	write_cmos_sensor(0x0202, 0x0E00);
	write_cmos_sensor(0x0204, 0x0020);
	write_cmos_sensor(0x37C0, 0x0002);
	write_cmos_sensor(0x37C2, 0x0103);
	write_cmos_sensor(0x3004, 0x0003);
	write_cmos_sensor(0x0114, 0x0300);
	write_cmos_sensor(0x304C, 0x0300);
	write_cmos_sensor(0x602A, 0x13A4);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x6F12, 0x0000);
	
	}



static void normal_video_setting(kal_uint16 currefps)
	{
		
		write_cmos_sensor(0xf440, 0x402F);
		write_cmos_sensor(0x37F6, 0x0001);
		write_cmos_sensor(0x319A, 0x0100);
		write_cmos_sensor(0x319C, 0x0130);
		write_cmos_sensor(0x3056, 0x0100);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x1266);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x1268);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x1BB0);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x0B0E, 0x0100);
		write_cmos_sensor(0x30D4, 0x0001);
		write_cmos_sensor(0x30D8, 0x0100);
		write_cmos_sensor(0xB138, 0x0000);
		write_cmos_sensor(0x31B0, 0x0008);
		write_cmos_sensor(0x31D2, 0x0100);
		write_cmos_sensor(0x0340, 0x0E1A);
		write_cmos_sensor(0x0342, 0x1400);
		write_cmos_sensor(0x0344, 0x0008);
		write_cmos_sensor(0x0346, 0x0000);
		write_cmos_sensor(0x0348, 0x1227);
		write_cmos_sensor(0x034A, 0x0D9F);
		write_cmos_sensor(0x034C, 0x1220);
		write_cmos_sensor(0x034E, 0x0DA0);
		write_cmos_sensor(0x0900, 0x0011);
		write_cmos_sensor(0x0380, 0x0001);
		write_cmos_sensor(0x0382, 0x0001);
		write_cmos_sensor(0x0384, 0x0001);
		write_cmos_sensor(0x0386, 0x0001);
		write_cmos_sensor(0x0400, 0x0000);
		write_cmos_sensor(0x0404, 0x0010);
		write_cmos_sensor(0x0408, 0x0000);
		write_cmos_sensor(0x040A, 0x0000);
		write_cmos_sensor(0x0136, 0x1800);
		write_cmos_sensor(0x0300, 0x0003);
		write_cmos_sensor(0x0302, 0x0001);
		write_cmos_sensor(0x0304, 0x0006);
		write_cmos_sensor(0x0306, 0x0069);
		write_cmos_sensor(0x030C, 0x0004);
		write_cmos_sensor(0x030E, 0x007C);
		write_cmos_sensor(0x300A, 0x0000);
		write_cmos_sensor(0x0200, 0x0200);
		write_cmos_sensor(0x0202, 0x0E00);
		write_cmos_sensor(0x0204, 0x0020);
		write_cmos_sensor(0x37C0, 0x0002);
		write_cmos_sensor(0x37C2, 0x0103);
		write_cmos_sensor(0x3004, 0x0003);
		write_cmos_sensor(0x0114, 0x0300);
		write_cmos_sensor(0x304C, 0x0300);
		write_cmos_sensor(0x602A, 0x13A4);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x0000);
		
		}


static void hs_video_setting(void)
	{
		write_cmos_sensor(0xf440, 0x402F);
		write_cmos_sensor(0x37F6, 0x0011);
		write_cmos_sensor(0x319A, 0x0000);
		write_cmos_sensor(0x319C, 0x0130);
		write_cmos_sensor(0x3056, 0x0100);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x1266);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x1268);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x1BB0);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x0B0E, 0x0100);
		write_cmos_sensor(0x30D4, 0x0001);
		write_cmos_sensor(0x30D8, 0x0000);
		write_cmos_sensor(0xB138, 0x0000);
		write_cmos_sensor(0x31B0, 0x0004);
		write_cmos_sensor(0x31D2, 0x0100);
		write_cmos_sensor(0x0340, 0x0E1A);
		write_cmos_sensor(0x0342, 0x1400);
		write_cmos_sensor(0x0344, 0x0008);
		write_cmos_sensor(0x0346, 0x0000);
		write_cmos_sensor(0x0348, 0x1227);
		write_cmos_sensor(0x034A, 0x0D9F);
		write_cmos_sensor(0x034C, 0x0910);
		write_cmos_sensor(0x034E, 0x06D0);
		write_cmos_sensor(0x0900, 0x0112);
		write_cmos_sensor(0x0380, 0x0001);
		write_cmos_sensor(0x0382, 0x0001);
		write_cmos_sensor(0x0384, 0x0001);
		write_cmos_sensor(0x0386, 0x0003);
		write_cmos_sensor(0x0400, 0x0001);
		write_cmos_sensor(0x0404, 0x0020);
		write_cmos_sensor(0x0408, 0x0000);
		write_cmos_sensor(0x040A, 0x0000);
		write_cmos_sensor(0x0136, 0x1800);
		write_cmos_sensor(0x0300, 0x0003);
		write_cmos_sensor(0x0302, 0x0001);
		write_cmos_sensor(0x0304, 0x0006);
		write_cmos_sensor(0x0306, 0x0069);
		write_cmos_sensor(0x030C, 0x0004);
		write_cmos_sensor(0x030E, 0x007C);
		write_cmos_sensor(0x300A, 0x0000);
		write_cmos_sensor(0x0200, 0x0200);
		write_cmos_sensor(0x0202, 0x0E00);
		write_cmos_sensor(0x0204, 0x0020);
		write_cmos_sensor(0x37C0, 0x0002);
		write_cmos_sensor(0x37C2, 0x0103);
		write_cmos_sensor(0x3004, 0x0003);
		write_cmos_sensor(0x0114, 0x0300);
		write_cmos_sensor(0x304C, 0x0300);
		write_cmos_sensor(0x602A, 0x13A4);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x0003);
		
		}



static void slim_video_setting(void)
{
	LOG_INF("E\n");
	preview_setting();
}


static kal_uint32 return_sensor_id(void)
{
	return ((read_cmos_sensor(0x0000) << 8) | read_cmos_sensor(0x0001));
}

/*************************************************************************
* FUNCTION
*	get_imgsensor_id
*
* DESCRIPTION
*	This function get the sensor ID
*
* PARAMETERS
*	*sensorID : return the sensor ID
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/


static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
    kal_uint8 i = 0;
    kal_uint8 retry = 2;

    /* sensor have two i2c address 0x5b 0x5a & 0x21 0x20, we should detect the module used i2c address */
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
	spin_lock(&imgsensor_drv_lock);
	imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
	spin_unlock(&imgsensor_drv_lock);
	do {
	    *sensor_id = return_sensor_id();

	    if (*sensor_id == imgsensor_info.sensor_id) {

				LOG_INF("i2c write id: 0x%x, ReadOut sensor id: 0x%x, imgsensor_info.sensor_id:0x%x.\n", imgsensor.i2c_write_id, *sensor_id, imgsensor_info.sensor_id);
		return ERROR_NONE;
	    }
			LOG_INF("Read sensor id fail, i2c write id: 0x%x, ReadOut sensor id: 0x%x, imgsensor_info.sensor_id:0x%x.\n", imgsensor.i2c_write_id, *sensor_id, imgsensor_info.sensor_id);
	    retry--;
	} while (retry > 0);
	i++;
        retry = 2;
    }
    if (*sensor_id != imgsensor_info.sensor_id) {
	/* if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF */
	*sensor_id = 0xFFFFFFFF;
	return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*	open
*
* DESCRIPTION
*	This function initialize the registers of CMOS sensor
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
//static u8 first_open_flag = 0 ;
static kal_uint32 open(void)
{
    /* const kal_uint8 i2c_addr[] = {IMGSENSOR_WRITE_ID_1, IMGSENSOR_WRITE_ID_2}; */
    kal_uint8 i = 0;
    kal_uint8 retry = 2;
    kal_uint32 sensor_id = 0;
	LOG_1;
//added by shibicheng on 20180830, for use n6600, system will open 3times when boot
/*printk("slichee s5k3l6 open boot_mode = %d\n",get_boot_mode());
if (get_boot_mode() == NORMAL_BOOT) {
	if( first_open_flag<3 ){
		first_open_flag++;
	    return ERROR_SENSOR_CONNECT_FAIL;
	}
}*/
//ended by shibicheng on 20180830, for use n6600, system will open 3times when boot
    /* sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address */
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
	spin_lock(&imgsensor_drv_lock);
	imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
	spin_unlock(&imgsensor_drv_lock);
	do {
	    sensor_id = return_sensor_id();
	    if (sensor_id == imgsensor_info.sensor_id) {
                LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, sensor_id);
		break;
	    }
            LOG_INF("Read sensor id fail, id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, sensor_id);
			retry--;
		} while (retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}
    if (imgsensor_info.sensor_id != sensor_id){
		return ERROR_SENSOR_CONNECT_FAIL;
    }
	/* initail sequence write in  */
	sensor_init();

	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x3D0;
	imgsensor.gain = 0x100;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_mode = 0;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}	/*	open  */



/*************************************************************************
 * FUNCTION
 *	close
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 close(void)
{
	LOG_INF("E\n");

	/*No Need to implement this function*/

	return ERROR_NONE;
}	/*	close  */


/*************************************************************************
 * FUNCTION
 * preview
 *
 * DESCRIPTION
 *	This function start the sensor preview.
 *
 * PARAMETERS
 *  *image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
				  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	preview_setting();
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/*	preview   */

/*************************************************************************
 * FUNCTION
 *	capture
 *
 * DESCRIPTION
 *	This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
				  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
	LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap.max_framerate/10);
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	 capture_setting(imgsensor.current_fps);
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
				  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	normal_video_setting(imgsensor.current_fps);
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
				  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	/*imgsensor.video_mode = KAL_TRUE;*/
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	/*imgsensor.current_fps = 300;*/
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/*	hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
				  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	/*imgsensor.video_mode = KAL_TRUE;*/
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	/*imgsensor.current_fps = 300;*/
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();
	set_mirror_flip(imgsensor.mirror);
	return ERROR_NONE;
}

static kal_uint32 Custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	imgsensor.pclk = imgsensor_info.custom1.pclk;
	imgsensor.line_length = imgsensor_info.custom1.linelength;
	imgsensor.frame_length = imgsensor_info.custom1.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps);/*using caputre_setting*/
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/*	custom1   */

static kal_uint32 Custom2(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM2;
	imgsensor.pclk = imgsensor_info.custom2.pclk;
	imgsensor.line_length = imgsensor_info.custom2.linelength;
	imgsensor.frame_length = imgsensor_info.custom2.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom2.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	preview_setting();/*using preview setting*/
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/*	custom2   */


static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("E\n");
	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


	sensor_resolution->SensorHighSpeedVideoWidth	 = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight	 = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth	 = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight	 = imgsensor_info.slim_video.grabwindow_height;
	sensor_resolution->SensorCustom1Width = imgsensor_info.custom1.grabwindow_width;
	sensor_resolution->SensorCustom1Height = imgsensor_info.custom1.grabwindow_height;

	sensor_resolution->SensorCustom2Width = imgsensor_info.custom2.grabwindow_width;
	sensor_resolution->SensorCustom2Height = imgsensor_info.custom2.grabwindow_height;

	return ERROR_NONE;
}	/*	get_resolution	*/

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
				  MSDK_SENSOR_INFO_STRUCT *sensor_info,
				  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;

	/* not use */
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
	/* inverse with datasheet*/
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;
	sensor_info->Custom1DelayFrame = imgsensor_info.custom1_delay_frame;
	sensor_info->Custom2DelayFrame = imgsensor_info.custom2_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;		 /* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->FrameTimeDelayFrame = imgsensor_info.frame_time_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	sensor_info->PDAF_Support = 5; /* PDAF_SUPPORT_CAMSV_DUALPD*/
	/* 0: NO PDAF, 1: PDAF Raw Data mode,
	 * 2:PDAF VC mode(Full),
	 * 3:PDAF VC mode(Binning)
	 */
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;  /* 0 is default 1x*/
	sensor_info->SensorHightSampling = 0;	/* 0 is default 1x*/
	sensor_info->SensorPacketECCOrder = 1;

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;

			break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.cap.mipi_data_lp2hs_settle_dc;

			break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:

	    sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx;
	    sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;

			break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;

			break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
	    sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx;
	    sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;

			break;
	    case MSDK_SCENARIO_ID_CUSTOM1:
	        sensor_info->SensorGrabStartX = imgsensor_info.custom1.startx; 
	        sensor_info->SensorGrabStartY = imgsensor_info.custom1.starty;

	        sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc; 

	        break;
	    case MSDK_SCENARIO_ID_CUSTOM2:
	        sensor_info->SensorGrabStartX = imgsensor_info.custom2.startx; 
	        sensor_info->SensorGrabStartY = imgsensor_info.custom2.starty;

	        sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom2.mipi_data_lp2hs_settle_dc; 

			break;
	default:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			break;
	}

	return ERROR_NONE;
}	/* get_info  */


static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			  MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.current_scenario_id = scenario_id;
	spin_unlock(&imgsensor_drv_lock);
	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			preview(image_window, sensor_config_data);
			break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			capture(image_window, sensor_config_data);
			break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			normal_video(image_window, sensor_config_data);
			break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			hs_video(image_window, sensor_config_data);
			break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
			slim_video(image_window, sensor_config_data);
			break;
	    case MSDK_SCENARIO_ID_CUSTOM1:
	        Custom1(image_window, sensor_config_data);
	        break;
	    case MSDK_SCENARIO_ID_CUSTOM2:
	        Custom2(image_window, sensor_config_data);
	        break;
	default:
			LOG_INF("Error ScenarioId setting");
			preview(image_window, sensor_config_data);
			return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	LOG_INF("framerate = %d\n ", framerate);
	/* SetVideoMode Function should fix framerate*/
	if (framerate == 0)
		/* Dynamic frame rate*/
		return ERROR_NONE;
	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d\n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable) /*enable auto flicker*/
		imgsensor.autoflicker_en = KAL_TRUE;
	else /*Cancel Auto flick*/
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(
	enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//if (imgsensor.frame_length > imgsensor.shutter)
				set_dummy();
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if (framerate == 0)
				return ERROR_NONE;
			frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ? (frame_length - imgsensor_info.normal_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;

		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);

		//if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
		LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",framerate,imgsensor_info.cap.max_framerate/10);
			frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);

			//if (imgsensor.frame_length > imgsensor.shutter)
				set_dummy();
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//if (imgsensor.frame_length > imgsensor.shutter)
				set_dummy();
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//if (imgsensor.frame_length > imgsensor.shutter)
				set_dummy();
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			frame_length = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.custom1.framelength) ? (frame_length - imgsensor_info.custom1.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			set_dummy();
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			frame_length = imgsensor_info.custom2.pclk / framerate * 10 / imgsensor_info.custom2.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.custom2.framelength) ? (frame_length - imgsensor_info.custom2.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.custom2.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			set_dummy();
			break;
	
		default:  /* coding with  preview scenario by default */
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//if (imgsensor.frame_length > imgsensor.shutter)
				set_dummy();
			LOG_INF("error scenario_id = %d, we use preview scenario\n", scenario_id);
		break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(
	enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*framerate = imgsensor_info.pre.max_framerate;
			break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*framerate = imgsensor_info.normal_video.max_framerate;
			break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*framerate = imgsensor_info.cap.max_framerate;
			break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*framerate = imgsensor_info.hs_video.max_framerate;
			break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*framerate = imgsensor_info.slim_video.max_framerate;
			break;
	case MSDK_SCENARIO_ID_CUSTOM1:
	    *framerate = imgsensor_info.custom1.max_framerate;
	    break;
	case MSDK_SCENARIO_ID_CUSTOM2:
	    *framerate = imgsensor_info.custom2.max_framerate;
	    break;
	default:
			break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	if (enable) {
	    /* 0x5E00[8]: 1 enable,  0 disable*/
	    /* 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK*/
		write_cmos_sensor(0x0600, 0x0002);
	} else {
	    /* 0x5E00[8]: 1 enable,  0 disable*/
	    /* 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK*/
		write_cmos_sensor(0x0600, 0x0000);
	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}
static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
				 UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;

	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
	struct SENSOR_VC_INFO_STRUCT *pvcinfo;
	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;


	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

    LOG_INF("feature_id = %d\n", feature_id);
	switch (feature_id) {
	case SENSOR_FEATURE_GET_PERIOD:
			*feature_return_para_16++ = imgsensor.line_length;
			*feature_return_para_16 = imgsensor.frame_length;
			*feature_para_len = 4;
			break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		pr_debug("imgsensor.pclk = %d,current_fps = %d\n",
			imgsensor.pclk, imgsensor.current_fps);
			*feature_return_para_32 = imgsensor.pclk;
			*feature_para_len = 4;
			break;
	case SENSOR_FEATURE_SET_ESHUTTER:
		set_shutter(*feature_data);
			break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
	    /*night_mode((BOOL) *feature_data); no need to implement this mode*/
			break;
	case SENSOR_FEATURE_SET_GAIN:
		set_gain((UINT16) *feature_data);
			break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
			break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			break;
	case SENSOR_FEATURE_SET_REGISTER:
			write_cmos_sensor(
			    sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			break;
	case SENSOR_FEATURE_GET_REGISTER:
			sensor_reg_data->RegData =
				read_cmos_sensor(sensor_reg_data->RegAddr);
			break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/* get the lens driver ID from EEPROM or
		 * just return LENS_DRIVER_ID_DO_NOT_CARE
		 */

		/* if EEPROM does not exist in camera module.*/
		*feature_return_para_32 = LENS_DRIVER_ID_DO_NOT_CARE;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_VIDEO_MODE:
		set_video_mode(*feature_data);
		break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
			get_imgsensor_id(feature_return_para_32);
			break;
	case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
		set_auto_flicker_mode(
			(BOOL)*feature_data_16, *(feature_data_16+1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario(
		  (enum MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data+1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario(
			(enum MSDK_SCENARIO_ID_ENUM)*(feature_data),
			(MUINT32 *)(uintptr_t)(*(feature_data+1)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL)*feature_data);
		break;

		/*for factory mode auto testing*/
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
			*feature_return_para_32 = imgsensor_info.checksum_value;
			*feature_para_len = 4;
			break;

	case SENSOR_FEATURE_SET_FRAMERATE:
	    LOG_INF("current fps :%d\n", (UINT32)*feature_data);
		spin_lock(&imgsensor_drv_lock);
	    imgsensor.current_fps = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;

/*	case SENSOR_FEATURE_SET_HDR:
		pr_debug("hdr enable :%d\n", *feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.ihdr_mode = (UINT8)*feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
	    break;*/

	case SENSOR_FEATURE_GET_CROP_INFO:
	    /* LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%lld\n", (UINT32)*feature_data); */

		wininfo =
	    (struct SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));

		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[1],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[2],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[3],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[4],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
	    case MSDK_SCENARIO_ID_CUSTOM1:
	        memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[5],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
	            break;
	    case MSDK_SCENARIO_ID_CUSTOM2:
	        memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[6],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
	            break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[0],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
/*	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		pr_debug("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16)*feature_data,
			(UINT16)*(feature_data+1), (UINT16)*(feature_data+2));
*/
		/* ihdr_write_shutter_gain((UINT16)*feature_data,
		 * (UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
		 */
		break;
	case SENSOR_FEATURE_GET_PDAF_INFO:
				LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%lld\n", *feature_data);
		PDAFinfo =
		  (struct SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG: /*full*/
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW: /*2x2 binning*/
		case MSDK_SCENARIO_ID_CUSTOM1:
		case MSDK_SCENARIO_ID_CUSTOM2:
			memcpy(
				(void *)PDAFinfo,
				(void *)&imgsensor_pd_info,
				sizeof(struct SET_PD_BLOCK_INFO_T));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		default:
				break;
		}
		break;
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%llu\n", *feature_data);
		/*PDAF capacity enable or not*/
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			/* video & capture use same setting*/
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			/*need to check*/
				*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
				break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		}
		break;
/*
	case SENSOR_FEATURE_GET_PDAF_INFO:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%llu\n", *feature_data);
		PDAFinfo= (struct SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));

		break;
*/
    case SENSOR_FEATURE_GET_VC_INFO:
        LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n", (UINT16)*feature_data);
        pvcinfo = (struct SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
		switch (*feature_data_32) {
		
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		
		case MSDK_SCENARIO_ID_CUSTOM1:
		    memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[1], sizeof(struct SENSOR_VC_INFO_STRUCT));
		    break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		    memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[2], sizeof(struct SENSOR_VC_INFO_STRUCT));
		    break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		
		case MSDK_SCENARIO_ID_CUSTOM2:
		default:
		    memcpy((void *)pvcinfo, (void *) &SENSOR_VC_INFO[0], sizeof(struct SENSOR_VC_INFO_STRUCT));
		    break;
		}
		break;  
		
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:/*lzl*/
			set_shutter_frame_length((UINT16)*feature_data,
					(UINT16)*(feature_data+1));
			break;
	/******************** STREAMING RESUME/SUSPEND *********/
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n", *feature_data);
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
        {
               kal_uint32 rate;
 
		switch (*feature_data) {
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                       rate = imgsensor_info.cap.mipi_pixel_rate;
                       break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				//memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,sizeof(struct SET_PD_BLOCK_INFO_T));
				rate = imgsensor_info.normal_video.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
                       rate = imgsensor_info.hs_video.mipi_pixel_rate;
                       break;
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
                       rate = imgsensor_info.slim_video.mipi_pixel_rate;
                       break;
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
                       rate = imgsensor_info.pre.mipi_pixel_rate;
                       break;
			default:
                       rate = 0;
				break;
		}
               *(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = rate;
        }
		break;
/*	case SENSOR_FEATURE_SET_PDAF:
		pr_debug("PDAF mode :%d\n", *feature_data_16);
		imgsensor.pdaf_mode = *feature_data_16;
		break;*/
	
/*	case SENSOR_FEATURE_GET_PDAF_DATA:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA\n");
		S5K3L6_read_eeprom((kal_uint16 )(*feature_data),(char*)(uintptr_t)(*(feature_data+1)),(kal_uint32)(*(feature_data+2)));
		break;*/
		
        /******************** PDAF END   <<< *********/
	default:
			break;
	}

	return ERROR_NONE;
}	/*	feature_control()  */

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 S5K2P7_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}
