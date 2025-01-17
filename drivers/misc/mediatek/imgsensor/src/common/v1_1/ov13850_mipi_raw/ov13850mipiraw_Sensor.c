/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/*****************************************************************************
 *
 * Filename:
 * ---------
 *     OV13850mipi_Sensor.c
 *
 * Project:
 * --------
 *     ALPS
 *
 * Description:
 * ------------
 *     Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
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

#include "ov13850mipiraw_Sensor.h"
#define LOG_INF(format, args...)    \
	pr_debug(PFX "[%s] " format, __func__, ##args)

#define MULTI_WRITE 1
static DEFINE_SPINLOCK(imgsensor_drv_lock);

int m_bpc_select;
/* sensor otp
 * attention
 * Here comment, just wait for otp implementation
 * extern void otp_cali(unsigned char writeid);
 */

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = OV13850_SENSOR_ID,	/* record sensor id defined in Kd_imgsensor.h */

    .checksum_value = 0xbde6b5f8,//0xf86cfdf4,        //checksum value for Camera Auto Test

    .pre = {
        .pclk = 240000000,                //record different mode's pclk
        .linelength = 2400,                //record different mode's linelength
        .framelength = 3328,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 2096,        //record different mode's width of grabwindow
        .grabwindow_height = 1552,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 100,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
		.mipi_pixel_rate = 216000000,
        .max_framerate = 300,
    },
    .cap = {
        .pclk = 480000000,
        .linelength = 4800,
        .framelength = 3328,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 4192, //4192,
        .grabwindow_height = 3104,//3104,
        .mipi_data_lp2hs_settle_dc = 100,//unit , ns
		.mipi_pixel_rate = 432000000,
        .max_framerate = 300,

    },
    .cap1 = {                            //capture for PIP 24fps relative information, capture1 mode must use same framelength, linelength with Capture mode for shutter calculate
        .pclk = 480000000,
        .linelength = 6000,
        .framelength = 3328,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 4192,
        .grabwindow_height = 3104,
        .mipi_data_lp2hs_settle_dc = 100,//unit , ns
		.mipi_pixel_rate = 432000000,
        .max_framerate = 240,    //less than 13M(include 13M),cap1 max framerate is 24fps,16M max framerate is 20fps, 20M max framerate is 15fps
    },
    .normal_video = {
        .pclk = 480000000,
        .linelength = 4800,
        .framelength = 3328,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 4192,
        .grabwindow_height = 3104,
        .mipi_data_lp2hs_settle_dc = 100,//unit , ns
		.mipi_pixel_rate = 432000000,
        .max_framerate = 300,
    },
    .hs_video = {
        .pclk = 240000000,
        .linelength = 2968,
        .framelength = 674,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 640,
        .grabwindow_height = 480,
        .mipi_data_lp2hs_settle_dc = 100,//unit , ns
		.mipi_pixel_rate = 432000000,
        .max_framerate = 1200,
    },
    .slim_video = {
        .pclk = 240000000,
        .linelength = 9600,//2400,
        .framelength = 834,//3328,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 1280,
        .grabwindow_height = 720,
        .mipi_data_lp2hs_settle_dc = 100,//unit , ns
		.mipi_pixel_rate = 432000000,
        .max_framerate = 300,

    },
    .margin = 8,            //sensor framelength & shutter margin
    .min_shutter = 0x3,        //min shutter
    .max_frame_length = 0x7fff,//max framelength by sensor register's limitation
    .ae_shut_delay_frame = 0,    //shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
    .ae_sensor_gain_delay_frame = 0,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
    .ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
	.frame_time_delay_frame = 1,	/* The delay frame of setting frame length  */
    .ihdr_support = 0,      //1, support; 0,not support
    .ihdr_le_firstline = 0,  //1,le first ; 0, se first
    .sensor_mode_num = 5,      //support sensor mode num

    .cap_delay_frame = 3,        //enter capture delay frame num
    .pre_delay_frame = 2,         //enter preview delay frame num
    .video_delay_frame = 2,        //enter video delay frame num
    .hs_video_delay_frame = 2,    //enter high speed video  delay frame num
    .slim_video_delay_frame = 2,//enter slim video delay frame num

    .isp_driving_current = ISP_DRIVING_6MA, //mclk driving current
    .sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
    .sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,//sensor output first pixel color
    .mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
    .mipi_lane_num = SENSOR_MIPI_4_LANE,//mipi lane num
    .i2c_addr_table = {0x20, 0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
	/* record sensor support all write id addr, only supprt 4must end with 0xff */
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,
	.sensor_mode = IMGSENSOR_MODE_INIT,
	/* IMGSENSOR_MODE enum value,record current sensor mode,
	 * such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	 */
	.shutter = 0x3D0,	/* current shutter */
	.gain = 0x100,		/* current gain */
	.dummy_pixel = 0,	/* current dummypixel */
	.dummy_line = 0,	/* current dummyline */
	.current_fps = 300,	/* full size current fps : 24fps for PIP, 30fps for Normal or ZSD */
	.autoflicker_en = KAL_FALSE,
	/* auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker */
	.test_pattern = KAL_FALSE,
	/* test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output */
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,	/* current scenario id */
	.ihdr_en = 0,		/* sensor need support LE, SE with HDR feature */
	.i2c_write_id = 0x20,
};

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] = 
{{ 4208, 3120, 0, 0, 4208, 3120, 2104, 1560, 0, 0, 2104, 1560, 0, 0, 2096, 1552}, // Preview
{ 4208, 3120, 0, 0, 4208, 3120, 4208, 3120, 0, 0, 4208, 3120, 0, 0, 4192, 3104}, // capture
{ 4208, 3120, 0, 0, 4208, 3120, 4208, 3120, 0, 0, 4208, 3120, 0, 0, 4192, 3104}, // video
{ 4208, 3120, 0, 0, 4208, 3120, 2104, 1560, 0, 0, 1056, 594, 0, 0, 640, 480}, //hight speed video
{ 4208, 3120, 0, 0, 4208, 3120, 1296, 730, 0, 0, 1296, 730, 0, 0, 1280, 720},
};				/* slim video */

/* add from OV13850F_v1_mirror_on_flip_off_mtk2.0.1.ini */
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 72,
	.i4OffsetY = 64,
	.i4PitchX = 32,
	.i4PitchY = 32,
	.i4PairNum = 8,
	.i4SubBlkW = 16,
	.i4SubBlkH = 8,		/* need check xb.pang */
	.i4PosL = {
	    {86, 70}, {102, 70}, {78, 74}, {94, 74}, {86, 86}, {102, 86}, {78, 90}, {94, 90} },
	.i4PosR = {
	    {86, 66}, {102, 66}, {78, 78}, {94, 78}, {86, 82}, {102, 82}, {78, 94}, {94, 94} },
};

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;

    char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
    iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);
    printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
    iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
    printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
}

static void set_dummy(void)
{
	/* check */
   // LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
    printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    /* you can set dummy by imgsensor.dummy_line and imgsensor.dummy_pixel, or you can set dummy by imgsensor.frame_length and imgsensor.line_length */
    write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
    write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
    write_cmos_sensor(0x380c, imgsensor.line_length >> 8);
    write_cmos_sensor(0x380d, imgsensor.line_length & 0xFF);
}

static kal_uint32 return_sensor_id(void)
{
    printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    return ((read_cmos_sensor(0x300A) << 8) | read_cmos_sensor(0x300B));
}

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	/* kal_int16 dummy_line; */
	kal_uint32 frame_length = imgsensor.frame_length;

   // LOG_INF("framerate = %d, min_framelength_en = %d \n", framerate,min_framelength_en);
printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	/* LOG_INF("frame_length =%d\n", frame_length); */
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length =
	    (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	/* LOG_INF("framerate = %d, min_framelength_en=%d\n", framerate,min_framelength_en); */
	set_dummy();
}



