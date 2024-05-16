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
	{0xe0, 1, {0x00}},
	{0xe1, 1, {0x93}},
	{0xe2, 1, {0x65}},
	{0xe3, 1, {0xf8}},
	{0x80, 1, {0x03}},
	{0xe0, 1, {0x01}},
	{0x00, 1, {0x00}},
	{0x01, 1, {0x3b}},
	{0x0c, 1, {0x74}},
	{0x17, 1, {0x00}},
	{0x18, 1, {0xaf}},
	{0x19, 1, {0x00}},
	{0x1a, 1, {0x00}},
	{0x1b, 1, {0xaf}},
	{0x1c, 1, {0x00}},
	{0x35, 1, {0x26}},
	{0x37, 1, {0x09}},
	{0x38, 1, {0x04}},
	{0x39, 1, {0x00}},
	{0x3a, 1, {0x01}},
	{0x3c, 1, {0x78}},
	{0x3d, 1, {0xff}},
	{0x3e, 1, {0xff}},
	{0x3f, 1, {0x7f}},
	{0x40, 1, {0x06}},
	{0x41, 1, {0xa0}},
	{0x42, 1, {0x81}},
	{0x43, 1, {0x14}},
	{0x44, 1, {0x23}},
	{0x45, 1, {0x28}},
	{0x55, 1, {0x02}},
	{0x57, 1, {0x69}},
	{0x59, 1, {0x0a}},
	{0x5a, 1, {0x2a}},
	{0x5b, 1, {0x17}},
	{0x5d, 1, {0x7f}},
	{0x5e, 1, {0x6b}},
	{0x5f, 1, {0x5c}},
	{0x60, 1, {0x4f}},
	{0x61, 1, {0x4d}},
	{0x62, 1, {0x3f}},
	{0x63, 1, {0x42}},
	{0x64, 1, {0x2b}},
	{0x65, 1, {0x44}},
	{0x66, 1, {0x43}},
	{0x67, 1, {0x43}},
	{0x68, 1, {0x63}},
	{0x69, 1, {0x52}},
	{0x6a, 1, {0x5a}},
	{0x6b, 1, {0x4f}},
	{0x6c, 1, {0x4e}},
	{0x6d, 1, {0x20}},
	{0x6e, 1, {0x0f}},
	{0x6f, 1, {0x00}},
	{0x70, 1, {0x7f}},
	{0x71, 1, {0x6b}},
	{0x72, 1, {0x5c}},
	{0x73, 1, {0x4f}},
	{0x74, 1, {0x4d}},
	{0x75, 1, {0x3f}},
	{0x76, 1, {0x42}},
	{0x77, 1, {0x2b}},
	{0x78, 1, {0x44}},
	{0x79, 1, {0x43}},
	{0x7a, 1, {0x43}},
	{0x7b, 1, {0x63}},
	{0x7c, 1, {0x52}},
	{0x7d, 1, {0x5a}},
	{0x7e, 1, {0x4f}},
	{0x7f, 1, {0x4e}},
	{0x80, 1, {0x20}},
	{0x81, 1, {0x0f}},
	{0x82, 1, {0x00}},
	{0xe0, 1, {0x02}},
	{0x00, 1, {0x02}},
	{0x01, 1, {0x02}},
	{0x02, 1, {0x00}},
	{0x03, 1, {0x00}},
	{0x04, 1, {0x1e}},
	{0x05, 1, {0x1e}},
	{0x06, 1, {0x1f}},
	{0x07, 1, {0x1f}},
	{0x08, 1, {0x1f}},
	{0x09, 1, {0x17}},
	{0x0a, 1, {0x17}},
	{0x0b, 1, {0x37}},
	{0x0c, 1, {0x37}},
	{0x0d, 1, {0x47}},
	{0x0e, 1, {0x47}},
	{0x0f, 1, {0x45}},
	{0x10, 1, {0x45}},
	{0x11, 1, {0x4b}},
	{0x12, 1, {0x4b}},
	{0x13, 1, {0x49}},
	{0x14, 1, {0x49}},
	{0x15, 1, {0x1f}},
	{0x16, 1, {0x01}},
	{0x17, 1, {0x01}},
	{0x18, 1, {0x00}},
	{0x19, 1, {0x00}},
	{0x1a, 1, {0x1e}},
	{0x1b, 1, {0x1e}},
	{0x1c, 1, {0x1f}},
	{0x1d, 1, {0x1f}},
	{0x1e, 1, {0x1f}},
	{0x1f, 1, {0x17}},
	{0x20, 1, {0x17}},
	{0x21, 1, {0x37}},
	{0x22, 1, {0x37}},
	{0x23, 1, {0x46}},
	{0x24, 1, {0x46}},
	{0x25, 1, {0x44}},
	{0x26, 1, {0x44}},
	{0x27, 1, {0x4a}},
	{0x28, 1, {0x4a}},
	{0x29, 1, {0x48}},
	{0x2a, 1, {0x48}},
	{0x2b, 1, {0x1f}},
	{0x2c, 1, {0x01}},
	{0x2d, 1, {0x01}},
	{0x2e, 1, {0x00}},
	{0x2f, 1, {0x00}},
	{0x30, 1, {0x1f}},
	{0x31, 1, {0x1f}},
	{0x32, 1, {0x1e}},
	{0x33, 1, {0x1e}},
	{0x34, 1, {0x1f}},
	{0x35, 1, {0x17}},
	{0x36, 1, {0x17}},
	{0x37, 1, {0x37}},
	{0x38, 1, {0x37}},
	{0x39, 1, {0x08}},
	{0x3a, 1, {0x08}},
	{0x3b, 1, {0x0a}},
	{0x3c, 1, {0x0a}},
	{0x3d, 1, {0x04}},
	{0x3e, 1, {0x04}},
	{0x3f, 1, {0x06}},
	{0x40, 1, {0x06}},
	{0x41, 1, {0x1f}},
	{0x42, 1, {0x02}},
	{0x43, 1, {0x02}},
	{0x44, 1, {0x00}},
	{0x45, 1, {0x00}},
	{0x46, 1, {0x1f}},
	{0x47, 1, {0x1f}},
	{0x48, 1, {0x1e}},
	{0x49, 1, {0x1e}},
	{0x4a, 1, {0x1f}},
	{0x4b, 1, {0x17}},
	{0x4c, 1, {0x17}},
	{0x4d, 1, {0x37}},
	{0x4e, 1, {0x37}},
	{0x4f, 1, {0x09}},
	{0x50, 1, {0x09}},
	{0x51, 1, {0x0b}},
	{0x52, 1, {0x0b}},
	{0x53, 1, {0x05}},
	{0x54, 1, {0x05}},
	{0x55, 1, {0x07}},
	{0x56, 1, {0x07}},
	{0x57, 1, {0x1f}},
	{0x58, 1, {0x40}},
	{0x5b, 1, {0x30}},
	{0x5c, 1, {0x16}},
	{0x5d, 1, {0x34}},
	{0x5e, 1, {0x05}},
	{0x5f, 1, {0x02}},
	{0x63, 1, {0x00}},
	{0x64, 1, {0x6a}},
	{0x67, 1, {0x73}},
	{0x68, 1, {0x1d}},
	{0x69, 1, {0x08}},
	{0x6a, 1, {0x6a}},
	{0x6b, 1, {0x08}},
	{0x6c, 1, {0x00}},
	{0x6d, 1, {0x00}},
	{0x6e, 1, {0x00}},
	{0x6f, 1, {0x88}},
	{0x75, 1, {0xff}},
	{0x77, 1, {0xdd}},
	{0x78, 1, {0x3f}},
	{0x79, 1, {0x15}},
	{0x7a, 1, {0x17}},
	{0x7d, 1, {0x14}},
	{0x7e, 1, {0x82}},
	{0xe0, 1, {0x04}},
	{0x00, 1, {0x0e}},
	{0x02, 1, {0xb3}},
	{0x09, 1, {0x61}},
	{0x0e, 1, {0x48}},
	{0xe0, 1, {0x00}},
	{0xe6, 1, {0x02}},
	{0xe7, 1, {0x0c}},

     {0x11,1,{0x00}},
    {REGFLAG_DELAY,120,{}},
    
    {0x29,1,{0x00}},
    {REGFLAG_DELAY,20,{}},
    {0x35, 1, {0x00} },

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
			break;
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


    params->dsi.vertical_sync_active          = 4;
    params->dsi.vertical_backporch            = 20;
    params->dsi.vertical_frontporch         = 20;
    params->dsi.vertical_active_line        = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active          = 20;
    params->dsi.horizontal_backporch        = 20;
    params->dsi.horizontal_frontporch       = 40;
  params->dsi.horizontal_active_pixel = FRAME_WIDTH;

  params->dsi.PLL_CLOCK = 210;
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

struct LCM_DRIVER gs717_sat_jd9365da_h3_boe_wxga_101_lcm_drv = {
	.name = "gs717_sat_jd9365da_h3_boe_wxga_101",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
