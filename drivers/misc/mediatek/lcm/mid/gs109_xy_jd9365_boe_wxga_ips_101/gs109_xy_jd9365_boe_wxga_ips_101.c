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

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(800)
#define FRAME_HEIGHT 										(1280)

#ifdef BUILD_LK
#define GPIO_LCM_PWR_EN                                     (GPIO122 |0x80000000)
#define GPIO_LCM_BL_EN                                      (GPIO26 | 0x80000000)
#define GPIO_LCM_RST                                        (GPIO83 | 0x80000000)
#else
extern unsigned int GPIO_LCM_PWR;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_VDD;
#define GPIO_LCM_PWR_EN  GPIO_LCM_VDD
#define GPIO_LCM_BL_EN   GPIO_LCM_PWR
#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0
#endif

#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util = {0};

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#ifdef BUILD_LK
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
    mt_set_gpio_mode(GPIO, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO, (output>0)? GPIO_OUT_ONE: GPIO_OUT_ZERO);
}
#else
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	if (GPIO == 0xFFFFFFFF) {
		return;
	}
	gpio_set_value(GPIO, (output > 0) ? GPIO_OUT_ONE : GPIO_OUT_ZERO);
}
#endif

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

static void avdd_enable(unsigned char enabled)
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
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	params->physical_height = 218;
	params->physical_width 	= 135;
	
	// enable tearing-free ****
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED; //LCM_DBI_TE_MODE_VSYNC_OR_HSYNC;//LCM_DBI_TE_MODE_DISABLED;//LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_FALLING;

// #if (LCM_DSI_CMD_MODE)
// 	params->dsi.mode   = CMD_MODE;
// #else
	params->dsi.mode   = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
// #endif
	
	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size=256;

	// Video mode setting		
	params->dsi.intermediat_buffer_num = 2; //because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	
	params->dsi.word_count=FRAME_WIDTH*3; 

	params->dsi.vertical_sync_active = 4; 
	params->dsi.vertical_backporch =  8; 
	params->dsi.vertical_frontporch = 24; 
	params->dsi.vertical_active_line = FRAME_HEIGHT;//1304

	params->dsi.horizontal_sync_active = 18; 
	params->dsi.horizontal_backporch =	18; 
	params->dsi.horizontal_frontporch = 18; 
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;//854

	params->dsi.PLL_CLOCK=220; //
}

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static void __attribute__((unused)) push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
#define REGFLAG_MDELAY		    0xFFFC
#define REGFLAG_UDELAY	        0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW	    0xFFFE
#define REGFLAG_RESET_HIGH	    0xFFFF
	unsigned int i;

	for (i = 0; i < count; i++) {
		unsigned int cmd;
		cmd = table[i].cmd;
		switch (cmd) {
            case REGFLAG_MDELAY:
                MDELAY(table[i].count);
                break;
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
    }
}

