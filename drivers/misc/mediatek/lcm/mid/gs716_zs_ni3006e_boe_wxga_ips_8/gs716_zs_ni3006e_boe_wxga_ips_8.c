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
struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

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
#define REGFLAG_DELAY 0xFFFF
#define REGFLAG_END_OF_TABLE 0xFFFE
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

		{0xFF,3,{0x98,0x81,0x03}},
		{0x01,1,{0x00}},
		{0x02,1,{0x00}},
		{0x03,1,{0x53}},
		{0x04,1,{0x53}},
		{0x05,1,{0x13}},
		{0x06,1,{0x04}},
		{0x07,1,{0x02}},
		{0x08,1,{0x02}},
		{0x09,1,{0x00}},
		{0x0a,1,{0x00}},
		{0x0b,1,{0x00}},
		{0x0c,1,{0x00}},
		{0x0d,1,{0x00}},
		{0x0e,1,{0x00}},
		{0x0f,1,{0x00}},
		{0x10,1,{0x00}},
		{0x11,1,{0x00}},
		{0x12,1,{0x00}},
		{0x13,1,{0x00}},
		{0x14,1,{0x00}},
		{0x15,1,{0x00}},
		{0x16,1,{0x00}},
		{0x17,1,{0x00}},
		{0x18,1,{0x00}},
		{0x19,1,{0x00}},
		{0x1a,1,{0x00}},
		{0x1b,1,{0x00}},
		{0x1c,1,{0x00}},
		{0x1d,1,{0x00}},
		{0x1e,1,{0xc0}},
		{0x1f,1,{0x80}},
		{0x20,1,{0x02}},
		{0x21,1,{0x09}},
		{0x22,1,{0x00}},
		{0x23,1,{0x00}},
		{0x24,1,{0x00}},
		{0x25,1,{0x00}},
		{0x26,1,{0x00}},
		{0x27,1,{0x00}},
		{0x28,1,{0x55}},
		{0x29,1,{0x03}},
		{0x2a,1,{0x00}},
		{0x2b,1,{0x00}},
		{0x2c,1,{0x00}},
		{0x2d,1,{0x00}},
		{0x2e,1,{0x00}},
		{0x2f,1,{0x00}},
		{0x30,1,{0x00}},
		{0x31,1,{0x00}},
		{0x32,1,{0x00}},
		{0x33,1,{0x00}},
		{0x34,1,{0x00}},
		{0x35,1,{0x00}},
		{0x36,1,{0x00}},
		{0x37,1,{0x00}},
		{0x38,1,{0x3C}},
		{0x39,1,{0x00}},
		{0x3a,1,{0x00}},
		{0x3b,1,{0x00}},
		{0x3c,1,{0x00}},
		{0x3d,1,{0x00}},
		{0x3e,1,{0x00}},
		{0x3f,1,{0x00}},
		{0x40,1,{0x00}},
		{0x41,1,{0x00}},
		{0x42,1,{0x00}},
		{0x43,1,{0x00}},
		{0x44,1,{0x00}},
		{0x50,1,{0x01}},
		{0x51,1,{0x23}},
		{0x52,1,{0x45}},
		{0x53,1,{0x67}},
		{0x54,1,{0x89}},
		{0x55,1,{0xab}},
		{0x56,1,{0x01}},
		{0x57,1,{0x23}},
		{0x58,1,{0x45}},
		{0x59,1,{0x67}},
		{0x5a,1,{0x89}},
		{0x5b,1,{0xab}},
		{0x5c,1,{0xcd}},
		{0x5d,1,{0xef}},
		{0x5e,1,{0x01}},
		{0x5f,1,{0x08}},
		{0x60,1,{0x02}},
		{0x61,1,{0x02}},
		{0x62,1,{0x0A}},
		{0x63,1,{0x15}},
		{0x64,1,{0x14}},
		{0x65,1,{0x02}},
		{0x66,1,{0x11}},
		{0x67,1,{0x10}},
		{0x68,1,{0x02}},
		{0x69,1,{0x0F}},
		{0x6a,1,{0x0E}},
		{0x6b,1,{0x02}},
		{0x6c,1,{0x0D}},
		{0x6d,1,{0x0C}},
		{0x6e,1,{0x06}},
		{0x6f,1,{0x02}},
		{0x70,1,{0x02}},
		{0x71,1,{0x02}},
		{0x72,1,{0x02}},
		{0x73,1,{0x02}},
		{0x74,1,{0x02}},
		{0x75,1,{0x06}},
		{0x76,1,{0x02}},
		{0x77,1,{0x02}},
		{0x78,1,{0x0A}},
		{0x79,1,{0x15}},
		{0x7a,1,{0x14}},
		{0x7b,1,{0x02}},
		{0x7c,1,{0x10}},
		{0x7d,1,{0x11}},
		{0x7e,1,{0x02}},
		{0x7f,1,{0x0C}},
		{0x80,1,{0x0D}},
		{0x81,1,{0x02}},
		{0x82,1,{0x0E}},
		{0x83,1,{0x0F}},
		{0x84,1,{0x08}},
		{0x85,1,{0x02}},
		{0x86,1,{0x02}},
		{0x87,1,{0x02}},
		{0x88,1,{0x02}},
		{0x89,1,{0x02}},
		{0x8A,1,{0x02}},
		{0xFF,3,{0x98,0x81,0x04}},
		{0x6C,1,{0x15}},
		{0x6E,1,{0x30}},
		{0x6F,1,{0x33}},
		{0x8D,1,{0x1F}},
		{0x87,1,{0xBA}},
		{0x26,1,{0x76}},
		{0xB2,1,{0xD1}},
		{0x35,1,{0x1F}},
		{0x33,1,{0x14}},
		{0x3A,1,{0xA9}},
		{0x3B,1,{0x98}},
		{0x38,1,{0x01}},
		{0x39,1,{0x00}},
		{0x3B,1,{0xC0}},
		{0xFF,3,{0x98,0x81,0x01}},
		{0x22,1,{0x0A}},
		{0x31,1,{0x00}},
		{0x50,1,{0xC0}},
		{0x51,1,{0xC0}},
		{0x53,1,{0x47}},
		{0x55,1,{0x46}},
		{0x60,1,{0x28}},
		{0x2E,1,{0xC8}},
		{0x40,1,{0x53}},
		{0xA0,1,{0x01}},
		{0xA1,1,{0x10}},
		{0xA2,1,{0x1B}},
		{0xA3,1,{0x0C}},
		{0xA4,1,{0x14}},
		{0xA5,1,{0x25}},
		{0xA6,1,{0x1A}},
		{0xA7,1,{0x1D}},
		{0xA8,1,{0x68}},
		{0xA9,1,{0x1B}},
		{0xAA,1,{0x26}},
		{0xAB,1,{0x5B}},
		{0xAC,1,{0x1B}},
		{0xAD,1,{0x17}},
		{0xAE,1,{0x4F}},
		{0xAF,1,{0x24}},
		{0xB0,1,{0x2A}},
		{0xB1,1,{0x4E}},
		{0xB2,1,{0x5F}},
		{0xB3,1,{0x39}},
		{0xC0,1,{0x0F}},
		{0xC1,1,{0x1B}},
		{0xC2,1,{0x27}},
		{0xC3,1,{0x16}},
		{0xC4,1,{0x14}},
		{0xC5,1,{0x28}},
		{0xC6,1,{0x1D}},
		{0xC7,1,{0x21}},
		{0xC8,1,{0x6C}},
		{0xC9,1,{0x1B}},
		{0xCA,1,{0x26}},
		{0xCB,1,{0x5B}},
		{0xCC,1,{0x1B}},
		{0xCD,1,{0x1B}},
		{0xCE,1,{0x4F}},
		{0xCF,1,{0x24}},
		{0xD0,1,{0x2A}},
		{0xD1,1,{0x4E}},
		{0xD2,1,{0x5F}},
		{0xD3,1,{0x39}},
		{0xFF,3,{0x98,81,00}},
		{0x35,1,{0x00}},
	
	
		{0x11,1,{0x00}},
		{REGFLAG_DELAY, 120, {}},
		{0x29,1,{0x00}},
		{REGFLAG_DELAY, 20, {}},		
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

