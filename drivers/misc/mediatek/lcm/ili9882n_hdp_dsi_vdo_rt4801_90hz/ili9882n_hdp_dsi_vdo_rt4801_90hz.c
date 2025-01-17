// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define LOG_TAG "LCM"

#ifndef BUILD_LK
	#include <linux/string.h>
	#include <linux/kernel.h>
#endif

#include "lcm_drv.h"
#include "lcm_util.h"


#ifdef BUILD_LK
	#include <platform/upmu_common.h>
	#include <platform/mt_gpio.h>
	#include <platform/mt_i2c.h>
	#include <platform/mt_pmic.h>
	#include <platform/mt_gpt.h>
	#include <string.h>
#ifndef MACH_FPGA
	#include <lcm_pmic.h>
#endif
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
/*#include <mach/mt_pm_ldo.h>*/
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_gpio.h>
#endif
#endif
#ifdef CONFIG_MTK_LEGACY
#include <cust_gpio_usage.h>
#endif
#ifndef CONFIG_FPGA_EARLY_PORTING
#if defined(CONFIG_MTK_LEGACY)
#include <cust_i2c.h>
#endif
#endif

#ifdef BUILD_LK
	#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
	#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
	#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
	#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

static const unsigned int BL_MIN_LEVEL = 20;
static struct LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))


#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
		lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
		lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
		lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)	lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
		lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
	/*DynFPS*/
#define dfps_dsi_send_cmd( \
			cmdq, cmd, count, para_list, force_update, sendmode) \
			lcm_util.dsi_dynfps_send_cmd( \
			cmdq, cmd, count, para_list, force_update, sendmode)

#ifndef BUILD_LK
	#include <linux/kernel.h>
	#include <linux/module.h>
	#include <linux/fs.h>
	#include <linux/slab.h>
	#include <linux/init.h>
	#include <linux/list.h>
	#include <linux/i2c.h>
	#include <linux/irq.h>
	#include <linux/uaccess.h>
	#include <linux/interrupt.h>
	#include <linux/io.h>
	#include <linux/platform_device.h>
#ifndef CONFIG_FPGA_EARLY_PORTING
#define I2C_I2C_LCD_BIAS_CHANNEL 0
#define TPS_I2C_BUSNUM  I2C_I2C_LCD_BIAS_CHANNEL	/* for I2C channel 0 */
#define I2C_ID_NAME "tps65132"
#define TPS_ADDR 0x3E

#if defined(CONFIG_MTK_LEGACY)
static struct i2c_board_info tps65132_board_info __initdata = {
	I2C_BOARD_INFO(I2C_ID_NAME, TPS_ADDR)
};
#endif
#if !defined(CONFIG_MTK_LEGACY)
static const struct of_device_id lcm_of_match[] = {
		{.compatible = "mediatek,I2C_LCD_BIAS"},
		{},
};
#endif

/*static struct i2c_client *tps65132_i2c_client;*/
struct i2c_client *tps65132_i2c_client;

/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int tps65132_probe(struct i2c_client *client,
		const struct i2c_device_id *id);
static int tps65132_remove(struct i2c_client *client);
/*****************************************************************************
 * Data Structure
 *****************************************************************************/

struct tps65132_dev {
	struct i2c_client *client;

};

static const struct i2c_device_id tps65132_id[] = {
	{I2C_ID_NAME, 0},
	{}
};

/* #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)) */
/* static struct i2c_client_address_data addr_data = { .forces = forces,}; */
/* #endif */
static struct i2c_driver tps65132_iic_driver = {
	.id_table = tps65132_id,
	.probe = tps65132_probe,
	.remove = tps65132_remove,
	/* .detect               = mt6605_detect, */
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "tps65132",
#if !defined(CONFIG_MTK_LEGACY)
			.of_match_table = lcm_of_match,
#endif
		   },
};

static int tps65132_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	LCM_LOGI("tps65132_iic_probe\n");
	LCM_LOGI("TPS: info==>name=%s addr=0x%x\n", client->name, client->addr);
	tps65132_i2c_client = client;
	return 0;
}