static __attribute__((unused)) struct LCM_setting_table init_setting[] = {
  	//Page0
	{0xE0, 1, {0x00}},
	//--- PASSWORD  ----//
	{0xE1, 1, {0x93}},
	{0xE2, 1, {0x65}},
	{0xE3, 1, {0xF8}},
	{0x80, 1, {0x03}},//0X03ï¼?-LANE;0X02ï¼?-LANE;0X01:2-LANE


	//--- Page1  ----//
	{0xE0, 1, {0x01}},
	//Set VCOM
	{0x00, 1, {0x00}},
	{0x01, 1, {0x6A}},
	//Set VCOM_Reverse
	//{0x03, 1, {0x00}},
	//{0x04, 1, {0xA0}},
	//Set Gamma Power, 1, { VGMP, 1, {VGMN, 1, {VGSP, 1, {VGSN
	{0x17, 1, {0x00}},
	{0x18, 1, {0xAF}},//4.3V
	{0x19, 1, {0x01}},//0.3V
	{0x1A, 1, {0x00}},//
	{0x1B, 1, {0xAF}},//4.3V
	{0x1C, 1, {0x01}},//0.3V
				   
	//Set Gate Power
	{0x1F, 1, {0x3E}},     //VGH_R  = 15V                       
	{0x20, 1, {0x28}},     //VGL_R  = -12V                      
	{0x21, 1, {0x28}},     //VGL_R2 = -12V                      
	{0x22, 1, {0x7E}},     //PA[6]=1, 1, { PA[5]=1, 1, { PA[4]=1, 1, { PA[0]=0 
	//SETPANEL
	{0x35, 1, {0x26}},	//ASP=0110

	//SETPANEL
	{0x37, 1, {0x09}},	//SS=1, 1, {BGR=1
	//SET RGBCYC
	{0x38, 1, {0x04}},	//JDT=100 column inversion
	{0x39, 1, {0x00}},	//RGB_N_EQ1, 1, { 0x12
	{0x3A, 1, {0x01}},	//RGB_N_EQ2, 1, { 0x18
	{0x3C, 1, {0x7C}},	//SET EQ3 for TE_H
	{0x3D, 1, {0xFF}},	//SET CHGEN_ON, 1, { 
	{0x3E, 1, {0xFF}},	//SET CHGEN_OFF, 1, { 
	{0x3F, 1, {0x7F}},	//SET CHGEN_OFF2, 1, {

	//Set TCON
	{0x40, 1, {0x06}},	//RSO=800 RGB
	{0x41, 1, {0xA0}},	//LN=640->1280 line
	{0x42, 1, {0x81}},	//SLT
	{0x43, 1, {0x08}},	//VFP=8
	{0x44, 1, {0x0B}},	//VBP=12
	{0x45, 1, {0x28}},  //HBP=40
	//--- power voltage  ----//
	{0x55, 1, {0x0F}},	//DCDCM=0001, 1, { JD PWR_IC
	{0x57, 1, {0x69}},
	{0x59, 1, {0x0A}},	//VCL = -2.9V
	{0x5A, 1, {0x28}},	//VGH = 15V
	{0x5B, 1, {0x14}},	//VGL = -11V

	//--- Gamma  ----//
	{0x5D, 1, {0x7C}},              
	{0x5E, 1, {0x65}},      
	{0x5F, 1, {0x55}},    
	{0x60, 1, {0x47}},    
	{0x61, 1, {0x43}},    
	{0x62, 1, {0x32}},    
	{0x63, 1, {0x34}},    
	{0x64, 1, {0x1C}},    
	{0x65, 1, {0x33}},    
	{0x66, 1, {0x31}},    
	{0x67, 1, {0x30}},    
	{0x68, 1, {0x4E}},    
	{0x69, 1, {0x3C}},    
	{0x6A, 1, {0x44}},    
	{0x6B, 1, {0x35}},    
	{0x6C, 1, {0x31}},    
	{0x6D, 1, {0x23}},    
	{0x6E, 1, {0x11}},    
	{0x6F, 1, {0x00}},    
	{0x70, 1, {0x7C}},    
	{0x71, 1, {0x65}},    
	{0x72, 1, {0x55}},    
	{0x73, 1, {0x47}},    
	{0x74, 1, {0x43}},    
	{0x75, 1, {0x32}},    
	{0x76, 1, {0x34}},    
	{0x77, 1, {0x1C}},    
	{0x78, 1, {0x33}},    
	{0x79, 1, {0x31}},    
	{0x7A, 1, {0x30}},    
	{0x7B, 1, {0x4E}},    
	{0x7C, 1, {0x3C}},    
	{0x7D, 1, {0x44}},    
	{0x7E, 1, {0x35}},    
	{0x7F, 1, {0x31}},    
	{0x80, 1, {0x23}},    
	{0x81, 1, {0x11}},    
	{0x82, 1, {0x00}},    

									 
	//Page2, 1, { for GIP                                      
	{0xE0, 1, {0x02}},                                
	//GIP_L Pin mapping                                   
	{0x00, 1, {0x1E}},//1  VDS                        
	{0x01, 1, {0x1E}},//2  VDS                        
	{0x02, 1, {0x41}},//3  STV2                       
	{0x03, 1, {0x41}},//4  STV2                       
	{0x04, 1, {0x43}},//5  STV4                       
	{0x05, 1, {0x43}},//6  STV4                       
	{0x06, 1, {0x1F}},//7  VSD                        
	{0x07, 1, {0x1F}},//8  VSD                        
	{0x08, 1, {0x35}},//9  GCL                        
	{0x09, 1, {0x1F}},//10                            
	{0x0A, 1, {0x15}},//11 GCH                        
	{0x0B, 1, {0x15}},//12 GCH                        
	{0x0C, 1, {0x1F}},//13                            
	{0x0D, 1, {0x47}},//14 CLK8                       
	{0x0E, 1, {0x47}},//15 CLK8                       
	{0x0F, 1, {0x45}},//16 CLK6                       
	{0x10, 1, {0x45}},//17 CLK6                       
	{0x11, 1, {0x4B}},//18 CLK4                       
	{0x12, 1, {0x4B}},//19 CLK4                       
	{0x13, 1, {0x49}},//20 CLK2                       
	{0x14, 1, {0x49}},//21 CLK2                       
	{0x15, 1, {0x1F}},//22 VGL                        
														  
														  
	//GIP_R Pin mapping                                   
	{0x16, 1, {0x1E}},//1  VDS                 
	{0x17, 1, {0x1E}},//2  VDS                
	{0x18, 1, {0x40}},//3  STV1               
	{0x19, 1, {0x40}},//4  STV1               
	{0x1A, 1, {0x42}},//5  STV3               
	{0x1B, 1, {0x42}},//6  STV3               
	{0x1C, 1, {0x1F}},//7  VSD                
	{0x1D, 1, {0x1F}},//8  VSD                
	{0x1E, 1, {0x35}},//9  GCL                
	{0x1F, 1, {0x1F}},//10                    
	{0x20, 1, {0x15}},//11 GCH                
	{0x21, 1, {0x15}},//12 GCH                
	{0x22, 1, {0x1f}},//13                    
	{0x23, 1, {0x46}},//14 CLK7               
	{0x24, 1, {0x46}},//15 CLK7               
	{0x25, 1, {0x44}},//16 CLK5               
	{0x26, 1, {0x44}},//17 CLK5               
	{0x27, 1, {0x4A}},//18 CLK3               
	{0x28, 1, {0x4A}},//19 CLK3               
	{0x29, 1, {0x48}},//20 CLK1               
	{0x2A, 1, {0x48}},//21 CLK1               
	{0x2B, 1, {0x1F}},//22 VGL                                 



	//GIP Timing  
	{0x58, 1, {0x40}}, 
	{0x5B, 1, {0x30}}, //STV_NUM, 1, {STV_S0
	{0x5C, 1, {0x03}}, //STV_S0
	{0x5D, 1, {0x30}}, //STV_W / S1
	{0x5E, 1, {0x01}}, //STV_S2
	{0x5F, 1, {0x02}}, //STV_S3
	{0x63, 1, {0x14}}, //SETV_ON  
	{0x64, 1, {0x6A}}, //SETV_OFF 
	{0x67, 1, {0x73}}, 
	{0x68, 1, {0x05}}, 
	{0x69, 1, {0x14}}, 
	{0x6A, 1, {0x6A}}, 
	{0x6B, 1, {0x08}}, //Dummy clk

	{0x6C, 1, {0x00}}, 
	{0x6D, 1, {0x00}}, 
	{0x6E, 1, {0x00}}, 
	{0x6F, 1, {0x88}}, 

	{0x77, 1, {0xDD}}, 
	{0x79, 1, {0x0E}},//0x0C 
	{0x7A, 1, {0x03}},//0x04
	{0x7D, 1, {0x14}}, 
	{0x7E, 1, {0x6A}}, 


	//Page4
	{0xE0, 1, {0x04}},
	{0x09, 1, {0x11}},
	{0x0E, 1, {0x48}},
	{0x2B, 1, {0x2B}},
	{0x2D, 1, {0x03}},//defult 0x01
	{0x2E, 1, {0x44}},

	//Page0
	{0xE0, 1, {0x00}},

	{0xE6, 1, {0x02}},
	{0xE7, 1, {0x0C}},

};

static void init_lcm_registers(void)
{
    unsigned int data_array[16];

	push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);

    data_array[0] = 0x00110500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);

    data_array[0] = 0x00290500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(5);
}

static void lcm_init(void)
{
	avdd_enable(1);//3.3
	MDELAY(100);
	lcd_power_en(1);//bias en
	MDELAY(180);
	lcd_reset(1);
	MDELAY(20);
	lcd_reset(0);
	MDELAY(20);
	lcd_reset(1);
	MDELAY(150);//Must > 5ms

	init_lcm_registers();
    MDELAY(20);

}

static void lcm_suspend(void)
{
	lcd_power_en(0);
    MDELAY(150);
    lcd_reset(0);
    MDELAY(100);
    avdd_enable(0);
   // DSI_clk_HS_mode(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
  lcm_init();
}

struct LCM_DRIVER gs109_xy_jd9365_boe_wxga_ips_101_lcm_drv =
{
    .name			= "gs109_xy_jd9365_boe_wxga_ips_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
