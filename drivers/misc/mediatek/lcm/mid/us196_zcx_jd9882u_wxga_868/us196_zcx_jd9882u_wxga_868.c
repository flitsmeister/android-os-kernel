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

#define FRAME_WIDTH  										(800)
#define FRAME_HEIGHT 										(1340)


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
  {0xFF,3,{0x98,0x82,0x01}},
  {0x00,1,{0x45}},
  {0x01,1,{0x16}},
  {0x02,1,{0x00}},
  {0x03,1,{0x00}},
  {0x04,1,{0x01}},
  {0x05,1,{0x13}},
  {0x06,1,{0x00}},
  {0x07,1,{0x00}},
  {0x08,1,{0x80}},
  {0x09,1,{0x81}},
  {0x0a,1,{0x71}},
  {0x0b,1,{0x00}},
  {0x0c,1,{0x00}},
  {0x0d,1,{0x00}},
  {0x0e,1,{0x00}},
  {0x0f,1,{0x00}},
  {0x2c,1,{0x31}},
  {0x2d,1,{0x43}},
  {0x31,1,{0x00}},
  {0x32,1,{0x00}},
  {0x33,1,{0x01}},
  {0x34,1,{0x01}},
  {0x35,1,{0x0D}},
  {0x36,1,{0x0D}},
  {0x37,1,{0x02}},
  {0x38,1,{0x02}},
  {0x39,1,{0x28}},
  {0x3A,1,{0x28}},
  {0x3B,1,{0x29}},
  {0x3C,1,{0x29}},
  {0x3D,1,{0x17}},
  {0x3E,1,{0x17}},
  {0x3F,1,{0x15}},
  {0x40,1,{0x15}},
  {0x41,1,{0x13}},
  {0x42,1,{0x13}},
  {0x43,1,{0x11}},
  {0x44,1,{0x11}},
  {0x45,1,{0x09}},
  {0x46,1,{0x09}},
  {0x47,1,{0x00}},
  {0x48,1,{0x00}},
  {0x49,1,{0x01}},
  {0x4A,1,{0x01}},
  {0x4B,1,{0x0C}},
  {0x4C,1,{0x0C}},
  {0x4D,1,{0x02}},
  {0x4E,1,{0x02}},
  {0x4F,1,{0x28}},
  {0x50,1,{0x28}},
  {0x51,1,{0x29}},
  {0x52,1,{0x29}},
  {0x53,1,{0x16}},
  {0x54,1,{0x16}},
  {0x55,1,{0x14}},
  {0x56,1,{0x14}},
  {0x57,1,{0x12}},
  {0x58,1,{0x12}},
  {0x59,1,{0x10}},
  {0x5A,1,{0x10}},
  {0x5B,1,{0x08}},
  {0x5C,1,{0x08}},
  {0x61,1,{0x00}},
  {0x62,1,{0x00}},
  {0x63,1,{0x01}},
  {0x64,1,{0x01}},
  {0x65,1,{0x08}},
  {0x66,1,{0x08}},
  {0x67,1,{0x02}},
  {0x68,1,{0x02}},
  {0x69,1,{0x28}},
  {0x6A,1,{0x28}},
  {0x6B,1,{0x29}},
  {0x6C,1,{0x29}},
  {0x6D,1,{0x14}},
  {0x6E,1,{0x14}},
  {0x6F,1,{0x16}},
  {0x70,1,{0x16}},
  {0x71,1,{0x10}},
  {0x72,1,{0x10}},
  {0x73,1,{0x12}},
  {0x74,1,{0x12}},
  {0x75,1,{0x0C}},
  {0x76,1,{0x0C}},
  {0x77,1,{0x00}},
  {0x78,1,{0x00}},
  {0x79,1,{0x01}},
  {0x7A,1,{0x01}},
  {0x7B,1,{0x09}},
  {0x7C,1,{0x09}},
  {0x7D,1,{0x02}},
  {0x7E,1,{0x02}},
  {0x7F,1,{0x28}},
  {0x80,1,{0x28}},
  {0x81,1,{0x29}},
  {0x82,1,{0x29}},
  {0x83,1,{0x15}},
  {0x84,1,{0x15}},
  {0x85,1,{0x17}},
  {0x86,1,{0x17}},
  {0x87,1,{0x11}},
  {0x88,1,{0x11}},
  {0x89,1,{0x13}},
  {0x8A,1,{0x13}},
  {0x8B,1,{0x0D}},
  {0x8C,1,{0x0D}},
  {0xFF, 3,{0x98,0x82,0x05}},
  {0x03,1,{0x00}},
  {0x04,1,{0x0F}},
  {0x69,1,{0x97}},
  {0x6A,1,{0x97}},
  {0x6D,1,{0xC9}},
  {0x73,1,{0xCF}},
  {0x79,1,{0xA1}},
  {0x7F,1,{0x93}},
  {0xFF,3,{0x98,0x82,0x06}},
  {0xD9,1,{0x1F}},
  {0xC0,1,{0x3C}},
  {0xC1,1,{0x05}},
  {0xCA,1,{0x20}},
  {0xCB,1,{0x03}},
  {0xC3,1,{0x0E}},
  {0xFF,3,{0x98,0x82,0x08}},
  {0xE0,27,{0x40,0x13,0x8C,0xC4,0x06,0x55,0x38,0x5E,0x8A,0xAC,0xA9,0xE1,0x0A,0x2E,0x52,0xAA,0x77,0xA7,0xC5,0xEB,0xFF,0x0D,0x3A,0x6F,0x9B,0x03,0xD7}},
  {0xE1,27,{0x40,0x13,0x8C,0xC4,0x06,0x55,0x38,0x5E,0x8A,0xAC,0xA9,0xE1,0x0A,0x2E,0x52,0xAA,0x77,0xA7,0xC5,0xEB,0xFF,0x0D,0x3A,0x6F,0x9B,0x03,0xD7}},
  {0xFF,3,{0x98,0x82,0x02}},
  {0x1B,1,{0x00}},
  {0x07,1,{0xA8}},
  {0x08,1,{0x00}},
  {0x09,1,{0x1C}},
  {0x0A,1,{0xC8}},
  {0x39,1,{0x01}},
  {0x3A,1,{0x1C}},
  {0x3B,1,{0xC8}},
  {0x3C,1,{0xAA}},
  {0x40,1,{0x49}},
  {0xFF,3,{0x98,0x82,0x0B}},  
  {0x9A,1,{0x85}},
  {0xAA,1,{0x12}},
  {0x9B,1,{0x4F}},
  {0x9C,1,{0x04}},
  {0x9D,1,{0x04}},
  {0x9E,1,{0x85}},
  {0x9F,1,{0x85}},
  {0xAB,1,{0xE0}},
  {0xFF,3,{0x98,0x82,0x0E}},
  {0x11,1,{0x4D}},
  {0x13,1,{0x10}},
  {0x00,1,{0xA0}},
  {0xFF,3,{0x98,0x82,0x00}},  
  {0x35,1,{0x00}},
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
	params->dsi.vertical_backporch      = 20;
	params->dsi.vertical_frontporch     = 200;
	params->dsi.vertical_active_line    = FRAME_HEIGHT;
	
	params->dsi.horizontal_sync_active    = 7;
	params->dsi.horizontal_backporch      = 32;
	params->dsi.horizontal_frontporch     = 32;
	params->dsi.horizontal_active_pixel   = FRAME_WIDTH;
	params->dsi.ssc_disable=1;

	params->dsi.PLL_CLOCK = 240;

	
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


struct LCM_DRIVER us196_zcx_jd9882u_wxga_868_lcm_drv = 
{
    .name			= "us196_zcx_jd9882u_wxga_868",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