static int tps65132_remove(struct i2c_client *client)
{
	LCM_LOGI("%s\n", __func__);
	tps65132_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}

/*static int tps65132_write_bytes(unsigned char addr, unsigned char value)*/
#if !defined(CONFIG_ARCH_MT6797)
int tps65132_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = tps65132_i2c_client;
	char write_data[2] = { 0 };

	write_data[0] = addr;
	write_data[1] = value;
	ret = i2c_master_send(client, write_data, 2);
	if (ret < 0)
		LCM_LOGI("tps65132 write data fail !!\n");
	return ret;
}
#endif

static int __init tps65132_iic_init(void)
{
	LCM_LOGI("%s\n", __func__);
#if defined(CONFIG_MTK_LEGACY)
	i2c_register_board_info(TPS_I2C_BUSNUM, &tps65132_board_info, 1);
#endif
	LCM_LOGI("tps65132_iic_init2\n");
	i2c_add_driver(&tps65132_iic_driver);
	LCM_LOGI("%s success\n", __func__);
	return 0;
}

static void __exit tps65132_iic_exit(void)
{
	LCM_LOGI("%s\n", __func__);
	i2c_del_driver(&tps65132_iic_driver);
}


module_init(tps65132_iic_init);
module_exit(tps65132_iic_exit);

MODULE_AUTHOR("Mike Liu");
MODULE_DESCRIPTION("MTK TPS65132 I2C Driver");
MODULE_LICENSE("GPL");
#endif
#endif

#define LCM_DSI_CMD_MODE			0
#define FRAME_WIDTH										(720)
#define FRAME_HEIGHT									(1520)

#define LCM_PHYSICAL_WIDTH									(64800)
#define LCM_PHYSICAL_HEIGHT									(129600)
#define LCM_DENSITY	(320)

#ifndef CONFIG_FPGA_EARLY_PORTING
#define GPIO_65132_EN GPIO_LCD_BIAS_ENP_PIN
#endif

#define REGFLAG_DELAY			0xFFFC
#define REGFLAG_UDELAY			0xFFFB
#define REGFLAG_END_OF_TABLE		0xFFFD
#define REGFLAG_RESET_LOW		0xFFFE
#define REGFLAG_RESET_HIGH		0xFFFF

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{ 0xFF, 0x03, {0x98, 0x82, 0x00} },
	{0x28, 0, {} },
	{REGFLAG_DELAY, 20, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 120, {} }
};

