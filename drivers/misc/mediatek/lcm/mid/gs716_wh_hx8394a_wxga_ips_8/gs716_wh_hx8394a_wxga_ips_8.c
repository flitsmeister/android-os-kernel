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

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

    params->type   = LCM_TYPE_DSI;

    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    params->dsi.mode    = BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 



    params->dsi.vertical_sync_active                = 6;
    params->dsi.vertical_backporch                  = 6;
    params->dsi.vertical_frontporch                 = 8;
    params->dsi.vertical_active_line                = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active              = 8;
    params->dsi.horizontal_backporch                = 120;
    params->dsi.horizontal_frontporch               = 40;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 220 ;//198
   params->dsi.ssc_disable = 1;
    params->dsi.cont_clock      = 1;
    params->dsi.edp_panel = 1;
}


#define   LCM_DSI_CMD_MODE	0
#if 1
static void init_lcm_registers(void)
{
    unsigned int data_array[16];  
    printk("LCM : gs716_wh_hx8394a_wxga_ips_8 %s %d \n",__FUNCTION__,__LINE__);
data_array[0] = 0x00043902;
data_array[1] = 0x9483FFB9;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);

data_array[0] = 0x00033902;
data_array[1] = 0x008373BA;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);


data_array[0] = 0x00103902;
data_array[1] = 0x15156CB1;
data_array[2] = 0xF111E424;
data_array[3] = 0x23D7E480;
data_array[4] = 0x58D2C080;
dsi_set_cmdq(data_array, 5, 1);
MDELAY(1);

data_array[0] = 0x000C3902;
data_array[1] = 0x106400B2;
data_array[2] = 0x081C2007;
data_array[3] = 0x004D1C08;

dsi_set_cmdq(data_array, 4, 1);
MDELAY(1);



data_array[0] = 0x000D3902;
data_array[1] = 0x03FF00B4;
data_array[2] = 0x035A035A;
data_array[3] = 0x016A015A;
data_array[4] = 0x0000006A;
dsi_set_cmdq(data_array, 5, 1);
MDELAY(1);




// data_array[0] = 0x00053902;
// data_array[1] = 0x7F0180B0;
// data_array[2] = 0x0000000F;//bit
// dsi_set_cmdq(data_array, 3, 1);
// MDELAY(1);

data_array[0] = 0x001F3902;
data_array[1] = 0x000600D3;
data_array[2] = 0x00081A40;
data_array[3] = 0x00071032;
data_array[4] = 0x0F155407;
data_array[5] = 0x12020405;
data_array[6] = 0x33070510;
data_array[7] = 0x370B0B33;
data_array[8] = 0x00070710;
dsi_set_cmdq(data_array, 9, 1);
MDELAY(1);




data_array[0] = 0x002D3902;
data_array[1] = 0x181919D5;
data_array[2] = 0x1B1A1A18;

data_array[3] = 0x0605041B;
data_array[4] = 0x02010007;
data_array[5] = 0x18212003;

data_array[6] = 0x18181818;
data_array[7] = 0x18181818;
data_array[8] = 0x22181818;

data_array[9] =  0x18181823;

data_array[10] = 0x18181818;
data_array[11] = 0x18181818;
data_array[12] = 0x00000018;
dsi_set_cmdq(data_array, 13, 1);
MDELAY(1);

data_array[0] = 0x002D3902;
data_array[1] = 0x191818D6;
data_array[2] = 0x1B1A1A19;

data_array[3] = 0x0102031B;
data_array[4] = 0x05060700;
data_array[5] = 0x18222304;

data_array[6] = 0x18181818;
data_array[7] = 0x18181818;
data_array[8] = 0x21181818;

data_array[9] = 0x18181820;

data_array[10] = 0x18181818;
data_array[11] = 0x18181818;
data_array[12] = 0x00000018;
dsi_set_cmdq(data_array, 13, 1);
MDELAY(1);


data_array[0] = 0x002B3902;
data_array[1] = 0x0C0600E0;
data_array[2] = 0x1D3F3431;

