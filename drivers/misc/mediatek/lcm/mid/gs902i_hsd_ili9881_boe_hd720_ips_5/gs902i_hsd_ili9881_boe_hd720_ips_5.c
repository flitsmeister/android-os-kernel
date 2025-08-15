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

// extern unsigned int GPIO_LCM_PWR;
extern unsigned int GPIO_LCM_RST;
// extern unsigned int GPIO_LCM_VDD;
extern unsigned int GPIO_LCM_DSI_TE_EN;

#define GPIO_LCD_RST_EN        GPIO_LCM_RST
// #define GPIO_LCD_PWR_EN         GPIO_LCM_PWR
// #define GPIO_LCM_BIAS_EN            GPIO_LCM_VDD

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	gpio_direction_output(GPIO, output);
	gpio_set_value(GPIO, output);
}

/* ------------------------------------------------------------------- */
/* Local Constants */
/* ------------------------------------------------------------------- */

#define FRAME_WIDTH  (720)
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


    params->dsi.vertical_sync_active                = 4;
    params->dsi.vertical_backporch                  = 20;
    params->dsi.vertical_frontporch                 = 10;
    params->dsi.vertical_active_line                = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active              = 18;
    params->dsi.horizontal_backporch                = 18;
    params->dsi.horizontal_frontporch               = 18;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 202 ;//198
   // params->dsi.cont_clock=0;
}


#define   LCM_DSI_CMD_MODE	0

#if 1
#define REGFLAG_DELAY             							0XFFFE
#define REGFLAG_END_OF_TABLE      							0xFFFF   // END OF REGISTERS MARKER
struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[128];
};