static struct LCM_setting_table init_setting_vdo[] = {
	{ 0xFF, 0x03, {0x98, 0x82, 0x01} },
	{ 0x00, 0x01, {0x48} },
	{ 0x01, 0x01, {0x34} },
	{ 0x02, 0x01, {0x35} },
	{ 0x03, 0x01, {0x5E} },
	{ 0x08, 0x01, {0x86} },
	{ 0x09, 0x01, {0x01} },
	{ 0x0a, 0x01, {0x73} },
	{ 0x0b, 0x01, {0x00} },
	{ 0x0c, 0x01, {0x5A} },
	{ 0x0d, 0x01, {0x5A} },
	{ 0x0e, 0x01, {0x05} },
	{ 0x0f, 0x01, {0x05} },
	{ 0x28, 0x01, {0x48} },
	{ 0x29, 0x01, {0x86} },
	{ 0x2A, 0x01, {0x48} },
	{ 0x2B, 0x01, {0x86} },
	{ 0x31, 0x01, {0x07} },
	{ 0x32, 0x01, {0x23} },
	{ 0x33, 0x01, {0x00} },
	{ 0x34, 0x01, {0x0B} },
	{ 0x35, 0x01, {0x09} },
	{ 0x36, 0x01, {0x02} },
	{ 0x37, 0x01, {0x15} },
	{ 0x38, 0x01, {0x17} },
	{ 0x39, 0x01, {0x11} },
	{ 0x3A, 0x01, {0x13} },
	{ 0x3B, 0x01, {0x22} },
	{ 0x3C, 0x01, {0x01} },
	{ 0x3D, 0x01, {0x07} },
	{ 0x3E, 0x01, {0x07} },
	{ 0x3F, 0x01, {0x07} },
	{ 0x40, 0x01, {0x07} },
	{ 0x41, 0x01, {0x07} },
	{ 0x42, 0x01, {0x07} },
	{ 0x43, 0x01, {0x07} },
	{ 0x44, 0x01, {0x07} },
	{ 0x45, 0x01, {0x07} },
	{ 0x46, 0x01, {0x07} },
	{ 0x47, 0x01, {0x07} },
	{ 0x48, 0x01, {0x23} },
	{ 0x49, 0x01, {0x00} },
	{ 0x4A, 0x01, {0x0A} },
	{ 0x4B, 0x01, {0x08} },
	{ 0x4C, 0x01, {0x02} },
	{ 0x4D, 0x01, {0x14} },
	{ 0x4E, 0x01, {0x16} },
	{ 0x4F, 0x01, {0x10} },
	{ 0x50, 0x01, {0x12} },
	{ 0x51, 0x01, {0x22} },
	{ 0x52, 0x01, {0x01} },
	{ 0x53, 0x01, {0x07} },
	{ 0x54, 0x01, {0x07} },
	{ 0x55, 0x01, {0x07} },
	{ 0x56, 0x01, {0x07} },
	{ 0x57, 0x01, {0x07} },
	{ 0x58, 0x01, {0x07} },
	{ 0x59, 0x01, {0x07} },
	{ 0x5a, 0x01, {0x07} },
	{ 0x5b, 0x01, {0x07} },
	{ 0x5c, 0x01, {0x07} },
	{ 0x61, 0x01, {0x07} },
	{ 0x62, 0x01, {0x23} },
	{ 0x63, 0x01, {0x00} },
	{ 0x64, 0x01, {0x08} },
	{ 0x65, 0x01, {0x0A} },
	{ 0x66, 0x01, {0x02} },
	{ 0x67, 0x01, {0x12} },
	{ 0x68, 0x01, {0x10} },
	{ 0x69, 0x01, {0x16} },
	{ 0x6a, 0x01, {0x14} },
	{ 0x6b, 0x01, {0x22} },
	{ 0x6c, 0x01, {0x01} },
	{ 0x6d, 0x01, {0x07} },
	{ 0x6e, 0x01, {0x07} },
	{ 0x6f, 0x01, {0x07} },
	{ 0x70, 0x01, {0x07} },
	{ 0x71, 0x01, {0x07} },
	{ 0x72, 0x01, {0x07} },
	{ 0x73, 0x01, {0x07} },
	{ 0x74, 0x01, {0x07} },
	{ 0x75, 0x01, {0x07} },
	{ 0x76, 0x01, {0x07} },
	{ 0x77, 0x01, {0x07} },
	{ 0x78, 0x01, {0x23} },
	{ 0x79, 0x01, {0x00} },
	{ 0x7a, 0x01, {0x09} },
	{ 0x7b, 0x01, {0x0B} },
	{ 0x7c, 0x01, {0x02} },
	{ 0x7d, 0x01, {0x13} },
	{ 0x7e, 0x01, {0x11} },
	{ 0x7f, 0x01, {0x17} },
	{ 0x80, 0x01, {0x15} },
	{ 0x81, 0x01, {0x22} },
	{ 0x82, 0x01, {0x01} },
	{ 0x83, 0x01, {0x07} },
	{ 0x84, 0x01, {0x07} },
	{ 0x85, 0x01, {0x07} },
	{ 0x86, 0x01, {0x07} },
	{ 0x87, 0x01, {0x07} },
	{ 0x88, 0x01, {0x07} },
	{ 0x89, 0x01, {0x07} },
	{ 0x8a, 0x01, {0x07} },
	{ 0x8b, 0x01, {0x07} },
	{ 0x8c, 0x01, {0x07} },

// RTN. Internal VBP, Internal VFP
	{ 0xFF, 0x03, {0x98, 0x82, 0x02} },
	{ 0xF1, 0x01, {0x1C} },
	{ 0x4B, 0x01, {0x5A} },
	{ 0x50, 0x01, {0xCA} },
	{ 0x51, 0x01, {0x00} },
	{ 0x06, 0x01, {0x5C} },
	{ 0x0B, 0x01, {0xA0} },
	{ 0x0C, 0x01, {0x80} },
	{ 0x0D, 0x01, {0x12} },
	{ 0x0E, 0x01, {0x6D} },
	{ 0x40, 0x01, {0x4C} },
	{ 0x4E, 0x01, {0x11} },

// Power Setting
	{ 0xFF, 0x03, {0x98, 0x82, 0x05} },
	{ 0x03, 0x01, {0x01} },
	{ 0x04, 0x01, {0x43} },
	{ 0x50, 0x01, {0x2F} },
	{ 0x58, 0x01, {0x63} },
	{ 0x63, 0x01, {0x9C} },
	{ 0x64, 0x01, {0x8D} },
	{ 0x68, 0x01, {0x65} },
	{ 0x69, 0x01, {0x81} },
	{ 0x6A, 0x01, {0xC9} },
	{ 0x6B, 0x01, {0xCF} },
	{ 0x46, 0x01, {0x00} },
	{ 0x85, 0x01, {0x37} },

// Resolution
	{ 0xFF, 0x03, {0x98, 0x82, 0x06} },
	{ 0xD9, 0x01, {0x1F} },
	{ 0x08, 0x01, {0x00} },
	{ 0xC0, 0x01, {0xF0} },
	{ 0xC1, 0x01, {0x15} },

// Gamma Register
	{ 0xFF, 0x03, {0x98, 0x82, 0x08} },
	{ 0xE0, 0x1B, {0x00, 0x24, 0x41, 0x5A, 0x81, 0x40,
		0xA6, 0xC8, 0xF2, 0x18, 0x55, 0x58,
		0x8F, 0xC4, 0xF7, 0xAA, 0x2E, 0x6F,
		0x97, 0xC9, 0xFE, 0xF2, 0x26, 0x63,
		0x96, 0x03, 0xEC} },
	{ 0xE1, 0x1B, {0x00, 0x24, 0x41, 0x5A, 0x81, 0x40,
		0xA6, 0xC8, 0xF2, 0x18, 0x55, 0x58,
		0x8F, 0xC4, 0xF7, 0xAA, 0x2E, 0x6F,
		0x97, 0xC9, 0xFE, 0xF2, 0x26, 0x63,
		0x96, 0x03, 0xEC} },

// OSC Auto Trim Setting} },
	{ 0xFF, 0x03, {0x98, 0x82, 0x0B} },
	{ 0x9A, 0x01, {0x42} },
	{ 0x9B, 0x01, {0xF6} },
	{ 0x9C, 0x01, {0x02} },
	{ 0x9D, 0x01, {0x02} },
	{ 0x9E, 0x01, {0x4A} },
	{ 0x9F, 0x01, {0x4A} },
	{ 0xAB, 0x01, {0xE0} },