data_array[3] = 0x0C0A0641;
data_array[4] = 0x15120F17;
data_array[5] = 0x12071413;

data_array[6] = 0x06001615;
data_array[7] = 0x3F34300B;
data_array[8] = 0x0A07401D;

data_array[9] = 0x120E180D;

data_array[10] = 0x08141214;
data_array[11] = 0x00191413;
dsi_set_cmdq(data_array, 12, 1);
MDELAY(1);




data_array[0] = 0x00033902;
data_array[1] = 0x006060B6;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);

data_array[0] = 0x00033902;
data_array[1] = 0x000808C6;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);


data_array[0] = 0x00023902;
data_array[1] = 0x000009CC;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);

data_array[0] = 0x00023902;
data_array[1] = 0x000055D2;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);

data_array[0] = 0x00033902;
data_array[1] = 0x001430C0;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);

data_array[0] = 0x00043902;
data_array[1] = 0x010E41BF;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);


data_array[0] = 0x00053902;
data_array[1] = 0x40C000C7;
data_array[2] = 0x000000C0;
dsi_set_cmdq(data_array, 3, 1);
MDELAY(1);



data_array[0] = 0x8EDF1500;        //VN52
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           



    
    data_array[0] = 0x00110500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(120);

data_array[0] = 0x00290500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(5);
    printk("LCM : gs716_wh_hx8394a_wxga_ips_8 %s %d \n",__FUNCTION__,__LINE__);
}
#endif
#if 0
#define REGFLAG_DELAY             							0XFFFE
#define REGFLAG_END_OF_TABLE      							0xFFFF   // END OF REGISTERS MARKER
struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[128];
};


static struct LCM_setting_table lcm_initialization_setting[] = 
{	

    // {0xB9,3,{0xFF,0x83,0x94}},


    // {0xBA,16,{0x13,0x82,0x00,0x16,0xC5,0x00,0x10,0xFF,0x0F,0x24,0x03,0x21,0x24,0x25,0x20,0x08}},
    // {0xB1,17,{0x01,0x00,0x04,0xC4,0x03,0x12,0xF1,0x24,0x2C,0x3F,0x3F,0x57,0x02,0x00,0xE6,0xE2,0xA6}},
    // {0xB2,6,{0x00,0xC8,0x0E,0x30,0x00,0x11}},
    // {0xB4,31,{0x80,0x04,0x32,0x10,0x08,0x54,0x15,0x0F,0x22,0x10,0x08,0x47,0x43,0x44,0x0A,0x4B,0x43,0x44,0x02,0x55,0x55,0x02,0x06,0x44,0x06,0x5F,0x0A,0x6B,0x70,0x05,0x08}},




    // {0xB6,1,{0x38}},
    // {0xD5,54,{0x00,0x00,0x00,0x00,0x0A,0x00,0x01,0x22,0x00,0x22,0x66,0x11,0x01,0x01,0x23,0x45,0x67,0x9A,0xBC,0xAA,0xBB,0x45,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x67,0x88,0x88,0x08,0x81,0x29,0x83,0x88,0x08,0x48,0x81,0x85,0x28,0x68,0x83,0x87,0x88,0x48,0x85,0x00,0x00,0x00,0x00,0x3C,0x01}},
    // {0xCC,1,{0x01}},
    // {0xBF,4,{0x06,0x02,0x10,0x04}},
    // {0xC7,4,{0x00,0x10,0x00,0x10}},
    // {0xE0,42,{0x00,0x35,0x33,0x31,0x35,0x3f,0x38,0x47,0x06,0x0c,0x0e,0x13,0x15,0x13,0x14,0x11,0x18,0x00,0x33,0x35,0x31,0x35,0x3f,0x38,0x47,0x06,0x0c,0x0e,0x13,0x15,0x13,0x14,0x11,0x18,0x0b,0x17,0x07,0x12,0x0b,0x17,0x07,0x12}},
    // {0xC0,2,{0x0C,0x17}},
    // {0xC6,2,{0x08,0x08}},
    // {0xD4,1,{0x32}},
      {0xB9,3,{0xFF,0x83,0x94}},