/*************************************************************************
* FUNCTION
*    set_shutter
*
* DESCRIPTION
*    This function set e-shutter of sensor to change exposure time.
*
* PARAMETERS
*    iShutter : exposured lines
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	/* kal_uint32 frame_length = 0; */
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
//	LOG_INF("Enter! shutter =%d \n", shutter);
printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);

	/* write_shutter(shutter); */
	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */

	/* OV Recommend Solution */
	/* if shutter bigger than frame_length, should extend frame length first */
	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter =
	    (shutter >
	     (imgsensor_info.max_frame_length -
	      imgsensor_info.margin)) ? (imgsensor_info.max_frame_length -
					 imgsensor_info.margin) : shutter;

	/* frame_length and shutter should be an even number. */
	shutter = (shutter >> 1) << 1;
	imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8) & 0x7f);
			write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
		}
	} else {
		/* Extend frame length */
		/* write_cmos_sensor(0x380e, imgsensor.frame_length >> 8); */
		/* write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF); */
		write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8) & 0x7f);
		write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
	}

	/* Update Shutter */
	/* write_cmos_sensor(0x3502, (shutter << 4) & 0xFF); */
	/* write_cmos_sensor(0x3501, (shutter >> 4) & 0xFF); */
	/* write_cmos_sensor(0x3500, (shutter >> 12) & 0x0F); */

	write_cmos_sensor(0x3500, (shutter >> 12) & 0x0F);
	write_cmos_sensor(0x3501, (shutter >> 4) & 0xFF);
	write_cmos_sensor(0x3502, (shutter << 4) & 0xF0);
//	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);

}				/*    set_shutter */


static void set_shutter_frame_length(kal_uint16 shutter, kal_uint16 frame_length)
{

	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
//	LOG_INF("Enter set_shutter_frame_length! shutter =%d\n", shutter);


	/* write_shutter(shutter); */
	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */

	/* OV Recommend Solution */
	/* if shutter bigger than frame_length, should extend frame length first */
	spin_lock(&imgsensor_drv_lock);
	/* Change frame time */
	if (frame_length > 1)
		dummy_line = frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + dummy_line;

	/*  */
	if (shutter > imgsensor.frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter =
	    (shutter >
	     (imgsensor_info.max_frame_length -
	      imgsensor_info.margin)) ? (imgsensor_info.max_frame_length -
					 imgsensor_info.margin) : shutter;

	/* frame_length and shutter should be an even number. */
	shutter = (shutter >> 1) << 1;
	imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8) & 0x7f);
			write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
		}
	} else {
		/* Extend frame length */
		/* write_cmos_sensor(0x380e, imgsensor.frame_length >> 8); */
		/* write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF); */
		write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8) & 0x7f);
		write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
	}

	/* Update Shutter */
	/* write_cmos_sensor(0x3502, (shutter << 4) & 0xFF); */
	/* write_cmos_sensor(0x3501, (shutter >> 4) & 0xFF); */
	/* write_cmos_sensor(0x3500, (shutter >> 12) & 0x0F); */

	write_cmos_sensor(0x3500, (shutter >> 12) & 0x0F);
	write_cmos_sensor(0x3501, (shutter >> 4) & 0xFF);
	write_cmos_sensor(0x3502, (shutter << 4) & 0xF0);
//	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);

}

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 iReg = 0x0000;

		iReg = gain*16/BASEGAIN;
		if(iReg < 0x10)
		{
			iReg = 0x10;
		}
		if(iReg > 0xf8)
		{
			iReg = 0xf8;
		}
	return iReg;		/* ov13850f. sensorGlobalGain */
}

/*************************************************************************
* FUNCTION
*    set_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    iGain : sensor global gain(base: 0x40)
*
* RETURNS
*    the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{
    kal_uint16 reg_gain;
 printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
		reg_gain = gain2reg(gain);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.gain = reg_gain; 
		spin_unlock(&imgsensor_drv_lock);
//		LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);
	
		write_cmos_sensor(0x350a, reg_gain >> 8);
		write_cmos_sensor(0x350b, reg_gain & 0xFF);

	return gain;
}				/*    set_gain  */

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
    //printk("le:0x%x, se:0x%x, gain:0x%x\n",le,se,gain);
    printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    if (imgsensor.ihdr_en) {

		spin_lock(&imgsensor_drv_lock);
		if (le > imgsensor.min_frame_length - imgsensor_info.margin)
			imgsensor.frame_length = le + imgsensor_info.margin;
		else
			imgsensor.frame_length = imgsensor.min_frame_length;
		if (imgsensor.frame_length > imgsensor_info.max_frame_length)
			imgsensor.frame_length = imgsensor_info.max_frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (le < imgsensor_info.min_shutter)
			le = imgsensor_info.min_shutter;
		if (se < imgsensor_info.min_shutter)
			se = imgsensor_info.min_shutter;


		/* Extend frame length first */
		write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);

		write_cmos_sensor(0x3502, (le << 4) & 0xFF);
		write_cmos_sensor(0x3501, (le >> 4) & 0xFF);
		write_cmos_sensor(0x3500, (le >> 12) & 0x0F);

        write_cmos_sensor(0x3508, (se << 4) & 0xFF);
        write_cmos_sensor(0x3507, (se >> 4) & 0xFF);
        write_cmos_sensor(0x3506, (se >> 12) & 0x0F);

        set_gain(gain);
    }

}

#if 0
static void set_mirror_flip(kal_uint8 image_mirror)
{
	printk("image_mirror = %d\n", image_mirror);

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
}
#endif

/*************************************************************************
* FUNCTION
*    night_mode
*
* DESCRIPTION
*    This function night mode of sensor.
*
* PARAMETERS
*    bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void night_mode(kal_bool enable)
{
/*No Need to implement this function*/
}				/*    night_mode    */

