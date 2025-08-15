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

#define GPIO_LCD_RST_EN        GPIO_LCM_RST
#define GPIO_LCD_PWR_EN         GPIO_LCM_PWR
#define GPIO_LCM_BIAS_EN            GPIO_LCM_VDD

#define REGFLAG_DELAY 0xFD
#define REGFLAG_END_OF_TABLE 0xFE

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

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

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

#define   LCM_DSI_CMD_MODE	0

static struct LCM_setting_table lcm_initialization_setting[] = {	
  {0xFF, 3, {0x98, 0x81, 0x03} },
    {0x01, 1, {0x00} },
    {0x02, 1, {0x00} },
    {0x03, 1, {0x53} },
    {0x04, 1, {0x53} },
    {0x05, 1, {0x13} },
    {0x06, 1, {0x04} },
    {0x07, 1, {0x02} },
    {0x08, 1, {0x02} },
    {0x09, 1, {0x00} },
    {0x0a, 1, {0x00} },
    {0x0b, 1, {0x00} },
    {0x0c, 1, {0x00} },
    {0x0d, 1, {0x00} },
    {0x0e, 1, {0x00} },
    {0x0f, 1, {0x00} },
    {0x10, 1, {0x00} },
    {0x11, 1, {0x00} },
    {0x12, 1, {0x00} },
    {0x13, 1, {0x00} },
    {0x14, 1, {0x00} },
    {0x15, 1, {0x05} },
    {0x16, 1, {0x05} },
    {0x17, 1, {0x03} },
    {0x18, 1, {0x03} },
    {0x19, 1, {0x00} },
    {0x1a, 1, {0x00} },
    {0x1b, 1, {0x00} },
    {0x1c, 1, {0x00} },
    {0x1d, 1, {0x00} },
    {0x1e, 1, {0xc0} },
    {0x1f, 1, {0x00} },
    {0x20, 1, {0x02} },
    {0x21, 1, {0x09} },
    {0x22, 1, {0x00} },
    {0x23, 1, {0x00} },
    {0x24, 1, {0x00} },
    {0x25, 1, {0x00} },
    {0x26, 1, {0x00} },
    {0x27, 1, {0x00} },
    {0x28, 1, {0x55} },
    {0x29, 1, {0x03} },
    {0x2a, 1, {0x00} },
    {0x2b, 1, {0x00} },
    {0x2c, 1, {0x00} },
    {0x2d, 1, {0x00} },
    {0x2e, 1, {0x00} },
    {0x2f, 1, {0x00} },
    {0x30, 1, {0x00} },
    {0x31, 1, {0x00} },
    {0x32, 1, {0x00} },
    {0x33, 1, {0x00} },
    {0x34, 1, {0x00} },
    {0x35, 1, {0x00} },
    {0x36, 1, {0x00} },
    {0x37, 1, {0x00} },
    {0x38, 1, {0x3C} },
    {0x39, 1, {0x00} },
    {0x3a, 1, {0x00} },
    {0x3b, 1, {0x00} },
    {0x3c, 1, {0x00} },
    {0x3d, 1, {0x00} },
    {0x3e, 1, {0x00} },
    {0x3f, 1, {0x00} },
    {0x40, 1, {0x00} },
    {0x41, 1, {0x00} },
    {0x42, 1, {0x00} },
    {0x43, 1, {0x00} },
    {0x44, 1, {0x00} },
    {0x50, 1, {0x01} },
    {0x51, 1, {0x23} },
    {0x52, 1, {0x45} },
    {0x53, 1, {0x67} },
    {0x54, 1, {0x89} },
    {0x55, 1, {0xab} },
    {0x56, 1, {0x01} },
    {0x57, 1, {0x23} },
    {0x58, 1, {0x45} },
    {0x59, 1, {0x67} },
    {0x5a, 1, {0x89} },
    {0x5b, 1, {0xab} },
    {0x5c, 1, {0xcd} },
    {0x5d, 1, {0xef} },
    {0x5e, 1, {0x01} },
    {0x5f, 1, {0x0A} },
    {0x60, 1, {0x02} },
    {0x61, 1, {0x02} },
    {0x62, 1, {0x08} },
    {0x63, 1, {0x15} },
    {0x64, 1, {0x14} },
    {0x65, 1, {0x02} },
    {0x66, 1, {0x11} },
    {0x67, 1, {0x10} },
    {0x68, 1, {0x02} },
    {0x69, 1, {0x0F} },
    {0x6a, 1, {0x0E} },
    {0x6b, 1, {0x02} },
    {0x6c, 1, {0x0D} },
    {0x6d, 1, {0x0C} },
    {0x6e, 1, {0x06} },
    {0x6f, 1, {0x02} },
    {0x70, 1, {0x02} },
    {0x71, 1, {0x02} },
    {0x72, 1, {0x02} },
    {0x73, 1, {0x02} },
    {0x74, 1, {0x02} },
    {0x75, 1, {0x0A} },
    {0x76, 1, {0x02} },
    {0x77, 1, {0x02} },
    {0x78, 1, {0x06} },
    {0x79, 1, {0x15} },
    {0x7a, 1, {0x14} },
    {0x7b, 1, {0x02} },
    {0x7c, 1, {0x10} },
    {0x7d, 1, {0x11} },
    {0x7e, 1, {0x02} },
    {0x7f, 1, {0x0C} },
    {0x80, 1, {0x0D} },
    {0x81, 1, {0x02} },
    {0x82, 1, {0x0E} },
    {0x83, 1, {0x0F} },
    {0x84, 1, {0x08} },
    {0x85, 1, {0x02} },
    {0x86, 1, {0x02} },
    {0x87, 1, {0x02} },
    {0x88, 1, {0x02} },
    {0x89, 1, {0x02} },
    {0x8A, 1, {0x02} },
    {0xFF, 3, {0x98, 0x81, 0x04} },
    {0x3B, 1, {0xC0} },
    {0x6C, 1, {0x15} },
    {0x6E, 1, {0x30} },
    {0x6F, 1, {0x33} },
    {0x8D, 1, {0x1F} },
    {0x3A, 1, {0xA4} },
    {0x35, 1, {0x17} },
    {0x87, 1, {0xBA} },
    {0x26, 1, {0x76} },
    {0xB2, 1, {0xD1} },
    {0xB5, 1, {0x07} },
    {0x88, 1, {0x0B} },
    {0x21, 1, {0x30} },
    {0xFF, 3, {0x98, 0x81, 0x01} },
    {0x22, 1, {0x0A} },
    {0x31, 1, {0x09} },
    {0x40, 1, {0x33} },
    {0x42, 1, {0x44} },
    {0x53, 1, {0x30} },
    {0x55, 1, {0x88} },
    {0x50, 1, {0x95} },
    {0x51, 1, {0x95} },
    {0x60, 1, {0x30} },
    {0x2E, 1, {0xC8} },
    {0xA0, 1, {0x0F} },
    {0xA1, 1, {0x18} },
    {0xA2, 1, {0x26} },
    {0xA3, 1, {0x11} },
    {0xA4, 1, {0x19} },
    {0xA5, 1, {0x27} },
    {0xA6, 1, {0x1C} },
    {0xA7, 1, {0x1D} },
    {0xA8, 1, {0x7A} },
    {0xA9, 1, {0x1D} },
    {0xAA, 1, {0x28} },
    {0xAB, 1, {0x67} },
    {0xAC, 1, {0x19} },
    {0xAD, 1, {0x17} },
    {0xAE, 1, {0x4C} },
    {0xAF, 1, {0x21} },
    {0xB0, 1, {0x28} },
    {0xB1, 1, {0x4B} },
    {0xB2, 1, {0x6B} },
    {0xB3, 1, {0x3F} },
    {0xC0, 1, {0x01} },
    {0xC1, 1, {0x18} },
    {0xC2, 1, {0x25} },
    {0xC3, 1, {0x11} },
    {0xC4, 1, {0x0F} },
    {0xC5, 1, {0x27} },
    {0xC6, 1, {0x1B} },
    {0xC7, 1, {0x1E} },
    {0xC8, 1, {0x7A} },
    {0xC9, 1, {0x1C} },
    {0xCA, 1, {0x29} },
    {0xCB, 1, {0x68} },
    {0xCC, 1, {0x19} },
    {0xCD, 1, {0x17} },
    {0xCE, 1, {0x4C} },
    {0xCF, 1, {0x20} },
    {0xD0, 1, {0x28} },
    {0xD1, 1, {0x4A} },
    {0xD2, 1, {0x6B} },
    {0xD3, 1, {0x3F} },
    {0xFF, 3, {0x98, 0x81, 0x00} },
    // {0x35, 1, {0x00} },
    // {0x11, 1, {0x00} },
    // {0x29, 1, {0x00} },
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
static void init_lcm_registers(void)
{
    unsigned int data_array[16];
	
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	
    data_array[0] = 0x00350500;     // 
    dsi_set_cmdq(data_array, 1, 1);


    data_array[0] = 0x00110500;  	// SLPOUT
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);

//DISP ON
    data_array[0] = 0x00290500;  	// DSPON
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(5);
	


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

