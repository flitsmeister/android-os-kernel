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

#define   LCM_DSI_CMD_MODE	0



#if 1
#define REGFLAG_DELAY 0xFFFD
#define REGFLAG_END_OF_TABLE 0xFFFE
struct LCM_setting_table {
    //unsigned char package;
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = 
{
    {0xE0, 0x02, {0xAB,0xBA} },
    {0xE1, 0x02, {0xBA,0xAB} },
    {0xB0, 0x01, {0x00} },
    {0xB1, 0x04, {0x10,0x01,0x47,0xFF} },//bits
    {0xB2, 0x06, {0x0c,0x14,0x04,0x50,0x50,0x14} },
    {0xB3, 0x03, {0x56,0xD3,0x00} },
    {0xB4, 0x03, {0x22,0x30,0x04} },
    {0xB5, 0x01, {0x00} },
    {0xB6, 0x07, {0xb0,0x00,0x00,0x10,0x00,0x10,0x00} },
    {0xB7, 0x08, {0x0E,0x00,0xFF,0x08,0x08,0xFF,0xFF,0x00} },
    {0xB8, 0x07, {0x16,0x12,0x29,0x49,0x48,0x00,0x00} },
    {0xB9, 0x26, {0x4C,0x44,0x3C,0x33,0x33,0x26,0x2F,0x1B,0x35,0x35,0x35,0x53,0x41,0x48,0x3D,0x38,0x2B,0x19,0x06,0x4C,0x44,0x3C,0x33,0x33,0x26,0x2F,0x1B,0x35,0x35,0x35,0x53,0x41,0x48,0x3D,0x38,0x2B,0x19,0x06} },
    {0xBA, 0x08, {0x00,0x00,0x00,0x44,0x24,0x00,0x00,0x00} },
    {0xBB, 0x03, {0x76,0x00,0x00} },
    {0xBC, 0x02, {0x00,0x00} },
    {0xBD, 0x05, {0xFF,0x00,0x00,0x00,0x00} },
    {0xBE, 0x01, {0x00} },
    {0xC0, 0x10, {0x98,0x76,0x12,0x34,0x33,0x33,0x44,0x44,0x06,0x04,0x8A,0x04,0x0F,0x00,0x00,0x00} },

    {0xC1, 0x0A, {0x53,0x94,0x02,0x85,0x06,0x04,0x8A,0x04,0x54,0x00} },
    {0xC2, 0x0C, {0x37,0x09,0x08,0x89,0x08,0x10,0x22,0x21,0x44,0xBB,0x18,0x00} },
    {0xC3, 0x16, {0x9C,0x1D,0x1E,0x1F,0x10,0x12,0x0C,0x0E,0x05,0x24,0x24,0x24,0x24,0x24,0x24,0x07,0x24,0x24,0x24,0x24,0x24,0x24} },
    {0xC4, 0x16, {0x1C,0x1D,0x1E,0x1F,0x11,0x13,0x0D,0x0F,0x04,0x24,0x24,0x24,0x24,0x24,0x24,0x06,0x24,0x24,0x24,0x24,0x24,0x24} },
    {0xC5, 0x03, {0xE8,0x85,0x76} },
    {0xC6, 0x02, {0x20,0x20} },
    {0xC7, 0x16, {0x41,0x01,0x0D,0x11,0x09,0x15,0x19,0x4F,0x10,0xD7,0xCF,0x19,0x1B,0x1D,0x03,0x02,0x25,0x30,0x00,0x03,0xFF,0x00} },
    {0xC8, 0x06, {0x61,0x00,0x31,0x42,0x54,0x16} },
    {0xC9, 0x05, {0xA1,0x22,0xFF,0xC4,0x23} },
    {0xCA, 0x02, {0x4B,0x43} },
    {0xCC, 0x04, {0x2E,0x02,0x04,0x08} },
    {0xCD, 0x08, {0x0E,0x64,0x64,0x20,0x1E,0x6B,0x06,0x83} },
    {0xD0, 0x03, {0x07,0x10,0x80} },
    {0xD1, 0x04, {0x00,0x0D,0xFF,0x0F} },
    {0xD2, 0x04, {0xE3,0x2B,0x38,0x00} },
    {0xD4, 0x0B, {0x00,0x01,0x00,0x0E,0x04,0x44,0x08,0x10,0x00,0x07,0x00} },
    {0xD5, 0x01, {0x00} },
    {0xD6, 0x01, {0x00,0x00} },
    {0xD7, 0x04, {0x00,0x00,0x00,0x00} },
    {0xE4, 0x03, {0x08,0x55,0x03} },
    {0xE6, 0x08, {0x00,0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF} },
    {0xE7, 0x03, {0x00,0x00,0x00} },

    {0xE8, 0x07, {0xD5,0xFF,0xFF,0xFF,0x00,0x00,0x00} },
    {0xE9, 0x01, {0xFF} },
    {0xF0, 0x05, {0x12,0x03,0x20,0x00,0xFF} },
    {0xF1, 0x1A, {0xA6,0xC8,0xEA,0xE6,0xE4,0xCC,0xE4,0xBE,0xF0,0xB2,0xAA,0xC7,0xFF,0x66,0x98,0xE3,0x87,0xC8,0x99,0xC8,0x8C,0xBE,0x96,0x91,0x8F,0xFF} },
    {0xF3, 0x01, {0x03} },
    {0xF4, 0x1A, {0xFF,0xFE,0xFC,0xFA,0xF8,0xF4,0xF0,0xE8,0xE0,0xD0,0xC0,0xA0,0x80,0x7F,0x5F,0x3F,0x2F,0x1F,0x17,0x0F,0x0B,0x07,0x05,0x03,0x01,0x00} },
    {0xF5, 0x1A, {0xFF,0xFE,0xFC,0xFA,0xF8,0xF4,0xF0,0xE8,0xE0,0xD0,0xC0,0xA0,0x80,0x7F,0x5F,0x3F,0x2F,0x1F,0x17,0x0F,0x0B,0x07,0x05,0x03,0x01,0x00} },
    {0xF6, 0x1A, {0xFF,0xFE,0xFC,0xFA,0xF8,0xF4,0xF0,0xE8,0xE0,0xD0,0xC0,0xA0,0x80,0x7F,0x5F,0x3F,0x2F,0x1F,0x17,0x0F,0x0B,0x07,0x05,0x03,0x01,0x00} },
    {0xF7, 0x07, {0x00,0x00,0x00,0x00,0x00,0x00,0x00} },
    {0xF8, 0x07, {0x00,0x00,0x00,0x00,0x00,0x00,0x00} },
    {0xF9, 0x07, {0x00,0x00,0x00,0x00,0x00,0x00,0x00} },
    {0xFA, 0x19, {0x00,0x84,0x12,0x21,0x48,0x48,0x21,0x12,0x84,0x69,0x69,0x5A,0xA5,0x96,0x96,0xA5,0x5A,0xB7,0xDE,0xED,0x7B,0x7B,0xED,0xDE,0xB7} },
    {0xFB, 0x17, {0x00,0x12,0x0F,0xFF,0xFF,0xFF,0x00,0x38,0x40,0x08,0x70,0x0B,0x40,0x19,0x50,0x21,0xC0,0x27,0x60,0x2D,0x00,0x00,0x0F} },
    {0xE3, 0x02, {0x20,0x21} },

    {0x11,0,{0x00}},
    {REGFLAG_DELAY,150,{}},   
    {0x29,0,{0x00}},
    {REGFLAG_DELAY, 20, {}},
    
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

//printf("[LK/LCM]:%s %d enter  \n", __func__,__LINE__);
//printf("[LK/LCM]push_table init cmd =%02X,  i = %u, count = %u ,table[i].para_list = %hhu ,force_update = %hhu\n", table[i].cmd,i, count,table[i].para_list,force_update);
    for(i = 0; i < count; i++) {
        unsigned cmd;
        cmd = table[i].cmd;
       // printf("---------[LK/LCM]%s %d i = %d-----------\n", __func__,__LINE__,i);
       // printf("[LK/LCM]push_table init cmd =%02X,  i = %u, count = %u ,table[i].para_list = %hhu ,force_update = %hhu\n", table[i].cmd,i, count,table[i].para_list,force_update);
        switch (cmd) {
        /*    case 0xb6:
            table[i].para_list[0] = vcom;
            table[i].para_list[1] = vcom;
            dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
            vcom -= 1;
            break;
            */
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
#endif


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


    params->dsi.vertical_sync_active                            = 4; //2; //4;
    params->dsi.vertical_backporch                              = 12; //10; //16;
    params->dsi.vertical_frontporch                             = 30;//5; 
    params->dsi.vertical_active_line                            = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active                          = 4; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 60; //60; //60; //80;
    params->dsi.horizontal_frontporch                           = 60; //60; 
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    //params->dsi.cont_clock    = 1;//1
    //params->dsi.ssc_disable = 0;//0
    params->dsi.PLL_CLOCK = 221;
    // 
    // 
    //     params->dsi.vertical_sync_active                            = 8; //2; //4;
    // params->dsi.vertical_backporch                              = 22; //10; //16;
    // params->dsi.vertical_frontporch                             = 16;//5; 
    // params->dsi.vertical_active_line                            = FRAME_HEIGHT; 

    // params->dsi.horizontal_sync_active                          = 20; // 10; //5;//6;
    // params->dsi.horizontal_backporch                            = 80; //60; //60; //80;
    // params->dsi.horizontal_frontporch                           = 80; //60; 
    // params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    // //params->dsi.cont_clock    = 1;//1
    // //params->dsi.ssc_disable = 0;//0
    // params->dsi.PLL_CLOCK = 234;
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


/* 	data_array[0] = 0x00280500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(40);
	data_array[0] = 0x00100500;
	dsi_set_cmdq(data_array, 1, 1); */
	     /* enable AVDD & AVEE */

	//printk("suspend_kzh:%s:%d GPIO_LCD_PWR_EN3.3V:%d GPIO_LCM_BIAS_EN:%d GPIO_LCD_RST_EN:%d\n",__func__,__LINE__,gpio_get_value(GPIO_LCD_PWR_EN),gpio_get_value(GPIO_LCM_BIAS_EN),gpio_get_value(GPIO_LCD_RST_EN));
}
  
static void lcm_resume(void)
{
	lcm_init_lcm();
}

struct LCM_DRIVER gs716_xy_er88577_wxganl_ips_8_lcm_drv = {
	.name = "gs716_xy_er88577_wxganl_ips_8",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