static void sensor_init(void)
{
//	printk("E\n");
printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
		write_cmos_sensor(0x0103, 0x01);
		write_cmos_sensor(0x0102, 0x01);//for debug xb.pang color line
		write_cmos_sensor(0x0300, 0x01);
		write_cmos_sensor(0x0301, 0x00);
		write_cmos_sensor(0x0302, 0x28);
		write_cmos_sensor(0x0303, 0x00);
		write_cmos_sensor(0x030a, 0x00);
		write_cmos_sensor(0x300f, 0x11);
		write_cmos_sensor(0x3010, 0x01);
		write_cmos_sensor(0x3011, 0x76);
		write_cmos_sensor(0x3012, 0x41);
		write_cmos_sensor(0x3013, 0x12);
		write_cmos_sensor(0x3014, 0x11);
		write_cmos_sensor(0x301f, 0x03);
		write_cmos_sensor(0x3106, 0x00);
		write_cmos_sensor(0x3210, 0x47);
		write_cmos_sensor(0x3500, 0x00);
		write_cmos_sensor(0x3501, 0x60);
		write_cmos_sensor(0x3502, 0x00);
		write_cmos_sensor(0x3506, 0x00);
		write_cmos_sensor(0x3507, 0x02);
		write_cmos_sensor(0x3508, 0x00);
		write_cmos_sensor(0x350a, 0x00);
		write_cmos_sensor(0x350b, 0x80);
		write_cmos_sensor(0x350e, 0x00);
		write_cmos_sensor(0x350f, 0x10);
		write_cmos_sensor(0x351a, 0x00);
		write_cmos_sensor(0x351b, 0x10);
		write_cmos_sensor(0x351c, 0x00);
		write_cmos_sensor(0x351d, 0x20);
		write_cmos_sensor(0x351e, 0x00);
		write_cmos_sensor(0x351f, 0x40);
		write_cmos_sensor(0x3520, 0x00);
		write_cmos_sensor(0x3521, 0x80);
		write_cmos_sensor(0x3600, 0xc0);
		write_cmos_sensor(0x3601, 0xfc);
		write_cmos_sensor(0x3602, 0x02);
		write_cmos_sensor(0x3603, 0x78);
		write_cmos_sensor(0x3604, 0xb1);
		write_cmos_sensor(0x3605, 0xb5);
		write_cmos_sensor(0x3606, 0x73);
		write_cmos_sensor(0x3607, 0x07);
		write_cmos_sensor(0x3609, 0x40);
		write_cmos_sensor(0x360a, 0x30);
		write_cmos_sensor(0x360b, 0x91);
		write_cmos_sensor(0x360c, 0x49);
		write_cmos_sensor(0x360f, 0x02);
		write_cmos_sensor(0x3611, 0x10);
		write_cmos_sensor(0x3612, 0x27);
		write_cmos_sensor(0x3613, 0x33);
		write_cmos_sensor(0x3615, 0x0c);
		write_cmos_sensor(0x3616, 0x0e);
		write_cmos_sensor(0x3641, 0x02);
		write_cmos_sensor(0x3660, 0x82);
		write_cmos_sensor(0x3668, 0x54);
		write_cmos_sensor(0x3669, 0x00);
		write_cmos_sensor(0x366a, 0x3f);
		write_cmos_sensor(0x3667, 0xa0);
		write_cmos_sensor(0x3702, 0x40);
		write_cmos_sensor(0x3703, 0x44);
		write_cmos_sensor(0x3704, 0x2c);
		write_cmos_sensor(0x3705, 0x01);
		write_cmos_sensor(0x3706, 0x15);
		write_cmos_sensor(0x3707, 0x44);
		write_cmos_sensor(0x3708, 0x3c);
		write_cmos_sensor(0x3709, 0x1f);
		write_cmos_sensor(0x370a, 0x27);
		write_cmos_sensor(0x370b, 0x3c);
		write_cmos_sensor(0x3720, 0x55);
		write_cmos_sensor(0x3722, 0x84);
		write_cmos_sensor(0x3728, 0x40);
		write_cmos_sensor(0x372a, 0x00);
		write_cmos_sensor(0x372b, 0x02);
		write_cmos_sensor(0x372e, 0x22);
		write_cmos_sensor(0x372f, 0x90);
		write_cmos_sensor(0x3730, 0x00);
		write_cmos_sensor(0x3731, 0x00);
		write_cmos_sensor(0x3732, 0x00);
		write_cmos_sensor(0x3733, 0x00);
		write_cmos_sensor(0x3710, 0x28);
		write_cmos_sensor(0x3716, 0x03);
		write_cmos_sensor(0x3718, 0x10);
		write_cmos_sensor(0x3719, 0x08);
		write_cmos_sensor(0x371a, 0x08);
		write_cmos_sensor(0x371c, 0xfc);
		write_cmos_sensor(0x3748, 0x00);
		write_cmos_sensor(0x3760, 0x13);
		write_cmos_sensor(0x3761, 0x33);
		write_cmos_sensor(0x3762, 0x86);
		write_cmos_sensor(0x3763, 0x16);
		write_cmos_sensor(0x3767, 0x24);
		write_cmos_sensor(0x3768, 0x06);
		write_cmos_sensor(0x3769, 0x45);
		write_cmos_sensor(0x376c, 0x23);
		write_cmos_sensor(0x376f, 0x80);
		write_cmos_sensor(0x3773, 0x06);
		write_cmos_sensor(0x3d84, 0x00);
		write_cmos_sensor(0x3d85, 0x17);
		write_cmos_sensor(0x3d8c, 0x73);
		write_cmos_sensor(0x3d8d, 0xbf);
		write_cmos_sensor(0x3800, 0x00);
		write_cmos_sensor(0x3801, 0x08);
		write_cmos_sensor(0x3802, 0x00);
		write_cmos_sensor(0x3803, 0x04);
		write_cmos_sensor(0x3804, 0x10);
		write_cmos_sensor(0x3805, 0x97);
		write_cmos_sensor(0x3806, 0x0c);
		write_cmos_sensor(0x3807, 0x4b);
		write_cmos_sensor(0x3808, 0x08);
		write_cmos_sensor(0x3809, 0x40);
		write_cmos_sensor(0x380a, 0x06);
		write_cmos_sensor(0x380b, 0x20);
		write_cmos_sensor(0x380c, 0x09);
		write_cmos_sensor(0x380d, 0x60);
		write_cmos_sensor(0x380e, 0x06);
		write_cmos_sensor(0x380f, 0x80);
		write_cmos_sensor(0x3810, 0x00);
		write_cmos_sensor(0x3811, 0x04);
		write_cmos_sensor(0x3812, 0x00);
		write_cmos_sensor(0x3813, 0x02);
		write_cmos_sensor(0x3814, 0x31);
		write_cmos_sensor(0x3815, 0x31);
		write_cmos_sensor(0x3820, 0x02);
		write_cmos_sensor(0x3821, 0x06);
		write_cmos_sensor(0x3823, 0x00);
		write_cmos_sensor(0x3826, 0x00);
		write_cmos_sensor(0x3827, 0x02);
		write_cmos_sensor(0x3834, 0x00);
		write_cmos_sensor(0x3835, 0x1c);
		write_cmos_sensor(0x3836, 0x08);
		write_cmos_sensor(0x3837, 0x02);
		write_cmos_sensor(0x4000, 0xf1);
		write_cmos_sensor(0x4001, 0x00);
		write_cmos_sensor(0x400b, 0x0c);
		write_cmos_sensor(0x4011, 0x00);
		write_cmos_sensor(0x401a, 0x00);
		write_cmos_sensor(0x401b, 0x00);
		write_cmos_sensor(0x401c, 0x00);
		write_cmos_sensor(0x401d, 0x00);
		write_cmos_sensor(0x4020, 0x00);
		write_cmos_sensor(0x4021, 0xe4);
		write_cmos_sensor(0x4022, 0x04);
		write_cmos_sensor(0x4023, 0xd7);
		write_cmos_sensor(0x4024, 0x05);
		write_cmos_sensor(0x4025, 0xbc);
		write_cmos_sensor(0x4026, 0x05);
		write_cmos_sensor(0x4027, 0xbf);
		write_cmos_sensor(0x4028, 0x00);
		write_cmos_sensor(0x4029, 0x02);
		write_cmos_sensor(0x402a, 0x04);
		write_cmos_sensor(0x402b, 0x08);
		write_cmos_sensor(0x402c, 0x02);
		write_cmos_sensor(0x402d, 0x02);
		write_cmos_sensor(0x402e, 0x0c);
		write_cmos_sensor(0x402f, 0x08);
		write_cmos_sensor(0x403d, 0x2c);
		write_cmos_sensor(0x403f, 0x7f);
		write_cmos_sensor(0x4041, 0x07);
		write_cmos_sensor(0x4500, 0x82);
		write_cmos_sensor(0x4501, 0x3c);
		write_cmos_sensor(0x458b, 0x00);
		write_cmos_sensor(0x459c, 0x00);
		write_cmos_sensor(0x459d, 0x00);
		write_cmos_sensor(0x459e, 0x00);
		write_cmos_sensor(0x4601, 0x83);
		write_cmos_sensor(0x4602, 0x22);
		write_cmos_sensor(0x4603, 0x01);
		write_cmos_sensor(0x4837, 0x19);
		write_cmos_sensor(0x4d00, 0x04);
		write_cmos_sensor(0x4d01, 0x42);
		write_cmos_sensor(0x4d02, 0xd1);
		write_cmos_sensor(0x4d03, 0x90);
		write_cmos_sensor(0x4d04, 0x66);
		write_cmos_sensor(0x4d05, 0x65);
		write_cmos_sensor(0x4d0b, 0x00);
		write_cmos_sensor(0x5000, 0x0e);//0e
		write_cmos_sensor(0x5001, 0x01);
		write_cmos_sensor(0x5002, 0x07);
		write_cmos_sensor(0x5013, 0x40);
		write_cmos_sensor(0x501c, 0x00);
		write_cmos_sensor(0x501d, 0x10);
		write_cmos_sensor(0x510f, 0xfc);
		write_cmos_sensor(0x5110, 0xf0);
		write_cmos_sensor(0x5111, 0x10);
		write_cmos_sensor(0x536d, 0x02);
		write_cmos_sensor(0x536e, 0x67);
		write_cmos_sensor(0x536f, 0x01);
		write_cmos_sensor(0x5370, 0x4c);
		write_cmos_sensor(0x5400, 0x00);
		write_cmos_sensor(0x5400, 0x00);
		write_cmos_sensor(0x5401, 0x61);
		write_cmos_sensor(0x5402, 0x00);
		write_cmos_sensor(0x5403, 0x00);
		write_cmos_sensor(0x5404, 0x00);
		write_cmos_sensor(0x5405, 0x40);
		write_cmos_sensor(0x540c, 0x05);
		write_cmos_sensor(0x5501, 0x00);
		write_cmos_sensor(0x5b00, 0x00);
		write_cmos_sensor(0x5b01, 0x00);
		write_cmos_sensor(0x5b02, 0x01);
		write_cmos_sensor(0x5b03, 0xff);
		write_cmos_sensor(0x5b04, 0x02);
		write_cmos_sensor(0x5b05, 0x6c);
		write_cmos_sensor(0x5b09, 0x02);
		write_cmos_sensor(0x5e00, 0x00);
		write_cmos_sensor(0x5e10, 0x1c);
		write_cmos_sensor(0x0100, 0x01);//
}    /*    sensor_init  */