/*static void init_lcm_registers(void)
{
    unsigned int data_array[16];  
data_array[0] = 0x00E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
//--- PASSWORD ----//
data_array[0] = 0x93E11500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x65E21500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xF8E31500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//Page0
data_array[0] = 0x00E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
//--- Sequence Ctrl ----//
data_array[0] = 0x02701500; //DC0,DC1;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x23711500; //DC2,DC3;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x06721500; //DC7;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x03801500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x02E61500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x02E71500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//--- Page1 ----//
data_array[0] = 0x01E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//Set VCOM
data_array[0] = 0x00001500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xA0011500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
//Set VCOM_Reverse
data_array[0] = 0x00031500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xA0041500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//Set Gamma Power, VGMP,VGMN,VGSP,VGSN
data_array[0] = 0x00171500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xB1181500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x00191500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x001A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xB11B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x001C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//Set Gate Power
data_array[0] = 0x3E1F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2D201500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2D211500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x0E221500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//SETPANEL
data_array[0] = 0x19371500; //SS=1,BGR=1;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//SET RGBCYC
data_array[0] = 0x05381500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x08391500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x123A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x783C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x803E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x803F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);


//Set TCON
data_array[0] = 0x06401500; //RSO=800 RGB;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xA0411500; //LN=640->1280 line;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//--- power voltage ----//
data_array[0] = 0x0F551500; //DCDCM=0001, JD PWR_IC;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x01561500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x69571500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x0A581500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x0A591500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x285A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x195B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//--- Gamma ----//
data_array[0] = 0x7C5D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x5E5E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4E5F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x41601500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x3F611500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x31621500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x37631500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x23641500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x3E651500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x3F661500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x40671500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x5E681500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4F691500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x576A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4B6B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x496C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2F6D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x036E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x006F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x7C701500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x5E711500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4E721500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x41731500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x3F741500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x31751500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x37761500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x23771500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x3E781500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x3F791500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x407A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x5E7B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4F7C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x577D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4B7E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x497F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2F801500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x03811500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x00821500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//Page2, for GIP
data_array[0] = 0x02E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//GIP_L Pin mapping
data_array[0] = 0x47001500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x47011500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x45021500
;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x45031500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4B041500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4B051500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x49061500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x49071500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x41081500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F091500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F0A1500
;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F0B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F0C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F0D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F0E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x430F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F101500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F111500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F121500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F131500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F141500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F151500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//GIP_R Pin mapping
data_array[0] = 0x46161500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x46171500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x44181500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x44191500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4A1A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4A1B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x481C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x481D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x401E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F1F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F201500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F211500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F221500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F231500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F241500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x42251500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F261500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F271500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F281500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F291500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F2A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1F2B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//GIP Timing 
data_array[0] = 0x10581500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x00591500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x005A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x305B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x035C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x305D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x015E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x025F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x30601500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x01611500 ;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x02621500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x04631500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x7F641500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x05651500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x12661500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x73671500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x05681500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x04691500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x7F6A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x096B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x006C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x046D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x046E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x886F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x00701500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x00711500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x06721500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x7B731500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x00741500 ;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x3C751500 ;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x00761500 ;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x0D771500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2C781500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x00791500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x007A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x007B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x007C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x037D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x7B7E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//Page1
data_array[0] = 0x01E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x010E1500; //LEDON output VCSW2;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//Page3
data_array[0] = 0x03E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2F981500; //From 2E to 2F, LED_VOL;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x04E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2B2B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x442E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x10091500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x032D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

//Page0
data_array[0] = 0x00E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x00110500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(120);

data_array[0] = 0x00290500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(5);

//--- TE----//
data_array[0] = 0x00351500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

}*/

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


	params->dsi.vertical_sync_active 			    = 4;
    params->dsi.vertical_backporch				    = 10;
    params->dsi.vertical_frontporch 				= 10;
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active			    = 16;
    params->dsi.horizontal_backporch				= 60;
    params->dsi.horizontal_frontporch				= 60;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 420;
   // params->dsi.cont_clock=0;
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
	//init_lcm_registers();
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
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
	printk("[Kernel/LCM-kzh]: gs716 enter lcm_suspend   !\n");
}
  
static void lcm_resume(void)
{
	lcm_init_lcm();
	//printk("resume_kzh:%s:%d GPIO_LCD_PWR_EN3.3V:%d GPIO_LCM_BIAS_EN:%d GPIO_LCD_RST_EN:%d\n",__func__,__LINE__,gpio_get_value(GPIO_LCD_PWR_EN),gpio_get_value(GPIO_LCM_BIAS_EN),gpio_get_value(GPIO_LCD_RST_EN));
	printk("[Kernel/LCM-kzh]: gs716 enter lcm_resume   !\n");
}

struct LCM_DRIVER gs716_zs_ni3006e_boe_wxga_ips_8_lcm_drv = {
	.name = "gs716_zs_ni3006e_boe_wxga_ips_8",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
