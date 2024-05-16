

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

#define FRAME_WIDTH  										(1200)
#define FRAME_HEIGHT 										(1920)

#define REGFLAG_DELAY             							0xFE
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util;

//#define GPIO_LCM_PWREN                                      (GPIO122 |0x80000000)
//#define GPIO_LCM_EN                                         (GPIO26 | 0x80000000)
//#define GPIO_LCM_RST                                        (GPIO83 | 0x80000000)
extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_BL_EN;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))
#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0

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


struct LCM_setting_table {
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = 
{

	{0xB0, 1, {0x05}},
	{0xB1, 1, {0xE5}},
	{0xB3, 1, {0x52}},
	{0xC0, 1, {0x05}},
	{0xD9, 1, {0x85}},
	{0xC2, 1, {0x55}},

	{0xB0, 1, {0x00}},
	{0xB3, 1, {0x88}},
	{0xB6, 1, {0x0B}},
	{0xBA, 1, {0x8B}},
	{0xBF, 1, {0x1A}},
	{0xC0, 1, {0x0F}},
	{0xC2, 1, {0x0C}},
	{0xC3, 1, {0x02}},
	{0xC4, 1, {0x0C}},
	{0xC5, 1, {0x02}},

	{0xB0, 1, {0x01}},
	{0xE0, 1, {0x26}},
	{0xE1, 1, {0x26}},
	{0xDC, 1, {0x00}},
	{0xDD, 1, {0x00}},
	{0xCC, 1, {0x26}},
	{0xCD, 1, {0x26}},
	{0xC8, 1, {0x00}},
	{0xC9, 1, {0x00}},
	{0xD2, 1, {0x03}},
	{0xD3, 1, {0x03}},
	{0xE6, 1, {0x04}},
	{0xE7, 1, {0x04}},
	{0xC4, 1, {0x09}},
	{0xC5, 1, {0x09}},
	{0xD8, 1, {0x0A}},
	{0xD9, 1, {0x0A}},
	{0xC2, 1, {0x0B}},
	{0xC3, 1, {0x0B}},
	{0xD6, 1, {0x0C}},
	{0xD7, 1, {0x0C}},
	{0xC0, 1, {0x05}},
	{0xC1, 1, {0x05}},
	{0xD4, 1, {0x06}},
	{0xD5, 1, {0x06}},
	{0xCA, 1, {0x07}},
	{0xCB, 1, {0x07}},
	{0xDE, 1, {0x08}},
	{0xDF, 1, {0x08}},

	{0xB0, 1, {0x02}},
	{0xC0, 1, {0x00}},
	{0xC1, 1, {0x0F}},
	{0xC2, 1, {0x1A}},
	{0xC3, 1, {0x2B}},
	{0xC4, 1, {0x38}},
	{0xC5, 1, {0x39}},
	{0xC6, 1, {0x38}},
	{0xC7, 1, {0x38}},
	{0xC8, 1, {0x36}},
	{0xC9, 1, {0x34}},
	{0xCA, 1, {0x35}},
	{0xCB, 1, {0x36}},
	{0xCC, 1, {0x39}},
	{0xCD, 1, {0x2D}},
	{0xCE, 1, {0x2D}},
	{0xCF, 1, {0x2C}},
	{0xD0, 1, {0x07}},
	{0xD2, 1, {0x00}},
	{0xD3, 1, {0x0F}},
	{0xD4, 1, {0x1A}},
	{0xD5, 1, {0x2B}},
	{0xD6, 1, {0x38}},
	{0xD7, 1, {0x39}},
	{0xD8, 1, {0x38}},
	{0xD9, 1, {0x38}},
	{0xDA, 1, {0x36}},
	{0xDB, 1, {0x34}},
	{0xDC, 1, {0x35}},
	{0xDD, 1, {0x36}},
	{0xDE, 1, {0x39}},
	{0xDF, 1, {0x2D}},
	{0xE0, 1, {0x2D}},
	{0xE1, 1, {0x2C}},
	{0xE2, 1, {0x07}},

	{0xB0, 1, {0x03}},
	{0xC8, 1, {0x0B}},
	{0xC9, 1, {0x07}},
	{0xC3, 1, {0x00}},
	{0xE7, 1, {0x00}},
	{0xC5, 1, {0x2A}},
	{0xDE, 1, {0x2A}},
	{0xCA, 1, {0x43}},
	{0xC9, 1, {0x07}},
	{0xE4, 1, {0xC0}},
	{0xE5, 1, {0x0D}},
	{0xCB, 1, {0x00}},

	{0xB0, 1, {0x06}},
	{0xB8, 1, {0xA5}},
	{0xC0, 1, {0xA5}},
	{0xC7, 1, {0x0F}},
	{0xD5, 1, {0x32}},
	{0xB8, 1, {0x00}},
	{0xC0, 1, {0x00}},
	{0xBC, 1, {0x00}},

	{0xB0, 1, {0x07}},
	{0xB1, 1, {0x08}},
	{0xB2, 1, {0x09}},
	{0xB3, 1, {0x13}},
	{0xB4, 1, {0x23}},
	{0xB5, 1, {0x37}},
	{0xB6, 1, {0x4B}},
	{0xB7, 1, {0x6D}},
	{0xB8, 1, {0x9C}},
	{0xB9, 1, {0xD8}},
	{0xBA, 1, {0x17}},
	{0xBB, 1, {0x93}},
	{0xBC, 1, {0x1B}},
	{0xBD, 1, {0x1F}},
	{0xBE, 1, {0x9B}},
	{0xBF, 1, {0x19}},
	{0xC0, 1, {0x57}},
	{0xC1, 1, {0x93}},
	{0xC2, 1, {0xAE}},
	{0xC3, 1, {0xCA}},
	{0xC4, 1, {0xD7}},
	{0xC5, 1, {0xE5}},
	{0xC6, 1, {0xF3}},
	{0xC7, 1, {0xF9}},
	{0xC8, 1, {0xFC}},
	{0xC9, 1, {0x00}},
	{0xCA, 1, {0x00}},
	{0xCB, 1, {0x16}},
	{0xCC, 1, {0xAF}},
	{0xCD, 1, {0xFF}},
	{0xCE, 1, {0xFF}},

	{0xB0, 1, {0x08}},
	{0xB1, 1, {0x04}},
	{0xB2, 1, {0x04}},
	{0xB3, 1, {0x10}},
	{0xB4, 1, {0x23}},
	{0xB5, 1, {0x37}},
	{0xB6, 1, {0x4B}},
	{0xB7, 1, {0x6D}},
	{0xB8, 1, {0x9C}},
	{0xB9, 1, {0xD8}},
	{0xBA, 1, {0x17}},
	{0xBB, 1, {0x93}},
	{0xBC, 1, {0x1C}},
	{0xBD, 1, {0x20}},
	{0xBE, 1, {0x9D}},
	{0xBF, 1, {0x1C}},
	{0xC0, 1, {0x5B}},
	{0xC1, 1, {0x96}},
	{0xC2, 1, {0xB1}},
	{0xC3, 1, {0xCD}},
	{0xC4, 1, {0xDA}},
	{0xC5, 1, {0xE7}},
	{0xC6, 1, {0xF4}},
	{0xC7, 1, {0xFA}},
	{0xC8, 1, {0xFC}},
	{0xC9, 1, {0x00}},
	{0xCA, 1, {0x00}},
	{0xCB, 1, {0x16}},
	{0xCC, 1, {0xAF}},
	{0xCD, 1, {0xFF}},
	{0xCE, 1, {0xFF}},

	{0xB0, 1, {0x09}},
	{0xB1, 1, {0x04}},
	{0xB2, 1, {0x04}},
	{0xB3, 1, {0x0D}},
	{0xB4, 1, {0x22}},
	{0xB5, 1, {0x37}},
	{0xB6, 1, {0x4C}},
	{0xB7, 1, {0x6E}},
	{0xB8, 1, {0x9D}},
	{0xB9, 1, {0xDA}},
	{0xBA, 1, {0x19}},
	{0xBB, 1, {0x9A}},
	{0xBC, 1, {0x25}},
	{0xBD, 1, {0x29}},
	{0xBE, 1, {0xA7}},
	{0xBF, 1, {0x26}},
	{0xC0, 1, {0x63}},
	{0xC1, 1, {0x9C}},
	{0xC2, 1, {0xB6}},
	{0xC3, 1, {0xD1}},
	{0xC4, 1, {0xDC}},
	{0xC5, 1, {0xE9}},
	{0xC6, 1, {0xF5}},
	{0xC7, 1, {0xFA}},
	{0xC8, 1, {0xFC}},
	{0xC9, 1, {0x00}},
	{0xCA, 1, {0x00}},
	{0xCB, 1, {0x16}},
	{0xCC, 1, {0xAF}},
	{0xCD, 1, {0xFF}},
	{0xCE, 1, {0xFF}},

	{0xB0, 1, {0x0A}},
	{0xB1, 1, {0x08}},
	{0xB2, 1, {0x09}},
	{0xB3, 1, {0x13}},
	{0xB4, 1, {0x23}},
	{0xB5, 1, {0x37}},
	{0xB6, 1, {0x4B}},
	{0xB7, 1, {0x6D}},
	{0xB8, 1, {0x9C}},
	{0xB9, 1, {0xD8}},
	{0xBA, 1, {0x17}},
	{0xBB, 1, {0x93}},
	{0xBC, 1, {0x1B}},
	{0xBD, 1, {0x1F}},
	{0xBE, 1, {0x9B}},
	{0xBF, 1, {0x19}},
	{0xC0, 1, {0x57}},
	{0xC1, 1, {0x93}},
	{0xC2, 1, {0xAE}},
	{0xC3, 1, {0xCA}},
	{0xC4, 1, {0xD7}},
	{0xC5, 1, {0xE5}},
	{0xC6, 1, {0xF3}},
	{0xC7, 1, {0xF9}},
	{0xC8, 1, {0xFC}},
	{0xC9, 1, {0x00}},
	{0xCA, 1, {0x00}},
	{0xCB, 1, {0x16}},
	{0xCC, 1, {0xAF}},
	{0xCD, 1, {0xFF}},
	{0xCE, 1, {0xFF}},

	{0xB0, 1, {0x0B}},
	{0xB1, 1, {0x04}},
	{0xB2, 1, {0x04}},
	{0xB3, 1, {0x10}},
	{0xB4, 1, {0x23}},
	{0xB5, 1, {0x37}},
	{0xB6, 1, {0x4B}},
	{0xB7, 1, {0x6D}},
	{0xB8, 1, {0x9C}},
	{0xB9, 1, {0xD8}},
	{0xBA, 1, {0x17}},
	{0xBB, 1, {0x93}},
	{0xBC, 1, {0x1C}},
	{0xBD, 1, {0x20}},
	{0xBE, 1, {0x9D}},
	{0xBF, 1, {0x1C}},
	{0xC0, 1, {0x5B}},
	{0xC1, 1, {0x96}},
	{0xC2, 1, {0xB1}},
	{0xC3, 1, {0xCD}},
	{0xC4, 1, {0xDA}},
	{0xC5, 1, {0xE7}},
	{0xC6, 1, {0xF4}},
	{0xC7, 1, {0xFA}},
	{0xC8, 1, {0xFC}},
	{0xC9, 1, {0x00}},
	{0xCA, 1, {0x00}},
	{0xCB, 1, {0x16}},
	{0xCC, 1, {0xAF}},
	{0xCD, 1, {0xFF}},
	{0xCE, 1, {0xFF}},
	
	{0xB0, 1, {0x0C}},
	{0xB1, 1, {0x04}},
	{0xB2, 1, {0x04}},
	{0xB3, 1, {0x0D}},
	{0xB4, 1, {0x22}},
	{0xB5, 1, {0x37}},
	{0xB6, 1, {0x4C}},
	{0xB7, 1, {0x6E}},
	{0xB8, 1, {0x9D}},
	{0xB9, 1, {0xDA}},
	{0xBA, 1, {0x19}},
	{0xBB, 1, {0x9A}},
	{0xBC, 1, {0x25}},
	{0xBD, 1, {0x29}},
	{0xBE, 1, {0xA7}},
	{0xBF, 1, {0x26}},
	{0xC0, 1, {0x63}},
	{0xC1, 1, {0x9C}},
	{0xC2, 1, {0xB6}},
	{0xC3, 1, {0xD1}},
	{0xC4, 1, {0xDC}},
	{0xC5, 1, {0xE9}},
	{0xC6, 1, {0xF5}},
	{0xC7, 1, {0xFA}},
	{0xC8, 1, {0xFC}},
	{0xC9, 1, {0x00}},
	{0xCA, 1, {0x00}},
	{0xCB, 1, {0x16}},
	{0xCC, 1, {0xAF}},
	{0xCD, 1, {0xFF}},
	{0xCE, 1, {0xFF}},
	{0xB0, 1, {0x00}},
	{0xB3, 1, {0x08}},

	{0xB0, 1, {0x04}},
	{0xB5, 1, {0x02}},
	{0xB6, 1, {0x01}},
	{0xB8, 1, {0xFF}},
	
	{0x11,0,{}},
	{REGFLAG_DELAY, 120, {}},
	//sleep out
	{0x29, 0, {}},
	{REGFLAG_DELAY, 20, {}},
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {

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
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	   memset(params, 0, sizeof(struct LCM_PARAMS));
	
		params->type = LCM_TYPE_DSI;
		params->width = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;

		params->dsi.mode = BURST_VDO_MODE;
	
		/* DSI */
		/* Command mode setting */
		params->dsi.LANE_NUM = LCM_FOUR_LANE;
		/* The following defined the fomat for data coming from LCD engine. */
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	
		/* Highly depends on LCD driver capability. */
		/* Not support in MT6573 */
		params->dsi.packet_size = 256;
		params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
		params->dsi.vertical_sync_active				= 4;
		params->dsi.vertical_backporch				= 10;
		params->dsi.vertical_frontporch 				= 14;
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 
	
		params->dsi.horizontal_sync_active			= 24;	
		params->dsi.horizontal_backporch				= 80;//62;	
		params->dsi.horizontal_frontporch			= 60;//80;
		params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;

		params->dsi.PLL_CLOCK = 468;//400
}

static void lcm_init(void)
{

	avdd_enable(1);
	lcd_power_en(1);
	MDELAY(100);
	lcd_reset(1);
	MDELAY(5);
	lcd_reset(0);	
	MDELAY(20);
	lcd_reset(1);
	MDELAY(50);//Must > 5ms
	//init_lcm_registers();
	MDELAY(180);
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	lcd_power_en(0);
    MDELAY(150);
    lcd_reset(0);
    MDELAY(100);
    avdd_enable(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
  lcm_init();
}


struct LCM_DRIVER us828_hsx_hx8279d_101bf40a_boe_wuxga_101_lcm_drv = 
{
    .name			= "us828_hsx_hx8279d_101bf40a_boe_wuxga_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
};