static void preview_setting(void)
{
    printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
		write_cmos_sensor(0x0100, 0x00);//
		write_cmos_sensor(0x0300, 0x01);
		write_cmos_sensor(0x0302, 0x28);
		write_cmos_sensor(0x0303, 0x00);
		write_cmos_sensor(0x3612, 0x27);
		write_cmos_sensor(0x3501, 0x60);
		write_cmos_sensor(0x3702, 0x40);
		write_cmos_sensor(0x370a, 0x27);
		write_cmos_sensor(0x3718, 0x1c);//add new20140801
		write_cmos_sensor(0x372a, 0x00);
		write_cmos_sensor(0x372f, 0xa0);//0x90
		write_cmos_sensor(0x3800, 0x0);
		write_cmos_sensor(0x3801, 0x00);//0x08
		write_cmos_sensor(0x3802, 0x00);
		write_cmos_sensor(0x3803, 0x04);
		write_cmos_sensor(0x3804, 0x10);
		write_cmos_sensor(0x3805, 0x9f);//0x97
		write_cmos_sensor(0x3806, 0x0c);
		write_cmos_sensor(0x3807, 0x4b);
		write_cmos_sensor(0x3808, 0x08);
		write_cmos_sensor(0x3809, 0x40);
		write_cmos_sensor(0x380a, 0x06);
		write_cmos_sensor(0x380b, 0x20);
		write_cmos_sensor(0x380c, ((imgsensor_info.pre.linelength >> 8) & 0xFF)); // hts = 2688
	    write_cmos_sensor(0x380d, (imgsensor_info.pre.linelength & 0xFF));        // hts
	    write_cmos_sensor(0x380e, ((imgsensor_info.pre.framelength >> 8) & 0xFF));  // vts = 1984
	    write_cmos_sensor(0x380f, (imgsensor_info.pre.framelength & 0xFF));         // vts
		write_cmos_sensor(0x3811, 0x08);//0x04
		write_cmos_sensor(0x3813, 0x02);
		write_cmos_sensor(0x3814, 0x31);
		write_cmos_sensor(0x3815, 0x31);
		write_cmos_sensor(0x3820, 0x01);//0x02
		write_cmos_sensor(0x3821, 0x06);
		write_cmos_sensor(0x3834, 0x00);
		write_cmos_sensor(0x3836, 0x08);
		write_cmos_sensor(0x3837, 0x02);
		write_cmos_sensor(0x4020, 0x00);
		write_cmos_sensor(0x4021, 0xe4);
		write_cmos_sensor(0x4022, 0x04);
		write_cmos_sensor(0x4023, 0xd7);
		write_cmos_sensor(0x4024, 0x05);
		write_cmos_sensor(0x4025, 0xbc);
		write_cmos_sensor(0x4026, 0x05);
		write_cmos_sensor(0x4027, 0xbf);
		write_cmos_sensor(0x402a, 0x04);
		write_cmos_sensor(0x402b, 0x08);
		write_cmos_sensor(0x402c, 0x02);
		write_cmos_sensor(0x402e, 0x0c);
		write_cmos_sensor(0x402f, 0x08);
		write_cmos_sensor(0x4501, 0x3c);
		write_cmos_sensor(0x4601, 0x83);
		write_cmos_sensor(0x4603, 0x01);
		write_cmos_sensor(0x4837, 0x19);
		write_cmos_sensor(0x5401, 0x61);
		write_cmos_sensor(0x5405, 0x40);
		write_cmos_sensor(0x0100, 0x01);//
		mdelay(10);
}				/*    preview_setting  */