	{ 0xFF, 0x03, {0x98, 0x82, 0x0E} },
	{ 0x11, 0x01, {0x10} },
	{ 0x13, 0x01, {0x10} },
	{ 0x00, 0x01, {0xA0} },

	{ 0xFF, 0x03, {0x98, 0x82, 0x00} },
	{ 0x11, 0x00, {} },
	{ REGFLAG_DELAY, 120, {} },
	{ 0x29, 0x00, {} },
	{ REGFLAG_DELAY, 20, {} },
	{ 0x35, 0x01, {0x00} }
};

static struct LCM_setting_table bl_level[] = {
	{ 0xFF, 0x03, {0x98, 0x82, 0x00} },
	{ 0x51, 0x02, {0x00, 0xFF} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};

/*******************Dynfps start*************************/
#ifdef CONFIG_MTK_HIGH_FRAME_RATE

#define DFPS_MAX_CMD_NUM 10

struct LCM_dfps_cmd_table {
	bool need_send_cmd;
	enum LCM_Send_Cmd_Mode sendmode;
	struct LCM_setting_table prev_f_cmd[DFPS_MAX_CMD_NUM];
};

static struct LCM_dfps_cmd_table
	dfps_cmd_table[DFPS_LEVELNUM][DFPS_LEVELNUM] = {

/**********level 0 to 0,1 cmd*********************/
[DFPS_LEVEL0][DFPS_LEVEL0] = {
	false,
	LCM_SEND_IN_VDO,
	/*prev_frame cmd*/
	{
	{REGFLAG_END_OF_TABLE, 0x00, {} },
	},
},
/*60->90*/
[DFPS_LEVEL0][DFPS_LEVEL1] = {
	true,
	LCM_SEND_IN_VDO,
	/*prev_frame cmd*/
	{
	{0xFF, 1, {0x25} },
	{0xFB, 1, {0x01} },
	{0x18, 1, {0x20} },
	{REGFLAG_END_OF_TABLE, 0x00, {} },
	},
},

/**********level 1 to 0,1 cmd*********************/
/*90->60*/
[DFPS_LEVEL1][DFPS_LEVEL0] = {
	true,
	LCM_SEND_IN_VDO,
	/*prev_frame cmd*/
	{
	{0xFF, 1, {0x25} },
	{0xFB, 1, {0x01} },
	{0x18, 1, {0x21} },
	{REGFLAG_END_OF_TABLE, 0x00, {} },
	},
},

[DFPS_LEVEL1][DFPS_LEVEL1] = {
	false,
	LCM_SEND_IN_VDO,
	/*prev_frame cmd*/
	{
	{REGFLAG_END_OF_TABLE, 0x00, {} },
	},

},

};
#endif
/*******************Dynfps end*************************/

static void push_table(void *cmdq, struct LCM_setting_table *table,
	unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned int cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;
		switch (cmd) {
		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;
		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;
		case REGFLAG_END_OF_TABLE:
			break;
		default:
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count, table[i].para_list, force_update);
			break;
		}
	}
}


