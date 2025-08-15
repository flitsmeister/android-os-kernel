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
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
 {0xFF, 3, {0x61, 0x36, 0x07} },
    {0x03, 1, {0x20} },
    {0x04, 1, {0x04} },
    {0x05, 1, {0x00} },
    {0x06, 1, {0x00} },
    {0x07, 1, {0x00} },
    {0x08, 1, {0x00} },
    {0x09, 1, {0x00} },
    {0x0a, 1, {0x01} },
    {0x0b, 1, {0x01} },
    {0x0c, 1, {0x01} },
    {0x0d, 1, {0x1e} },
    {0x0e, 1, {0x01} },
    {0x0f, 1, {0x01} },
    {0x10, 1, {0x40} },
    {0x11, 1, {0x02} },
    {0x12, 1, {0x05} },
    {0x13, 1, {0x00} },
    {0x14, 1, {0x00} },
    {0x15, 1, {0x00} },
    {0x16, 1, {0x01} },
    {0x17, 1, {0x01} },
    {0x18, 1, {0x00} },
    {0x19, 1, {0x00} },
    {0x1a, 1, {0x00} },
    {0x1b, 1, {0xc0} },
    {0x1c, 1, {0xbb} },
    {0x1d, 1, {0x0b} },
    {0x1e, 1, {0x00} },
    {0x1f, 1, {0x00} },
    {0x20, 1, {0x00} },
    {0x21, 1, {0x00} },
    {0x22, 1, {0x00} },
    {0x23, 1, {0xc0} },
    {0x24, 1, {0x30} },
    {0x25, 1, {0x00} },
    {0x26, 1, {0x00} },
    {0x27, 1, {0x03} },
    {0x30, 1, {0x01} },
    {0x31, 1, {0x23} },
    {0x32, 1, {0x55} },
    {0x33, 1, {0x67} },
    {0x34, 1, {0x89} },
    {0x35, 1, {0xab} },
    {0x36, 1, {0x01} },
    {0x37, 1, {0x23} },
    {0x38, 1, {0x45} },
    {0x39, 1, {0x67} },
    {0x3a, 1, {0x44} },
    {0x3b, 1, {0x55} },
    {0x3c, 1, {0x66} },
    {0x3d, 1, {0x77} },
    {0x50, 1, {0x00} },
    {0x51, 1, {0x0d} },
    {0x52, 1, {0x0d} },
    {0x53, 1, {0x0c} },
    {0x54, 1, {0x0c} },
    {0x55, 1, {0x0f} },
    {0x56, 1, {0x0f} },
    {0x57, 1, {0x0e} },
    {0x58, 1, {0x0e} },
    {0x59, 1, {0x06} },
    {0x5a, 1, {0x07} },
    {0x5b, 1, {0x1f} },
    {0x5c, 1, {0x1f} },
    {0x5d, 1, {0x1f} },
    {0x5e, 1, {0x1f} },
    {0x5f, 1, {0x1f} },
    {0x60, 1, {0x1f} },
    {0x67, 1, {0x06} },
    {0x68, 1, {0x13} },
    {0x69, 1, {0x0f} },
    {0x6a, 1, {0x12} },
    {0x6b, 1, {0x0e} },
    {0x6c, 1, {0x11} },
    {0x6d, 1, {0x0d} },
    {0x6e, 1, {0x10} },
    {0x6f, 1, {0x0c} },
    {0x70, 1, {0x14} },
    {0x71, 1, {0x15} },
    {0x72, 1, {0x08} },
    {0x73, 1, {0x01} },
    {0x74, 1, {0x00} },
    {0x75, 1, {0x02} },
    {0x76, 1, {0x02} },
    {0x83, 1, {0x01} },
    {REGFLAG_DELAY,10,{}},
    {0xFF, 3, {0x61, 0x36, 0x08} },
    {0x1C, 1, {0xA0} },
    {0xFF, 3, {0x61, 0x36, 0x08} },
    {0x4C, 1, {0x00} },
    {0x78, 1, {0x04} },
    {0x8E, 1, {0x12} },
    {0x76, 1, {0xB4} },
    {0x93, 1, {0x02} },
    {0xFF, 3, {0x61, 0x36, 0x01} },
    {0x31, 1, {0x19} },
    {0x50, 1, {0xA9} },
    {0x51, 1, {0xA4} },
    {0x53, 1, {0x80} },
    {0xFF, 3, {0x61, 0x36, 0x01} },
    {0xA0, 1, {0x0E} },
    {0xA1, 1, {0x14} },
    {0xA2, 1, {0x1B} },
    {0xA3, 1, {0x24} },
    {0xA4, 1, {0x1C} },
    {0xA5, 1, {0x22} },
    {0xA6, 1, {0x25} },
    {0xA7, 1, {0x24} },
    {0xA8, 1, {0x29} },
    {0xA9, 1, {0x2D} },
    {0xAA, 1, {0x32} },
    {0xAB, 1, {0x3A} },
    {0xAC, 1, {0x39} },
    {0xAD, 1, {0x30} },
    {0xAE, 1, {0x2C} },
    {0xAF, 1, {0x35} },
    {0xB0, 1, {0x33} },
    {0xFF, 3, {0x61, 0x36, 0x01} },
    {0x36, 1, {0x00} },
    {0xC0, 1, {0x0C} },
    {0xC1, 1, {0x15} },
    {0xC2, 1, {0x1D} },
    {0xC3, 1, {0x24} },
    {0xC4, 1, {0x1B} },
    {0xC5, 1, {0x1F} },
    {0xC6, 1, {0x22} },
    {0xC7, 1, {0x21} },
    {0xC8, 1, {0x27} },
    {0xC9, 1, {0x2C} },
    {0xCA, 1, {0x31} },
    {0xCB, 1, {0x38} },
    {0xCC, 1, {0x38} },
    {0xCD, 1, {0x30} },
    {0xCE, 1, {0x2D} },
    {0xCF, 1, {0x31} },
    {0xD0, 1, {0x34} },
    {0xFF, 3, {0x61, 0x36, 0x08} },
    {0xAB, 1, {0x24} },
    {0xFF, 3, {0x61, 0x36, 0x06} },
    {0x72, 1, {0x01} },
    {0xFF, 3, {0x61, 0x36, 0x00} },
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

    params->dsi.mode    = SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;//LCM_THREE_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 


params->dsi.vertical_sync_active                            =  4; //2; //4;
    params->dsi.vertical_backporch                              = 8; //10; //16;
    params->dsi.vertical_frontporch                             = 4;//5; 
    params->dsi.vertical_active_line                            = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active                          = 8; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 48; //60; //60; //80;
    params->dsi.horizontal_frontporch                           = 15; //60; 
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 325;
   
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
	MDELAY(20);
	
	lcd_bias_en(1);//bias en 偏压使能
	MDELAY(100);
	
	lcd_reset(1);
	MDELAY(50);
	lcd_reset(0);//LCD rst
	MDELAY(50);
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

struct LCM_DRIVER gs716_zg_ili6136s_cpt_wxga_ips_8_lcm_drv = {
	.name = "gs716_zg_ili6136s_cpt_wxga_ips_8",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
