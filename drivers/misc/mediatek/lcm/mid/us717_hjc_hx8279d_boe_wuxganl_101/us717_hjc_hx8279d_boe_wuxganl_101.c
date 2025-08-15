#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
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


#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util;

extern unsigned int GPIO_LCM_PWR_EN;//3.3V
extern unsigned int GPIO_LCM_RST;//reset
extern unsigned int GPIO_LCM_BL_EN;//bias en pin

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

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

static void backlight_enable(unsigned char enabled)
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
	params->type                   = LCM_TYPE_DSI;
	params->width                  = FRAME_WIDTH;
	params->height                 = FRAME_HEIGHT;
	params->dsi.mode               = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM           = LCM_FOUR_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;
	params->dsi.packet_size = 256;
		params->dsi.vertical_sync_active				= 4;
		params->dsi.vertical_backporch				= 16;
		params->dsi.vertical_frontporch 				= 20;
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 
	
		params->dsi.horizontal_sync_active			= 10;	
		params->dsi.horizontal_backporch				= 40;	
		params->dsi.horizontal_frontporch			= 80;
		params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;

		params->dsi.PLL_CLOCK = 469;
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
	{0xB0, 1, {0x01}},

	{0xC3, 1, {0x0F}},
	{0xC4, 1, {0x00}},
	{0xC5, 1, {0x00}},
	{0xC6, 1, {0x00}},
	{0xC7, 1, {0x00}},
	{0xC8, 1, {0x0D}},
	{0xC9, 1, {0x12}},
	{0xCA, 1, {0x11}},

	{0xCD, 1, {0x1D}},
	{0xCE, 1, {0x1B}},
	{0xCF, 1, {0x0B}},
	{0xD0, 1, {0x09}},
	{0xD1, 1, {0x07}},
	{0xD2, 1, {0x05}},
	{0xD3, 1, {0x01}},

	{0xD7, 1, {0x10}},
	{0xD8, 1, {0x00}},
	{0xD9, 1, {0x00}},
	{0xDA, 1, {0x00}},
	{0xDB, 1, {0x00}},
	{0xDC, 1, {0x0E}},
	{0xDD, 1, {0x12}},
	{0xDE, 1, {0x11}},

	{0xE1, 1, {0x1E}},
	{0xE2, 1, {0x1C}},
	{0xE3, 1, {0x0C}},
	{0xE4, 1, {0x0A}},
	{0xE5, 1, {0x08}},
	{0xE6, 1, {0x06}},
	{0xE7, 1, {0x02}},
	{0xB0, 1, {0x03}},
	{0xBE, 1, {0x03}},

	{0xCC, 1, {0x44}},
	{0xC8, 1, {0x07}},
	{0xC9, 1, {0x05}},

	{0xCA, 1, {0x42}},
	{0xCD, 1, {0x3E}},
	{0xCF, 1, {0x60}},
	{0xD2, 1, {0x04}},
	{0xD3, 1, {0x04}},

	{0xD4, 1, {0x01}},
	{0xD5, 1, {0x00}},
	{0xD6, 1, {0x03}},
	{0xD7, 1, {0x04}},
	{0xD9, 1, {0x01}},
	{0xDB, 1, {0x01}},
	{0xE4, 1, {0xF0}},
	{0xE5, 1, {0x0A}},
	
	{0xB0, 1, {0x00}},
//	{0xB2, 1, {0x41}},//bits
	{0xBD, 1, {0x44}},
	{0xC2, 1, {0x08}},

	{0xC4, 1, {0x10}},
	{0xCC, 1, {0x00}},

	{0xB0, 1, {0x02}},
	{0xC0, 1, {0x00}},

	{0xC1, 1, {0x0A}},
	{0xC2, 1, {0x20}},
	{0xC3, 1, {0x24}},

//	{0xB2, 1, {0x41}},	//bits	
//	{0xBA, 1, {0x8F}},
	{0xC4, 1, {0x23}},
	


	{0xC5, 1, {0x29}},
	{0xC6, 1, {0x23}},
	{0xC7, 1, {0x1C}},

	{0xC8, 1, {0x19}},
	{0xC9, 1, {0x17}},
	{0xCA, 1, {0x17}},
	{0xCB, 1, {0x18}},

	{0xCC, 1, {0x1A}},
	{0xCD, 1, {0x1E}},
	{0xCE, 1, {0x20}},
	{0xCF, 1, {0x23}},
	{0xD0, 1, {0x07}},
	{0xD1, 1, {0x00}},
	{0xD2, 1, {0x00}},
	{0xD3, 1, {0x0A}},
	{0xD4, 1, {0x13}},
	{0xD5, 1, {0x1C}},

	{0xD6, 1, {0x1A}},
	{0xD7, 1, {0x13}},
	{0xD8, 1, {0x17}},
	{0xD9, 1, {0x1C}},
	{0xDA, 1, {0x19}},
	{0xDB, 1, {0x17}},
	{0xDC, 1, {0x17}},
	{0xDD, 1, {0x18}},
	{0xDE, 1, {0x1A}},
	{0xDF, 1, {0x1E}},
	{0xE0, 1, {0x20}},
	{0xE1, 1, {0x23}},
	{0xE2, 1, {0x07}},


};

static void init_lcm_registers(void)
{
    unsigned int data_array[16];
	push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
    data_array[0] = 0x00110500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
    data_array[0] = 0x00290500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(20);
}

static void lcm_init(void)
{
	lcd_power_en(1);
	MDELAY(100);
	lcd_reset(0);
	MDELAY(20);
	lcd_reset(1);
	MDELAY(20);//Must > 5ms
    backlight_enable(1);
	init_lcm_registers();
}

static void lcm_suspend(void)
{
    backlight_enable(0);
    MDELAY(150);
    lcd_reset(0);
    MDELAY(100);
	lcd_power_en(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
	lcm_init();
}

struct LCM_DRIVER us717_hjc_hx8279d_boe_wuxganl_101_lcm_drv =
{
    .name			= "us717_hjc_hx8279d_boe_wuxganl_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