static struct LCM_setting_table lcm_initialization_setting[] = 
{	
    {0xFF, 3, {0x98, 0x81, 0x01} },
    {0x91, 1, {0x00} },
    {0x92, 1, {0x00} },
    {0x93, 1, {0x53} },
    {0x94, 1, {0x53} },
    {0x95, 1, {0x00} },
    {0x96, 1, {0x04} },
    {0x97, 1, {0x00} },
    {0x98, 1, {0x00} },
    {0x09, 1, {0x00} },
    {0x0a, 1, {0x1D} },
    {0x0b, 1, {0x00} },
    {0x0c, 1, {0x00} },
    {0x0d, 1, {0x00} },
    {0x0e, 1, {0x00} },
    {0x0f, 1, {0x1D} },
    {0x10, 1, {0x1D} },
    {0x11, 1, {0x00} },
    {0x12, 1, {0x00} },
    {0x13, 1, {0x00} },
    {0x14, 1, {0x00} },
    {0x15, 1, {0x08} },
    {0x16, 1, {0x10} },
    {0x17, 1, {0x00} },
    {0x18, 1, {0x08} },
    {0x19, 1, {0x00} },
    {0x1a, 1, {0x00} },
    {0x1b, 1, {0x00} },
    {0x1c, 1, {0x00} },
    {0x1d, 1, {0x00} },
    {0x1e, 1, {0xC4} },
    {0x1f, 1, {0x00} },
    {0x20, 1, {0x02} },
    {0x21, 1, {0x03} },
    {0x22, 1, {0x00} },
    {0x23, 1, {0x00} },
    {0x24, 1, {0x00} },
    {0x25, 1, {0x00} },
    {0x26, 1, {0x00} },
    {0x27, 1, {0x00} },
    {0x28, 1, {0x33} },
    {0x29, 1, {0x33} },
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
    {0x34, 1, {0x04} },
    {0x35, 1, {0x00} },
    {0x36, 1, {0x00} },
    {0x37, 1, {0x00} },
    {0x38, 1, {0x3C} },
    {0x39, 1, {0x07} },
    {0x3a, 1, {0x00} },
    {0x3b, 1, {0x00} },
    {0x3c, 1, {0x00} },
    {0x40, 1, {0x03} },
    {0x41, 1, {0x20} },
    {0x42, 1, {0x00} },
    {0x43, 1, {0x00} },
    {0x44, 1, {0x03} },
    {0x45, 1, {0x00} },
    {0x46, 1, {0x01} },
    {0x47, 1, {0x08} },
    {0x48, 1, {0x00} },
    {0x49, 1, {0x00} },
    {0x4a, 1, {0x00} },
    {0x4b, 1, {0x00} },
    {0x4c, 1, {0x01} },
    {0x4d, 1, {0x54} },
    {0x4e, 1, {0x9B} },
    {0x4f, 1, {0x57} },
    {0x50, 1, {0x2B} },
    {0x51, 1, {0x22} },
    {0x52, 1, {0x22} },
    {0x53, 1, {0x72} },
    {0x54, 1, {0x22} },
    {0x55, 1, {0x22} },
    {0x56, 1, {0x22} },
    {0x57, 1, {0x01} },
    {0x58, 1, {0x54} },
    {0x59, 1, {0x8A} },
    {0x5a, 1, {0x46} },
    {0x5b, 1, {0x2A} },
    {0x5c, 1, {0x22} },
    {0x5d, 1, {0x22} },
    {0x5e, 1, {0x62} },
    {0x5f, 1, {0x22} },
    {0x60, 1, {0x22} },
    {0x61, 1, {0x22} },
    {0x62, 1, {0x06} },
    {0x63, 1, {0x01} },
    {0x64, 1, {0x00} },
    {0x65, 1, {0xA4} },
    {0x66, 1, {0xA5} },
    {0x67, 1, {0x54} },
    {0x68, 1, {0x56} },
    {0x69, 1, {0x58} },
    {0x6a, 1, {0x5A} },
    {0x6b, 1, {0x06} },
    {0x6c, 1, {0x02} },
    {0x6d, 1, {0x02} },
    {0x6e, 1, {0x02} },
    {0x6f, 1, {0x02} },
    {0x70, 1, {0x02} },
    {0x71, 1, {0x02} },
    {0x72, 1, {0x0A} },
    {0x73, 1, {0x02} },
    {0x74, 1, {0x02} },
    {0x75, 1, {0x02} },
    {0x76, 1, {0x02} },
    {0x77, 1, {0x02} },
    {0x78, 1, {0x02} },
    {0x79, 1, {0x01} },
    {0x7a, 1, {0x00} },
    {0x7b, 1, {0xA4} },
    {0x7c, 1, {0xA5} },
    {0x7d, 1, {0x55} },
    {0x7e, 1, {0x57} },
    {0x7f, 1, {0x59} },
    {0x80, 1, {0x5B} },
    {0x81, 1, {0x07} },
    {0x82, 1, {0x02} },
    {0x83, 1, {0x02} },
    {0x84, 1, {0x02} },
    {0x85, 1, {0x02} },
    {0x86, 1, {0x02} },
    {0x87, 1, {0x02} },
    {0x88, 1, {0x0B} },
    {0x89, 1, {0x02} },
    {0x8a, 1, {0x02} },
    {0x8b, 1, {0x02} },
    {0x8c, 1, {0x02} },
    {0x8d, 1, {0x02} },
    {0x8e, 1, {0x02} },
    {0xa0, 1, {0x35} },
    {0xa1, 1, {0x00} },
    {0xa2, 1, {0x00} },
    {0xa3, 1, {0x00} },
    {0xa4, 1, {0x00} },
    {0xa5, 1, {0x00} },
    {0xa6, 1, {0x08} },
    {0xa7, 1, {0x00} },
    {0xa8, 1, {0x00} },
    {0xa9, 1, {0x00} },
    {0xaa, 1, {0x00} },
    {0xab, 1, {0x00} },
    {0xac, 1, {0x00} },
    {0xad, 1, {0x00} },
    {0xae, 1, {0xff} },
    {0xaf, 1, {0x00} },
    {0xb0, 1, {0x00} },
    {0xFF, 3, {0x98, 0x81, 0x02} },