static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}
#ifdef CONFIG_MTK_HIGH_FRAME_RATE
/*DynFPS*/
static void lcm_dfps_int(struct LCM_DSI_PARAMS *dsi)
{
	struct dfps_info *dfps_params = dsi->dfps_params;

	dsi->dfps_enable = 1;
	dsi->dfps_default_fps = 6000;/*real fps * 100, to support float*/
	dsi->dfps_def_vact_tim_fps = 9000;/*real vact timing fps * 100*/

	/*traversing array must less than DFPS_LEVELS*/
	/*DPFS_LEVEL0*/
	dfps_params[0].level = DFPS_LEVEL0;
	dfps_params[0].fps = 6000;/*real fps * 100, to support float*/
	dfps_params[0].vact_timing_fps = 9000;/*real vact timing fps * 100*/
	/*if mipi clock solution*/
	/*dfps_params[0].PLL_CLOCK = xx;*/
	/*dfps_params[0].data_rate = xx; */
	/*if HFP solution*/
	/*dfps_params[0].horizontal_frontporch = xx;*/
	dfps_params[0].vertical_frontporch = 1260;
	dfps_params[0].vertical_frontporch_for_low_power = 2200;

	/*if need mipi hopping params add here*/
	/*dfps_params[0].PLL_CLOCK_dyn =xx;
	 *dfps_params[0].horizontal_frontporch_dyn =xx ;
	 * dfps_params[0].vertical_frontporch_dyn = 1291;
	 */

	/*DPFS_LEVEL1*/
	dfps_params[1].level = DFPS_LEVEL1;
	dfps_params[1].fps = 9000;/*real fps * 100, to support float*/
	dfps_params[1].vact_timing_fps = 9000;/*real vact timing fps * 100*/
	/*if mipi clock solution*/
	/*dfps_params[1].PLL_CLOCK = xx;*/
	/*dfps_params[1].data_rate = xx; */
	/*if HFP solution*/
	/*dfps_params[1].horizontal_frontporch = xx;*/
	dfps_params[1].vertical_frontporch = 324;
	dfps_params[1].vertical_frontporch_for_low_power = 2200;

	/*if need mipi hopping params add here*/
	/*dfps_params[1].PLL_CLOCK_dyn =xx;
	 *dfps_params[1].horizontal_frontporch_dyn =xx ;
	 * dfps_params[1].vertical_frontporch_dyn= 54;
	 * dfps_params[1].vertical_frontporch_for_low_power_dyn =xx;
	 */

	dsi->dfps_num = 2;
}
#endif
static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
	params->density = LCM_DENSITY;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
	lcm_dsi_mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
	lcm_dsi_mode = SYNC_PULSE_VDO_MODE;
