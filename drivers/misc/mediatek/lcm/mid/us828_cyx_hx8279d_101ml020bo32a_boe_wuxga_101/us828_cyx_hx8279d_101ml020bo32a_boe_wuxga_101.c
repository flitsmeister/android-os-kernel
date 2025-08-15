

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

	{REGFLAG_DELAY, 20, {} },
	{0xB0,  1,  {0x00} },
	{0xB2,  1,  {0x00} },
	{0xB0,  1,  {0x00} },
	//{0xB6,  1,  {0x03} },
	{0xBA,  1,  {0x8F} },
	//{0xBD,  1,  {0x73} },
	{0xBD,  1,  {0x62} },
	{0xBF,  1,  {0x15} },
	{0xC0,  1,  {0x0F} },
	{0xC2,  1,  {0x0C} },
	{0xC3,  1,  {0x02} },
	{0xC4,  1,  {0x0C} },
	{0xC5,  1,  {0x02} },

	{0xB0,  1,  {0x01} },
	{0xE0,  1,  {0x0F} },
	{0xE1,  1,  {0x0F} },//
	{0xDC,  1,  {0x10} },
	{0xDD,  1,  {0x10} },
	{0xCC,  1,  {0x0F} },
	{0xCD,  1,  {0x0F} },
	{0xC8,  1,  {0x10} },
	{0xC9,  1,  {0x10} },

	{0xB0,  1,  {0x03} },
	{0xC5,  1,  {0x2C} },
	{0xDE,  1,  {0x2C} },
	{0xCA,  1,  {0x42} },
	{0xD6,  1,  {0x88} },
	{0xD7,  1,  {0x08} },
	{0xD2,  1,  {0x06} },
	{0xD3,  1,  {0x06} },

	{0xB0,  1,  {0x05} },
	{0xB3,  1,  {0x52} },

	{0xB0,  1,  {0x06} },
	{0xB8,  1,  {0xA5} },
	{0xC0,  1,  {0xA5} },
	{0xC7,  1,  {0x0F} },
	{0xD5,  1,  {0x32} },

	{0xB0,  1,  {0x02} },
	{0xC0,  1,  {0x00} },
	{0xC1,  1,  {0x14} },
	{0xC2,  1,  {0x16} },
	{0xC3,  1,  {0x24} },
	{0xC4,  1,  {0x25} },
	{0xC5,  1,  {0x31} },
	{0xC6,  1,  {0x2A} },
	{0xC7,  1,  {0x2D} },
	{0xC8,  1,  {0x33} },
	{0xC9,  1,  {0x37} },
	{0xCA,  1,  {0x3F} },
	{0xCB,  1,  {0x3F} },
	{0xCC,  1,  {0x3F} },
	{0xCD,  1,  {0x2D} },
	{0xCE,  1,  {0x2E} },
	{0xCF,  1,  {0x29} },
	{0xD0,  1,  {0x04} },
	{0xD2,  1,  {0x00} },
	{0xD3,  1,  {0x14} },
	{0xD4,  1,  {0x14} },
	{0xD5,  1,  {0x26} },
	{0xD6,  1,  {0x2D} },
	{0xD7,  1,  {0x31} },
	{0xD8,  1,  {0x38} },
	{0xD9,  1,  {0x3D} },
	{0xDA,  1,  {0x3F} },
	{0xDB,  1,  {0x3F} },
	{0xDC,  1,  {0x3F} },
	{0xDD,  1,  {0x3F} },
	{0xDE,  1,  {0x3F} },
	{0xDF,  1,  {0x38} },
	{0xE0,  1,  {0x35} },
	{0xE1,  1,  {0x2D} },
	{0xE2,  1,  {0x0A} },

	{0xB0,  1,  {0x06} },
	{0xB8,  1,  {0xA5} },
	{0xBC,  1,  {0x33} },

	{0xB0,  1,  {0x00} },
	{0xB2,  1,  {0x40} },
	
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
		params->dsi.vertical_backporch				= 16;
		params->dsi.vertical_frontporch 				= 16;
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 
	
		params->dsi.horizontal_sync_active			= 1;	
		params->dsi.horizontal_backporch				= 80;//62;	
		params->dsi.horizontal_frontporch			= 60;//80;
		params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;

		params->dsi.PLL_CLOCK = 468;//400
}

static void lcm_init(void)
{

	avdd_enable(1);
	lcd_power_en(1);
	MDELAY(20);
	lcd_reset(1);
	MDELAY(5);
	lcd_reset(0);	
	MDELAY(50);
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


struct LCM_DRIVER us828_cyx_hx8279d_101ml020bo32a_boe_wuxga_101_lcm_drv = 
{
    .name			= "us828_cyx_hx8279d_101ml020bo32a_boe_wuxga_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
};

