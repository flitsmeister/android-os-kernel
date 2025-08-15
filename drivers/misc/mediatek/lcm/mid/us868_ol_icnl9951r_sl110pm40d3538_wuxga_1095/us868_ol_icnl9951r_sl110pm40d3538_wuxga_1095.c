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
#include <linux/i2c.h>
#include <linux/fs.h>

#include "lcm_drv.h"

extern unsigned int GPIO_LCM_1V8_EN;
extern unsigned int GPIO_LCM_BL_EN;
extern unsigned int GPIO_LCM_BIAS_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_TP_RST;

#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0


#define OCP2131_BIAS_I2C_ENABLE   1//外部i2c偏压
#if 1
/************* add for bias ic-i2c-control interface start****************************/
#if OCP2131_BIAS_I2C_ENABLE
static struct i2c_client *ocp2131_client = NULL;
static const struct i2c_device_id ocp2131_i2c_id[] = { {"ocp2131", 0}, {} };

static int ocp2131_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);

#ifdef CONFIG_OF
static const struct of_device_id ocp2131_of_match[] = {
	{.compatible = "mid,ocp2131",},
	{},
};

MODULE_DEVICE_TABLE(i2c, ocp2131_of_match);
#endif

static struct i2c_driver ocp2131_driver = {
	.driver = {
		   .name = "ocp2131",
#ifdef CONFIG_OF
		   .of_match_table = ocp2131_of_match,
#endif
	},
	.probe = ocp2131_i2c_probe,
	.id_table = ocp2131_i2c_id,
};

static int ocp2131_read(u8 addr, u8 *data)
{
	u8 buf;
	int ret = 0;
	struct i2c_client *client = ocp2131_client;
		buf = addr;
		//printk(" %s:%d i2c->addr:0x%x\n",__func__,__LINE__,ocp2131_client->addr);
		ret = i2c_master_send(client, (const char *)&buf, 1);
		if (ret != 1) {
			printk("send command error!!\n");
			return -EFAULT;
		}
		ret = i2c_master_recv(client, (char *)&buf, 1);
		if (ret != 1) {
			printk("reads data error!!\n");
			return -EFAULT;
		}
		else
			printk("%s(0x%02X) = 0x%02X\n", __func__, addr, buf);
		*data = buf;
		return 0;
}
static int ocp2131_write(u8 reg, unsigned char value)
{
        int ret;
        u8 write_cmd[2] = {0};

        write_cmd[0] = reg;
        write_cmd[1] = value;

        ret = i2c_master_send(ocp2131_client, write_cmd, 2);
        if (ret != 2) {
                printk("ocp2131_write error->[REG=0x%02x,val=0x%02x,ret=%d]\n", reg, value, ret);
                return -1;
        }
		 printk("ocp2131_write success->[REG=0x%02x,val=0x%02x,ret=%d]\n", reg, value, ret);
        return 0;
}
static void ocp2131_dump_register(void)
{
	unsigned char ocp2131_reg[4];
	int reg_index;
	for (reg_index = 0; reg_index < 4; reg_index++) {
		ocp2131_read(reg_index, &ocp2131_reg[reg_index]);
		printk(" [0x%x]=0x%x ", reg_index, ocp2131_reg[reg_index]);
	}
	return;
}
static int ocp2131_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{

	ocp2131_client = i2c;
	
	printk(" %s:%d i2c->addr:0x%x\n",__func__,__LINE__,ocp2131_client->addr);
	if(0x3e == ocp2131_client->addr){
		printk(" %s:%d enter ocp2131_dump_register\n",__func__,__LINE__);
		ocp2131_dump_register();
	}
	return 0;
}

static int __init ocp2131_init(void)
{
	printk("******** ocp2131_init! ********\n");
	if (i2c_add_driver(&ocp2131_driver) != 0) {
		printk("[ocp2131_init] failed to register ocp2131 i2c driver.\n");
	} else {
		printk("[ocp2131_init] Success to register ocp2131 i2c driver.\n");
	}
	
	return 0;
}



static void __exit ocp2131_exit(void)
{
	i2c_del_driver(&ocp2131_driver);
    printk("[ocp2131] ocp2131_exit !!!! \n");	
	
}

module_init(ocp2131_init);
module_exit(ocp2131_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C ocp2131 bias avdd avee Driver");
MODULE_AUTHOR("mid@szroco.com");

#endif
#endif
/************* add for bias ic-i2c-control interface end****************************/

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	gpio_direction_output(GPIO, output);
	gpio_set_value(GPIO, output);
}

/* ------------------------------------------------------------------- */
/* Local Constants */
/* ------------------------------------------------------------------- */

#define FRAME_WIDTH  (1200)
#define FRAME_HEIGHT (1920)

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
        lcm_set_gpio_output(GPIO_LCM_1V8_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_1V8_EN, GPIO_OUT_ZERO);
    }
}