#endif
	pr_debug("%s:lcm_dsi_mode %d\n", __func__, lcm_dsi_mode);
	params->dsi.switch_mode_enable = 0;

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	/* video mode timing */

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 12;
	params->dsi.vertical_backporch = 8;
	params->dsi.vertical_frontporch = 1260;
	/* params->dsi.vertical_frontporch_for_low_power = 540; */
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 8;
	params->dsi.horizontal_backporch = 36;
	params->dsi.horizontal_frontporch = 36;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;

#ifndef MACH_FPGA
	params->dsi.PLL_CLOCK = 442;	/* this value must be in MTK suggested table */
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
#ifdef CONFIG_MTK_HIGH_FRAME_RATE
	/****DynFPS start****/
	lcm_dfps_int(&(params->dsi));
	/****DynFPS end****/
#endif
}
#ifdef BUILD_LK
#ifndef CONFIG_FPGA_EARLY_PORTING
#define TPS65132_SLAVE_ADDR_WRITE  0x7C
static struct mt_i2c_t TPS65132_i2c;

static int TPS65132_write_byte(kal_uint8 addr, kal_uint8 value)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0] = addr;
	write_data[1] = value;

	TPS65132_i2c.id = I2C_I2C_LCD_BIAS_CHANNEL;	/* I2C2; */
/* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
	TPS65132_i2c.addr = (TPS65132_SLAVE_ADDR_WRITE >> 1);
	TPS65132_i2c.mode = ST_MODE;
	TPS65132_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&TPS65132_i2c, write_data, len);
	/* printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code); */

	return ret_code;
}

#else

/* extern int mt8193_i2c_write(u16 addr, u32 data); */
/* extern int mt8193_i2c_read(u16 addr, u32 *data); */

/* #define TPS65132_write_byte(add, data)  mt8193_i2c_write(add, data) */
/* #define TPS65132_read_byte(add)  mt8193_i2c_read(add) */

#endif
#endif

/* turn on gate ic & control voltage to 5.8V */
static void lcm_init_power(void)
{
	unsigned char cmd = 0x0;
	unsigned char data = 0xFF;
#ifndef CONFIG_FPGA_EARLY_PORTING
	int ret = 0;
#endif
	if (lcm_util.set_gpio_lcd_enp_bias) {
		lcm_util.set_gpio_lcd_enp_bias(1);

#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef CONFIG_MTK_LEGACY
	mt_set_gpio_mode(GPIO_65132_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_EN, GPIO_OUT_ONE);
#else
	lcm_util.set_gpio_lcd_enp_bias(1);
#endif
	MDELAY(5);
#ifdef BUILD_LK
	ret = TPS65132_write_byte(cmd, data);
#else
#if !defined(CONFIG_ARCH_MT6797)
	// ret = tps65132_write_bytes(cmd, data);
#endif
#endif

	if (ret < 0) {
		pr_info("ili9881c----tps6132----cmd=%0x--i2c write error----\n",
			cmd);
	} else {
		pr_info("ili9881c----tps6132----cmd=%0x--i2c write success----\n",
			cmd);
	}

	cmd = 0x01;
	data = 0x0E;

#ifdef BUILD_LK
	ret = TPS65132_write_byte(cmd, data);
#else
#if !defined(CONFIG_ARCH_MT6797)
	// ret = tps65132_write_bytes(cmd, data);
#endif
#endif

	if (ret < 0) {
		pr_info("ili9881c----tps6132----cmd=%0x--i2c write error----\n",
			cmd);
	} else {
		pr_info("ili9881c----tps6132----cmd=%0x--i2c write success----\n",
			cmd);
	}

#endif
	} else
		LCM_LOGI("set_gpio_lcd_enp_bias not defined\n");
}