int capture_first_flag;
int pre_currefps;
static void capture_setting(kal_uint16 currefps)
{
    printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
//	printk("E! currefps:%d\n", currefps);

	if (pre_currefps != currefps) {
		capture_first_flag = 0;
		pre_currefps = currefps;
	} else
		capture_first_flag = 1;
if (capture_first_flag == 0)
{

    if (currefps == 240) { //24fps for PIP
        //@@full_132PCLK_24.75
      		write_cmos_sensor(0x0300, 0x00);//
			write_cmos_sensor(0x0302, 0x32);//
			write_cmos_sensor(0x0303, 0x00);//
			write_cmos_sensor(0x3612, 0x07);//
			write_cmos_sensor(0x3501, 0xc0);//
			write_cmos_sensor(0x3702, 0x40);// ;add for VGA differences
			write_cmos_sensor(0x370a, 0x24);//
			//write_cmos_sensor(0x3718, 0x10);//
			write_cmos_sensor(0x372a, 0x04);//
			write_cmos_sensor(0x372f, 0xa0);//
			write_cmos_sensor(0x3801, 0x0C);//
			write_cmos_sensor(0x3802, 0x00);// ;add for VGA differences
			write_cmos_sensor(0x3803, 0x04);//
			write_cmos_sensor(0x3805, 0x93);//
			write_cmos_sensor(0x3806, 0x0c);// ;add for VGA differences
			write_cmos_sensor(0x3807, 0x4B);// 
			write_cmos_sensor(0x3808, 0x10);//
			write_cmos_sensor(0x3809, 0x80);// 
			write_cmos_sensor(0x380a, 0x0c);//
			write_cmos_sensor(0x380b, 0x40);// 
			write_cmos_sensor(0x380c, ((imgsensor_info.cap1.linelength >> 8) & 0xFF)); // hts = 2688
      		write_cmos_sensor(0x380d, (imgsensor_info.cap1.linelength & 0xFF));        // hts
      		write_cmos_sensor(0x380e, ((imgsensor_info.cap1.framelength >> 8) & 0xFF));  // vts = 1984
      		write_cmos_sensor(0x380f, (imgsensor_info.cap1.framelength & 0xFF));         // vts
			write_cmos_sensor(0x3811, 0x04);// ;add for VGA differences
			write_cmos_sensor(0x3813, 0x04);//
			write_cmos_sensor(0x3814, 0x11);//
			write_cmos_sensor(0x3815, 0x11);//
			write_cmos_sensor(0x3820, 0x00);//
			write_cmos_sensor(0x3821, 0x04);//
			write_cmos_sensor(0x3834, 0x00);// ;add for VGA differences
			write_cmos_sensor(0x3836, 0x04);//
			write_cmos_sensor(0x3837, 0x01);//
			write_cmos_sensor(0x4020, 0x02);//
			write_cmos_sensor(0x4021, 0x4C);// 
			write_cmos_sensor(0x4022, 0x0E);//
			write_cmos_sensor(0x4023, 0x37);//
			write_cmos_sensor(0x4024, 0x0F);//
			write_cmos_sensor(0x4025, 0x1C);//
			write_cmos_sensor(0x4026, 0x0F);//
			write_cmos_sensor(0x4027, 0x1F);//
			write_cmos_sensor(0x402a, 0x04);//;add for VGA differences
			write_cmos_sensor(0x402b, 0x08);//;add for VGA differences
			write_cmos_sensor(0x402c, 0x02);//;add for VGA differences
			write_cmos_sensor(0x402e, 0x0c);//;add for VGA differences
			write_cmos_sensor(0x402f, 0x08);//;add for VGA differences
			write_cmos_sensor(0x4501, 0x38);//
			write_cmos_sensor(0x4601, 0x04);//
			write_cmos_sensor(0x4603, 0x00);//
			write_cmos_sensor(0x4837, 0x0d);//
			write_cmos_sensor(0x5401, 0x71);//
			write_cmos_sensor(0x5405, 0x80);//
			write_cmos_sensor(0x0100, 0x01);//

		} else {	/* full size 15fps for PIP */

      		write_cmos_sensor(0x0300, 0x00);//
			write_cmos_sensor(0x0302, 0x32);//
			write_cmos_sensor(0x0303, 0x00);//
			write_cmos_sensor(0x3612, 0x07);//
			write_cmos_sensor(0x3501, 0xc0);//
			write_cmos_sensor(0x3702, 0x40);// ;add for VGA differences
			write_cmos_sensor(0x370a, 0x24);//
			//write_cmos_sensor(0x3718, 0x10);//
			write_cmos_sensor(0x372a, 0x04);//
			write_cmos_sensor(0x372f, 0xa0);//
			write_cmos_sensor(0x3801, 0x0C);//
			write_cmos_sensor(0x3802, 0x00);// ;add for VGA differences
			write_cmos_sensor(0x3803, 0x04);//
			write_cmos_sensor(0x3805, 0x93);//
			write_cmos_sensor(0x3806, 0x0c);// ;add for VGA differences
			write_cmos_sensor(0x3807, 0x4B);//
			write_cmos_sensor(0x3808, 0x10);//
			write_cmos_sensor(0x3809, 0x80);//
			write_cmos_sensor(0x380a, 0x0c);//
			write_cmos_sensor(0x380b, 0x40);//
			write_cmos_sensor(0x380c, ((imgsensor_info.cap.linelength >> 8) & 0xFF)); // hts = 2688
      		write_cmos_sensor(0x380d, (imgsensor_info.cap.linelength & 0xFF));        // hts
      		write_cmos_sensor(0x380e, ((imgsensor_info.cap.framelength >> 8) & 0xFF));  // vts = 1984
      		write_cmos_sensor(0x380f, (imgsensor_info.cap.framelength & 0xFF));         // vts
			write_cmos_sensor(0x3811, 0x04);// ;add for VGA differences
			write_cmos_sensor(0x3813, 0x04);//
			write_cmos_sensor(0x3814, 0x11);//
			write_cmos_sensor(0x3815, 0x11);//
			write_cmos_sensor(0x3820, 0x00);//
			write_cmos_sensor(0x3821, 0x04);//
			write_cmos_sensor(0x3834, 0x00);// ;add for VGA differences
			write_cmos_sensor(0x3836, 0x04);//
			write_cmos_sensor(0x3837, 0x01);//
			write_cmos_sensor(0x4020, 0x02);//
			write_cmos_sensor(0x4021, 0x4C);// 
			write_cmos_sensor(0x4022, 0x0E);//
			write_cmos_sensor(0x4023, 0x37);//
			write_cmos_sensor(0x4024, 0x0F);//
			write_cmos_sensor(0x4025, 0x1C);//
			write_cmos_sensor(0x4026, 0x0F);//
			write_cmos_sensor(0x4027, 0x1F);//
			write_cmos_sensor(0x402a, 0x04);//;add for VGA differences
			write_cmos_sensor(0x402b, 0x08);//;add for VGA differences
			write_cmos_sensor(0x402c, 0x02);//;add for VGA differences
			write_cmos_sensor(0x402e, 0x0c);//;add for VGA differences
			write_cmos_sensor(0x402f, 0x08);//;add for VGA differences
			write_cmos_sensor(0x4501, 0x38);//
			write_cmos_sensor(0x4601, 0x04);//
			write_cmos_sensor(0x4603, 0x00);//
			write_cmos_sensor(0x4837, 0x0d);//
			write_cmos_sensor(0x5401, 0x71);//
			write_cmos_sensor(0x5405, 0x80);//
			write_cmos_sensor(0x0100, 0x01);//

			if (imgsensor.ihdr_en)
				printk("imgsensor ihdr enable\n");
				/* no implementation */
			else
				printk("imgsensor ihdr disable\n");
				/* no implementation */
		}

		mdelay(20);
		capture_first_flag = 1;
	}

}

static void normal_video_setting(kal_uint16 currefps)
{
    printk("E! currefps:%d\n",currefps);
    printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
		write_cmos_sensor(0x0100, 0x00);//
		write_cmos_sensor(0x0300, 0x00);
		write_cmos_sensor(0x0302, 0x32);
		write_cmos_sensor(0x0303, 0x00);
		write_cmos_sensor(0x3612, 0x07);
		write_cmos_sensor(0x3501, 0xc0);
		write_cmos_sensor(0x3702, 0x40);
		write_cmos_sensor(0x370a, 0x24);
		write_cmos_sensor(0x372a, 0x04);
		write_cmos_sensor(0x372f, 0xa0);
		write_cmos_sensor(0x3801, 0x14);
		write_cmos_sensor(0x3802, 0x00);
		write_cmos_sensor(0x3803, 0x0c);
		write_cmos_sensor(0x3805, 0x8b);
		write_cmos_sensor(0x3806, 0x0c);
		write_cmos_sensor(0x3807, 0x43);
		write_cmos_sensor(0x3808, 0x10);
		write_cmos_sensor(0x3809, 0x70);
		write_cmos_sensor(0x380a, 0x0c);
		write_cmos_sensor(0x380b, 0x30);
		write_cmos_sensor(0x380c, ((imgsensor_info.normal_video.linelength >> 8) & 0xFF)); // hts = 2688
        write_cmos_sensor(0x380d, (imgsensor_info.normal_video.linelength & 0xFF));        // hts
        write_cmos_sensor(0x380e, ((imgsensor_info.normal_video.framelength >> 8) & 0xFF));  // vts = 1984
    	write_cmos_sensor(0x380f, (imgsensor_info.normal_video.framelength & 0xFF));         // vts
		write_cmos_sensor(0x3811, 0x04);
		write_cmos_sensor(0x3813, 0x04);
		write_cmos_sensor(0x3814, 0x11);
		write_cmos_sensor(0x3815, 0x11);
		write_cmos_sensor(0x3820, 0x00);
		write_cmos_sensor(0x3821, 0x04);
		write_cmos_sensor(0x3834, 0x00);
		write_cmos_sensor(0x3836, 0x04);
		write_cmos_sensor(0x3837, 0x01);
		write_cmos_sensor(0x4020, 0x02);
		write_cmos_sensor(0x4021, 0x3C);
		write_cmos_sensor(0x4022, 0x0E);
		write_cmos_sensor(0x4023, 0x37);
		write_cmos_sensor(0x4024, 0x0F);
		write_cmos_sensor(0x4025, 0x1C);
		write_cmos_sensor(0x4026, 0x0F);
		write_cmos_sensor(0x4027, 0x1F);
		write_cmos_sensor(0x402a, 0x04);
		write_cmos_sensor(0x402b, 0x08);
		write_cmos_sensor(0x402c, 0x02);
		write_cmos_sensor(0x402e, 0x0c);
		write_cmos_sensor(0x402f, 0x08);
		write_cmos_sensor(0x4501, 0x38);
		write_cmos_sensor(0x4601, 0x04);
		write_cmos_sensor(0x4603, 0x00);
		write_cmos_sensor(0x4837, 0x0d);
		write_cmos_sensor(0x5401, 0x71);
		write_cmos_sensor(0x5405, 0x80);
		write_cmos_sensor(0x0100, 0x01);//


	if (imgsensor.ihdr_en)
		printk("imgsensor ihdr enable\n");
		/* no implementation */
	else
		printk("imgsensor ihdr disable\n");
		/* no implementation */

	mdelay(20);
}

