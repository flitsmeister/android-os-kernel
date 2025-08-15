
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

#define FRAME_WIDTH												(800)
#define FRAME_HEIGHT											(1280)
#define PHYSICAL_WIDTH                                  (135)
#define PHYSICAL_HIGHT                                  (217)

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
	{0x30,1,{0x01}},//page1
	{0x78,4,{0x49,0x61,0x02,0x00}},
	{0x30,1,{0x02}},//page2
	{0x31,1,{0x22}},
	{0x32,1,{0x08}},
	{0x33,1,{0x3F}},
	{0x3C,1,{0x04}},
	{0x3d,1,{0x78}},
	{0x3e,1,{0x43}},
	{0x3f,1,{0x33}},
	{0x42,1,{0xA2}},
	{0x43,1,{0x30}},
	{0x44,1,{0x21}},
	{0x49,1,{0xC0}},
	{0x6D,1,{0x3A}},
	{0x6E,1,{0x19}},
	{0x4E,2,{0x92,0x92}},
	{0x50,2,{0x92,0x92}},
	{0x55,2,{0xFF,0xFF}},
	{0x56,2,{0x00,0x00}},
	{0x66,2,{0x7F,0x7F}},
	{0x67,2,{0x7F,0x7F}},
	{0x64,3,{0xFC,0xFF,0x0F}},
	{0x65,3,{0xFC,0xFF,0x0F}},
	{0x6F,3,{0x30,0x00,0x00}},
	{0x70,3,{0x30,0x00,0x00}},
	{0x41,12,{0x03,0x03,0x03,0x03,0x03,0x03,0x0B,0x0B,0x03,0x03,0x03,0x03}},
	{0x5a,11,{0x23,0x23,0x01,0x01,0x1F,0x1F,0x23,0x23,0x25,0x25,0x22}},
	{0x5b,11,{0x22,0x34,0x34,0x0B,0x0B,0x09,0x09,0x0F,0x0F,0x0D,0x0D}},
	{0x5c,11,{0x23,0x23,0x00,0x00,0x1E,0x1E,0x23,0x23,0x25,0x25,0x22}},
	{0x5d,11,{0x22,0x34,0x34,0x0A,0x0A,0x08,0x08,0x0E,0x0E,0x0C,0x0C}},
																		
	
	
	{0x4C,2,{0x10,0x10}},
	{0x6A,1,{0x30}},
	{0x6B,1,{0xA6}},
	{0x30,1,{0x07}},//page7
	{0x31,1,{0xC0}},
	{0x30,1,{0x08}},//page8
	{0x33,1,{0x05}},
	{0x40,1,{0x50}},
	{0x41,1,{0x80}},
	{0x42,1,{0x1A}},
	{0x47,1,{0x0A}},
	{0x48,1,{0x0C}},
	{0x50,1,{0x20}},
	{0x5A,1,{0x20}},
	{0x5B,1,{0x00}},
	{0x5C,1,{0x53}},
	{0x62,1,{0x04}},
	{0x65,1,{0x5F}},
	{0x5D,1,{0x81}},
	{0x5E,1,{0x20}},
	{0x5F,1,{0x10}},
	{0x60,1,{0xE1}},
	{0x73,1,{0x01}},
	{0x30,1,{0x0a}},//page a
	{0x32,1,{0xFF}},
	{0x33,1,{0x28}},
	{0x3f,1,{0x53}},
	{0x47,1,{0x20}},
	{0x48,1,{0x80}},
	{0x49,1,{0x03}},
	{0x30,1,{0x0b}},//page B
	{0x33,2,{0x00,0x49}},//VCOM
	{0x3c,2,{0x00,0x92}},
	{0x43,1,{0xAF}},
	{0x44,1,{0x2F}},
	
	{0x3e,5,{0x00,0x05,0x0F,0x17,0x20}},
	{0x3f,14,{0x37,0x52,0x59,0x60,0x5B,0x76,0x7B,0x81,0x90,0x8F,0x97,0x9E,0xAF,0xAF}},
	{0x40,9,{0x55,0x5B,0x63,0x65,0x00,0x05,0x0F,0x17,0x20}},
	{0x41,14,{0x37,0x52,0x57,0x5F,0x5D,0x76,0x7C,0x81,0x90,0x8F,0x97,0x9E,0xAF,0xAF}},
	{0x42,4,{0x55,0x5B,0x63,0x65}},
	{0x45,1,{0x75}},
	{0x46,1,{0x3A}},
	{0x48,1,{0x3F}},
	{0x49,1,{0x18}},
	{0x4A,1,{0x2B}},
	{0x30,1,{0x0c}},//page c
	{0x32,1,{0x62}},
	{0x71,1,{0x43}},
	{0x30,1,{0x0d}},//page d
	{0x4c,1,{0x74}},
	{0x30,1,{0x08}},//page 8
	{0x52,1,{0xC3}},
	{0x54,1,{0x20}},
	{0x30,1,{0x00}},//page 0
	
	{0x35,1,{0x00}},//TE

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
    params->type                   = LCM_TYPE_DSI;
    params->width                  = FRAME_WIDTH;
    params->height                 = FRAME_HEIGHT;
    params->physical_width         = PHYSICAL_WIDTH;
    params->physical_height        = PHYSICAL_HIGHT;

    params->dsi.mode               = SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM           = LCM_FOUR_LANE;
    params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

    params->dsi.vertical_sync_active                            = 8;
    params->dsi.vertical_backporch                              = 20;
    params->dsi.vertical_frontporch                             = 140;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 8;
    params->dsi.horizontal_backporch                            = 28;
    params->dsi.horizontal_frontporch                           = 40;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 243;
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


struct LCM_DRIVER us163_sq_jd9366tc_inx_wxga_ips_101_lcm_drv = 
{
    .name			= "us163_sq_jd9366tc_inx_wxga_ips_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