static void lcm_suspend_power(void)
{
	SET_RESET_PIN(0);
	if (lcm_util.set_gpio_lcd_enp_bias)
		lcm_util.set_gpio_lcd_enp_bias(0);
	else
		LCM_LOGI("set_gpio_lcd_enp_bias not defined\n");
}

static void lcm_resume_power(void)
{
	SET_RESET_PIN(0);
	lcm_init_power();
}

static void lcm_init(void)
{
	SET_RESET_PIN(0);

	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(5);

	push_table(NULL, init_setting_vdo, sizeof(init_setting_vdo) /
		   sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) /
		   sizeof(struct LCM_setting_table), 1);

	SET_RESET_PIN(0);
	MDELAY(10);
}

static void lcm_resume(void)
{
	lcm_init();
}

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{

}

#define LCM_ID_ILI9882N (0x46)
#define LCM_VERSION_ILI9882N  (0x33)

static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0, version_id = 0;
	unsigned char buffer[2];
	unsigned int array[16];
	struct LCM_setting_table switch_table_page1[] = {
		{ 0xFF, 0x03, {0x98, 0x82, 0x01} }
	};
	struct LCM_setting_table switch_table_page0[] = {
		{ 0xFF, 0x03, {0x98, 0x82, 0x00} }
	};

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);

	SET_RESET_PIN(1);
	MDELAY(20);

	push_table(NULL, switch_table_page1, sizeof(switch_table_page1) / sizeof(struct LCM_setting_table), 1);

	array[0] = 0x00023700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x00, buffer, 1);
	id = buffer[0];		/* we only need ID */

	read_reg_v2(0x01, buffer, 1);
	version_id = buffer[0];

	LCM_LOGI("%s,ili9882n_id=0x%08x,version_id=0x%x\n", __func__, id, version_id);
	push_table(NULL, switch_table_page0, sizeof(switch_table_page0) / sizeof(struct LCM_setting_table), 1);

	if (id == LCM_ID_ILI9882N && version_id == LCM_VERSION_ILI9882N)
		return 1;
	else
		return 0;

}

/* return TRUE: need recovery */
/* return FALSE: No need recovery */
static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	char buffer[3];
	int array[4];

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x53, buffer, 1);

	if (buffer[0] != 0x24) {
		LCM_LOGI("[LCM ERROR] [0x53]=0x%02x\n", buffer[0]);
		return TRUE;
	}
	LCM_LOGI("[LCM NORMAL] [0x53]=0x%02x\n", buffer[0]);
	return FALSE;
#else
	return FALSE;
#endif

}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
	unsigned int ret = 0;
	unsigned int x0 = FRAME_WIDTH / 4;
	unsigned int x1 = FRAME_WIDTH * 3 / 4;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);

	unsigned int data_array[3];
	unsigned char read_buf[4];

	LCM_LOGI("ATA check size = 0x%x,0x%x,0x%x,0x%x\n",
		 x0_MSB, x0_LSB, x1_MSB, x1_LSB);
	data_array[0] = 0x0005390A; /* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00043700; /* read id return two byte,version and id */
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x2A, read_buf, 4);

	if ((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB) &&
	    (read_buf[2] == x1_MSB) && (read_buf[3] == x1_LSB))
		ret = 1;
	else
		ret = 0;

	x0 = 0;
	x1 = FRAME_WIDTH - 1;

	x0_MSB = ((x0 >> 8) & 0xFF);
	x0_LSB = (x0 & 0xFF);
	x1_MSB = ((x1 >> 8) & 0xFF);
	x1_LSB = (x1 & 0xFF);

	data_array[0] = 0x0005390A; /* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	return ret;
#else
	return 0;
#endif
}

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
	LCM_LOGI("%s,ili9882n backlight: level = %d\n", __func__, level);

	LCM_LOGI("%s,ili9882n backlight: get default level = %d\n", __func__, bl_level[1].para_list[0]);

	//for 12bit switch to 8bit
	bl_level[1].para_list[0] = (level&0xF0)>>4;
	bl_level[1].para_list[1] = (level&0xF)<<4;

	push_table(NULL, bl_level, sizeof(bl_level) /
		   sizeof(struct LCM_setting_table), 1);
}

static void *lcm_switch_mode(int mode)
{
	return NULL;
}

#if (LCM_DSI_CMD_MODE)
/* partial update restrictions:
 * 1. roi width must be 1080 (full lcm width)
 * 2. vertical start (y) must be multiple of 16
 * 3. vertical height (h) must be multiple of 16
 */