static void hs_video_setting(void)
{
    printk("E\n");
printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    write_cmos_sensor(0x0100, 0x00);//
		write_cmos_sensor(0x0300, 0x01);
		write_cmos_sensor(0x0302, 0x28);
		write_cmos_sensor(0x0303, 0x00);
		write_cmos_sensor(0x3612, 0x27);
		write_cmos_sensor(0x3501, 0x20);
		write_cmos_sensor(0x3702, 0x5a);
		write_cmos_sensor(0x370a, 0xa9);
		write_cmos_sensor(0x372a, 0x00);
		write_cmos_sensor(0x372f, 0x88);
		write_cmos_sensor(0x3801, 0x00);
		write_cmos_sensor(0x3802, 0x01);
		write_cmos_sensor(0x3803, 0x78);
		write_cmos_sensor(0x3805, 0x9f);
		write_cmos_sensor(0x3806, 0x0a);
		write_cmos_sensor(0x3807, 0xcf);
		write_cmos_sensor(0x3808, 0x04);
		write_cmos_sensor(0x3809, 0x20);
		write_cmos_sensor(0x380a, 0x02);
		write_cmos_sensor(0x380b, 0x52);
		write_cmos_sensor(0x380c, ((imgsensor_info.hs_video.linelength >> 8) & 0xFF)); // hts = 2688
	    write_cmos_sensor(0x380d, (imgsensor_info.hs_video.linelength & 0xFF));        // hts
	    write_cmos_sensor(0x380e, ((imgsensor_info.hs_video.framelength >> 8) & 0xFF));  // vts = 1984
	    write_cmos_sensor(0x380f, (imgsensor_info.hs_video.framelength & 0xFF));         // vts
		write_cmos_sensor(0x3811, 0x08);
		write_cmos_sensor(0x3813, 0x02);
		write_cmos_sensor(0x3814, 0x31);
		write_cmos_sensor(0x3815, 0x35);
		write_cmos_sensor(0x3820, 0x02);
		write_cmos_sensor(0x3821, 0x06);
		write_cmos_sensor(0x3834, 0x02);
		write_cmos_sensor(0x3836, 0x08);
		write_cmos_sensor(0x3837, 0x04);
		write_cmos_sensor(0x4020, 0x00);
		write_cmos_sensor(0x4021, 0xe4);
		write_cmos_sensor(0x4022, 0x03);
		write_cmos_sensor(0x4023, 0x3f);
		write_cmos_sensor(0x4024, 0x04);
		write_cmos_sensor(0x4025, 0x20);
		write_cmos_sensor(0x4026, 0x04);
		write_cmos_sensor(0x4027, 0x25);
		write_cmos_sensor(0x402a, 0x02);
		write_cmos_sensor(0x402b, 0x04);
		write_cmos_sensor(0x402c, 0x06);
		write_cmos_sensor(0x402e, 0x08);
		write_cmos_sensor(0x402f, 0x04);
		write_cmos_sensor(0x4501, 0x3c);
		write_cmos_sensor(0x4601, 0x40);
		write_cmos_sensor(0x4603, 0x01);
		write_cmos_sensor(0x4837, 0x19);
		write_cmos_sensor(0x5401, 0x51);
		write_cmos_sensor(0x5405, 0x20);

	if (imgsensor.ihdr_en)
		printk("imgsensor ihdr enable\n");
		/* no implementation */
	else
		printk("imgsensor ihdr disable\n");
		/* no implementation */

    write_cmos_sensor(0x0100, 0x01);//
	mdelay(10);
}

static void slim_video_setting(void)
{
	printk("E");
        printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
		write_cmos_sensor(0x0100, 0x00);
		write_cmos_sensor(0x0300, 0x01);
		write_cmos_sensor(0x0301, 0x00);
		write_cmos_sensor(0x0302, 0x28);
		write_cmos_sensor(0x0303, 0x00);
		write_cmos_sensor(0x3612, 0x27);
		write_cmos_sensor(0x3612, 0x27);
		write_cmos_sensor(0x3614, 0x28);
		write_cmos_sensor(0x370a, 0x27);
		write_cmos_sensor(0x372a, 0x00);
		write_cmos_sensor(0x372f, 0x90);
		write_cmos_sensor(0x3718, 0x10);
		write_cmos_sensor(0x3767, 0x24);
		write_cmos_sensor(0x3801, 0x38);
		write_cmos_sensor(0x3802, 0x03);
		write_cmos_sensor(0x3803, 0x48);
		write_cmos_sensor(0x3805, 0x67);
		write_cmos_sensor(0x3806, 0x09);
		write_cmos_sensor(0x3807, 0x03);
		write_cmos_sensor(0x3808, 0x05);
	    write_cmos_sensor(0x3809, 0x10);//
	    write_cmos_sensor(0x380A, 0x02);//
	    write_cmos_sensor(0x380B, 0xda);//
	    write_cmos_sensor(0x380C, ((imgsensor_info.slim_video.linelength >> 8) & 0xFF)); // hts = 9600
		write_cmos_sensor(0x380D,     (imgsensor_info.slim_video.linelength & 0xFF));        // hts
		write_cmos_sensor(0x380E,   ((imgsensor_info.slim_video.framelength >> 8) & 0xFF));  // vts = 834
		write_cmos_sensor(0x380F,   (imgsensor_info.slim_video.framelength & 0xFF));         // vts
		write_cmos_sensor(0x3811, 0x04);
		write_cmos_sensor(0x3813, 0x02);
		write_cmos_sensor(0x3814, 0x31);
		write_cmos_sensor(0x3815, 0x31);
		write_cmos_sensor(0x3820, 0x02);
		write_cmos_sensor(0x3821, 0x05);
		write_cmos_sensor(0x3836, 0x08);
		write_cmos_sensor(0x3837, 0x02);
		write_cmos_sensor(0x4001, 0x00);
		write_cmos_sensor(0x4020, 0x1 );
		write_cmos_sensor(0x4021, 0xD4);
		write_cmos_sensor(0x4022, 0x3 );
		write_cmos_sensor(0x4023, 0x43);
		write_cmos_sensor(0x4024, 0x05);
		write_cmos_sensor(0x4025, 0x14);
		write_cmos_sensor(0x4026, 0x05);
		write_cmos_sensor(0x4027, 0x17);
		write_cmos_sensor(0x402a, 0x04);
		write_cmos_sensor(0x402b, 0x08);
		write_cmos_sensor(0x402c, 0x02);
		write_cmos_sensor(0x402e, 0x0c);
		write_cmos_sensor(0x402f, 0x08);
		write_cmos_sensor(0x4501, 0x3c);
		write_cmos_sensor(0x4600, 0x00);
		write_cmos_sensor(0x4601, 0x50);
	    write_cmos_sensor(0x4837, 0x19);
	    write_cmos_sensor(0x5401, 0x61);
	    write_cmos_sensor(0x5405, 0x40);
	    write_cmos_sensor(0x350b, 0x40);
	    write_cmos_sensor(0x3500, 0x00);
	    write_cmos_sensor(0x3501, 0x33);
	    write_cmos_sensor(0x3502, 0x90);

	if (imgsensor.ihdr_en){
		printk("imgsensor ihdr enable\n");
	/* no implementation */}
	else{
		printk("imgsensor ihdr disable\n");
		/* no implementation */
		write_cmos_sensor(0x0100, 0x01);//
	mdelay(10);
	}
}

#if 0
static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
    LOG_WRN("enable: %d\n", enable);
printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    if (enable) {
        // 0x5E00[8]: 1 enable,  0 disable
        // 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
        write_cmos_sensor(0x5E00, 0x80);
    } else {
        // 0x5E00[8]: 1 enable,  0 disable
        // 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
        write_cmos_sensor(0x5E00, 0x00);
    }
    spin_lock(&imgsensor_drv_lock);
    imgsensor.test_pattern = enable;
    spin_unlock(&imgsensor_drv_lock);
    return ERROR_NONE;
}
#endif


