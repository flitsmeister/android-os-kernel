
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

extern unsigned int GPIO_LCM_BIAS_ENN;
extern unsigned int GPIO_LCM_BIAS_ENP;
extern unsigned int GPIO_TP_RST;
//extern unsigned int GPIO_CTP_TP_RST_EN;//hx83102 tp rst
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
	{ 0xB9, 3, {0x83, 0x10, 0x2E}},
{ 0xD1, 4, {0x67, 0x0C, 0xFF, 0x05}},
{ 0xB1, 17, {0x10, 0xFA, 0xAF, 0xAF, 0x2B, 0x2B, 0xC1, 0x75, 0x39, 0x36, 0x36, 0x36, 0x36, 0x22, 0x21, 0x15, 0x00}},
{ 0xB2, 16, {0x00, 0xB0, 0x47, 0x80, 0x00, 0x22, 0x62, 0x2F, 0x00, 0x00, 0x00, 0x00, 0x15, 0x20, 0xD7, 0x00}},
{ 0xB4, 16, {0x7F, 0x64, 0x7F, 0x64, 0x89, 0x6C, 0x68, 0x50, 0x01, 0x9C, 0x01, 0x58, 0x00, 0xFF, 0x00, 0xFF}},
{ 0xBF, 3, {0xFC, 0x85, 0x80}},
{ 0xD2, 2, {0x2B, 0x2B}},
{ 0xD3, 43, {0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x00, 0x14, 0x0C, 0x27, 0x27, 0x22, 0x2F, 0x1F, 0x1F, 0x04, 0x04, 0x32, 0x10, 0x1D, 0x00, 0x1D, 0x32, 0x17, 0xB6, 0x07, 0xB6, 0x32, 0x10, 0x1A, 0x00, 0x1A, 0x00, 0x00, 0x1A, 0x3A, 0x01, 0x55, 0x1A, 0x39, 0x60, 0x76, 0x0F}},
{ 0xE0, 46, {0x00, 0x03, 0x0B, 0x11, 0x17, 0x24, 0x3B, 0x44, 0x4C, 0x4A, 0x68, 0x71, 0x79, 0x8B, 0x8A, 0x94, 0x9E, 0xB1, 0xB0, 0x57, 0x5E, 0x69, 0x73, 0x00, 0x03, 0x0B, 0x11, 0x17, 0x24, 0x3B, 0x44, 0x4C, 0x4A, 0x68, 0x71, 0x79, 0x8B, 0x8A, 0x94, 0x9E, 0xB1, 0xB0, 0x57, 0x5E, 0x69, 0x73}},
{ 0xBD, 1, {0x01}},
{ 0xB1, 4, {0x01, 0x9B, 0x01, 0x31}},
{ 0xCB, 10, {0xF4, 0x36, 0x12, 0x16, 0xC0, 0x28, 0x6C, 0x85, 0x3F, 0x04}},
{ 0xD3, 11, {0x01, 0x00, 0xFC, 0x00, 0x00, 0x11, 0x10, 0x00, 0x0E, 0x00, 0x01}},
{ 0xBD, 1, {0x02}},
{ 0xB4, 6, {0x4E, 0x00, 0x33, 0x11, 0x33, 0x88}},
{ 0xBF, 3, {0xF2, 0x00, 0x02}},
{ 0xBD, 1, {0x00}},
{ 0xC0, 14, {0x23, 0x23, 0x22, 0x11, 0xA2, 0x17, 0x00, 0x80, 0x00, 0x00, 0x08, 0x00, 0x63, 0x63}},
{ 0xC6, 1, {0xF9}},
{ 0xC7, 1, {0x30}},
{ 0xC8, 8, {0x00, 0x04, 0x04, 0x00, 0x00, 0x82, 0x13, 0xFF}},
{ 0xD0, 3, {0x07, 0x04, 0x05}},
{ 0xD5, 44, {0x2D, 0x2D, 0x2D, 0x2D, 0x18, 0x18, 0x18, 0x18, 0x21, 0x20, 0x21, 0x20, 0x25, 0x24, 0x25, 0x24, 0x18, 0x18, 0x18, 0x18, 0x39, 0x39, 0x39, 0x39, 0x2E, 0x2E, 0x2E, 0x2E, 0x03, 0x02, 0x03, 0x02, 0x01, 0x00, 0x01, 0x00, 0x07, 0x06, 0x07, 0x06, 0x05, 0x04, 0x05, 0x04}},
{ 0xD6, 44, {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x20, 0x21, 0x20, 0x21, 0x24, 0x25, 0x24, 0x25, 0x2D, 0x2D, 0x2D, 0x2D, 0x39, 0x39, 0x39, 0x39, 0x2E, 0x2E, 0x2E, 0x2E, 0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07, 0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03}},
{ 0xE7, 23, {0x12, 0x13, 0x02, 0x02, 0x48, 0x48, 0x0E, 0x0E, 0x18, 0x23, 0x26, 0x78, 0x25, 0x7F, 0x01, 0x27, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x68}},
{ 0xBD, 1, {0x01}},
{ 0xE7, 7, {0x02, 0x37, 0x01, 0x8E, 0x0D, 0xD3, 0x0E}},
{ 0xBD, 1, {0x02}},
{ 0xE7, 28, {0xFF, 0x01, 0xFD, 0x01, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0x00, 0x02, 0x40}},
{ 0xBD, 1, {0x00}},
{ 0xBD, 1, {0x00}},
{ 0xBA, 8, {0x70, 0x03, 0xA8, 0x85, 0xF2, 0x00, 0xC0, 0x0D}},
{ 0xBD, 1, {0x02}},
{ 0xD8, 12, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0}},
{ 0xBD, 1, {0x03}},
{ 0xD8, 24, {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xA0, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xA0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x50, 0x55, 0x55, 0x55, 0x55, 0x55, 0x50}},
{ 0xBD, 1, {0x00}},
{ 0xCC, 1, {0x02}},
{ 0xBD, 1, {0x03}},
{ 0xB2, 1, {0x80}},
{ 0xBD, 1, {0x00}},
{ 0xE1, 2, {0x02, 0x04}},

    {0x11, 0, {}},
    {REGFLAG_DELAY, 120, {}},
    //sleep out
    {0x29, 0, {}},
    {REGFLAG_DELAY, 20, {}},
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


    params->dsi.vertical_sync_active                = 8;
    params->dsi.vertical_backporch                = 28;
    params->dsi.vertical_frontporch                 =98;
    params->dsi.vertical_active_line                = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active            = 14;
    params->dsi.horizontal_backporch                = 26;
    params->dsi.horizontal_frontporch            = 32;
    params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 490;	
	params->dsi.ssc_disable = 1;
	
}

static void tp_reset(unsigned char enabled)
{
   //if (enabled)
   //{
   //    tpd_gpio_output(0, 1);
   //}
   //else
   //{   
   //    tpd_gpio_output(0, 0);
   //
   //}
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


static void lcd_bias_en(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_ENN, GPIO_OUT_ONE);
		lcm_set_gpio_output(GPIO_LCM_BIAS_ENP, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_ENN, GPIO_OUT_ZERO);
		lcm_set_gpio_output(GPIO_LCM_BIAS_ENP, GPIO_OUT_ZERO);
    }
}


static void lcm_init(void)
{
	unsigned int data_array[16];

	lcd_power_en(1);//1.8
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
	lcd_power_en(0);
}


static void lcm_resume(void)
{
  lcm_init();
}


struct LCM_DRIVER us163_boe_hx83102_wuxga_103_lcm_drv = 
{
    .name			= "us163_boe_hx83102_wuxga_103",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

