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

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#else
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#endif
#endif

#include "lcm_drv.h"

extern unsigned int GPIO_LCM_PWR;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_VDD;

#define GPIO_LCD_RST_EN             GPIO_LCM_RST
#define GPIO_LCD_PWR_EN             GPIO_LCM_PWR
#define GPIO_LCM_BIAS_EN            GPIO_LCM_VDD

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	gpio_direction_output(GPIO, output);
	gpio_set_value(GPIO, output);
}

/* ------------------------------------------------------------------- */
/* Local Constants */
/* ------------------------------------------------------------------- */

#define FRAME_WIDTH  (800)
#define FRAME_HEIGHT (1280)

/* ------------------------------------------------------------------- */
/* Local Variables */
/* ------------------------------------------------------------------- */

static struct LCM_UTIL_FUNCS lcm_util = { 0 };

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

/* ------------------------------------------------------------------- */
/* Local Functions */
/* ------------------------------------------------------------------- */
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
	lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define REGFLAG_DELAY 0xFFFD
#define REGFLAG_END_OF_TABLE 0xFFFE

struct LCM_setting_table {
    unsigned cmd;
    unsigned int count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = 
{
	{0xee, 0x01, {0x50}},				// ENTER PAGE1
	{0xea, 0x02, {0x85,0x55}},    // write enable
	{0x30, 0x01, {0x00}},      // bist=1 
	{0x31, 0x01, {0x50}},      // bist=1 
	{0x32, 0x01, {0x20}},      // bist=1 
	{0x36, 0x03, {0x14,0x14,0x8C}}, // hsa Hbp Hfp		      	
 	{0x39, 0x03, {0x02,0x08,0x09}}, // vsa vbp vfp	
 	{0x90, 0x02, {0x91,0xe3}},   // ss_tp location  90 07
 	{0x92, 0x03, {0x5a,0x5b,0x00}},   // ss_tp location  85 86 e0
	{0x24, 0x01, {0x20}},        //    mirror te
	{0x99, 0x01, {0x00}},        // ss_tp de ndg
	{0x95, 0x01, {0x70}},        // column invertion
  {0x97, 0x01, {0x07}},       //  smart gip
	{0x79, 0x01, {0x09}},        // zigzag
 	{0x7a, 0x01, {0x20}},        // 
  {0xee, 0x01, {0x60}},     // enter page2
  {0x21, 0x01, {0x01}},		// OSC
  {0x23, 0x01, {0x04}},   //  BIST OSC
  {0x25, 0x01, {0x70}},	   //vref_apf1[2:0]
  {0x27, 0x01, {0x23}},	   //vddd      22
  {0x29, 0x01, {0x87}},	   //Sd_i_set<3:0> 8d
  {0x2a, 0x01, {0x26}},	   //Sd_trim<2:0>  26
  {0x30, 0x01, {0x01}},       // 4 LANE
  {0x34, 0x01, {0x2f}},	   //dsi_ihrs<1:0>
	{0x3a, 0x01, {0xa4}},	   //gas off
	{0x3b, 0x01, {0x00}},	   //Flicker画面水波纹改
  {0x3c, 0x01, {0x23}},	   //VCOM SET
  {0x3d, 0x01, {0x11}},	   //vgl
  {0x3e, 0x01, {0x93}},      //vgh
  {0x42, 0x01, {0x58}},	   //vspr   
  {0x43, 0x01, {0x58}},	   //vsnr  
  {0x7f, 0x01, {0x24}},        // vcsw delay
  {0x80, 0x01, {0x24}},        //
  {0x86, 0x01, {0x03}}, 
  {0x89, 0x01, {0x03}},        // blkh,1
  {0x8b, 0x01, {0x90}},        // blkh,1
  {0x8d, 0x01, {0x46}},        //45
  {0x91, 0x01, {0x11}},        // frq_vgh_clk frq_vgl_clk/
  {0x92, 0x01, {0x22}},	    //	frq_cp1_clk[2:0]
  {0x93, 0x01, {0x9f}},	    //   fp7721 power    93 for Jf
  {0x9a, 0x01, {0x00}},		   //   s_out=800
  {0x9c, 0x01, {0x80}},		   //  vlength=1280	
  {0x5a, 0x05, {0x15,0x26,0x2f,0x35,0x30}},    //gamma n 0.4.8.12.20
  {0x47, 0x05, {0x15,0x26,0x2f,0x35,0x30}},    ///gamma P0.4.8.12.20
 	{0x4c, 0x05, {0x3b,0x39,0x52,0x3d,0x43}},  //28.44.64.96.128. 
 	{0x5f, 0x05, {0x3b,0x39,0x52,0x3d,0x43}}, //28.44.64.96.128. 
	{0x64, 0x05, {0x47,0x2e,0x43,0x3d,0x4a}},//159.191.211.227.235  
  {0x51, 0x05, {0x47,0x2e,0x43,0x3d,0x4a}},  //159.191.211.227.235 
	{0x69, 0x04, {0x4b,0x57,0x65,0x7f}},  //243.247.251.255
  {0x56, 0x04, {0x4b,0x57,0x65,0x7f}},  //243.247.251.255
	{0xee, 0x01, {0x70}},  
	{0x00, 0x05, {0x02,0x04,0x00,0x01,0x00}},  
 	{0x05, 0x05, {0x00,0x00,0x00,0x00,0x00}},  
	{0x0a, 0x05, {0x00,0x00,0x05,0x60,0x00}},  
	{0x10, 0x05, {0x06,0x08,0x00,0x01,0x00}},  
 	{0x15, 0x05, {0x00,0x01,0x0d,0x08,0x00}},  
	{0x29, 0x02, {0x05,0x60}},
	{0x30, 0x05, {0x10,0x11,0x55,0x4d,0x5d}},  
	{0x35, 0x05, {0x08,0x11,0x10,0x55,0x4d}}, 
	{0x3a, 0x02, {0x5d,0x08}}, 
	{0x60, 0x05, {0x3f,0x3c,0x20,0x21,0x10}},  
 	{0x65, 0x05, {0x12,0x14,0x16,0x00,0x3c}},  
	{0x6a, 0x05, {0x3c,0x3c,0x3c,0x3c,0x3c}},  
	{0x6f, 0x05, {0x02,0x3c,0x3c,0x3c,0x3c}},
	{0x74, 0x02, {0x3c,0x3c}},
	{0x80, 0x05, {0x3f,0x3c,0x20,0x21,0x11}},  
 	{0x85, 0x05, {0x13,0x15,0x17,0x01,0x3c}},  
	{0x8a, 0x05, {0x3c,0x3c,0x3c,0x3c,0x3c}},  
	{0x8f, 0x05, {0x03,0x3c,0x3c,0x3c,0x3c}},
	{0x94, 0x02, {0x3c,0x3c}},
	{0xea, 0x02, {0x00,0x00}},    // write enable
	{0xee, 0x01, {0x00}},		// ENTER PAGE0
  {0x11,1,{0x00}},
    {REGFLAG_DELAY,120,{}},
    
    {0x29,1,{0x00}},
    {REGFLAG_DELAY,20,{}},
    //{0x35, 1, {0x00} },

    {REGFLAG_END_OF_TABLE,0x00,{}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;
	for(i = 0; i < count; i++) {
		unsigned cmd;
		cmd = table[i].cmd;
		switch (cmd) {
		case REGFLAG_DELAY :
			MDELAY(table[i].count);
			break;
		case REGFLAG_END_OF_TABLE :
			break;
		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

// ---------------------------------------------------------------------------
// LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    params->dsi.mode    = BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;//LCM_THREE_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 


	params->dsi.vertical_sync_active                            =  4; //4; //2; //4;
    params->dsi.vertical_backporch                              = 8; //4; //10; //16;
    params->dsi.vertical_frontporch                             = 20;  //8;//5; 9
    params->dsi.vertical_active_line                            = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active                          = 20; //16; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 20; //48; //60; //60; //80;
    params->dsi.horizontal_frontporch                           = 40; //16; //60; 
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

   // params->dsi.PLL_CLOCK = 231;
   params->dsi.PLL_CLOCK = 220;
}

static void lcd_reset(unsigned char enabled)
{
	lcm_set_gpio_output(GPIO_LCD_RST_EN, enabled);
}
	
static void lcd_power_en(unsigned char enabled)
{
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, enabled);
}
static void lcd_bias_en(unsigned char enabled)
{
	lcm_set_gpio_output(GPIO_LCM_BIAS_EN, enabled);
}



static void lcm_init_lcm(void)
{
    lcd_power_en(1);//LCD 3.3V
	MDELAY(10);
	
	lcd_bias_en(1);//bias en 偏压使能
	//MDELAY(10);
	
	lcd_reset(1);
	MDELAY(20);
	lcd_reset(0);//LCD rst
	MDELAY(20);
	lcd_reset(1);
	MDELAY(120);//Must > 5ms

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

}

static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0]=0x00280500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10); 

	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);
	
	lcd_bias_en(0);//bias en 偏压使能
    MDELAY(20);
	
	lcd_reset(0);
	MDELAY(20);
	
   	lcd_power_en(0);
	MDELAY(20);

}
 
static void lcm_resume(void)
{
	lcm_init_lcm();
}

struct LCM_DRIVER gs716_bns_gh8555bl_wxga_ips_101_lcm_drv = {
	.name = "gs716_bns_gh8555bl_wxga_ips_101",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