static void lcm_validate_roi(int *x, int *y, int *width, int *height)
{

}
#endif

/*******************Dynfps start*************************/
#ifdef CONFIG_MTK_HIGH_FRAME_RATE
static void dfps_dsi_push_table(
	void *cmdq, struct LCM_setting_table *table,
	unsigned int count, unsigned char force_update, enum LCM_Send_Cmd_Mode sendmode)
{
	unsigned int i;
	unsigned int cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;
		switch (cmd) {
		case REGFLAG_END_OF_TABLE:
			return;
		default:
			dfps_dsi_send_cmd(
				cmdq, cmd, table[i].count,
				table[i].para_list, force_update, sendmode);
			break;
		}
	}

}
static bool lcm_dfps_need_inform_lcm(
	unsigned int from_level, unsigned int to_level, struct LCM_PARAMS *params)
{
	struct LCM_dfps_cmd_table *p_dfps_cmds = NULL;

	if (from_level == to_level) {
		LCM_LOGI("%s,same level\n", __func__);
		return false;
	}
	p_dfps_cmds =
		&(dfps_cmd_table[from_level][to_level]);
	params->sendmode = p_dfps_cmds->sendmode;

	return p_dfps_cmds->need_send_cmd;
}

static void lcm_dfps_inform_lcm(void *cmdq_handle,
unsigned int from_level, unsigned int to_level, struct LCM_PARAMS *params)
{
	struct LCM_dfps_cmd_table *p_dfps_cmds = NULL;
	enum LCM_Send_Cmd_Mode sendmode = LCM_SEND_IN_CMD;

	if (from_level == to_level) {
		LCM_LOGI("%s,same level\n", __func__);
		goto done;
	}
	p_dfps_cmds =
		&(dfps_cmd_table[from_level][to_level]);

	if (p_dfps_cmds &&
		!(p_dfps_cmds->need_send_cmd)) {
		LCM_LOGI("%s,no cmd[L%d->L%d]\n",
			__func__, from_level, to_level);
		goto done;
	}

	sendmode = params->sendmode;

	dfps_dsi_push_table(
		cmdq_handle, p_dfps_cmds->prev_f_cmd,
		ARRAY_SIZE(p_dfps_cmds->prev_f_cmd), 1, sendmode);
done:
	LCM_LOGI("%s,done %d->%d\n",
		__func__, from_level, to_level);

}
#endif
/*******************Dynfps end*************************/
struct LCM_DRIVER ili9882n_hdp_dsi_vdo_rt4801_90hz_lcm_drv = {
	.name = "ili9882n_hdp_dsi_vdo_rt4801_90hz_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.esd_check = lcm_esd_check,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.ata_check = lcm_ata_check,
	.update = lcm_update,
	.switch_mode = lcm_switch_mode,
#if (LCM_DSI_CMD_MODE)
	.validate_roi = lcm_validate_roi,
#endif
#ifdef CONFIG_MTK_HIGH_FRAME_RATE
	/*DynFPS*/
	.dfps_send_lcm_cmd = lcm_dfps_inform_lcm,
	.dfps_need_send_cmd = lcm_dfps_need_inform_lcm,
#endif
};

