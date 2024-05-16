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
	params->dsi.mode               = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM           = LCM_FOUR_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

    params->dsi.vertical_sync_active                            = 4; //2; //4;
    params->dsi.vertical_backporch                              = 12; //10; //16;
    params->dsi.vertical_frontporch                             = 20;//5; 
    params->dsi.vertical_active_line                            = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active                          = 18; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 18; //60; //60; //80;
    params->dsi.horizontal_frontporch                           = 18; //60; 
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 200;
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
	unsigned int i;

	for (i = 0; i < count; i++) {
		unsigned int cmd;
		cmd = table[i].cmd;
		switch (cmd) {
            case REGFLAG_MDELAY:
                MDELAY(table[i].count);
                break;
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
    }
}

static __attribute__((unused)) struct LCM_setting_table init_setting[] = {
		{0xE0,1,{0x00}},
		{0xE1,1,{0x93}},//========== Internal setting ==========
		{0xE2,1,{0x65}},
		{0xE3,1,{0xF8}},
		{0x80,1,{0x03}},
			
	 	{0xE0,1,{0x04}},// MIPI related Timing Setting
		{0x2D,1,{0x03}},
		{0x09,1,{0x14}},
		
		{0xE0,1,{0x01}},//  Improve ESD option
		
		{0x00,1,{0x00}},//  Improve ESD option
		{0x01,1,{0x6F}},
		
		{0x03,1,{0x00}},
		{0x04,1,{0x6F}},
		
		{0x0C,1,{0x74}},
		
		{0x17,1,{0x00}},// Vcom floating
		{0x18,1,{0xF2}},
		{0x19,1,{0x00}},
		{0x1A,1,{0x00}},
		{0x1B,1,{0xF2}},
		{0x1C,1,{0x00}},
		
		{0x1F,1,{0x7E}},// Vcom floating
		{0x20,1,{0x24}},
		{0x21,1,{0x24}},
		{0x22,1,{0x0E}},
		
		{0x37,1,{0x09}},// Vcom floating
			
		{0x38,1,{0x04}},
		{0x39,1,{0x08}},
		{0x3A,1,{0x12}},
		{0x3C,1,{0x78}},
		{0x3D,1,{0xFF}},
		{0x3E,1,{0xFF}},
		{0x3F,1,{0x7F}},
		
		
		{0x40,1,{0x06}},
		{0x41,1,{0xA0}},
		
        {0x4B,1,{0x04}},
	
		{0x55,1,{0x01}},//EQ control function
		{0x56,1,{0x01}},
		{0x57,1,{0x65}},// Set bias current for GOP and SOP
		{0x58,1,{0x08}},// Inversion setting 
		{0x59,1,{0x28}},
		{0x5A,1,{0x2B}},// Inversion setting 
		{0x5B,1,{0x13}},
	
		
		
		{0x5D,1,{0x70}},
		{0x5E,1,{0x5E}},
		{0x5F,1,{0x50}},
		{0x60,1,{0x45}},//========== page1 relative ==========
		{0x61,1,{0x42}},// Setting AVDD, AVEE clamp
		{0x62,1,{0x33}},
		{0x63,1,{0x37}},
		{0x64,1,{0x21}},// VGMP, VGMN, VGSP, VGSN setting
		{0x65,1,{0x39}},
		{0x66,1,{0x36}},// gate signal control
		{0x67,1,{0x36}},// power IC control
		{0x68,1,{0x54}},// VCL SET -2.5V
		{0x69,1,{0x41}},// VCOM = -1.888V
		{0x6A,1,{0x4A}},// Setting VGH=15V, VGL=-11V
		{0x6B,1,{0x3D}},
		{0x6C,1,{0x3A}},// power control for VGH, VGL
		{0x6D,1,{0x2F}},
		{0x6E,1,{0x1F}},//========== page2 relative ==========
		{0x6F,1,{0x02}},//gamma setting
		{0x70,1,{0x70}},// DSP Timing Settings update for BIST
		{0x71,1,{0x5E}},
		{0x72,1,{0x50}},
		{0x73,1,{0x45}},//========== page1 relative ==========
		{0x74,1,{0x42}},// Setting AVDD, AVEE clamp
		{0x75,1,{0x33}},
		{0x76,1,{0x37}},
		{0x77,1,{0x21}},// VGMP, VGMN, VGSP, VGSN setting
		{0x78,1,{0x39}},
		{0x79,1,{0x36}},// gate signal control
		{0x7A,1,{0x36}},// power IC control
		{0x7B,1,{0x54}},// VCL SET -2.5V
		{0x7C,1,{0x41}},// VCOM = -1.888V
		{0x7D,1,{0x4A}},// Setting VGH=15V, VGL=-11V
		{0x7E,1,{0x3D}},
		{0x7F,1,{0x3A}},// power control for VGH, VGL
		{0x80,1,{0x2F}},
		{0x81,1,{0x1F}},//========== page2 relative ==========
		{0x82,1,{0x02}},//gamma setting
		
		{0XE0,1,{0x02}},
		
		{0x00,1,{0x1E}},// DSP Timing Settings update for BIST
		{0x01,1,{0x1F}},
		{0x02,1,{0x17}},
		{0x03,1,{0x37}},//========== page1 relative ==========
		{0x04,1,{0x08}},// Setting AVDD, AVEE clamp
		{0x05,1,{0x0A}},
		{0x06,1,{0x04}},
		{0x07,1,{0x06}},// VGMP, VGMN, VGSP, VGSN setting
		{0x08,1,{0x00}},
		{0x09,1,{0x1F}},// gate signal control
		{0x0A,1,{0x1F}},// power IC control
		{0x0B,1,{0x1F}},// VCL SET -2.5V
		{0x0C,1,{0x1F}},// VCOM = -1.888V
		{0x0D,1,{0x1F}},// Setting VGH=15V, VGL=-11V
		{0x0E,1,{0x1F}},
		{0x0F,1,{0x02}},// power control for VGH, VGL
		{0x10,1,{0x1F}},
		{0x11,1,{0x1F}},//========== page2 relative ==========
		{0x12,1,{0x1F}},//gamma setting
		{0x13,1,{0x1F}},
		{0x14,1,{0x1F}},//========== page2 relative ==========
		{0x15,1,{0x1F}},
		
		
		
		{0x16,1,{0x1E}},// DSP Timing Settings update for BIST
		{0x17,1,{0x1F}},
		{0x18,1,{0x17}},
		{0x19,1,{0x37}},//========== page1 relative ==========
		{0x1A,1,{0x09}},// Setting AVDD, AVEE clamp
		{0x1B,1,{0x0B}},
		{0x1C,1,{0x05}},
		{0x1D,1,{0x07}},// VGMP, VGMN, VGSP, VGSN setting
		{0x1E,1,{0x01}},
		{0x1F,1,{0x1F}},// gate signal control
		{0x20,1,{0x1F}},// power IC control
		{0x21,1,{0x1F}},// VCL SET -2.5V
		{0x22,1,{0x1F}},// VCOM = -1.888V
		{0x23,1,{0x1F}},// Setting VGH=15V, VGL=-11V
		{0x24,1,{0x1F}},
		{0x25,1,{0x03}},// power control for VGH, VGL
		{0x26,1,{0x1F}},
		{0x27,1,{0x1F}},//========== page2 relative ==========
		{0x28,1,{0x1F}},//gamma setting
		{0x29,1,{0x1F}},
		{0x2A,1,{0x1F}},//========== page2 relative ==========
		{0x2B,1,{0x1F}},
		
		
		{0x2C,1,{0x1F}},// DSP Timing Settings update for BIST
		{0x2D,1,{0x1E}},
		{0x2E,1,{0x17}},
		{0x2F,1,{0x37}},//========== page1 relative ==========
		{0x30,1,{0x07}},// Setting AVDD, AVEE clamp
		{0x31,1,{0x05}},
		{0x32,1,{0x0B}},
		{0x33,1,{0x09}},// VGMP, VGMN, VGSP, VGSN setting
		{0x34,1,{0x03}},
		{0x35,1,{0x1F}},// gate signal control
		{0x36,1,{0x1F}},// power IC control
		{0x37,1,{0x1F}},// VCL SET -2.5V
		{0x38,1,{0x1F}},// VCOM = -1.888V
		{0x39,1,{0x1F}},// Setting VGH=15V, VGL=-11V
		{0x3A,1,{0x1F}},
		{0x3B,1,{0x01}},// power control for VGH, VGL
		{0x3C,1,{0x1F}},
		{0x3D,1,{0x1F}},//========== page2 relative ==========
		{0x3E,1,{0x1F}},//gamma setting
		{0x3F,1,{0x1F}},
		{0x40,1,{0x1F}},//========== page2 relative ==========
		{0x41,1,{0x1F}},
		
		
		
		{0x42,1,{0x1F}},// DSP Timing Settings update for BIST
		{0x43,1,{0x1E}},
		{0x44,1,{0x17}},
		{0x45,1,{0x37}},//========== page1 relative ==========
		{0x46,1,{0x06}},// Setting AVDD, AVEE clamp
		{0x47,1,{0x04}},
		{0x48,1,{0x0A}},
		{0x49,1,{0x08}},// VGMP, VGMN, VGSP, VGSN setting
		{0x4A,1,{0x02}},
		{0x4B,1,{0x1F}},// gate signal control
		{0x4C,1,{0x1F}},// power IC control
		{0x4D,1,{0x1F}},// VCL SET -2.5V
		{0x4E,1,{0x1F}},// VCOM = -1.888V
		{0x4F,1,{0x1F}},// Setting VGH=15V, VGL=-11V
		{0x50,1,{0x1F}},
		{0x51,1,{0x00}},// power control for VGH, VGL
		{0x52,1,{0x1F}},
		{0x53,1,{0x1F}},//========== page2 relative ==========
		{0x54,1,{0x1F}},//gamma setting
		{0x55,1,{0x1F}},
		{0x56,1,{0x1F}},//========== page2 relative ==========
		{0x57,1,{0x1F}},
		
		{0x58,1,{0x10}},// DSP Timing Settings update for BIST
		{0x59,1,{0x00}},
		{0x5A,1,{0x00}},
		{0x5B,1,{0x30}},//========== page1 relative ==========
		{0x5C,1,{0x05}},// Setting AVDD, AVEE clamp
		{0x5D,1,{0x30}},
		{0x5E,1,{0x01}},
		{0x5F,1,{0x02}},// VGMP, VGMN, VGSP, VGSN setting
		{0x60,1,{0x30}},
		{0x61,1,{0x03}},// gate signal control
		{0x62,1,{0x04}},// power IC control
		{0x63,1,{0x03}},// VCL SET -2.5V
		{0x64,1,{0x6A}},// VCOM = -1.888V
		{0x65,1,{0x75}},// Setting VGH=15V, VGL=-11V
		{0x66,1,{0x0D}},
		{0x67,1,{0x73}},// power control for VGH, VGL
		{0x68,1,{0x09}},
		{0x69,1,{0x06}},//========== page2 relative ==========
		{0x6A,1,{0x6A}},//gamma setting
		{0x6B,1,{0x08}},
		{0x6C,1,{0x00}},//========== page2 relative ==========
		{0x6D,1,{0x0C}},
		
		{0x6E,1,{0x04}},// gate signal control
		{0x6F,1,{0x88}},// power IC control
		{0x70,1,{0x00}},// VCL SET -2.5V
		{0x71,1,{0x00}},// VCOM = -1.888V
		{0x72,1,{0x06}},// Setting VGH=15V, VGL=-11V
		{0x73,1,{0x7B}},
		{0x74,1,{0x00}},// power control for VGH, VGL
		{0x75,1,{0x80}},
		{0x76,1,{0x00}},//========== page2 relative ==========
		{0x77,1,{0x0D}},//gamma setting
		{0x78,1,{0x24}},
		{0x79,1,{0x00}},//========== page2 relative ==========
		{0x7A,1,{0x00}},
		{0x7B,1,{0x00}},//gamma setting
		{0x7C,1,{0x00}},
		{0x7D,1,{0x03}},//========== page2 relative ==========
		{0x7E,1,{0x7B}},
		
		
		
		{0xE0,1,{0x04}},//========== page2 relative ==========
        {0x09,1,{0x10}},
		{0x2B,1,{0x2B}},
		{0x2E,1,{0x44}},//========== page2 relative ==========
		
		
		{0xE0,1,{0x00}},//gamma setting
		{0xE6,1,{0x02}},
		{0xE7,1,{0x02}},//========== page2 relative ==========	 
};

static void init_lcm_registers(void)
{
    unsigned int data_array[16];

	//push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);

    data_array[0] = 0x00110500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);

    data_array[0] = 0x00290500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(20);
}

static void lcm_init(void)
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
}

static void lcm_suspend(void)
{
	lcd_bias_en(0);//bias en 偏压使能
	//unsigned int data_array[16];
	lcd_reset(0);
	MDELAY(100);
   	lcd_power_en(0);
	MDELAY(150);
}

static void lcm_resume(void)
{
  lcm_init();
}

struct LCM_DRIVER gs716_gx_jd9365_boe_wxga_ips_8_lcm_drv =
{
    .name			= "gs716_gx_jd9365_boe_wxga_ips_8",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
