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

#define   LCM_DSI_CMD_MODE	0

static void init_lcm_registers(void)
{
    unsigned int data_array[16];  
data_array[0] = 0x00E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x93E11500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x65E21500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xF8E31500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x03801500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);



data_array[0] = 0x01E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x00001500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x26011500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);


data_array[0] = 0x00171500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xD7181500;//4.8V
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x01191500;//0.3V
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x001A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xD71B1500; //VGMN=0
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x011C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x701F1500; //VGH_REG=16.2V
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2D201500; //VGL_REG=-12V
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2D211500; //VGL_REG2=-12V
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x7E221500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xFD241500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x19371500; //SS=1,BGR=1
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x28351500; //SAP
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x05381500; //JDT=101 zigzag inversion
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x08391500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x123A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x7E3C1500; //SET EQ3 for TE_H
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xFF3D1500; //SET CHGEN_ON, modify 20140806
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xFF3E1500; //SET CHGEN_OFF, modify 20140806
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x7F3F1500; //SET CHGEN_OFF2, modify 20140806
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);


data_array[0] = 0x06401500; //RSO=
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xA0411500; //LN=640->1280 line
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1E431500; //VFP=
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x0B441500; //VBP
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);



data_array[0] = 0x01551500; //DCDCM=1111
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x01561500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x6A571500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x09581500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x0A591500; //VCL = -2.5V
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2E5A1500; //VGH = 16.2V
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x1A5B1500; //VGL = -12V
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x155C1500; //pump clk
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);



data_array[0] = 0x7F5D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x5D5E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4B5F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x3E601500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x3A611500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2B621500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2F631500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x19641500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x32651500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x31661500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x31671500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4F681500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x3E691500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x476A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x366B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x316C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x246D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x126E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x026F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x7F701500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x5D711500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4B721500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x3E731500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x3A741500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2B751500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2F761500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x19771500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x32781500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x31791500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x317A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4F7B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x3E7C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x477D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x367E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x317F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x24801500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x12811500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x02821500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x02E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x52001500;//RESET_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55011500;//VSSG_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55021500;//VSSG_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x50031500;//STV2_ODD
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x77041500;//VDD2_ODD
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x57051500;//VDD1_ODD
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55061500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4E071500;//CK11
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4C081500;//CK9
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x5F091500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4A0A1500;//CK7
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x480B1500;//CK5
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x550C1500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x460D1500;//CK3
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x440E1500;//CK1
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x400F1500;//STV1_ODD
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55101500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55111500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55121500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55131500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55141500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55151500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x53161500;//RESET__EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55171500;//VSSG_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55181500;//VSSG_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x51191500;//STV2_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x771A1500;//VDD2_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x571B1500;//VDD1_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x551C1500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4F1D1500;//CK12
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4D1E1500;//CK10
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x5F1F1500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x4B201500;//CK8
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x49211500;//CK6
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55221500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x47231500;//CK4
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x45241500;//CK2
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x41251500;//STV1_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55261500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55271500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55281500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x55291500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x552A1500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x552B1500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x132C1500;//RESET_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x152D1500;//VSSG_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x152E1500;//VSSG_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x012F1500;//STV2_ODD
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x37301500;//VDD2_ODD
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x17311500;//VDD1_ODD
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x15321500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x0D331500;//CK11
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x0F341500;//CK9
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x15351500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x05361500;//CK7
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x07371500;//CK5
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x15381500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x09391500;//CK3
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x0B3A1500;//CK1
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x113B1500;//STV1_ODD
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x153C1500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x153D1500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x153E1500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x153F1500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x15401500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x15411500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x12421500;//RESET__EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x15431500;//VSSG_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x15441500;//VSSG_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x00451500;//STV2_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x37461500;//VDD2_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x17471500;//VDD1_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x15481500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x0C491500;//CK12
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x0E4A1500;//CK10
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x154B1500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x044C1500;//CK8
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x064D1500;//CK6
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x154E1500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x084F1500;//CK4
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x0A501500;//CK2
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x10511500;//STV1_EVEN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x15521500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x15531500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x15541500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x15551500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x15561500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x15571500;//x
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x40581500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x105B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x065C1500;//STV_S0
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x405D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x005E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x005F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x40601500;//ETV_W
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x03611500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x04621500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x6C631500;//CKV_ON
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x6C641500;//CKV_OFF
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x75651500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x08661500;//ETV_S0
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xB4671500; //ckv_num/ckv_w
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x08681500; //CKV_S0
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x6C691500;//CKV_ON
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x6C6A1500;//CKV_OFF
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x0C6B1500; //dummy
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x006D1500;//GGND1
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x006E1500;//GGND2
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x886F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0xBB751500;//FLM_EN
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x00761500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x05771500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x2A781500;//FLM_OFF
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x04E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x11091500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x480E1500; //Source EQ option
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x032D1500;//defult 0x01
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);



data_array[0] = 0x00E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x02E61500;//WD_Timer
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
data_array[0] = 0x06E71500;//WD_Timer
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

data_array[0] = 0x00E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);


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

    params->dsi.mode    = SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;//LCM_THREE_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 


	params->dsi.vertical_sync_active 			    = 4;
    params->dsi.vertical_backporch				    = 8;
    params->dsi.vertical_frontporch 				= 30;
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active			    = 20;
    params->dsi.horizontal_backporch				= 20;
    params->dsi.horizontal_frontporch				= 20;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 200;
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
	init_lcm_registers();
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

struct LCM_DRIVER gs716_sat_jd9366_boe_wxga_ips_101_lcm_drv = {
	.name = "gs716_sat_jd9366_boe_wxga_ips_101",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