static void tp_reset_en(unsigned char enabled)
{
    if (enabled) {
        lcm_set_gpio_output(GPIO_TP_RST, GPIO_OUT_ONE);
    } else {
        lcm_set_gpio_output(GPIO_TP_RST, GPIO_OUT_ZERO);
    }
}

#if OCP2131_BIAS_I2C_ENABLE
static void lcd_bias_en(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_EN, GPIO_OUT_ONE);
        lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_EN, GPIO_OUT_ZERO);
        lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ZERO);
    }
}
#endif

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
    params->dsi.vertical_backporch                              = 40; //10; //16;
    params->dsi.vertical_frontporch                             = 150;//5; 
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 4; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 40; //60; //60; //80;
    params->dsi.horizontal_frontporch                           = 40; //60; 
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 480;
 
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
    {0xF0, 2, {0x5A,0x59}},
    {0xF1, 2, {0xA5,0xA6}},
    {0xB2, 30, {0x06,0x05,0x89,0x8A,0x77,0x44,0x85,0x89,0x22,0xB7,0xB7,0xB7,0xB7,0xB7,0xB7,0xB7,0xB7,0xB7,0xB7,0xB7,0xB7,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x55,0x11}},
    {0xB3, 29, {0x73,0x05,0x01,0x05,0x81,0xB7,0x00,0x00,0xB7,0x00,0x00,0x11,0x34,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x06,0x80,0x80,0x01}},
    {0xB4, 31, {0x0D,0x1F,0x0D,0x1F,0x56,0x26,0x01,0x3B,0x00,0x22,0x00,0x22,0x00,0x00,0x00,0x01,0x02,0x13,0x20,0x30,0x05,0x90,0x23,0x20,0x40,0x11,0x10,0x20,0x00,0x00,0x00}},
    //正扫
    {0xB5, 31, {0x25,0x24,0x81,0x00,0x00,0xA8,0xA9,0x00,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x23,0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFC}},
    {0xB6, 31, {0x25,0x24,0x81,0x00,0x00,0xA8,0xA9,0x00,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x23,0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFC}},
    ///反扫
    //RB5 22 23 00 81 00 A8 A9 00 13 12 11 10 0F 0E 0D 0C 24 25 00 00 00 00 00 00 00 00 00 00 FF FF FC
    //RB6 22 23 00 81 00 A8 A9 00 13 12 11 10 0F 0E 0D 0C 24 25 00 00 00 00 00 00 00 00 00 00 FF FF FC
    //R36 03
    ////
    {0xB7, 30, {0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xAA,0xAA,0xAA,0xAA,0xA0}},
    {0xB8, 30, {0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xAA,0xAA,0xAA,0xAA,0xA0}},
    {0xB9, 26, {0x01,0x55,0x55,0x55,0x55,0x55,0x50,0x55,0x55,0x55,0x55,0x55,0x50,0x55,0x55,0x55,0x55,0x55,0x50,0x55,0x55,0x55,0x55,0x55,0x50}},
    {0xBB, 15, {0x01,0x02,0x03,0x0A,0x04,0x13,0x14,0x12,0x16,0x5C,0x00,0x15,0x16,0x03,0x00}},
    {0xBC, 25, {0xFE,0xF8,0xF0,0x00,0x00,0x00,0x00,0x04,0x00,0x05,0x80,0x02,0x23,0x00,0xC9,0x99,0x99,0x00,0xC4,0x09,0xC3,0x86,0x03,0x2E,0x11}},
    {0xBD, 7, {0x00,0x23,0x42,0x52,0x52,0x1F,0x00}},//GASF
    {0xBE, 23, {0x5C,0x48,0x64,0x46,0x0A,0x88,0x58,0x33,0x33,0x33,0x93,0x00,0xDD,0xDD,0x00,0x00,0x00,0x00,0xB2,0xAF,0xB2,0xAF,0x00}},
    {0xBF, 10, {0x0C,0x19,0x0C,0x19,0x00,0x11,0x22,0x04,0x5D,0x07}},
    {0xC0, 22, {0x40,0x90,0x17,0x12,0x34,0xF5,0x67,0x89,0xFF,0xFF,0xFF,0xFF,0xFF,0x3F,0x00,0xFF,0x00,0xCC,0x02,0x00,0x01,0xB3}},
    {0xC1, 22, {0x00,0x96,0x00,0x28,0x00,0x28,0x04,0x28,0x28,0x04,0xC7,0x80,0x0F,0x00,0xC0,0x22,0x7F,0x80,0x10,0xFF,0x0F,0xE7}},
    {0xC2, 1, {0x00}},
    {0xC3, 32, {0x00,0xFF,0x42,0x4D,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xB9,0x10,0x10,0x2A,0x2A,0x2A,0x2A,0x2A,0x2A,0x2A,0x2A,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
    {0xC4, 14, {0x0C,0x35,0x28,0x49,0x00,0x3F,0x00,0x50,0x00,0x1F,0x00,0xA2,0xF0,0xE7}},
    //LONG-V
    {0xC5, 32, {0x03,0x13,0x10,0x56,0x5D,0x38,0x04,0x05,0x08,0x04,0x19,0x00,0xB4,0x2C,0x2B,0x2B,0xAF,0xAF,0x20,0x00,0x02,0x00,0x80,0x0C,0x0C,0x06,0x13,0x64,0xFF,0x03,0x20,0xFF}},
    {0xC8, 7, {0x42,0x00,0x48,0xEC,0xE0,0x00,0x23}},
    {0xD5, 4, {0x01,0x30,0xD8,0x10}},
    ///RD7 3F 04 0A 00 00 06
    {0xD7, 6, {0x3F,0x04,0x0A,0x00,0x00,0x0E}},
    //VCOM SETTING
    //R89 57 57 11//如果烧录VCOM，请关闭
    //GAMMA
    {0xC7, 29, {0x76,0x54,0x32,0x22,0x34,0x56,0x77,0x77,0x20,0x76,0x54,0x32,0x22,0x34,0x56,0x77,0x77,0x20,0x42,0x00,0x21,0xFF,0xFF,0x04,0x04,0x03,0x0E,0x07,0x00}},
    {0x80, 32, {0xE8,0xD9,0xBE,0xA8,0x95,0x83,0x74,0x65,0x57,0x2C,0x09,0xEC,0xD4,0xBE,0xA7,0x7D,0x59,0x35,0x0D,0x0B,0xE3,0xBB,0x8F,0x5D,0x20,0xDA,0xB1,0x7A,0x6E,0x5C,0x48,0x38}},
    {0x81, 32, {0xE8,0xD8,0xBB,0xA4,0x8F,0x7D,0x6D,0x5F,0x52,0x26,0x04,0xE8,0xD0,0xB9,0xA2,0x79,0x55,0x30,0x09,0x07,0xDF,0xB7,0x8C,0x5A,0x1D,0xD7,0xAF,0x77,0x6B,0x59,0x45,0x36}},
    {0x82, 32, {0xE8,0xD8,0xBC,0xA6,0x92,0x80,0x70,0x62,0x54,0x29,0x06,0xEA,0xD2,0xBB,0xA4,0x7B,0x57,0x32,0x0B,0x09,0xE1,0xB9,0x8D,0x5B,0x1E,0xD8,0xB0,0x78,0x6C,0x5A,0x46,0x37}},
    {0x83, 25, {0x09,0x2D,0x1D,0x09,0x00,0x2B,0x1B,0x07,0x00,0x2C,0x1C,0x08,0x00,0x28,0x18,0x08,0x00,0x28,0x18,0x08,0x00,0x28,0x18,0x08,0x00}},
    {0x84, 27, {0xFF,0xFF,0xFE,0xAA,0xAA,0x55,0x40,0x00,0x00,0xFF,0xFF,0xFE,0xAA,0xAA,0x55,0x40,0x00,0x00,0xFF,0xFF,0xFE,0xAA,0xAA,0x55,0x40,0x00,0x00}},
    {0x35, 1, {0x00}},

    // {0xC0, 22, {0x42,0x90,0x17,0x12,0x34,0xF5,0x67,0x89,0xFF,0xFF,0xFF,0xFF,0xFF,0x3F,0x00,0xFF,0x00,0xCC,0x02,0x00,0x01,0xB3}},
    // {0xD7, 6, {0x3F,0x04,0x0A,0x00,0x00,0x0E}},

    {0x11, 0, {}},
    {REGFLAG_DELAY, 120, {}},
    {0xBD, 7, {0xED,0x23,0x42,0x52,0x52,0x1F,0x00}},//GASN//出睡眠须添加的代码
    {0xF1, 2, {0x5A,0x59}},
    {0xF0, 2, {0xA5,0xA6}},
    {0x29, 0, {}},
    {REGFLAG_DELAY, 20, {}},
    // //TP MARK
    {0xAC, 1, {0x05}},//出睡眠须添加的代码



    // {0xAC, 1, {0x0A}},//进睡眠
    // {0x28, 0, {}},
    // {REGFLAG_DELAY, 10, {}},
    // {0x10, 0, {}},
    // {REGFLAG_DELAY, 20, {}},
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

#if OCP2131_BIAS_I2C_ENABLE
	lcd_bias_en(1);
	MDELAY(10);
	ocp2131_write(0x00,0x11);
	ocp2131_write(0x01,0x11);
    MDELAY(20);
#endif
    tp_reset_en(1);
    MDELAY(20);
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

#if OCP2131_BIAS_I2C_ENABLE
    lcd_bias_en(0);
    ocp2131_write(0x00,0x0);
	ocp2131_write(0x01,0x0);
    MDELAY(50);
#endif
    tp_reset_en(0);
    MDELAY(20);
    lcd_reset(0);
    MDELAY(10);
	lcd_power_en(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
	lcm_init();

}


struct LCM_DRIVER us868_ol_icnl9951r_sl110pm40d3538_wuxga_1095_lcm_drv = 
{
    .name			= "us868_ol_icnl9951r_sl110pm40d3538_wuxga_1095",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