    {0xA0, 20, {0x00, 0x14, 0x21, 0x14, 0x17, 0x29, 0x1E, 0x1F, 0x74, 0x1B, 0x27, 0x66, 0x1A, 0x19, 0x4D, 0x23, 0x28, 0x51, 0x64, 0x3F} },
    {0xC0, 20, {0x00, 0x14, 0x21, 0x14, 0x17, 0x29, 0x1E, 0x1F, 0x74, 0x1B, 0x27, 0x66, 0x1A, 0x19, 0x4D, 0x23, 0x28, 0x51, 0x64, 0x3F} },
    {0x18, 1, {0xF4} },
    {0xFF, 3, {0x98, 0x81, 0x04} },
    {0x5D, 1, {0x6B} },
    {0x5E, 1, {0x6B} },
    {0x60, 1, {0x75} },
    {0x62, 1, {0x75} },
    {0x82, 1, {0x38} },
    {0x84, 1, {0x38} },
    {0x86, 1, {0x19} },
    {0x66, 1, {0x04} },
    {0xC1, 1, {0x70} },
    {0x70, 1, {0x60} },
    {0x71, 1, {0x00} },
    {0x5B, 1, {0x33} },
    {0x6C, 1, {0x10} },
    {0x77, 1, {0x03} },
    {0x7B, 1, {0x02} },
    {0xFF, 3, {0x98, 0x81, 0x01} },
    {0xF0, 1, {0x00} },
    {0xF1, 1, {0xC8} },
    {0xFF, 3, {0x98, 0x81, 0x02} },
    //{0x01, 1, {0x01} },//bits
    {0x42, 1, {0x2C} },
    {0xFF, 3, {0x98, 0x81, 0x05} },
    {0x22, 1, {0x3A} },
    {0xFF, 3, {0x98, 0x81, 0x00} },   
  
    {0x11,0,{0x00}},
    {REGFLAG_DELAY,120,{}},   
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
	
static void dsi_te_enable(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_DSI_TE_EN, 1);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_DSI_TE_EN, 0);
    }
}

static void vio28_enable(unsigned char enabled)
{
    if (enabled)
    {
       // pmic_config_interface(MT6357_LDO_VIO28_CON1 , 0x1, PMIC_DA_VIO28_EN_MASK, PMIC_DA_VIO28_EN_SHIFT);
    }
    else
    {
      //  pmic_config_interface(MT6357_LDO_VIO28_CON1 , 0x0, PMIC_DA_VIO28_EN_MASK, PMIC_DA_VIO28_EN_SHIFT);
    }
}

static void vio18_enable(unsigned char enabled)
{
    if (enabled)
    {
      //  pmic_config_interface(MT6357_LDO_VIO18_CON1 , 0x1, PMIC_DA_VIO18_EN_MASK, PMIC_DA_VIO18_EN_SHIFT);
    }
    else
    {
      //  pmic_config_interface(MT6357_LDO_VIO18_CON1 , 0x0, PMIC_DA_VIO18_EN_MASK, PMIC_DA_VIO18_EN_SHIFT);
    }
}



static void lcm_init_lcm(void)
{
      vio28_enable(1);
    MDELAY(30);
    vio18_enable(1);
    MDELAY(30);
    dsi_te_enable(1);
    MDELAY(30);


    lcd_reset(1);//LCD rst
    MDELAY(20);
    lcd_reset(0);   
    MDELAY(20);
    lcd_reset(1);
    MDELAY(150);//Must > 5ms
	//init_lcm_registers();
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	MDELAY(50);
//	lcm_set_gpio_output(GPIO_LCD_BL_EN,1);

}

static void lcm_suspend(void)
{
         unsigned int data_array[16];
    data_array[0]=0x00280500; // Display Off
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120); 

    data_array[0] = 0x00100500; // Sleep In
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(5);

    vio28_enable(0);
    MDELAY(30);
    vio18_enable(0);
    MDELAY(30);
    dsi_te_enable(0);
    MDELAY(30);
    lcd_reset(0);
    MDELAY(30);





	printk("[Kernel/LCM-kzh]: gs716 enter lcm_suspend   !\n");
}
  
static void lcm_resume(void)
{
	lcm_init_lcm();
	
	printk("[Kernel/LCM-kzh]: gs716 enter lcm_resume   !\n");
}

struct LCM_DRIVER gs902i_hsd_ili9881_boe_hd720_ips_5_lcm_drv = {
	.name = "gs902i_hsd_ili9881_boe_hd720_ips_5",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
