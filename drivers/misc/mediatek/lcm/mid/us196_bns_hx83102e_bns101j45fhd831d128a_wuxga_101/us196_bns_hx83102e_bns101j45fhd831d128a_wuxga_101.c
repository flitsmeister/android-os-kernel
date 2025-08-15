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

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(1200)
#define FRAME_HEIGHT 										(1920)


#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util;

extern unsigned int GPIO_LCM_PWR_EN;//1.8
extern unsigned int GPIO_LCM_RST;

extern unsigned int GPIO_LCM_3V3_EN;//3.3
extern unsigned int GPIO_LCM_BIAS_EN;
//extern unsigned int GPIO_CTP_TP_RST_EN;//hx83102 tp rst
extern int _lcm_i2c_write_bytes(unsigned char addr, unsigned char value);
extern void tpd_gpio_output(int pin, int level);

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))
#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0

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

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {
	{0xB9, 3, {0x83,0x10,0x2E}},
	{0x51, 2, {0x0F,0xFF}},
	{0x53, 1, {0x2C}},
	{0xC9, 3, {0x04,0x0B,0x5D}},
	{0xD1, 4, {0x67,0x0C,0xFF,0x05}},
	{0xD2, 2, {0x2D,0x2D}},
	{0xB1, 17, {0x10,0xFA,0xAF,0xAF,0x2D,0x2D,0xC1,0x6B,0x39,0x36,0x36,0x36,0x36,0x22,0x21,0x15,0x00}},
	{0xB2, 16, {0x00,0xB0,0x47,0x80,0x00,0x26,0x56,0x2C,0x00,0x00,0x00,0x00,0x15,0x20,0xD7,0x00}},
	{0xB4, 16, {0x08,0x74,0x7B,0x74,0x78,0x74,0x68,0x50,0x01,0xA0,0x01,0x58,0x00,0xFF,0x00,0xFF}},
	{0xBB, 13, {0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x05,0x00,0x04,0xAB,0x01}},
	{0xBF, 3, {0xFC,0x85,0x80}},
	{0xD3, 43, {0x00,0x00,0x00,0x00,0x01,0x04,0x00,0x00,0x0C,0x37,0x27,0x63,0x4F,0x26,0x27,0x04,0x04,0x32,0x10,0x23,0x00,0x23,0x32,0x17,0xA8,0x00,0x02,0x32,0x17,0xA8,0x07,0xA8,0x00,0x00,0x21,0x38,0x01,0x55,0x21,0x38,0x01,0x55,0x0F}},

	{0xE0, 46, {0x00,0x04,0x0B,0x11,0x17,0x23,0x38,0x3E,0x42,0x3E,0x57,0x5B,0x5F,0x6C,0x69,0x71,0x7C,0x91,0x93,0x4B,0x54,0x61,0x73,0x00,0x04,0x0B,0x11,0x17,0x23,0x38,0x3E,0x42,0x3E,0x57,0x5B,0x5F,0x6C,0x69,0x71,0x7C,0x91,0x93,0x4B,0x54,0x61,0x73}},
	
	{0xBD, 1, {0x01}},
	{0xB1, 4, {0x01,0x9B,0x01,0x31}},
	{0xD3, 11, {0x01,0x00,0xF8,0x00,0x00,0x11,0x10,0x00,0x0A,0x00,0x01}},
	{0xBD, 1, {0x02}},
	{0xB4, 6, {0x4E,0x00,0x33,0x11,0x33,0x88}},
	{0xBF, 3, {0xF2,0x00,0x02}},
	{0xBD, 1, {0x00}},
	{0xC0, 14, {0x23,0x23,0x22,0x11,0xA2,0x10,0x00,0x80,0x00,0x00,0x08,0x00,0x63,0x63}},
	{0xC6, 1, {0xF9}},
	{0xC7, 1, {0x30}},
	{0xC8, 8, {0x00,0x04,0x04,0x00,0x00,0x02,0x13,0xFF}},
	{0xD0, 3, {0x07,0x04,0x05}},

	{0xD5, 44, {0x19,0x19,0x19,0x19,0x28,0x29,0x28,0x29,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x1A,0x1A,0x1A,0x1A,0x1B,0x1B,0x1B,0x1B,0x06,0x07,0x06,0x07,0x04,0x05,0x04,0x05,0x02,0x03,0x02,0x03,0x00,0x01,0x00,0x01,0x20,0x21,0x20,0x21}},
	{0xD6, 44, {0x18,0x18,0x18,0x18,0x21,0x20,0x21,0x20,0x19,0x19,0x19,0x19,0x18,0x18,0x18,0x18,0x1A,0x1A,0x1A,0x1A,0x1B,0x1B,0x1B,0x1B,0x01,0x00,0x01,0x00,0x03,0x02,0x03,0x02,0x05,0x04,0x05,0x04,0x07,0x06,0x07,0x06,0x29,0x28,0x29,0x28}},
	{0xE7, 44, {0x12,0x13,0x02,0x02,0x35,0x00,0x0E,0x0E,0x00,0x0C,0x27,0x77,0x3B,0x78,0x01,0x01,0x00,0x00,0x00,0x00,0x17,0x00,0x68}},
	{0xBD, 1, {0x01}},
	{0xE7, 7, {0x01,0x0C,0x01,0x8E,0x0D,0xD8,0x0E}},
	{0xBD, 1, {0x02}},
	{0xE7,49, {0xFF,0x01,0xFD,0x01,0x00,0x00,0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x81,0x00,0x02,0x40}},
	{0xBD, 1, {0x00}},
	{0xC2, 3, {0x43,0xFF,0x10}},

	{0xBD, 1, {0x02}},
	{0xD8,12, {0xFF,0xFF,0xFF,0xFF,0xFF,0xF0,0xFF,0xFF,0xFF,0xFF,0xFF,0xF0}},
	{0xBD, 1, {0x00}},
	{0xCC, 1, {0x02}},
	{0xE1, 2, {0x00,0x00}},
	{0xE2, 1, {0x02}},

	{0xB9, 3, {0x00,0x00,0x00}},
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
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	if (GPIO == 0xFFFFFFFF) {		
		return;
	}
	gpio_direction_output(GPIO, output);
	gpio_set_value(GPIO, (output > 0) ? GPIO_OUT_ONE : GPIO_OUT_ZERO);
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
	
	params->physical_width = 141;
	params->physical_height = 226;

    params->dsi.mode    = SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;//LCM_THREE_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;


	params->dsi.vertical_sync_active    = 8;
	params->dsi.vertical_backporch      = 32;
	params->dsi.vertical_frontporch     = 144;
	params->dsi.vertical_active_line    = FRAME_HEIGHT;
	
	params->dsi.horizontal_sync_active    = 14;
	params->dsi.horizontal_backporch      = 26;
	params->dsi.horizontal_frontporch     = 32;
	params->dsi.horizontal_active_pixel   = FRAME_WIDTH;
	params->dsi.ssc_disable=1;

	params->dsi.PLL_CLOCK = 512;
	
}

