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

extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_BL_EN;
extern unsigned int GPIO_LCM_RST;
#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0

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

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcd_power_en(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_PWR_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_PWR_EN, GPIO_OUT_ZERO);
    }
}

static void backlight_enable(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ZERO);
    }
}

static void lcd_reset(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_RST, 1);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_RST, 0);
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

    params->dsi.vertical_sync_active                            = 4; //2; //4;
    params->dsi.vertical_backporch                              = 12; //10; //16;
    params->dsi.vertical_frontporch                             = 30;//5; 
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 20; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 20; //60; //60; //80;
    params->dsi.horizontal_frontporch                           = 40; //60; 
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 421;

     params->dsi.ssc_disable = 1;  // disable ssc

     params->dsi.cont_clock = 1;  // clcok always hs mode
}
struct LCM_setting_table {
    unsigned cmd;
    unsigned int count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = 
{
   {0xFF, 3, {0x98, 0x81, 0x03} },
	//GIP_1

	{0x01, 1,{0x00}},
	{0x02, 1,{0x00}},
	{0x03, 1,{0x53}},        //STVA=STV2_4
	{0x04, 1,{0xD3}},        //STVB=STV1_3
	{0x05, 1,{0x00}},       
	{0x06, 1,{0x0D}},        //STVA_Rise
	{0x07, 1,{0x08}},        //STVB_Rise
	{0x08, 1,{0x00}},       
	{0x09, 1,{0x00}},        
	{0x0a, 1,{0x00}},        
	{0x0b, 1,{0x00}},        
	{0x0c, 1,{0x00}},        
	{0x0d, 1,{0x00}},       
	{0x0e, 1,{0x00}},        
	{0x0f, 1,{0x28}},        //CLW1(ALR) Duty=45%
	{0x10, 1,{0x28}},        //CLW2(ARR) Duty=45%
	{0x11, 1,{0x00}},           
	{0x12, 1,{0x00}},        
	{0x13, 1,{0x00}},        //CLWX(ATF)
	{0x14, 1,{0x00}},
	{0x15, 1,{0x00}},      
	{0x16, 1,{0x00}},       
	{0x17, 1,{0x00}},       
	{0x18, 1,{0x00}},        
	{0x19, 1,{0x00}},
	{0x1a, 1,{0x00}},
	{0x1b, 1,{0x00}},   
	{0x1c, 1,{0x00}},
	{0x1d, 1,{0x00}},
	{0x1e, 1,{0x40}},        //CLKA 40自動反 C0手動反(X8參考CLKB)
	{0x1f, 1,{0x80}},        
	{0x20, 1,{0x06}},        //CLKA_Rise
	{0x21, 1,{0x01}},        //CLKA_Fall
	{0x22, 1,{0x00}},        
	{0x23, 1,{0x00}},       
	{0x24, 1,{0x00}},        
	{0x25, 1,{0x00}},       
	{0x26, 1,{0x00}},
	{0x27, 1,{0x00}},
	{0x28, 1,{0x33}},       //CLK Phase
	{0x29, 1,{0x33}},       //CLK overlap
	{0x2a, 1,{0x00}},  
	{0x2b, 1,{0x00}},
	{0x2c, 1,{0x00}},      
	{0x2d, 1,{0x00}},      
	{0x2e, 1,{0x00}},              
	{0x2f, 1,{0x00}},   
	{0x30, 1,{0x00}},
	{0x31, 1,{0x00}},
	{0x32, 1,{0x00}},     
	{0x33, 1,{0x00}},
	{0x34, 1,{0x03}},     //VDD1&2 overlap 0us      
	{0x35, 1,{0x00}},              
	{0x36, 1,{0x00}},
	{0x37, 1,{0x00}},       
	{0x38, 1,{0x96}},     //VDD1&2 toggle 2.5sec	
	{0x39, 1,{0x00}},
	{0x3a, 1,{0x00}}, 
	{0x3b, 1,{0x00}},
	{0x3c, 1,{0x00}},
	{0x3d, 1,{0x00}},
	{0x3e, 1,{0x00}},
	{0x3f, 1,{0x00}},
	{0x40, 1,{0x00}},
	{0x41, 1,{0x00}},
	{0x42, 1,{0x00}},
	{0x43, 1,{0x00}},  
	{0x44, 1,{0x00}},
	
	
	//GIP_2
	{0x50, 1,{0x00}},
	{0x51, 1,{0x23}},
	{0x52, 1,{0x45}},
	{0x53, 1,{0x67}},
	{0x54, 1,{0x89}},
	{0x55, 1,{0xAB}},
	{0x56, 1,{0x01}},
	{0x57, 1,{0x23}},
	{0x58, 1,{0x45}},
	{0x59, 1,{0x67}},
	{0x5a, 1,{0x89}},
	{0x5b, 1,{0xAB}},
	{0x5c, 1,{0xCD}},
	{0x5d, 1,{0xEF}},
	
	//GIP_3
	{0x5e, 1,{0x00}},
	{0x5f, 1,{0x08}},     //FW_CGOUT_L[1]    STV3
	{0x60, 1,{0x08}},     //FW_CGOUT_L[2]    STV3
	{0x61, 1,{0x06}},     //FW_CGOUT_L[3]    STV4
	{0x62, 1,{0x06}},     //FW_CGOUT_L[4]    STV4
	{0x63, 1,{0x01}},     //FW_CGOUT_L[5]    VDS
	{0x64, 1,{0x01}},     //FW_CGOUT_L[6]    VDS
	{0x65, 1,{0x00}},     //FW_CGOUT_L[7]    VSD
	{0x66, 1,{0x00}},     //FW_CGOUT_L[8]    VSD
	{0x67, 1,{0x02}},     //FW_CGOUT_L[9]    
	{0x68, 1,{0x15}},     //FW_CGOUT_L[10]   VDD2
	{0x69, 1,{0x15}},     //FW_CGOUT_L[11]   VDD2 
	{0x6a, 1,{0x14}},     //FW_CGOUT_L[12]   VDD1
	{0x6b, 1,{0x14}},     //FW_CGOUT_L[13]   VDD1
	{0x6c, 1,{0x0D}},     //FW_CGOUT_L[14]   CLK8   
	{0x6d, 1,{0x0D}},     //FW_CGOUT_L[15]   CLK8
	{0x6e, 1,{0x0C}},     //FW_CGOUT_L[16]   CLK6    
	{0x6f, 1,{0x0C}},     //FW_CGOUT_L[17]   CLK6
	{0x70, 1,{0x0F}},     //FW_CGOUT_L[18]   CLK4
	{0x71, 1,{0x0F}},     //FW_CGOUT_L[19]   CLK4
	{0x72, 1,{0x0E}},     //FW_CGOUT_L[20]   CLK2
	{0x73, 1,{0x0E}},     //FW_CGOUT_L[21]   CLK2
	{0x74, 1,{0x02}},     //FW_CGOUT_L[22]   VGL
	{0x75, 1,{0x08}},     //BW_CGOUT_L[1]   
	{0x76, 1,{0x08}},     //BW_CGOUT_L[2]    
	{0x77, 1,{0x06}},     //BW_CGOUT_L[3]    
	{0x78, 1,{0x06}},     //BW_CGOUT_L[4]    
	{0x79, 1,{0x01}},     //BW_CGOUT_L[5]     
	{0x7a, 1,{0x01}},     //BW_CGOUT_L[6]     
	{0x7b, 1,{0x00}},     //BW_CGOUT_L[7]   
	{0x7c, 1,{0x00}},     //BW_CGOUT_L[8]    
	{0x7d, 1,{0x02}},     //BW_CGOUT_L[9]      
	{0x7e, 1,{0x15}},     //BW_CGOUT_L[10]
	{0x7f, 1,{0x15}},     //BW_CGOUT_L[11]    
	{0x80, 1,{0x14}},     //BW_CGOUT_L[12]   
	{0x81, 1,{0x14}},     //BW_CGOUT_L[13] 
	{0x82, 1,{0x0D}},     //BW_CGOUT_L[14]      
	{0x83, 1,{0x0D}},     //BW_CGOUT_L[15]   
	{0x84, 1,{0x0C}},     //BW_CGOUT_L[16]      
	{0x85, 1,{0x0C}},     //BW_CGOUT_L[17]
	{0x86, 1,{0x0F}},     //BW_CGOUT_L[18]
	{0x87, 1,{0x0F}},     //BW_CGOUT_L[19]
	{0x88, 1,{0x0E}},     //BW_CGOUT_L[20]   
	{0x89, 1,{0x0E}},     //BW_CGOUT_L[21]   
	{0x8A, 1,{0x02}},     //BW_CGOUT_L[22]   
	
	
	
	//CMD_Page 4
	{0xFF, 3, {0x98, 0x81, 0x04} },
	{0x6E, 1,{0x2B}},           //VGH 15V
	{0x6F, 1,{0x37}},           //reg vcl + pumping ratio VGH=3x VGL=-3x
	{0x3A, 1,{0xA4}},           //POWER SAVING
	{0x8D, 1,{0x1A}},           //VGL -11V
	{0x87, 1,{0xBA}},           //ESD
	{0xB2, 1,{0xD1}},
	{0x88, 1,{0x0B}},
	{0x38, 1,{0x01}},      
	{0x39, 1,{0x00}},
	{0xB5, 1,{0x07}},           
	{0x31, 1,{0x75}},           
	{0x3B, 1,{0x98}},  			
				
	//CMD_Page 1
	{0xFF, 3, {0x98, 0x81, 0x01} },
	{0x22, 1,{0x0A}},          //BGR, SS
	{0x31, 1,{0x00}},          //Column inversion
	{0x53, 1,{0x48}},          //VCOM1
	{0x55, 1,{0x48}},          //VCOM2 
	{0x50, 1,{0x99}},          //VREG1OUT 4.5V
	{0x51, 1,{0x94}},          //VREG2OUT -4.5V
	{0x60, 1,{0x10}},         //SDT 3.5us
	{0x62, 1,{0x20}},
	
	
	//============Gamma START=============
	
	//Pos Register
	{0xA0, 1,{0x00}},
	{0xA1, 1,{0x00}},
	{0xA2, 1,{0x15}},
	{0xA3, 1,{0x14}},
	{0xA4, 1,{0x1B}},
	{0xA5, 1,{0x2F}},
	{0xA6, 1,{0x25}},
	{0xA7, 1,{0x24}},
	{0xA8, 1,{0x80}},
	{0xA9, 1,{0x1F}},
	{0xAA, 1,{0x2C}},
	{0xAB, 1,{0x6C}},
	{0xAC, 1,{0x16}},
	{0xAD, 1,{0x14}},
	{0xAE, 1,{0x4D}},
	{0xAF, 1,{0x20}},
	{0xB0, 1,{0x29}},
	{0xB1, 1,{0x4F}},
	{0xB2, 1,{0x5F}},
	{0xB3, 1,{0x23}},
	
	
	
	//Neg Register
	{0xC0, 1,{0x00}},
	{0xC1, 1,{0x2E}},
	{0xC2, 1,{0x3B}},
	{0xC3, 1,{0x15}},
	{0xC4, 1,{0x16}},
	{0xC5, 1,{0x28}},
	{0xC6, 1,{0x1A}},
	{0xC7, 1,{0x1C}},
	{0xC8, 1,{0xA7}},
	{0xC9, 1,{0x1B}},
	{0xCA, 1,{0x28}},
	{0xCB, 1,{0x92}},
	{0xCC, 1,{0x1F}},
	{0xCD, 1,{0x1C}},
	{0xCE, 1,{0x4B}},
	{0xCF, 1,{0x1F}},
	{0xD0, 1,{0x28}},
	{0xD1, 1,{0x4E}},
	{0xD2, 1,{0x5C}},
	{0xD3, 1,{0x23}},
	
	{0xFF, 3, {0x98, 0x81, 0x00} },
	//============ Gamma END===========			
	//{0x35,1,{0x00}},
	{0x11,1,{0x00}},  
	{REGFLAG_DELAY,120,{}},
	{0x29,1,{0x00}},
	{REGFLAG_DELAY,20,{}},
	{0x35,1,{0x00}},


    //{REGFLAG_END_OF_TABLE,0x00,{}}
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

static void lcm_init(void)
{
	lcd_power_en(1);
	MDELAY(10);
	backlight_enable(1);

	lcd_reset(1);//LCD rst
	MDELAY(30);
	lcd_reset(0);	
	MDELAY(50);
	lcd_reset(1);
	MDELAY(20);//Must > 5ms

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
	
    backlight_enable(0);
    MDELAY(50);
    lcd_reset(0);
    MDELAY(10);
	lcd_power_en(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
	lcm_init();

}

struct LCM_DRIVER us868_bns_ili9881c_boe_wxga_ips_101_lcm_drv = {
	.name = "us868_bns_ili9881c_boe_wxga_ips_101",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
