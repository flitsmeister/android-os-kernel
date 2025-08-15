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

#define REGFLAG_DELAY 0xFFFD
#define REGFLAG_END_OF_TABLE 0xFFFE

#define   LCM_DSI_CMD_MODE	0

struct LCM_setting_table {
    unsigned cmd;
    unsigned int count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = 
{
{0xFF,0x03,{0x98,0x81,0x03}},

//GIP_1
{0x01,0x01,{0x00}},
{0x02,0x01,{0x00}},
{0x03,0x01,{0x53}},        //STVA
{0x04,0x01,{0x00}},        //STVB
{0x05,0x01,{0x00}},        //STVC
{0x06,0x01,{0x08}},        //STVA_Rise
{0x07,0x01,{0x00}},        //STVB_Rise
{0x08,0x01,{0x00}},        //STVC_Rise
{0x09,0x01,{0x00}},        //FTI1R(A)
{0x0a,0x01,{0x00}},        //FTI2R(B)
{0x0b,0x01,{0x00}},        //FTI3R(C)
{0x0c,0x01,{0x00}},        //FTI1F(A)
{0x0d,0x01,{0x00}},        //FTI2F(B)
{0x0e,0x01,{0x00}},        //FTI2F(C)
{0x0f,0x01,{0x26}},    //08        //CLW1(ALR) 45%
{0x10,0x01,{0x26}},    //08        //CLW2(ARR) 45%
{0x11,0x01,{0x00}},           
{0x12,0x01,{0x00}},        
{0x13,0x01,{0x00}},        //CLWX(ATF)
{0x14,0x01,{0x00}},
{0x15,0x01,{0x00}},        //GPMRi(ALR)
{0x16,0x01,{0x00}},        //GPMRii(ARR)
{0x17,0x01,{0x00}},        //GPMFi(ALF)
{0x18,0x01,{0x00}},        //GPMFii(AFF)
{0x19,0x01,{0x00}},
{0x1a,0x01,{0x00}},
{0x1b,0x01,{0x00}},   
{0x1c,0x01,{0x00}},
{0x1d,0x01,{0x00}},
{0x1e,0x01,{0x40}},        //CLKA 40自動反 C0手動反(X8參考CLKB)
{0x1f,0x01,{0xC0}},        //C0
{0x20,0x01,{0x06}},        //CLKA_Rise
{0x21,0x01,{0x01}},        //CLKA_Fall
{0x22,0x01,{0x07}},        //CLKB_Rise(keep toggle需設CLK A後一格)
{0x23,0x01,{0x00}},        //CLKB_Fall
{0x24,0x01,{0x8A}},        //CLK keep toggle(AL) 8X往左看
{0x25,0x01,{0x8A}},        //CLK keep toggle(AR) 8X往左看
{0x26,0x01,{0x00}},
{0x27,0x01,{0x00}},
{0x28,0x01,{0x33}},    //3B       //CLK Phase
{0x29,0x01,{0x33}},       //CLK overlap
{0x2a,0x01,{0x00}},  
{0x2b,0x01,{0x00}},
{0x2c,0x01,{0x08}},       //GCH R
{0x2d,0x01,{0x08}},       //GCL R 
{0x2e,0x01,{0x0B}},       //GCH F        
{0x2f,0x01,{0x0B}},       //GCL F
{0x30,0x01,{0x00}},
{0x31,0x01,{0x00}},
{0x32,0x01,{0x42}},       //GCH/L ext2/1行為  5E 01:31   5E 00:42
{0x33,0x01,{0x00}},
{0x34,0x01,{0x00}},       //VDD1&2 non-overlap 04:2.62us
{0x35,0x01,{0x0A}},       //GCH/L 區間 00:VS前 01:VS後 10:跨VS 11:frame中       
{0x36,0x01,{0x00}},
{0x37,0x01,{0x08}},       //GCH/L
{0x38,0x01,{0x3C}},	//VDD1&2 toggle 1sec
{0x39,0x01,{0x00}},
{0x3a,0x01,{0x00}}, 
{0x3b,0x01,{0x00}},
{0x3c,0x01,{0x00}},
{0x3d,0x01,{0x00}},
{0x3e,0x01,{0x00}},
{0x3f,0x01,{0x00}},
{0x40,0x01,{0x00}},
{0x41,0x01,{0x00}},
{0x42,0x01,{0x00}},
{0x43,0x01,{0x08}},       //GCH/L
{0x44,0x01,{0x00}},

//GIP_2
{0x50,0x01,{0x01}},
{0x51,0x01,{0x23}},
{0x52,0x01,{0x45}},
{0x53,0x01,{0x67}},
{0x54,0x01,{0x89}},
{0x55,0x01,{0xab}},
{0x56,0x01,{0x01}},
{0x57,0x01,{0x23}},
{0x58,0x01,{0x45}},
{0x59,0x01,{0x67}},
{0x5a,0x01,{0x89}},
{0x5b,0x01,{0xab}},
{0x5c,0x01,{0xcd}},
{0x5d,0x01,{0xef}},

//GIP_3
{0x5e,0x01,{0x00}},
{0x5f,0x01,{0x01}},     //FW_CGOUT_L[1]    VDS
{0x60,0x01,{0x01}},     //FW_CGOUT_L[2]    VDS
{0x61,0x01,{0x06}},     //FW_CGOUT_L[3]    STV2
{0x62,0x01,{0x06}},     //FW_CGOUT_L[4]    STV2
{0x63,0x01,{0x06}},     //FW_CGOUT_L[5]    STV4
{0x64,0x01,{0x06}},     //FW_CGOUT_L[6]    STV4
{0x65,0x01,{0x00}},     //FW_CGOUT_L[7]    VSD
{0x66,0x01,{0x00}},     //FW_CGOUT_L[8]    VSD
{0x67,0x01,{0x17}},     //FW_CGOUT_L[9]    GCL
{0x68,0x01,{0x02}},     //FW_CGOUT_L[10]   
{0x69,0x01,{0x16}},     //FW_CGOUT_L[11]   GCH  
{0x6a,0x01,{0x16}},     //FW_CGOUT_L[12]   GCH
{0x6b,0x01,{0x02}},     //FW_CGOUT_L[13]   
{0x6c,0x01,{0x0D}},     //FW_CGOUT_L[14]   CLK8   
{0x6d,0x01,{0x0D}},     //FW_CGOUT_L[15]   CLK8
{0x6e,0x01,{0x0C}},     //FW_CGOUT_L[16]   CLK6    
{0x6f,0x01,{0x0C}},     //FW_CGOUT_L[17]   CLK6
{0x70,0x01,{0x0F}},     //FW_CGOUT_L[18]   CLK4
{0x71,0x01,{0x0F}},     //FW_CGOUT_L[19]   CLK4
{0x72,0x01,{0x0E}},     //FW_CGOUT_L[20]   CLK2
{0x73,0x01,{0x0E}},     //FW_CGOUT_L[21]   CLK2
{0x74,0x01,{0x02}},     //FW_CGOUT_L[22]   VGL
  
{0x75,0x01,{0x01}},     //BW_CGOUT_L[1]   
{0x76,0x01,{0x01}},     //BW_CGOUT_L[2]    
{0x77,0x01,{0x06}},     //BW_CGOUT_L[3]    
{0x78,0x01,{0x06}},     //BW_CGOUT_L[4]    
{0x79,0x01,{0x06}},     //BW_CGOUT_L[5]     
{0x7a,0x01,{0x06}},     //BW_CGOUT_L[6]     
{0x7b,0x01,{0x00}},     //BW_CGOUT_L[7]   
{0x7c,0x01,{0x00}},     //BW_CGOUT_L[8]    
{0x7d,0x01,{0x17}},     //BW_CGOUT_L[9]      
{0x7e,0x01,{0x02}},     //BW_CGOUT_L[10]
{0x7f,0x01,{0x16}},     //BW_CGOUT_L[11]    
{0x80,0x01,{0x16}},     //BW_CGOUT_L[12]   
{0x81,0x01,{0x02}},     //BW_CGOUT_L[13] 
{0x82,0x01,{0x0D}},     //BW_CGOUT_L[14]      
{0x83,0x01,{0x0D}},     //BW_CGOUT_L[15]   
{0x84,0x01,{0x0C}},     //BW_CGOUT_L[16]      
{0x85,0x01,{0x0C}},     //BW_CGOUT_L[17]
{0x86,0x01,{0x0F}},     //BW_CGOUT_L[18]
{0x87,0x01,{0x0F}},     //BW_CGOUT_L[19]
{0x88,0x01,{0x0E}},     //BW_CGOUT_L[20]   
{0x89,0x01,{0x0E}},     //BW_CGOUT_L[21]   
{0x8A,0x01,{0x02}},     //BW_CGOUT_L[22]   



//CMD_Page 4
{0xFF,0x03,{0x98,0x81,0x04}},
//{0x2F,0x01,{0x01}},
{0x6E,0x01,{0x2B}},           //VGH 15V
{0x6F,0x01,{0x35}},    //37           // reg vcl + pumping ratio VGH=3x VGL=-2.5x
{0x3A,0x01,{0xA4}},           //POWER SAVING
{0x8D,0x01,{0x1A}},           //VGL -11V
{0x87,0x01,{0xBA}},           //ESD
{0xB2,0x01,{0xD1}},
{0x88,0x01,{0x0B}},
{0x38,0x01,{0x01}},      
{0x39,0x01,{0x00}},
{0xB5,0x01,{0x07}},           //gamma bias
{0x31,0x01,{0x75}},
{0x3B,0x01,{0x98}},  			
			
//CMD_Page 1
{0xFF,0x03,{0x98,0x81,0x01}},
{0x22,0x01,{0x0A}},          //BGR, SS
{0x31,0x01,{0x00}},          //Column inversion
{0x53,0x01,{0x53}},          //VCOM1
{0x55,0x01,{0x53}},          //VCOM2 
{0x50,0x01,{0x95}},          //VREG1OUT 4.5V
{0x51,0x01,{0x90}},          //VREG2OUT -4.5V
{0x60,0x01,{0x22}},    //06          //SDT
{0x62,0x01,{0x20}},
//============Gamma START=============

//Pos Register
{0xA0,0x01,{0x00}},
{0xA1,0x01,{0x1B}},
{0xA2,0x01,{0x2A}},
{0xA3,0x01,{0x14}},
{0xA4,0x01,{0x17}},
{0xA5,0x01,{0x2B}},
{0xA6,0x01,{0x1F}},
{0xA7,0x01,{0x20}},
{0xA8,0x01,{0x93}},
{0xA9,0x01,{0x1E}},
{0xAA,0x01,{0x2A}},
{0xAB,0x01,{0x7E}},
{0xAC,0x01,{0x1B}},
{0xAD,0x01,{0x19}},
{0xAE,0x01,{0x4C}},
{0xAF,0x01,{0x22}},
{0xB0,0x01,{0x28}},
{0xB1,0x01,{0x4B}},
{0xB2,0x01,{0x59}},
{0xB3,0x01,{0x23}},

//Neg Register
{0xC0,0x01,{0x00}},
{0xC1,0x01,{0x1B}},
{0xC2,0x01,{0x2A}},
{0xC3,0x01,{0x14}},
{0xC4,0x01,{0x17}},
{0xC5,0x01,{0x2B}},
{0xC6,0x01,{0x1F}},
{0xC7,0x01,{0x20}},
{0xC8,0x01,{0x93}},
{0xC9,0x01,{0x1E}},
{0xCA,0x01,{0x2A}},
{0xCB,0x01,{0x7E}},
{0xCC,0x01,{0x1B}},
{0xCD,0x01,{0x19}},
{0xCE,0x01,{0x4C}},
{0xCF,0x01,{0x22}},
{0xD0,0x01,{0x28}},
{0xD1,0x01,{0x4B}},
{0xD2,0x01,{0x59}},
{0xD3,0x01,{0x23}},
	
	//============ Gamma END===========			
	//CMD_Page 0			
	{0xFF,0x03,{0x98,0x81,0x00}},
	{0x11,0x01,{0x00}},  
	{REGFLAG_DELAY,120,{}},
	{0x29,0x01,{0x00}},
	{REGFLAG_DELAY,20,{}},
	{0x35,0x01,{0x00}},

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

    params->dsi.mode    = BURST_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;//LCM_THREE_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 

    params->dsi.vertical_sync_active                            = 6;
    params->dsi.vertical_backporch                              = 15;
    params->dsi.vertical_frontporch                             = 16; 
    params->dsi.vertical_active_line                            = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active                          = 8; 
    params->dsi.horizontal_backporch                            = 48; 
    params->dsi.horizontal_frontporch                           = 52; 
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    //params->dsi.cont_clock    = 1;//1
    //params->dsi.ssc_disable = 0;//0
    params->dsi.PLL_CLOCK = 230;
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
	
	lcd_reset(1);
	MDELAY(20);
	lcd_reset(0);//LCD rst
	MDELAY(20);
	lcd_reset(1);
	MDELAY(120);//Must > 5ms

	lcd_bias_en(1);//bias en 偏压使能
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
	MDELAY(120);
}

static void lcm_resume(void)
{
	lcm_init_lcm();
	//printk("resume_kzh:%s:%d GPIO_LCD_PWR_EN3.3V:%d GPIO_LCM_BIAS_EN:%d GPIO_LCD_RST_EN:%d\n",__func__,__LINE__,gpio_get_value(GPIO_LCD_PWR_EN),gpio_get_value(GPIO_LCM_BIAS_EN),gpio_get_value(GPIO_LCD_RST_EN));
	//printk("[Kernel/LCM-kzh]: gs716 enter lcm_resume   !\n");
}

struct LCM_DRIVER gs716_bns_ili9881c_boe_wxga_ips_101_lcm_drv = {
	.name = "gs716_bns_ili9881c_boe_wxga_ips_101",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