    {0xBA,2,{0x73,0x83}},
    //{0xB0,4,{0x80,0x01,0x7F,0x0F}},//bit    

    {0xB1,15,{0x6C,0x15,0x15,0x24,0xE4,0x11,0xF1,0x80,0xE4,0xD7,0x23,0x80,0xC0,0xD2,0x58}},
    {0xB2,11,{0x00,0x64,0x10,0x07,0x20,0x1C,0x08,0x08,0x1C,0x4D,0x00}},

     {0xB4,12,{0x00,0xFF,0x03,0x5A,0x03,0x5A,0x03,0x5A,0x01,0x6A,0x01,0x6A}},
    {0xD3,30,{0x00,0x06,0x00,0x40,0x1A,0x08,0x00,0x32,0x10,0x07,0x00,0x07,0x54,0x15,0x0F,0x05,0x04,0x02,0x12,0x10,0x05,0x07,0x33,0x33,0x0B,0x0B,0x37,0x10,0x07,0x07}},



    {0xD5,44,{0x19,0x19,0x18,0x18,0x1A,0x1A,0x1B,0x1B,0x04,0x05,0x06,0x07,0x00,0x01,0x02,0x03,0x20,0x21,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x22,0x23,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}},

    {0xD6,44,{0x18,0x18,0x19,0x19,0x1A,0x1A,0x1B,0x1B,0x03,0x02,0x01,0x00,0x07,0x06,0x05,0x04,0x23,0x22,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x21,0x20,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}},

    {0xE0,42,{0x00,0x06,0x0C,0x31,0x34,0x3F,0x1D,0x41,0x06,0x0A,0x0C,0x17,0x0F,0x12,0x15,0x13,0x14,0x07,0x12,0x15,0x16,0x00,0x06,0x0B,0x30,0x34,0x3F,0x1D,0x40,0x07,0x0A,0x0D,0x18,0x0E,0x12,0x14,0x12,0x14,0x08,0x13,0x14,0x19}},
   
    {0xB6,2,{0x60,0x60}},
    {0xCC,1,{0x09}},
    {0xD2,1,{0x55}},

    {0xC0,2,{0x30,0x14}},
    {0xBF,3,{0x41,0x0E,0x01}},
    {0xC7,4,{0x00,0xC0,0x40,0xC0}},
    {0xDF,1,{0x8E}},  

   


    {0x11,0,{0x00}},
    {REGFLAG_DELAY,150,{}},   
    {0x29,0,{0x00}},
    {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;
    for(i = 0; i < count; i++) {
        unsigned cmd;
        cmd = table[i].cmd;
        switch (cmd) {

		/*	  case 0xb6:
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
//	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
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

	//printk("suspend:%s:%d GPIO_LCD_PWR_EN3.3V:%d GPIO_LCM_BIAS_EN:%d GPIO_LCD_RST_EN:%d\n",__func__,__LINE__,gpio_get_value(GPIO_LCD_PWR_EN),gpio_get_value(GPIO_LCM_BIAS_EN),gpio_get_value(GPIO_LCD_RST_EN));
	printk("[Kernel/LCM]: gs716 enter lcm_suspend   !\n");
}
  
static void lcm_resume(void)
{
	lcm_init_lcm();
	//printk("resume:%s:%d GPIO_LCD_PWR_EN3.3V:%d GPIO_LCM_BIAS_EN:%d GPIO_LCD_RST_EN:%d\n",__func__,__LINE__,gpio_get_value(GPIO_LCD_PWR_EN),gpio_get_value(GPIO_LCM_BIAS_EN),gpio_get_value(GPIO_LCD_RST_EN));
	printk("[Kernel/LCM]: gs716 enter lcm_resume   !\n");
}

struct LCM_DRIVER gs716_wh_hx8394a_wxga_ips_8_lcm_drv = {
	.name = "gs716_wh_hx8394a_wxga_ips_8",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
