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
extern unsigned int GPIO_CTP_TP_RST_EN;

#define GPIO_LCD_RST_EN        GPIO_LCM_RST
#define GPIO_LCD_PWR_EN        GPIO_LCM_PWR
#define GPIO_LCM_VDD_EN        GPIO_LCM_VDD

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	gpio_direction_output(GPIO, output);
	gpio_set_value(GPIO, output);
}

/* ------------------------------------------------------------------- */
/* Local Constants */
/* ------------------------------------------------------------------- */

#define FRAME_WIDTH  (1200)
#define FRAME_HEIGHT (2000)

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
#define REGFLAG_DELAY       0XFFE
#define REGFLAG_END_OF_TABLE    0xFFF   // END OF REGISTERS MARKER

struct LCM_setting_table
{
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
#if 1	
{0x00, 1, {0x00}}, 
{0xFA, 1, {0x5A}}, 
{0x00, 1, {0x00}},
{0xFF, 3, {0x82,0x05,0x01}}, 
{0x00, 1, {0x80}},
{0xFF, 2, {0x82,0x05}},
{0x00, 1, {0x93}}, 
{0xC5, 1, {0x57}}, 
{0x00, 1, {0x97}}, 
{0xC5, 1, {0x57}},
{0x00, 1, {0x9E}}, 
{0xC5, 1, {0x05}},  
{0x00, 1, {0x9A}},  
{0xC5, 1, {0xE1}},  
{0x00, 1, {0x9C}},  
{0xC5, 1, {0xE1}}, 
{0x00, 1, {0xB6}}, 
{0xC5, 2, {0x43,0x43}},   
{0x00, 1, {0xB8}}, 
{0xC5, 2, {0x57,0x57}},  
{0x00, 1, {0xA0}},       
{0xA5, 1, {0x04}}, 
{0x00, 1, {0x00}}, 
{0xD8, 2, {0xC8,0xC8}},  
{0x00, 1, {0x00}}, 
{0xD9, 3, {0x00,0x8A,0x8A}},  
{0x00, 1, {0x82}}, 
{0xC5, 1, {0x95}},  
{0x00, 1, {0x83}}, 
{0xC5, 1, {0x07}},  
{0x00, 1, {0xD9}},
{0xCB, 1, {0x40}},
{0x00, 1, {0x00}},
{0xE1, 16, {0x00,0x07,0x18,0x2A,0x34,0x3F,0x51,0x60,0x63,0x71,0x75,0x8C,0x75,0x60,0x5F,0x54}},
{0x00, 1, {0x10}},
{0xE1, 8, {0x4C,0x41,0x33,0x2D,0x28,0x1F,0x19,0x17}},
{0x00, 1, {0x00}},
{0xE2, 16, {0x00,0x07,0x18,0x2A,0x34,0x3F,0x51,0x60,0x63,0x71,0x75,0x8C,0x75,0x60,0x5F,0x54}},
{0x00, 1, {0x10}},
{0xE2, 8, {0x4C,0x41,0x32,0x28,0x1C,0x0B,0x08,0x08}},
{0x00, 1, {0x80}},
{0xA4, 1, { 0x2C}},
{0x00, 1, { 0xA1}},
{0xB3, 2, {0x04,0xB0}},	
{0x00, 1, { 0xA3}},
{0xB3, 2, {0x07,0xD0}},	
{0x00, 1, { 0xA5}},
{0xB3, 2, {0x80,0x13}},	
{0x00, 1, { 0x80}},
{0xCB, 7, {0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C}},
{0x00, 1, { 0x87}},
{0xCB, 1, { 0x3C}},
{0x00, 1, { 0x88}},
{0xCB, 8, {0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C}},
{0x00, 1, { 0x90}},
{0xCB, 6, {0x3C,0x3C,0x3C,0x3C,0x3C,0x3C}},
{0x00, 1, { 0x97}},
{0xCB, 1, { 0x33}},
{0x00, 1, { 0x98}},
{0xCB, 8, {0xD4,0xD4,0xD4,0xD4,0xD4,0xD4,0xD4,0xD4}},
{0x00, 1, { 0xA0}},
{0xCB, 8, {0xD4,0xD4,0xD4,0xD7,0xD4,0xD4,0xD4,0xD4}},
{0x00, 1, { 0xA8}},
{0xCB, 6, {0xD4,0xD4,0xD4,0xD4,0xD4,0xD4}},
{0x00, 1, { 0xB0}},
{0xCB, 7, {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
{0x00, 1, { 0xB7}},
{0xCB, 1, { 0x00}},
{0x00, 1, { 0xB8}},
{0xCB, 8, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
{0x00, 1, { 0xC0}},
{0xCB, 7, {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
{0x00, 1, { 0xC7}},
{0xCB, 1, { 0x00}},
{0x00, 1, { 0x80}},
{0xCC, 8, {0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F}},
{0x00, 1, { 0x88}},
{0xCC, 8, {0x31,0x02,0x1C,0x2C,0x2D,0x16,0x14,0x12}},
{0x00, 1, { 0x90}},
{0xCC, 6, {0x10,0x0E,0x0C,0x04,0x2F,0x2F}},
{0x00, 1, { 0x80}},
{0xCD, 8, {0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F}},
{0x00, 1, { 0x88}},
{0xCD, 8, {0x31,0x01,0x1B,0x2C,0x2D,0x15,0x13,0x11}},
{0x00, 1, { 0x90}},
{0xCD, 6, {0x0F,0x0D,0x0B,0x03,0x2F,0x2F}},
{0x00, 1, { 0xA0}},
{0xCC, 8, {0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F}},
{0x00, 1, { 0xA8}},
{0xCC, 8, {0x31,0x01,0x03,0x2C,0x2D,0x13,0x15,0x0B}},
{0x00, 1, { 0xB0}},
{0xCC, 6, {0x0D,0x0F,0x11,0x1B,0x2F,0x2F}},
{0x00, 1, { 0xA0}},
{0xCD, 8, {0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F}},
{0x00, 1, { 0xA8}},
{0xCD, 8, {0x31,0x02,0x04,0x2C,0x2D,0x14,0x16,0x0C}},
{0x00, 1, { 0xB0}},
{0xCD, 6, {0x0E,0x10,0x12,0x1C,0x2F,0x2F}},
{0x00, 1, {0x80}},
{0xC2, 4, {0x85,0x03,0x00,0x96}},	
{0x00, 1, {0x84}},
{0xC2, 4, {0x84,0x03,0x00,0x96}},	
{0x00, 1, {0xC0}}, 
{0xC2, 7, {0x83,0x07,0x00,0x03,0x00,0x96,0x0B}},
{0x00, 1, {0xC7}}, 
{0xC2, 7, {0x82,0x08,0x00,0x03,0x00,0x96,0x0B}},
{0x00, 1, {0xD0}}, 
{0xC2, 7, {0x81,0x09,0x00,0x03,0x00,0x96,0x0B}},
{0x00, 1, {0xD7}}, 
{0xC2, 7, {0x80,0x0A,0x00,0x03,0x00,0x96,0x0B}},
{0x00, 1, {0xE0}}, 
{0xC2, 7, {0x01,0x0B,0x00,0x03,0x00,0x96,0x0B}},
{0x00, 1, {0xE7}}, 
{0xC2, 7, {0x02,0x0C,0x00,0x03,0x00,0x96,0x0B}},
{0x00, 1, {0xF0}}, 
{0xC2, 7, {0x03,0x0D,0x00,0x03,0x00,0x96,0x0B}},
{0x00, 1, {0xF7}}, 
{0xC2, 7, {0x04,0x0E,0x00,0x03,0x00,0x96,0x0B}},
{0x00, 1, {0x80}}, 
{0xC3, 7, {0x05,0x03,0x00,0x03,0x00,0x96,0x0B}},
{0x00, 1, {0x87}}, 
{0xC3, 7, {0x06,0x04,0x00,0x03,0x00,0x96,0x0B}},
{0x00, 1, {0x90}}, 
{0xC3, 7, {0x07,0x05,0x00,0x03,0x00,0x96,0x0B}},
{0x00, 1, {0x97}}, 
{0xC3, 7, {0x08,0x06,0x00,0x03,0x00,0x96,0x0B}},
{0x00, 1, {0xA0}},
{0xC2, 4, {0x03,0x43,0x00,0x96}},	
{0x00, 1, {0xA4}},
{0xC2, 4, {0x02,0x43,0x00,0x96}},	
{0x00, 1, {0xA0}},
{0xC3, 8, {0x00,0x70,0xD1,0x05,0x00,0x00,0x00,0x00}},	
{0x00, 1, {0xA8}},
{0xC3, 8, {0x00,0x70,0xD2,0x06,0x00,0x00,0x00,0x00}},	
{0x00, 1, {0xF0}},
{0xCC, 6, {0x3D,0x88,0x88,0x18,0x18,0x80}},
{0x00, 1, {0xA2}},
{0xF3, 1, {0x55}},
{0x00, 1, {0xE0}},
{0xC3, 4, {0x27,0x00,0x00,0x00}},	
{0x00, 1, {0xE4}},
{0xC3, 4, {0x27,0x00,0x00,0x00}},	
{0x00, 1, {0xEC}},
{0xC3, 2, {0xC2,0xC2}},

//GOFF1_VB
{0x00, 1, {0xF0}},
{0xC3, 4, {0x36,0x00,0x00,0x00}},
//GOFF2_VB
{0x00, 1, {0xF4}},
{0xC3, 4, {0x36,0x00,0x00,0x00}},


{0x00, 1, { 0x80}},
{0xC0, 6, { 0x00 ,0x91 ,0x00 ,0xE8 ,0x00 ,0x10}},
{0x00, 1, { 0x90}},
{0xC0, 6, { 0x00 ,0x91 ,0x00 ,0xE8 ,0x00 ,0x10}},
{0x00, 1, { 0xA0}},
{0xC0, 6, { 0x00 ,0xFA ,0x00 ,0xE8 ,0x00 ,0x10}},
{0x00, 1, { 0xB0}},
{0xC0, 5, { 0x00 ,0xA7 ,0x00 ,0xE8 ,0x10}},
{0x00, 1, { 0xA3}},
{0xC1, 6, { 0x00, 0x0F, 0x00 ,0x0F ,0x00 ,0x04}},
{0x00, 1, { 0x80}},
{0xCE, 12, { 0x01, 0x81 ,0x09 ,0x21 ,0x00 ,0x15 ,0x00 ,0xD6 ,0x00 ,0x2A ,0x00 ,0x3A }},
{0x00, 1, { 0x90}},
{0xCE, 14, { 0x00 ,0x88 ,0x0C ,0x93 ,0x00 ,0x44 ,0x80 ,0x09 ,0x21 ,0x00 ,0x03 ,0x00 ,0x22 ,0x0C }},
{0x00, 1, { 0xA0}},
{0xCE, 3, { 0x10 ,0x00 ,0x46}},
{0x00, 1, { 0xB0}},
{0xCE, 3, { 0x62 ,0x00 ,0x00}},
{0x00, 1, { 0xCC}},
{0xCE, 2, { 0x07 ,0xFD}},
{0x00, 1, { 0xD0}},
{0xCE, 8, { 0x01 ,0x00 ,0x0B ,0x01 ,0x01 ,0x00 ,0xE2 ,0x01}},
{0x00, 1, { 0xE1}},
{0xCE, 7, { 0x08 ,0x01 ,0x6B ,0x02 ,0x37  ,0x00 ,0x84}},
{0x00, 1, { 0xF0}},
{0xCE, 10, { 0x00, 0x12 ,0x09  ,0x00 ,0xC2 ,0x00 ,0xC2,0x00 ,0xF8 ,0x1D}},
{0x00, 1, { 0xB0}},
{0xCF, 4, { 0x07 ,0x07 ,0xD6 ,0xDA}},
{0x00, 1, { 0xB5}},
{0xCF, 4, { 0x03 ,0x03 ,0xA6 ,0xAA}},
{0x00, 1, { 0xC0}},
{0xCF, 4, { 0x07 ,0x07 ,0xEE ,0xF2}},
{0x00, 1, { 0xC5}},
{0xCF, 4, { 0x00,0x07,0x1C,0xD0}},
{0x00, 1, { 0x90}},
{0xC1, 1, { 0x22}},
{0x00, 1, { 0x9C}},
{0xC1, 1, { 0x08}},
{0x00, 1, {0x00}},	
{0x35, 1, {0x01}},	
{0x00, 1, {0x9F}},
{0xC5, 1, {0x00}},
{0x00, 1, {0x98}},
{0xC5, 1, {0x54}},
{0x00, 1, {0x91}},
{0xC5, 1, {0x4C}},
{0x00, 1, {0x8C}},
{0xCF, 2, {0x40,0x40}},
{0x00, 1, {0x93}},	
{0xC4, 1, {0x90}},
{0x00, 1, {0xD7}},	
{0xC0, 1, {0xF0}},	
{0x00, 1, {0xA2}},	
{0xF5, 1, {0x1F}},	
{0x00, 1, {0xB1}},
{0xF5, 1, {0x02}},
{0x00, 1, {0x88}},	
{0xB0, 1, {0x07}},	
{0x00, 1, {0x00}},	
{0xFA, 1, {0x01}},	
{0x00, 1, {0xA8}},	
{0xC5, 1, {0x99}},	
{0x00, 1, {0xB6}}, 
{0xC5, 2, {0x41,0x41}},   
{0x00, 1, {0xB8}}, 
{0xC5, 2, {0x55,0x55}},  
{0x00, 1, {0x9A}},	     
{0xF5, 1, { 0x00}},	
{0x00, 1, { 0xB2}},
{0xCE, 1, { 0x95}},
{0x00, 1, { 0x00}},	
{0xFA, 1, { 0x5A}},	
{0x00, 1, { 0x88}},	
{0xB0, 1, { 0x01}},	
{0x00, 1, { 0x00}},
{0xFF, 3, {0x00,0x00,0x00}}, 
{0x00, 1, {0x80}},
{0xFF, 2, {0x00,0x00}},
#else
{0x00, 1, {0x00}},  
{0xFA, 1, {0x5A}},  
{0x00, 1, {0x00}}, 
{0xFF, 3, {0x82, 0x05, 0x01}},  
{0x00, 1, {0x80}}, 
{0xFF, 2, {0x82,0x05}}, 
{0x00, 1, {0x93}},  //VGH_N 15V
{0xC5, 1, {0x57}},  
{0x00, 1, {0x97}},  //VGH_I 15V
{0xC5, 1, {0x57}}, 
{0x00, 1, {0x9E}},  
{0xC5, 1, {0x05}},   //2AVDD-AVEE(N&I)
{0x00, 1, {0x9A}},   //VGL_N -15V 
{0xC5, 1, {0xE1}},   //2AVEE-AVDD(N)
{0x00, 1, {0x9C}},   //VGL_I -14V
{0xC5, 1, {0xE1}},  //2AVEE-AVDD(I)
{0x00, 1, {0xB6}},  
{0xC5, 2, {0x43,0x43}},    //VGHO1_N_I 14V
{0x00, 1, {0xB8}},  
{0xC5, 2, {0x57,0x57}},   //VGLO1_N_I -14V
{0x00, 1, {0xA0}},        //GVDD&NGVDD En
{0xA5, 1, {0x04}},  
{0x00, 1, {0x00}},  
{0xD8, 2, {0xC8,0xC8}},   //GVDDP&NGVDD 5.5V/-5.5V
{0x00, 1, {0x82}},  
{0xC5, 1, {0x95}},   //LVD
{0x00, 1, {0x83}},  
{0xC5, 1, {0x07}},   //LVD Enable
{0x00, 1, {0xD9}}, 
{0xCB, 1, {0x40}}, //fill Power on blank OPT
{0x00, 1, {0x00}}, 
{0xE1, 16, {0x00,0x07,0x18,0x2A,0x34,0x3F,0x51,0x60,0x63,0x71,0x75,0x8C,0x75,0x60,0x5F,0x54}},
{0x00, 1, {0x10}}, 
{0xE1, 8, {0x4C,0x41,0x33,0x2D,0x28,0x1F,0x19,0x17}}, 
{0x00, 1, {0x00}}, 
{0xE2, 16, {0x00,0x07,0x18,0x2A,0x34,0x3F,0x51,0x60,0x63,0x71,0x75,0x8C,0x75,0x60,0x5F,0x54}}, 
{0x00, 1, {0x10}}, 
{0xE2, 8, {0x4C,0x41,0x32,0x28,0x1C,0x0B,0x08,0x08}}, 
{0x00, 1, {0x00}}, 
{0xEC, 15, {0x00,0x04,0x08,0x0C,0x29,0x10,0x15,0x18,0x1D,0x23,0x21,0x29,0x32,0x3A,0x4A}}, 
{0x00, 1, {0x10}}, 
{0xEC, 15, {0x41,0x4A,0x52,0x5B,0x37,0x63,0x6A,0x72,0x7A,0xFD,0x82,0x8B,0x93,0x9A,0x43}}, 
{0x00, 1, {0x20}}, 
{0xEC, 15, {0xA2,0xAB,0xB3,0xBC,0x52,0xC3,0xCA,0xD2,0xDA,0x20,0xE2,0xE9,0xF1,0xF9,0x18}}, 
{0x00, 1, {0x30}}, 
{0xEC, 4, {0xFC,0xFE,0xFF,0x05}}, 
{0x00, 1, {0x40}}, 
{0xEC, 15, {0x00,0x04,0x08,0x0B,0x45,0x0F,0x13,0x17,0x1A,0xC6,0x1F,0x26,0x2E,0x36,0xBC}}, 
{0x00, 1, {0x50}}, 
{0xEC, 15, {0x3D,0x44,0x4D,0x54,0xCA,0x5C,0x64,0x6B,0x72,0x92,0x7A,0x81,0x89,0x91,0x7D}}, 
{0x00, 1, {0x60}}, 
{0xEC, 15, {0x98,0xA0,0xA8,0xB0,0x95,0xB9,0xC0,0xC8,0xCF,0x8D,0xD7,0xDF,0xE7,0xF0,0x3D}}, 
{0x00, 1, {0x70}}, 
{0xEC, 4, {0xF3,0xF6,0xF6,0x33}}, 
{0x00, 1, {0x80}}, 
{0xEC, 15, {0x00,0x04,0x07,0x0B,0x31,0x0F,0x13,0x16,0x1A,0xF1,0x1F,0x27,0x2F,0x37,0xF4}}, 
{0x00, 1, {0x90}}, 
{0xEC, 15, {0x3E,0x47,0x4F,0x57,0x13,0x5F,0x66,0x6E,0x76,0x1D,0x7D,0x85,0x8D,0x95,0x2B}}, 
{0x00, 1, {0xA0}}, 
{0xEC, 15, {0x9C,0xA4,0xAC,0xB5,0x7D,0xBD,0xC5,0xCC,0xD4,0x53,0xDC,0xE4,0xEB,0xF4,0xFE}}, 
{0x00, 1, {0xB0}}, 
{0xEC, 4, {0xF9,0xFB,0xFC,0x00}}, 
{0x00, 1, {0x80}}, 
{0xA4, 1, {0x2C}}, //Default=0x8C
{0x00, 1, {0xA1}}, 
{0xB3, 2, {0x04,0xB0}}, 	//X=1200
{0x00, 1, {0xA3}}, 
{0xB3, 2, {0x07,0xD0}}, 	//Y=2000
{0x00, 1, {0xA5}}, 
{0xB3, 2, {0x80,0x13}}, 	
{0x00, 1, {0x80}}, 
{0xCB, 7, {0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C}}, 
{0x00, 1, {0x87}}, 
{0xCB, 1, {0x3C}}, 
{0x00, 1, {0x88}}, 
{0xCB, 8, {0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C}}, 
{0x00, 1, {0x90}}, 
{0xCB, 6, {0x3C,0x3C,0x3C,0x3C,0x3C,0x3C}}, 
{0x00, 1, {0x97}}, 
{0xCB, 1, {0x33}}, 
{0x00, 1, {0x98}}, 
{0xCB, 8, {0xD4,0xD4,0xD4,0xD4,0xD4,0xD4,0xD4,0xD4}}, 
{0x00, 1, {0xA0}}, 
{0xCB, 8, {0xD4,0xD4,0xD4,0xD7,0xD4,0xD4,0xD4,0xD4}}, 
{0x00, 1, {0xA8}}, 
{0xCB, 6, {0xD4,0xD4,0xD4,0xD4,0xD4,0xD4}}, 
{0x00, 1, {0xB0}}, 
{0xCB, 7, {0x00,0x00,0x00,0x00,0x00,0x00,0x00}}, 
{0x00, 1, {0xB7}}, 
{0xCB, 1, {0x00}}, 
{0x00, 1, {0xB8}}, 
{0xCB, 8, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}}, 
{0x00, 1, {0xC0}}, 
{0xCB, 7, {0x00,0x00,0x00,0x00,0x00,0x00,0x00}}, 
{0x00, 1, {0xC7}}, 
{0xCB, 1, {0x00}}, 
{0x00, 1, {0x80}}, //Left CGOUT
{0xCC, 8, {0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F}}, 
{0x00, 1, {0x88}}, 
{0xCC, 8, {0x31,0x02,0x1C,0x2C,0x2D,0x16,0x14,0x12}}, 
{0x00, 1, {0x90}}, 
{0xCC, 6, {0x10,0x0E,0x0C,0x04,0x2F,0x2F}}, 
{0x00, 1, {0x80}}, //Right CGOUT
{0xCD, 8, {0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F}}, 
{0x00, 1, {0x88}}, 
{0xCD, 8, {0x31,0x01,0x1B,0x2C,0x2D,0x15,0x13,0x11}}, 
{0x00, 1, {0x90}}, 
{0xCD, 6, {0x0F,0x0D,0x0B,0x03,0x2F,0x2F}}, 
{0x00, 1, {0xA0}}, //Left CGOUT
{0xCC, 8, {0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F}}, 
{0x00, 1, {0xA8}}, 
{0xCC, 8, {0x31,0x01,0x03,0x2C,0x2D,0x13,0x15,0x0B}}, 
{0x00, 1, {0xB0}}, 
{0xCC, 6, {0x0D,0x0F,0x11,0x1B,0x2F,0x2F}}, 
{0x00, 1, {0xA0}}, //Right CGOUT
{0xCD, 8, {0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F}}, 
{0x00, 1, {0xA8}}, 
{0xCD, 8, {0x31,0x02,0x04,0x2C,0x2D,0x14,0x16,0x0C}}, 
{0x00, 1, {0xB0}}, 
{0xCD, 6, {0x0E,0x10,0x12,0x1C,0x2F,0x2F}}, 
{0x00, 1, {0x80}}, //STV1_ODD
{0xC2, 4, {0x85,0x03,0x00,0x96}}, 	
{0x00, 1, {0x84}}, //STV1_EVEN
{0xC2, 4, {0x84,0x03,0x00,0x96}}, 						
{0x00, 1, {0xC0}},  //CK1
{0xC2, 7, {0x83,0x07,0x00,0x03,0x00,0x96,0x0B}}, 
{0x00, 1, {0xC7}},  //CK2
{0xC2, 7, {0x82,0x08,0x00,0x03,0x00,0x96,0x0B}}, 
{0x00, 1, {0xD0}},  //CK3
{0xC2, 7, {0x81,0x09,0x00,0x03,0x00,0x96,0x0B}}, 
{0x00, 1, {0xD7}},  //CK4
{0xC2, 7, {0x80,0x0A,0x00,0x03,0x00,0x96,0x0B}}, 				
{0x00, 1, {0xE0}},  //CK5
{0xC2, 7, {0x01,0x0B,0x00,0x03,0x00,0x96,0x0B}}, 
{0x00, 1, {0xE7}},  //CK6
{0xC2, 7, {0x02,0x0C,0x00,0x03,0x00,0x96,0x0B}}, 
{0x00, 1, {0xF0}},  //CK7
{0xC2, 7, {0x03,0x0D,0x00,0x03,0x00,0x96,0x0B}}, 
{0x00, 1, {0xF7}},  //CK8
{0xC2, 7, {0x04,0x0E,0x00,0x03,0x00,0x96,0x0B}}, 
{0x00, 1, {0x80}},  //CK9
{0xC3, 7, {0x05,0x03,0x00,0x03,0x00,0x96,0x0B}}, 
{0x00, 1, {0x87}},  //CK10
{0xC3, 7, {0x06,0x04,0x00,0x03,0x00,0x96,0x0B}}, 
{0x00, 1, {0x90}},  //CK11
{0xC3, 7, {0x07,0x05,0x00,0x03,0x00,0x96,0x0B}}, 
{0x00, 1, {0x97}},  //CK12
{0xC3, 7, {0x08,0x06,0x00,0x03,0x00,0x96,0x0B}}, 
{0x00, 1, {0xA0}}, //STV2_ODD
{0xC2, 4, {0x03,0x43,0x00,0x96}}, 	
{0x00, 1, {0xA4}}, //STV2_EVEN
{0xC2, 4, {0x02,0x43,0x00,0x96}}, 	
{0x00, 1, {0xA0}}, //RESET_ODD
{0xC3, 8, {0x00,0x70,0xD1,0x05,0x00,0x00,0x00,0x00}}, 	
{0x00, 1, {0xA8}}, //RESET_EVEN
{0xC3, 8, {0x00,0x70,0xD2,0x06,0x00,0x00,0x00,0x00}}, 	
{0x00, 1, {0xF0}}, //VDD1&VDD2=1s Switch
{0xCC, 6, {0x3D,0x88,0x88,0x18,0x18,0x80}}, 
{0x00, 1, {0xA2}}, 
{0xF3, 1, {0x55}}, //ECLK12&(~GOFF12)
{0x00, 1, {0xE0}}, //GOFF1_LH
{0xC3, 4, {0x27,0x00,0x00,0x90}}, 	
{0x00, 1, {0xE4}}, //GOFF2_LH
{0xC3, 4, {0x27,0x00,0x00,0x90}}, 	
{0x00, 1, {0xEC}}, 
{0xC3, 2, {0xC2,0xC2}}, 
{0x00, 1, {0x80}}, 
{0xC0, 6, {0x00 ,0x8F ,0x01 ,0x18 ,0x00 ,0x10}}, 
{0x00, 1, {0x90}}, 
{0xC0, 6, {0x00 ,0x8F ,0x01 ,0x18 ,0x00 ,0x10}}, 
{0x00, 1, {0xA0}}, 
{0xC0, 6, {0x00 ,0xD6 ,0x01 ,0x0C ,0x00 ,0x1C}}, 
{0x00, 1, {0xB0}}, 
{0xC0, 5, {0x00 ,0xA7 ,0x01 ,0x18 ,0x10}}, 
{0x00, 1, {0xA3}}, 
{0xC1, 6, {0x00, 0x32, 0x00 ,0x32 ,0x00 ,0x04}}, 
{0x00, 1, {0x80}}, 
{0xCE, 12, {0x01, 0x81 ,0x08 ,0x21 ,0x00 ,0x36 ,0x00 ,0xE8 ,0x00 ,0x2A ,0x00 ,0x3A }}, 
{0x00, 1, {0x90}}, 
{0xCE, 14, {0x00 ,0x88 ,0x0C ,0x7C ,0x00 ,0x44 ,0x80 ,0x08 ,0x21 ,0x00 ,0x03 ,0x00 ,0x1F ,0x14 }}, 
{0x00, 1, {0xA0}}, 
{0xCE, 3, {0x10 ,0x00 ,0x00}}, 
{0x00, 1, {0xB0}}, 
{0xCE, 3, {0x62 ,0x00 ,0x00}}, 
{0x00, 1, {0xCC}}, 
{0xCE, 2, {0x08 ,0x12}}, 
{0x00, 1, {0xD0}}, 
{0xCE, 8, {0x01 ,0x00 ,0x2B ,0x01 ,0x01 ,0x00 ,0xE1 ,0x01}}, 
{0x00, 1, {0xE1}}, 
{0xCE, 7, {0x07 ,0x01 ,0x82 ,0x02 ,0x37  ,0x00 ,0x97}}, 
{0x00, 1, {0xF0}}, 
{0xCE, 10, {0x00, 0x12 ,0x09  ,0x00 ,0xEB ,0x00 ,0xEB,0x00 ,0x28 ,0x52}}, 
{0x00, 1, {0xB0}}, 
{0xCF, 4, {0x07 ,0x07 ,0xF6 ,0xFA}}, 
{0x00, 1, {0xB5}}, 
{0xCF, 4, {0x03 ,0x03 ,0xA6 ,0xAA}}, 
{0x00, 1, {0xC0}}, 
{0xCF, 4, {0x07 ,0x07 ,0xDA ,0xDE}}, 
{0x00, 1, {0xC5}}, 
{0xCF, 4, {0x00 ,0x07 ,0x1C ,0xD0}}, 
{0x00, 1, {0x90}}, 
{0xC1, 1, {0x22}}, 
{0x00, 1, {0x9C}}, 
{0xC1, 1, {0x08}}, 
{0x00, 1, {0x00}}, 	
{0x35, 1, {0x01}}, 	
{0x00, 1, {0x9F}}, 
{0xC5, 1, {0x00}}, 
{0x00, 1, {0x98}}, 
{0xC5, 1, {0x54}}, 
{0x00, 1, {0x91}}, 
{0xC5, 1, {0x4C}}, 
{0x00, 1, {0x8C}}, 
{0xCF, 2, {0x40,0x40}}, 
{0x00, 1, {0x93}}, 	
{0xC4, 1, {0x90}}, 
{0x00, 1, {0xD7}}, 	
{0xC0, 1, {0xF0}}, 	
{0x00, 1, {0xA2}}, 	
{0xF5, 1, {0x1F}}, 	//Source PL
{0x00, 1, {0x90}}, 	
{0xE9, 1, {0x10}}, 	
{0x00, 1, {0xB1}}, 
{0xF5, 1, {0x02}}, 
{0x00, 1, {0x88}}, 	
{0xB0, 1, {0x07}}, 	
{0x00, 1, {0x00}}, 	
{0xFA, 1, {0x01}}, 	
{0x00, 1, {0xA8}}, 	
{0xC5, 1, {0x99}}, 	
{0x00, 1, {0xB6}},  
{0xC5, 2, {0x41,0x41}},    //VGHO1_N_I 13.8V
{0x00, 1, {0xB8}},  
{0xC5, 2, {0x55,0x55}},   //VGLO1_N_I -13.8V
{0x00, 1, {0x9A}}, 	     //vcom_en
{0xF5, 1, {0x00}}, 	
{0x00, 1, {0xB2}},        //slave PosDmy RTN
{0xCE, 1, {0x92}}, 
{0x00, 1, {0x00}}, 	
{0xFA, 1, {0x5A}}, 	
{0x00, 1, {0x88}}, 	//Return to normal
{0xB0, 1, {0x01}}, 	
{0x00, 1, {0x00}}, 
{0xFF, 3, {0x00,0x00,0x00}},  
{0x00, 1, {0x80}}, 
{0xFF, 2, {0x00,0x00}}, 	
#endif

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

    params->dsi.mode    = BURST_VDO_MODE; //SYNC_EVENT_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 

#if 1//old
	params->dsi.vertical_sync_active 			    = 4;
    params->dsi.vertical_backporch				    = 12;
    params->dsi.vertical_frontporch 				= 232;
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active			    = 10;
    params->dsi.horizontal_backporch				= 30;//hbp 不影响实际图像
    params->dsi.horizontal_frontporch				= 43;//28
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable                         = 1;
	//params->dsi.HS_TRAIL                            = 15;
	params->dsi.PLL_CLOCK = 552;//570;//612
#else//new
	params->dsi.vertical_sync_active 			    = 4;
    params->dsi.vertical_backporch				    = 42;
    params->dsi.vertical_frontporch 				= 250;
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active			    = 10;
    params->dsi.horizontal_backporch				= 30;//hbp 不影响实际图像
    params->dsi.horizontal_frontporch				= 110;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable                         = 1;
	//params->dsi.HS_TRAIL                            = 15;
	params->dsi.PLL_CLOCK = 552;//570;//612	
#endif	
}

static void lcd_reset(unsigned char enabled)
{
	lcm_set_gpio_output(GPIO_LCD_RST_EN, enabled);
}
//1.8V
static void lcd_power_en(unsigned char enabled)
{
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, enabled);
}
//3.3V
static void lcd_vdd_en(unsigned char enabled)
{
	lcm_set_gpio_output(GPIO_LCM_VDD_EN, enabled);
}
//TP_LDO

static void tp_reset(unsigned int enabled)
{
	gpio_direction_output(GPIO_CTP_TP_RST_EN, enabled);
}

static void lcm_init_lcm(void)
{
	unsigned int data_array[16];
	lcd_vdd_en(1);
	MDELAY(5);
    lcd_power_en(1);
	MDELAY(10);

	display_bias_enable();

	MDELAY(10);
	tp_reset(1);
	
//	tp_reset(1);
//	printk(" kzhkzh %s:%d GPIO_TP_RST:%d\n",__func__,__LINE__,GPIO_TP_RST);
	MDELAY(5);
	
	lcd_reset(1);
	MDELAY(10);
	lcd_reset(0);
	MDELAY(10);
	lcd_reset(1);
	MDELAY(120);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1); 
	
	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
}

static void lcm_suspend(void)
{
    unsigned int data_array[16];
	lcd_reset(0);
	data_array[0] = 0x00280500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);
	data_array[0] = 0x00100500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);
	//lcd_reset(0);
//	tp_reset(0);
	lcd_power_en(0);
	MDELAY(5);
	lcd_vdd_en(0);
	MDELAY(5);
	display_bias_disable();
	MDELAY(10);
	lcd_reset(0);
	MDELAY(5);
	tp_reset(0);
}
  
static void lcm_resume(void)
{
	lcm_init_lcm();
}

struct LCM_DRIVER pf828_dj_ft8205_wuxga2000_inx_103_lcm_drv = {
	.name = "pf828_dj_ft8205_wuxga2000_inx_103",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
