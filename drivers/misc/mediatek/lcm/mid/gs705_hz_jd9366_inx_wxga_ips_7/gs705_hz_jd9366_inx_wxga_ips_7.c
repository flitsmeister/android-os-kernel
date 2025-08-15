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

 #define REGFLAG_DELAY             							0XFE
 #define REGFLAG_END_OF_TABLE      							0xFF    // END OF REGISTERS MARKER

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

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
}; 

 static struct LCM_setting_table lcm_initialization_setting[] = {	

	{0xE0, 1, {0x00} },
    {0xE1, 1, {0x93} },
    {0xE2, 1, {0x65} },
    {0xE3, 1, {0xF8} },
    {0x80, 1, {0x03} },
    {0xE0, 1, {0x01} },
    {0x00, 1, {0x00} },
    {0x01, 1, {0xA9} },	//VCOM A8 AE AD
    {0x03, 1, {0x00} },
    {0x04, 1, {0xA8} },	
    {0x17, 1, {0x00} },
    {0x18, 1, {0xB1} },
    {0x19, 1, {0x01} },
    {0x1A, 1, {0x00} },
    {0x1B, 1, {0xB1} },  //VGMN=0
    {0x1C, 1, {0x01} },
    {0x1F, 1, {0x3E} },     //VGH_R  = 15V                       
    {0x20, 1, {0x2D} },     //VGL_R  = -12V                      
    {0x21, 1, {0x2D} },     //VGL_R2 = -12V                      
    {0x22, 1, {0x7E} },     //PA[6]=0, PA[5]=0, PA[4]=0, PA[0]=0 
    {0x37, 1, {0x19} },	//SS=1,BGR=1
    {0x38, 1, {0x05} },	//JDT=101 zigzag inversion
    {0x39, 1, {0x00} },	//RGB_N_EQ1, modify 20140806
    {0x3A, 1, {0x01} },	//RGB_N_EQ2, modify 20140806
    {0x3C, 1, {0x78} },	//SET EQ3 for TE_H
    {0x3D, 1, {0xFF} },	//SET CHGEN_OFF, modify 20140827 
    {0x3E, 1, {0xFF} },	//SET CHGEN_OFF, modify 20140827 
    {0x3F, 1, {0xFF} },	//SET CHGEN_OFF2, modify 20140827
    {0x40, 1, {0x06} },	//RSO=800 RGB
    {0x41, 1, {0xA0} },	//LN=640->1280 line
    {0x43, 1, {0x08} },
    {0x44, 1, {0x09} },
    {0x45, 1, {0x28} },
    {0x55, 1, {0x0F} },	//DCDCM=1111
    {0x57, 1, {0x89} },	//VGH_RT=100, Charge pump Ratio:3*AVDD-VCL
    {0x58, 1, {0x0A} },
    {0x59, 1, {0x0A} },	//VCL = -2.5V
    {0x5A, 1, {0x28} },	//VGH = 15V
    {0x5B, 1, {0x19} },	//VGL = -12V
    {0x5D, 1, {0x7C} }, // 
    {0x5E, 1, {0x66} }, //
    {0x5F, 1, {0x56} }, //
    {0x60, 1, {0x49} }, //
    {0x61, 1, {0x44} }, //
    {0x62, 1, {0x34} }, //
    {0x63, 1, {0x37} }, //
    {0x64, 1, {0x1F} }, //
    {0x65, 1, {0x36} }, //
    {0x66, 1, {0x33} }, //
    {0x67, 1, {0x32} }, //
    {0x68, 1, {0x4F} }, //
    {0x69, 1, {0x3D} }, //
    {0x6A, 1, {0x43} }, //
    {0x6B, 1, {0x35} }, //
    {0x6C, 1, {0x30} }, //
    {0x6D, 1, {0x24} }, //
    {0x6E, 1, {0x13} }, //
    {0x6F, 1, {0x00} }, //
    {0x70, 1, {0x7C} }, //
    {0x71, 1, {0x66} }, //
    {0x72, 1, {0x56} }, //
    {0x73, 1, {0x49} }, //
    {0x74, 1, {0x44} }, //
    {0x75, 1, {0x34} }, //
    {0x76, 1, {0x37} }, //
    {0x77, 1, {0x1F} }, //
    {0x78, 1, {0x36} }, //
    {0x79, 1, {0x33} }, //
    {0x7A, 1, {0x32} }, //
    {0x7B, 1, {0x4F} }, //
    {0x7C, 1, {0x3D} }, //
    {0x7D, 1, {0x43} }, //
    {0x7E, 1, {0x35} }, //
    {0x7F, 1, {0x30} }, //
    {0x80, 1, {0x24} }, //
    {0x81, 1, {0x13} }, //
    {0x82, 1, {0x00} }, //
    {0xE0, 1, {0x02} },
    {0x00, 1, {0x45} },//0x05
    {0x01, 1, {0x45} },//0x05  
    {0x02, 1, {0x44} },//0x04
    {0x03, 1, {0x44} },//0x04
    {0x04, 1, {0x47} },//0x07
    {0x05, 1, {0x47} },//0x07
    {0x06, 1, {0x46} },//0x06
    {0x07, 1, {0x46} },//0x06
    {0x08, 1, {0x40} },//0x00
    {0x09, 1, {0x1F} },//0x1F
    {0x0A, 1, {0x1F} },//0x1F
    {0x0B, 1, {0x1F} },//0x1F
    {0x0C, 1, {0x1F} },//0x1F
    {0x0D, 1, {0x1F} },//0x1F
    {0x0E, 1, {0x1F} },//0x1F
    {0x0F, 1, {0x41} },//0x01
    {0x10, 1, {0x1F} },//0x1F
    {0x11, 1, {0x1F} },//0x1F
    {0x12, 1, {0x1F} },//0x1F
    {0x13, 1, {0x1F} },//0x1F
    {0x14, 1, {0x1F} },//0x1F
    {0x15, 1, {0x1F} },//0x1F
    {0x16, 1, {0x45} },
    {0x17, 1, {0x45} },
    {0x18, 1, {0x44} },
    {0x19, 1, {0x44} },
    {0x1A, 1, {0x47} },
    {0x1B, 1, {0x47} },
    {0x1C, 1, {0x46} },
    {0x1D, 1, {0x46} },
    {0x1E, 1, {0x40} },
    {0x1F, 1, {0x1F} },
    {0x20, 1, {0x1F} },
    {0x21, 1, {0x1F} },
    {0x22, 1, {0x1F} },
    {0x23, 1, {0x1F} },
    {0x24, 1, {0x1F} },
    {0x25, 1, {0x41} },
    {0x26, 1, {0x1F} },
    {0x27, 1, {0x1F} },
    {0x28, 1, {0x1F} },
    {0x29, 1, {0x1F} },
    {0x2A, 1, {0x1F} },
    {0x2B, 1, {0x1F} },
    {0x58, 1, {0x40} },        // TGEQL, Initial porch
    {0x5B, 1, {0x10} },       //STV number =4, STV_S0[10:8]
    {0x5C, 1, {0x06} },       //STV_S0[7:0] =
    {0x5D, 1, {0x20} },//60   //STV_W=4 , STV_S1=0,1h timing
    {0x5E, 1, {0x01} },       //STV_S2 =1,1h timing
    {0x5F, 1, {0x02} },       //STV_S3 =2,1h timing
    {0x63, 1, {0x62} },       //SETV_ON
    {0x64, 1, {0x62} },       //SETV_OFF
    {0x67, 1, {0x32} },          //CKV0_NUM[3:0]=8, CKV0_W[3:0]=4
    {0x68, 1, {0x08} },   //0x06       
    {0x69, 1, {0x62} }, 
    {0x6A, 1, {0x66} }, //66 CLK Falling to Source Falling time 3us
    {0x6B, 1, {0x08} },  
    {0x6C, 1, {0x00} },
    {0x6D, 1, {0x04} }, //GEQ_GGND1
    {0x6E, 1, {0x04} }, //GEQ_GGND2
    {0x6F, 1, {0x88} },
    {0xE0, 1, {0x03} },
    {0x98, 1, {0x3E} },	//From 2E to 3E, LED_ON
    {0xE0, 1, {0x04} },  //ESD protect
    {0x09, 1, {0x10} },
    {0x2B, 1, {0x2B} },	
    {0x2D, 1, {0x03} },	
    {0x2E, 1, {0x44} },	
    {0xE0, 1, {0x00} }, //Watch dog
    {0xE6, 1, {0x02} },
    {0xE7, 1, {0x06} },			
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
	params->type                   = LCM_TYPE_DSI;
	params->width                  = FRAME_WIDTH;
	params->height                 = FRAME_HEIGHT;
	params->dsi.mode               = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM           = LCM_FOUR_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

    params->dsi.vertical_sync_active                            = 4; //2; //4;
    params->dsi.vertical_backporch                              = 6; //10; //16;
    params->dsi.vertical_frontporch                             = 8;//5;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 20; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 20; //60; //60; //80;
    params->dsi.horizontal_frontporch                           = 32; //60;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 203;
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
	unsigned int data_array[16];
    lcd_power_en(1);//LCD 3.3V
	MDELAY(10);
	
	lcd_bias_en(1);//bias en 偏压使能
	MDELAY(10);
	
	lcd_reset(1);
	MDELAY(50);
	lcd_reset(0);//LCD rst
	MDELAY(50);
	lcd_reset(1);
	MDELAY(120);//Must > 5ms

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
   // init_lcm_registers();
   
    data_array[0] = 0x00110500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);

    data_array[0] = 0x00290500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(5);
   
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

struct LCM_DRIVER gs705_hz_jd9366_inx_wxga_ips_7_lcm_drv = {
	.name = "gs705_hz_jd9366_inx_wxga_ips_7",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