    params->dsi.mode    = BURST_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;//LCM_THREE_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 


  // params->dsi.vertical_sync_active                            = 16; //2; //4;
  //   params->dsi.vertical_backporch                              = 60; //10; //16;
  //   params->dsi.vertical_frontporch                             = 60;//5; 
  //   params->dsi.vertical_active_line                            = FRAME_HEIGHT; 

  //   params->dsi.horizontal_sync_active                          = 4; // 10; //5;//6;
  //   params->dsi.horizontal_backporch                            = 10; //60; //60; //80;
  //   params->dsi.horizontal_frontporch                           = 10; //60; 
  //   params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

  //   //params->dsi.cont_clock    = 1;//1
  //   //params->dsi.ssc_disable = 0;//0
  //   params->dsi.PLL_CLOCK = 220;
     params->dsi.vertical_sync_active                            = 4; //2; //4;
    params->dsi.vertical_backporch                              = 10; //10; //16;
    params->dsi.vertical_frontporch                             = 10;//5; 
    params->dsi.vertical_active_line                            = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active                          = 16; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 60; //60; //60; //80;
    params->dsi.horizontal_frontporch                           = 60; //60; 
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    //params->dsi.cont_clock    = 1;//1
    //params->dsi.ssc_disable = 0;//0
    params->dsi.PLL_CLOCK = 240;
    params->dsi.noncont_clock = 1;
    params->dsi.noncont_clock_period= 2;
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
	MDELAY(100);
	