/*************************************************************************
* FUNCTION
*    get_imgsensor_id
*
* DESCRIPTION
*    This function get the sensor ID
*
* PARAMETERS
*    *sensorID : return the sensor ID
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
    kal_uint8 i = 0;
    kal_uint8 retry_total_cnt = 100; 
    kal_uint8 retry = retry_total_cnt;
    //sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            *sensor_id = return_sensor_id();
            if (*sensor_id == imgsensor_info.sensor_id) {
                printk( "[Kernel/ov13850mipiraw_Sensor]: i2c_write_id=0x%x, sensor_id=0x%x\n", imgsensor.i2c_write_id, *sensor_id );
                printk( "[Kernel/ov13850mipiraw_Sensor]: << get_imgsensor_id(): OK\n" );
                return ERROR_NONE;
            }
            printk("Read sensor id fail, write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
            retry--;
        } while(retry > 0);
        i++;
        retry = retry_total_cnt;
    }
    if (*sensor_id != imgsensor_info.sensor_id) {
        // if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF
        *sensor_id = 0xFFFFFFFF;
        printk( "cam << [Kernel/ov13850mipiraw_Sensor] get_imgsensor_id(): FAIL\n" );
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*    open
*
* DESCRIPTION
*    This function initialize the registers of CMOS sensor
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 open(void)
{
    kal_uint8 i = 0;
    kal_uint8 retry = 2;
    kal_uint32 sensor_id = 0;
    // LOG_1;
    // LOG_2;
    //sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            sensor_id = return_sensor_id();
            printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d imgsensor_info.sensor_id: 0x%x \n", __func__,__LINE__,imgsensor_info.sensor_id);
            if (sensor_id == imgsensor_info.sensor_id) {
                printk("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
                printk( "cam [Kernel/ov13850mipiraw_Sensor] i2c_write_id=0x%x, sensor_id=0x%x\n", imgsensor.i2c_write_id, sensor_id );
                printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  i2c_write_id=0x%x, sensor_id=0x%x \n", __func__,__LINE__,imgsensor.i2c_write_id, sensor_id);
                break;
            }
            printk("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
             printk( "cam [Kernel/ov13850mipiraw_Sensor] Read sensor id fail, write_id=0x%x, sensor_id=0x%x\n", imgsensor.i2c_write_id, sensor_id );
            retry--;
        } while(retry > 0);
        i++;
        if (sensor_id == imgsensor_info.sensor_id)
            break;
        retry = 2;
    }
    if (imgsensor_info.sensor_id != sensor_id)
        return ERROR_SENSOR_CONNECT_FAIL;
        
		if ((read_cmos_sensor(0x302a))==0xb1)
		{
				printk("----R1A---- \n");
		}else if((read_cmos_sensor(0x302a))==0xb2)
		{
				printk("----R2A---- \n");
		}
    /* initail sequence write in  */
    sensor_init();
	  //for OTP
	  //otp_cali(imgsensor.i2c_write_id);
	  write_cmos_sensor(0x0100, 0x00);
	  
    spin_lock(&imgsensor_drv_lock);

    imgsensor.autoflicker_en= KAL_FALSE;
    imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
    imgsensor.pclk = imgsensor_info.pre.pclk;
    imgsensor.frame_length = imgsensor_info.pre.framelength;
    imgsensor.line_length = imgsensor_info.pre.linelength;
    imgsensor.min_frame_length = imgsensor_info.pre.framelength;
    imgsensor.dummy_pixel = 0;
    imgsensor.dummy_line = 0;
    imgsensor.ihdr_en = 0;
    imgsensor.test_pattern = KAL_FALSE;
    imgsensor.current_fps = imgsensor_info.pre.max_framerate;
    //for zsd multi capture setting
	capture_first_flag = 0;
	pre_currefps = 0;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}				/*    open  */



/*************************************************************************
* FUNCTION
*    close
*
* DESCRIPTION
*
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 close(void)
{
    printk("E\n");
printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    /*No Need to implement this function*/

	return ERROR_NONE;
}				/*    close  */


/*************************************************************************
* FUNCTION
* preview
*
* DESCRIPTION
*    This function start the sensor preview.
*
* PARAMETERS
*    *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    printk("E\n");
printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
    imgsensor.pclk = imgsensor_info.pre.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.pre.linelength;
    imgsensor.frame_length = imgsensor_info.pre.framelength;
    imgsensor.min_frame_length = imgsensor_info.pre.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    //set_mirror_flip(imgsensor.mirror);
	//set_mirror_flip(sensor_config_data->SensorImageMirror);
    return ERROR_NONE;
}    /*    preview   */

/*************************************************************************
* FUNCTION
*    capture
*
* DESCRIPTION
*    This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    printk("E\n");
	printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
    if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
        imgsensor.pclk = imgsensor_info.cap1.pclk;
        imgsensor.line_length = imgsensor_info.cap1.linelength;
        imgsensor.frame_length = imgsensor_info.cap1.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
        imgsensor.autoflicker_en = KAL_FALSE;
    } else {
        if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
            printk("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap.max_framerate/10);
        imgsensor.pclk = imgsensor_info.cap.pclk;
        imgsensor.line_length = imgsensor_info.cap.linelength;
        imgsensor.frame_length = imgsensor_info.cap.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap.framelength;
        imgsensor.autoflicker_en = KAL_FALSE;
    }
    spin_unlock(&imgsensor_drv_lock);
    capture_setting(imgsensor.current_fps);
	  //set_mirror_flip(imgsensor.mirror);
    return ERROR_NONE;
}    /* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			       MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    printk("E\n");
printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
    imgsensor.pclk = imgsensor_info.normal_video.pclk;
    imgsensor.line_length = imgsensor_info.normal_video.linelength;
    imgsensor.frame_length = imgsensor_info.normal_video.framelength;
    imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
    //imgsensor.current_fps = 300;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    normal_video_setting(imgsensor.current_fps);	
	//set_mirror_flip(sensor_config_data->SensorImageMirror);
		//set_mirror_flip(imgsensor.mirror);
    return ERROR_NONE;
}    /*    normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    printk("E\n");
printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
    imgsensor.pclk = imgsensor_info.hs_video.pclk;
    //imgsensor.video_mode = KAL_TRUE;
    imgsensor.line_length = imgsensor_info.hs_video.linelength;
    imgsensor.frame_length = imgsensor_info.hs_video.framelength;
    imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
    imgsensor.dummy_line = 0;
    imgsensor.dummy_pixel = 0;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    hs_video_setting();	
	//set_mirror_flip(sensor_config_data->SensorImageMirror);
		//set_mirror_flip(imgsensor.mirror);
    return ERROR_NONE;
}    /*    hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			     MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    printk("E\n");
printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
    imgsensor.pclk = imgsensor_info.slim_video.pclk;
    imgsensor.line_length = imgsensor_info.slim_video.linelength;
    imgsensor.frame_length = imgsensor_info.slim_video.framelength;
    imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
    imgsensor.dummy_line = 0;
    imgsensor.dummy_pixel = 0;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    slim_video_setting();
	//set_mirror_flip(sensor_config_data->SensorImageMirror);
		//set_mirror_flip(imgsensor.mirror);
    return ERROR_NONE;
}    /*    slim_video     */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
    printk("E\n");
    printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
    sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


	sensor_resolution->SensorHighSpeedVideoWidth = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight = imgsensor_info.slim_video.grabwindow_height;
	return ERROR_NONE;
}				/*    get_resolution    */

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			   MSDK_SENSOR_INFO_STRUCT *sensor_info,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    printk("scenario_id = %d\n", scenario_id);
printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);

	/* sensor_info->SensorVideoFrameRate = imgsensor_info.normal_video.max_framerate/10; not use */
	/* sensor_info->SensorStillCaptureFrameRate= imgsensor_info.cap.max_framerate/10; not use */
	/* imgsensor_info->SensorWebCamCaptureFrameRate= imgsensor_info.v.max_framerate; not use */

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;	/* not use */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;	/* inverse with datasheet */
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4;	/* not use */
	sensor_info->SensorResetActiveHigh = FALSE;	/* not use */
	sensor_info->SensorResetDelayCount = 5;	/* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0;	/* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

    sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;          /* The frame of setting shutter default 0 for TG int */
    sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;    /* The frame of setting sensor gain */
    sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
    sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
    sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
    sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;

    sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
    sensor_info->SensorClockFreq = imgsensor_info.mclk;
    sensor_info->SensorClockDividCount = 3; /* not use */
    sensor_info->SensorClockRisingCount = 0;
    sensor_info->SensorClockFallingCount = 2; /* not use */
    sensor_info->SensorPixelClockCount = 3; /* not use */
    sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;	/* 0 is default 1x */
	sensor_info->SensorHightSampling = 0;	/* 0 is default 1x */
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

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
		    imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;
		break;
	default:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
		    imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
		break;
	}

	return ERROR_NONE;
}				/*    get_info  */


