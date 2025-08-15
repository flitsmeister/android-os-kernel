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
extern int _lcm_i2c_write_bytes(unsigned char addr, unsigned char value);


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
	
  {0xB0,1,{0x01}},
  {0xC0,1,{0x26}},//GOA,MUX,setting
  {0xC1,1,{0x10}},
  {0xC2,1,{0x0E}},
  {0xC3,1,{0x00}},
  {0xC4,1,{0x00}},
  {0xC5,1,{0x23}},
  {0xC6,1,{0x11}},
  {0xC7,1,{0x22}},
  {0xC8,1,{0x20}},
  {0xC9,1,{0x1E}},
  {0xCA,1,{0x1C}},
  {0xCB,1,{0x0C}},
  {0xCC,1,{0x0A}},
  {0xCD,1,{0x08}},
  {0xCE,1,{0x06}},
  {0xCF,1,{0x18}},
  {0xD0,1,{0x02}},
  {0xD1,1,{0x00}},
  {0xD2,1,{0x00}},
  {0xD3,1,{0x00}},
  {0xD4,1,{0x26}},
  {0xD5,1,{0x0F}},
  {0xD6,1,{0x0D}},
  {0xD7,1,{0x00}},
  {0xD8,1,{0x00}},
  {0xD9,1,{0x23}},
  {0xDA,1,{0x11}},
  {0xDB,1,{0x21}},
  {0xDC,1,{0x1F}},
  {0xDD,1,{0x1D}},
  {0xDE,1,{0x1B}},
  {0xDF,1,{0x0B}},
  {0xE0,1,{0x09}},
  {0xE1,1,{0x07}},
  {0xE2,1,{0x05}},
  {0xE3,1,{0x17}},
  {0xE4,1,{0x01}},
  {0xE5,1,{0x00}},
  {0xE6,1,{0x00}},
  {0xE7,1,{0x00}},
		
  {0xB0,1,{0x03}},//Page select
  {0xBE,1,{0x04}},//GOA timing setting
  {0xB9,1,{0x40}},
  {0xCC,1,{0x88}},
  {0xC8,1,{0x0C}},
  {0xC9,1,{0x07}},
  {0xCD,1,{0x01}},
  {0xCA,1,{0x40}},
  {0xCE,1,{0x1A}},
  {0xCF,1,{0x60}},
  {0xD2,1,{0x08}},
  {0xD3,1,{0x08}},
  {0xDB,1,{0x01}},
  {0xD9,1,{0x06}},
  {0xD4,1,{0x00}},
  {0xD5,1,{0x01}},
  {0xD6,1,{0x04}},
  {0xD7,1,{0x03}},
  {0xC2,1,{0x00}},
  {0xC3,1,{0x0E}},
  {0xC4,1,{0x00}},
  {0xC5,1,{0x0E}},
  {0xDD,1,{0x00}},
  {0xDE,1,{0x0E}},
  {0xE6,1,{0x00}},
  {0xE7,1,{0x0E}},
  {0xC2,1,{0x00}},
  {0xC3,1,{0x0E}},
  {0xC4,1,{0x00}},
  {0xC5,1,{0x0E}},
  {0xDD,1,{0x00}},
  {0xDE,1,{0x0E}},
  {0xE6,1,{0x00}},
  {0xE7,1,{0x0E}},
		
  {0xB0,1,{0x06}},//Page select
		
  {0xC0,1,{0xA5}},
  {0xD5,1,{0x1C}},//GOE setting=1us
  {0xC0,1,{0x00}},
		
  {0xB0,1,{0x00}},//Page select
  {0xBA,1,{0x8F}},//HIMAX0724 column inversion
  {0xF9,1,{0x5C}},//GOA setting
  {0xC2,1,{0x14}},//VGNH=5V
  {0xC4,1,{0x14}},//VGNH=5V
		
  {0xBF,1,{0x1A}},//VGH=16.5V  
  {0xC0,1,{0x11}},//VGL=-11.8V 
  {0xBD,1,{0x1D}},
/* 	{0x11,0,{}},
	{REGFLAG_DELAY,120,{}},
	
	{0x29,0,{}},
	{REGFLAG_DELAY,20,{}},
	
	{0x35,1,{0x00}},
	
	
	{REGFLAG_END_OF_TABLE,0x00,{}} */
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
	
	params->physical_width = 107;
	params->physical_height = 172;

    params->dsi.mode    = SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;//LCM_THREE_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;


	params->dsi.vertical_sync_active    = 2;
	params->dsi.vertical_backporch      = 10;
	params->dsi.vertical_frontporch     = 14;
	params->dsi.vertical_active_line    = FRAME_HEIGHT;
	
	params->dsi.horizontal_sync_active    = 24;
	params->dsi.horizontal_backporch      = 80;
	params->dsi.horizontal_frontporch     = 60;
	params->dsi.horizontal_active_pixel   = FRAME_WIDTH;
	params->dsi.ssc_disable=1;

	params->dsi.PLL_CLOCK = 506;
	
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
	MDELAY(30);

	lcd_3v3_en(1);//3.3
	MDELAY(30);

	lcd_bias_en(1);
	_lcm_i2c_write_bytes(0x00,0x11);
	_lcm_i2c_write_bytes(0x01,0x11);

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
	MDELAY(50);
	

    lcd_reset(0);
    MDELAY(50);

	lcd_bias_en(0);
	MDELAY(30);

	lcd_3v3_en(0);
	MDELAY(50);
	lcd_power_en(0);
}


static void lcm_resume(void)
{
  lcm_init();
}


struct LCM_DRIVER us196_bns_hx8279_j40fhd827d150a_wuxga_101_lcm_drv = 
{
    .name			= "us196_bns_hx8279_j40fhd827d150a_wuxga_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