static void tp_reset(unsigned char enabled)
{
    if (enabled)
    {
        tpd_gpio_output(0, 1);
    }
    else
    {   
        tpd_gpio_output(0, 0);

    }
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

static void lcd_3v3_en(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_3V3_EN, 1);
    }
    else
    {   
        lcm_set_gpio_output(GPIO_LCM_3V3_EN, 0);

    }
}

static void lcd_bias_en(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_EN, GPIO_OUT_ZERO);
    }
}


static void lcm_init(void)
{
	unsigned int data_array[16];

	lcd_power_en(1);//1.8
	MDELAY(5);

	lcd_3v3_en(1);//3.3
	MDELAY(5);
	
	tp_reset(1);
	MDELAY(5);
	
	lcd_bias_en(1);
	_lcm_i2c_write_bytes(0x00,0x11);
	_lcm_i2c_write_bytes(0x01,0x11);
	MDELAY(10);

	lcd_reset(1);
	MDELAY(10);
	lcd_reset(0);	
	MDELAY(10);
	lcd_reset(1);
	MDELAY(50);//Must > 5ms
//	init_lcm_registers();
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

	data_array[0] = 0x00110500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
    data_array[0] = 0x00290500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(50);
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];
    data_array[0] = 0x00280500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(50);
    data_array[0] = 0x00100500;
    dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
	
	tp_reset(0);
	MDELAY(20);
    lcd_reset(0);
    MDELAY(10);

	lcd_bias_en(0);
	MDELAY(10);

	lcd_3v3_en(0);
	MDELAY(10);
	lcd_power_en(0);
}


static void lcm_resume(void)
{
  lcm_init();
}


struct LCM_DRIVER us196_bns_hx83102e_bns101j45fhd831d128a_wuxga_101_lcm_drv = 
{
    .name			= "us196_bns_hx83102e_bns101j45fhd831d128a_wuxga_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