static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			  MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    printk("scenario_id = %d\n", scenario_id);
    printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    spin_lock(&imgsensor_drv_lock);
    imgsensor.current_scenario_id = scenario_id;
    spin_unlock(&imgsensor_drv_lock);
    switch (scenario_id) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			capture_first_flag = 0;
			pre_currefps = 0;
            preview(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
            capture(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			capture_first_flag = 0;
			pre_currefps = 0;
            normal_video(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			capture_first_flag = 0;
			pre_currefps = 0;
            hs_video(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
			capture_first_flag = 0;
			pre_currefps = 0;
            slim_video(image_window, sensor_config_data);
            break;
        default:
            printk("Error ScenarioId setting");
			capture_first_flag = 0;
			pre_currefps = 0;
            preview(image_window, sensor_config_data);
            return ERROR_INVALID_SCENARIO_ID;
    }
    return ERROR_NONE;
}    /* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
    printk("framerate = %d\n ", framerate);
    printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    // SetVideoMode Function should fix framerate
    if (framerate == 0)
        // Dynamic frame rate
        return ERROR_NONE;
    spin_lock(&imgsensor_drv_lock);
    if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
        imgsensor.current_fps = 296;
    else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
        imgsensor.current_fps = 146;
    else
        imgsensor.current_fps = framerate;
    spin_unlock(&imgsensor_drv_lock);
    set_max_framerate(imgsensor.current_fps,1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
    printk("enable = %d, framerate = %d \n", enable, framerate);
    printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    spin_lock(&imgsensor_drv_lock);
    if (enable) //enable auto flicker
        imgsensor.autoflicker_en = KAL_TRUE;
    else //Cancel Auto flick
        imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id,
						MUINT32 framerate)
{
	/* kal_int16 dummyLine; */
	kal_uint32 frame_length;

    printk("scenario_id = %d, framerate = %d\n", scenario_id, framerate);
printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    switch (scenario_id) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            if(framerate == 0)
                return ERROR_NONE;
            frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ? (frame_length - imgsensor_info.normal_video.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        	  if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
                frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            } else {
        		    if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
                    printk("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",framerate,imgsensor_info.cap.max_framerate/10);
                frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            }
            //set_dummy();
            break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
            frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
            imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            break;
        default:  //coding with  preview scenario by default
            frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            printk("error scenario_id = %d, we use preview scenario \n", scenario_id);
            break;
    }
    return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id,
						    MUINT32 *framerate)
{
    printk("scenario_id = %d\n", scenario_id);
printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
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
        default:
            break;
    }

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	/* check,power+volum+ */
	printk("enable: %d\n", enable);

    if (enable) {
        // 0x5E00[8]: 1 enable,  0 disable
        // 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
        write_cmos_sensor(0x5E00, 0x80);
    } else {
        // 0x5E00[8]: 1 enable,  0 disable
        // 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
        write_cmos_sensor(0x5E00, 0x00);
    }
    spin_lock(&imgsensor_drv_lock);
    imgsensor.test_pattern = enable;
    spin_unlock(&imgsensor_drv_lock);
    return ERROR_NONE;
}

static kal_uint32 streaming_control(kal_bool enable)
{
	printk("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable)
		write_cmos_sensor(0x0100, 0x01);
	else
		write_cmos_sensor(0x0100, 0x00);

	mdelay(10);
	return ERROR_NONE;
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
				  UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *)feature_para;
	/* unsigned long long *feature_return_para=(unsigned long long *) feature_para; */

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	/* add for OV13850 pdaf */
	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data = (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;
printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
	printk("feature_id = %d\n", feature_id);
	switch (feature_id) {
	case SENSOR_FEATURE_GET_PERIOD:
		*feature_return_para_16++ = imgsensor.line_length;
		*feature_return_para_16 = imgsensor.frame_length;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		*feature_return_para_32 = imgsensor.pclk;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_ESHUTTER:
		set_shutter(*feature_data);
		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
		night_mode((BOOL) * feature_data);
		break;
	case SENSOR_FEATURE_SET_GAIN:
		set_gain((UINT16) *feature_data);
		break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		write_cmos_sensor(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData = read_cmos_sensor(sensor_reg_data->RegAddr);
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/* get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE */
		/* if EEPROM does not exist in camera module. */
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
		set_auto_flicker_mode((BOOL) * feature_data_16, *(feature_data_16 + 1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM) *feature_data,
					      *(feature_data + 1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM) *(feature_data),
						  (MUINT32 *) (uintptr_t) (*(feature_data + 1)));
		break;
		/* case SENSOR_FEATURE_GET_PDAF_DATA:
		 * printk("SENSOR_FEATURE_GET_PDAF_DATA\n");
		 * read_OV13850_eeprom((kal_uint16 )(*feature_data),
		 * (char*)(uintptr_t)(*(feature_data+1)),(kal_uint32)(*(feature_data+2)));
		 * break;
		 */
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL) * feature_data);
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:	/* for factory mode auto testing */
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		printk("current fps :%d\n", (UINT32) *feature_data);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_SET_HDR:
		printk("ihdr enable :%d\n", (BOOL) * feature_data);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.ihdr_en = (BOOL) * feature_data_32;
		spin_unlock(&imgsensor_drv_lock);

		/* for disable ihdr */
		/*
		 * if(!imgsensor.ihdr_en)
		 * {
		 * write_cmos_sensor(0x38210, 0x04);
		 * write_cmos_sensor(0x38360, 0x04);
		 * write_cmos_sensor(0x38370, 0x01);
		 * write_cmos_sensor(0x37670, 0x24);
		 * write_cmos_sensor(0x40290, 0x02);
		 * write_cmos_sensor(0x372e0, 0x82);
		 * write_cmos_sensor(0x40010, 0x00);
		 * write_cmos_sensor(0x37180, 0x10);
		 * }
		 */
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		printk("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", (UINT32) *feature_data);

		wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *) (uintptr_t) (*(feature_data + 1));

		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[1],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[2],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[3],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[4],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[0],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length((UINT16) (*feature_data), (UINT16) (*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_GET_PDAF_INFO:
		printk("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%lld\n", *feature_data);
		PDAFinfo = (struct SET_PD_BLOCK_INFO_T *) (uintptr_t) (*(feature_data + 1));

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info,
			       sizeof(struct SET_PD_BLOCK_INFO_T));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			break;
		}
		break;
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		printk("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%lld\n", *feature_data);
		/* PDAF capacity enable or not, OV13850 only full size support PDAF */
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 1;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 0;	/* video & capture use same setting */
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 0;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 0;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 0;
			break;
		default:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 0;
			break;
		}
		break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		printk("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n", (UINT16) *feature_data,
			(UINT16) *(feature_data + 1), (UINT16) *(feature_data + 2));
		ihdr_write_shutter_gain((UINT16) *feature_data, (UINT16) *(feature_data + 1),
					(UINT16) *(feature_data + 2));
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
		{
			kal_uint32 rate;

			switch (*feature_data) {
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				rate = imgsensor_info.cap.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				rate = imgsensor_info.normal_video.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
				rate = imgsensor_info.hs_video.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
				rate = imgsensor_info.slim_video.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			default:
				rate = imgsensor_info.pre.mipi_pixel_rate;
				break;
			}
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = rate;
		}
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		printk("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		printk("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n", *feature_data);
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;

	default:
		break;
	}

	return ERROR_NONE;
}				/*    feature_control()  */

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
    open,
    get_info,
    get_resolution,
    feature_control,
    control,
    close
};

UINT32 OV13850_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
    printk("cam [Kernel/ov13850mipiraw_Sensor]: %s %d  \n", __func__,__LINE__);
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&sensor_func;
    return ERROR_NONE;
}    /*    OV13850_MIPI_RAW_SensorInit    */