	lcd_reset(1);
	MDELAY(20);
	lcd_reset(0);//LCD rst
	MDELAY(20);
	lcd_reset(1);
	MDELAY(150);//Must > 5ms

	lcd_bias_en(1);//bias en 偏压使能
	init_lcm_registers();
	MDELAY(50);
//	lcm_set_gpio_output(GPIO_LCD_BL_EN,1);

}

static void lcm_suspend(void)
{
	lcd_bias_en(0);//bias en 偏压使能
	//unsigned int data_array[16];
	lcd_reset(0);
	MDELAY(100);
   	lcd_power_en(0);
	MDELAY(150);


/* 	data_array[0] = 0x00280500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(40);
	data_array[0] = 0x00100500;
	dsi_set_cmdq(data_array, 1, 1); */
	     /* enable AVDD & AVEE */

	//printk("suspend_kzh:%s:%d GPIO_LCD_PWR_EN3.3V:%d GPIO_LCM_BIAS_EN:%d GPIO_LCD_RST_EN:%d\n",__func__,__LINE__,gpio_get_value(GPIO_LCD_PWR_EN),gpio_get_value(GPIO_LCM_BIAS_EN),gpio_get_value(GPIO_LCD_RST_EN));
	//printk("[Kernel/LCM-kzh]: gs716 enter lcm_suspend   !\n");
}
  
static void lcm_resume(void)
{
	lcm_init_lcm();
	//printk("resume_kzh:%s:%d GPIO_LCD_PWR_EN3.3V:%d GPIO_LCM_BIAS_EN:%d GPIO_LCD_RST_EN:%d\n",__func__,__LINE__,gpio_get_value(GPIO_LCD_PWR_EN),gpio_get_value(GPIO_LCM_BIAS_EN),gpio_get_value(GPIO_LCD_RST_EN));
	//printk("[Kernel/LCM-kzh]: gs716 enter lcm_resume   !\n");
}

struct LCM_DRIVER gs716_ry_ili9881_zs101ni4042j4h8ii_b_cpt_wxga_ips_101_lcm_drv = {
	.name = "gs716_ry_ili9881_zs101ni4042j4h8ii_b_cpt_wxga_ips_101",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
