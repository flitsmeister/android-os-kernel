

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#else
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#endif
#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(800)
#define FRAME_HEIGHT 										(1280)


#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util;

extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_BL_EN;

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n) 											(lcm_util.udelay(n))
#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
/* #include <linux/delay.h> */
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#endif

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	if (GPIO == 0xFFFFFFFF) {		
		return;
	}
	gpio_direction_output(GPIO, output);
	gpio_set_value(GPIO, (output > 0) ? GPIO_OUT_ONE : GPIO_OUT_ZERO);
}

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

    params->dsi.mode    = BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 

    params->dsi.vertical_sync_active                            = 6; //4;
    params->dsi.vertical_backporch                              = 15; //16;
    params->dsi.vertical_frontporch                             = 16; 
    params->dsi.vertical_active_line                            = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active                          = 8; //5;//6;
    params->dsi.horizontal_backporch                            = 48; //60; //80;
    params->dsi.horizontal_frontporch                           = 52; 
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    //params->dsi.cont_clock    = 1;//1
    //params->dsi.ssc_disable = 0;//0
    params->dsi.PLL_CLOCK = 215;
	params->dsi.cont_clock = 1;
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
#define REGFLAG_DELAY           0XFE
	unsigned int i;
    unsigned int cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;
		switch (cmd) {
            // case REGFLAG_MDELAY:
                // MDELAY(table[i].count}},
                // break;
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
                break;
		}
    }
}

static __attribute__((unused)) struct LCM_setting_table init_setting[] = {
	{0xFF, 3, {0x98,0x81,0x03}},
	//GIP_1
	{0x01, 1, {0x00}},
	{0x02, 1, {0x00}},
	{0x03, 1, {0x57}},
	{0x04, 1, {0xD3}},
	{0x05, 1, {0x00}},
	{0x06, 1, {0x11}},
	{0x07, 1, {0x08}},
	{0x08, 1, {0x00}},
	{0x09, 1, {0x00}},
	{0x0a, 1, {0x3F}},
	{0x0b, 1, {0x00}},
	{0x0c, 1, {0x00}},
	{0x0d, 1, {0x00}},
	{0x0e, 1, {0x00}},
	{0x0f, 1, {0x3F}},
	{0x10, 1, {0x3F}},
	{0x11, 1, {0x00}},
	{0x12, 1, {0x00}},
	{0x13, 1, {0x00}},
	{0x14, 1, {0x00}},
	{0x15, 1, {0x00}},
	{0x16, 1, {0x00}},
	{0x17, 1, {0x00}},
	{0x18, 1, {0x00}},
	{0x19, 1, {0x00}},
	{0x1a, 1, {0x00}},
	{0x1b, 1, {0x00}},
	{0x1c, 1, {0x00}},
	{0x1d, 1, {0x00}},
	{0x1e, 1, {0x40}},
	{0x1f, 1, {0x80}},
	{0x20, 1, {0x06}},
	{0x21, 1, {0x01}},
	{0x22, 1, {0x00}},
	{0x23, 1, {0x00}},
	{0x24, 1, {0x00}},
	{0x25, 1, {0x00}},
	{0x26, 1, {0x00}},
	{0x27, 1, {0x00}},
	{0x28, 1, {0x33}},
	{0x29, 1, {0x33}},
	{0x2a, 1, {0x00}},
	{0x2b, 1, {0x00}},
	{0x2c, 1, {0x00}},
	{0x2d, 1, {0x00}},
	{0x2e, 1, {0x00}},
	{0x2f, 1, {0x00}},
	{0x30, 1, {0x00}},
	{0x31, 1, {0x00}},
	{0x32, 1, {0x00}},
	{0x33, 1, {0x00}},
	{0x34, 1, {0x00}},
	{0x35, 1, {0x00}},
	{0x36, 1, {0x00}},
	{0x37, 1, {0x00}},
	{0x38, 1, {0x78}},
	{0x39, 1, {0x00}},
	{0x3a, 1, {0x00}},
	{0x3b, 1, {0x00}},
	{0x3c, 1, {0x00}},
	{0x3d, 1, {0x00}},
	{0x3e, 1, {0x00}},
	{0x3f, 1, {0x00}},
	{0x40, 1, {0x00}},
	{0x41, 1, {0x00}},
	{0x42, 1, {0x00}},
	{0x43, 1, {0x00}},
	{0x44, 1, {0x00}},
	//GIP_2
	{0x50, 1, {0x00}},
	{0x51, 1, {0x23}},
	{0x52, 1, {0x45}},
	{0x53, 1, {0x67}},
	{0x54, 1, {0x89}},
	{0x55, 1, {0xab}},
	{0x56, 1, {0x01}},
	{0x57, 1, {0x23}},
	{0x58, 1, {0x45}},
	{0x59, 1, {0x67}},
	{0x5a, 1, {0x89}},
	{0x5b, 1, {0xab}},
	{0x5c, 1, {0xcd}},
	{0x5d, 1, {0xef}},
	//GIP_3
	{0x5e, 1, {0x00}},
	{0x5f, 1, {0x0D}},
	{0x60, 1, {0x0D}},
	{0x61, 1, {0x0C}},
	{0x62, 1, {0x0C}},
	{0x63, 1, {0x0F}},
	{0x64, 1, {0x0F}},
	{0x65, 1, {0x0E}},
	{0x66, 1, {0x0E}},
	{0x67, 1, {0x08}},
	{0x68, 1, {0x02}},
	{0x69, 1, {0x02}},
	{0x6a, 1, {0x02}},
	{0x6b, 1, {0x02}},
	{0x6c, 1, {0x02}},
	{0x6d, 1, {0x02}},
	{0x6e, 1, {0x02}},
	{0x6f, 1, {0x02}},
	{0x70, 1, {0x14}},
	{0x71, 1, {0x15}},
	{0x72, 1, {0x06}},
	{0x73, 1, {0x02}},
	{0x74, 1, {0x02}},
	{0x75, 1, {0x0D}},
	{0x76, 1, {0x0D}},
	{0x77, 1, {0x0C}},
	{0x78, 1, {0x0C}},
	{0x79, 1, {0x0F}},
	{0x7a, 1, {0x0F}},
	{0x7b, 1, {0x0E}},
	{0x7c, 1, {0x0E}},
	{0x7d, 1, {0x08}},
	{0x7e, 1, {0x02}},
	{0x7f, 1, {0x02}},
	{0x80, 1, {0x02}},
	{0x81, 1, {0x02}},
	{0x82, 1, {0x02}},
	{0x83, 1, {0x02}},
	{0x84, 1, {0x02}},
	{0x85, 1, {0x02}},
	{0x86, 1, {0x14}},
	{0x87, 1, {0x15}},
	{0x88, 1, {0x06}},
	{0x89, 1, {0x02}},
	{0x8A, 1, {0x02}},
	//CMD_Page 4
	{0xFF, 3, {0x98,0x81,0x04}},
	{0x6E, 1, {0x3B}},
	{0x6F, 1, {0x57}},
	{0x3A, 1, {0x24}},
	{0x8D, 1, {0x1F}},
	{0x87, 1, {0xBA}},
	{0xB2, 1, {0xD1}},
	{0x88, 1, {0x0B}},
	{0x38, 1, {0x01}},
	{0x39, 1, {0x00}},
	{0xB5, 1, {0x07}},
	{0x31, 1, {0x75}},
	{0x3B, 1, {0x98}},
	//CMD_Page 1
	{0xFF, 3, {0x98,0x81,0x01}},
	{0x22, 1, {0x0A}},
	{0x31, 1, {0x09}},
	{0x35, 1, {0x07}},
	{0x53, 1, {0x84}},
	{0x55, 1, {0x84}},
	{0x50, 1, {0x86}},
	{0x51, 1, {0x82}},
	{0x60, 1, {0x10}},
	{0x62, 1, {0x00}},
	//============Gamma START=============
	{0xA0, 1, {0x00}},
	{0xA1, 1, {0x12}},
	{0xA2, 1, {0x1F}},
	{0xA3, 1, {0x12}},
	{0xA4, 1, {0x16}},
	{0xA5, 1, {0x29}},
	{0xA6, 1, {0x1E}},
	{0xA7, 1, {0x1F}},
	{0xA8, 1, {0x7E}},
	{0xA9, 1, {0x1B}},
	{0xAA, 1, {0x28}},
	{0xAB, 1, {0x6D}},
	{0xAC, 1, {0x19}},
	{0xAD, 1, {0x18}},
	{0xAE, 1, {0x4C}},
	{0xAF, 1, {0x1E}},
	{0xB0, 1, {0x23}},
	{0xB1, 1, {0x52}},
	{0xB2, 1, {0x6D}},
	{0xB3, 1, {0x3F}},
	//Neg Register
	{0xC0, 1, {0x00}},
	{0xC1, 1, {0x12}},
	{0xC2, 1, {0x20}},
	{0xC3, 1, {0x10}},
	{0xC4, 1, {0x13}},
	{0xC5, 1, {0x27}},
	{0xC6, 1, {0x1B}},
	{0xC7, 1, {0x1D}},
	{0xC8, 1, {0x75}},
	{0xC9, 1, {0x1F}},
	{0xCA, 1, {0x28}},
	{0xCB, 1, {0x68}},
	{0xCC, 1, {0x1A}},
	{0xCD, 1, {0x18}},
	{0xCE, 1, {0x4D}},
	{0xCF, 1, {0x25}},
	{0xD0, 1, {0x2E}},
	{0xD1, 1, {0x53}},
	{0xD2, 1, {0x60}},
	{0xD3, 1, {0x3F}},
	//============ Gamma END===========
	//CMD_Page 0
	{0xFF,3,{0x98,0x81,0x00}},
	{0x35,1,{0x00}},
};

static void init_lcm_registers(void)
{
    unsigned int data_array[16];
	
	push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
	
    data_array[0] = 0x00110500;  	// SLPOUT
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);

    data_array[0] = 0x00290500;  	// DSPON
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(5);
}

static void lcm_init(void)
{
    lcd_power_en(1);

    MDELAY(10);
    avdd_enable(1);
    MDELAY(20);

    lcd_reset(1);
    MDELAY(20);
    lcd_reset(0);
    MDELAY(20);
    lcd_reset(1);
	MDELAY(50);//Must > 5ms
	init_lcm_registers();
	MDELAY(180);  
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

struct LCM_DRIVER us717_bns_ili9881c_boe_wxga_ips_8_lcm_drv = 
{
    .name			= "us717_bns_ili9881c_boe_wxga_ips_8",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

