

#ifdef BUILD_LK
#include <platform/upmu_common.h>
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

extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_BL_EN;

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
{0xB0,1,{0x00}},	
{0xB6,1,{0x03}},	
{0xBA,1,{0x8B}},
{0xBD,1,{0x51}},
{0xBF,1,{0x15}},	
{0xC0,1,{0x0F}},	
{0xC2,1,{0x0C}},	
{0xC3,1,{0x02}},	
{0xC4,1,{0x0C}},	
{0xC5,1,{0x02}},	
{0xB0,1,{0x01}},	
{0xE0,1,{0x0F}},	
{0xE1,1,{0x0F}},	
{0xDC,1,{0x10}},	
{0xDD,1,{0x10}},	
{0xCC,1,{0x0F}},	
{0xCD,1,{0x0F}},	
{0xC8,1,{0x10}},	
{0xC9,1,{0x10}},	
{0xB0,1,{0x03}},	
{0xC5,1,{0x2C}},	
{0xDE,1,{0x2C}},	
{0xCA,1,{0x42}},	
{0xD6,1,{0x88}},	
{0xD7,1,{0x08}},	
{0xD2,1,{0x06}},	
{0xD3,1,{0x06}},	
{0xB0,1,{0x05}},	
{0xB3,1,{0x52}},	
{0xB0,1,{0x06}},	
{0xB8,1,{0xA5}},	
{0xC0,1,{0xA5}},	
{0xC7,1,{0x0F}},	
{0xD5,1,{0x32}},	
{0xB0,1,{0x02}},	
{0xC0,1,{0x00}},	
{0xC1,1,{0x14}},	
{0xC2,1,{0x16}},	
{0xC3,1,{0x24}},	
{0xC4,1,{0x25}},	
{0xC5,1,{0x31}},	
{0xC6,1,{0x2A}},	
{0xC7,1,{0x2D}},	
{0xC8,1,{0x33}},	
{0xC9,1,{0x37}},	
{0xCA,1,{0x3F}},	
{0xCB,1,{0x3F}},	
{0xCC,1,{0x3F}},	
{0xCD,1,{0x2D}},	
{0xCE,1,{0x2E}},	
{0xCF,1,{0x29}},	
{0xD0,1,{0x04}},	
{0xD2,1,{0x00}},	
{0xD3,1,{0x14}},	
{0xD4,1,{0x14}},	
{0xD5,1,{0x26}},	
{0xD6,1,{0x2D}},	
{0xD7,1,{0x31}},	
{0xD8,1,{0x38}},	
{0xD9,1,{0x3D}},	
{0xDA,1,{0x3F}},	
{0xDB,1,{0x3F}},	
{0xDC,1,{0x3F}},	
{0xDD,1,{0x3F}},	
{0xDE,1,{0x3F}},	
{0xDF,1,{0x38}},	
{0xE0,1,{0x35}},	
{0xE1,1,{0x2D}},	
{0xE2,1,{0x0A}},	
{0xB0,1,{0x06}},	
{0xB8,1,{0xA5}},	
{0xBC,1,{0x33}},	
{0xB0,1,{0x07}},	
{0xB2,1,{0x04}},	
{0xB3,1,{0x0C}},	
{0xB4,1,{0x1C}},	
{0xB5,1,{0x2C}},	
{0xB6,1,{0x3C}},	
{0xB7,1,{0x5C}},	
{0xB8,1,{0x7C}},	
{0xB9,1,{0xBC}},	
{0xBA,1,{0xF5}},	
{0xBB,1,{0x74}},	
{0xBC,1,{0xEF}},	
{0xBD,1,{0xF1}},	
{0xBE,1,{0x6B}},	
{0xBF,1,{0xE3}},	
{0xCB,1,{0x05}},	
{0xCC,1,{0x6B}},	
{0xC0,1,{0x20}},	
{0xC1,1,{0x6A}},	
{0xC2,1,{0x8A}},	
{0xC3,1,{0xA6}},	
{0xC4,1,{0xB5}},	
{0xC5,1,{0xC6}},	
{0xC6,1,{0xDA}},	
{0xC7,1,{0xE4}},	
{0xC8,1,{0xE8}},	
{0xB0,1,{0x08}},	
{0xB2,1,{0x04}},	
{0xB3,1,{0x0C}},	
{0xB4,1,{0x1C}},	
{0xB5,1,{0x2C}},	
{0xB6,1,{0x3C}},	
{0xB7,1,{0x5C}},	
{0xB8,1,{0x7C}},	
{0xB9,1,{0xBC}},	
{0xBA,1,{0xFF}},	
{0xBB,1,{0x81}},	
{0xBC,1,{0x01}},	
{0xBD,1,{0x03}},	
{0xBE,1,{0x82}},	
{0xBF,1,{0x02}},	
{0xCB,1,{0x06}},	
{0xCC,1,{0xAF}},	
{0xC0,1,{0x41}},	
{0xC1,1,{0x87}},	
{0xC2,1,{0xA5}},	
{0xC3,1,{0xC2}},	
{0xC4,1,{0xD2}},	
{0xC5,1,{0xE4}},	
{0xC6,1,{0xF7}},	
{0xC7,1,{0xFC}},	
{0xC8,1,{0xFC}},	
{0xB0,1,{0x09}},	
{0xB2,1,{0x04}},	
{0xB3,1,{0x0C}},	
{0xB4,1,{0x1C}},	
{0xB5,1,{0x2C}},	
{0xB6,1,{0x3C}},	
{0xB7,1,{0x5C}},	
{0xB8,1,{0x7C}},	
{0xB9,1,{0xB7}},	
{0xBA,1,{0xFD}},	
{0xBB,1,{0x83}},	
{0xBC,1,{0x05}},	
{0xBD,1,{0x07}},	
{0xBE,1,{0x86}},	
{0xBF,1,{0xFE}},	
{0xCB,1,{0x06}},	
{0xCC,1,{0xAB}},	
{0xC0,1,{0x40}},	
{0xC1,1,{0x87}},	
{0xC2,1,{0xA2}},	
{0xC3,1,{0xBC}},	
{0xC4,1,{0xCC}},	
{0xC5,1,{0xDD}},	
{0xC6,1,{0xF2}},	
{0xC7,1,{0xF8}},	
{0xC8,1,{0xFA}},	
{0xB0,1,{0x0A}},	
{0xB2,1,{0x04}},	
{0xB3,1,{0x0C}},	
{0xB4,1,{0x1C}},	
{0xB5,1,{0x2C}},	
{0xB6,1,{0x3C}},	
{0xB7,1,{0x5C}},	
{0xB8,1,{0x7C}},	
{0xB9,1,{0xBC}},	
{0xBA,1,{0xF5}},	
{0xBB,1,{0x74}},	
{0xBC,1,{0xEF}},	
{0xBD,1,{0xF1}},	
{0xBE,1,{0x6B}},	
{0xBF,1,{0xE3}},	
{0xCB,1,{0x05}},	
{0xCC,1,{0x6B}},	
{0xC0,1,{0x20}},	
{0xC1,1,{0x6A}},	
{0xC2,1,{0x8A}},	
{0xC3,1,{0xA6}},	
{0xC4,1,{0xB5}},	
{0xC5,1,{0xC6}},	
{0xC6,1,{0xCA}},	
{0xC7,1,{0xE4}},	
{0xC8,1,{0xE8}},	
{0xB0,1,{0x0B}},	
{0xB2,1,{0x04}},	
{0xB3,1,{0x0C}},	
{0xB4,1,{0x1C}},	
{0xB5,1,{0x2C}},	
{0xB6,1,{0x3C}},	
{0xB7,1,{0x5C}},	
{0xB8,1,{0x7C}},	
{0xB9,1,{0xBC}},	
{0xBA,1,{0xFF}},	
{0xBB,1,{0x81}},	
{0xBC,1,{0x01}},	
{0xBD,1,{0x03}},	
{0xBE,1,{0x82}},	
{0xBF,1,{0xFE}},	
{0xCB,1,{0x06}},	
{0xCC,1,{0xAB}},	
{0xC0,1,{0x41}},	
{0xC1,1,{0x87}},	
{0xC2,1,{0xA5}},	
{0xC3,1,{0xC2}},	
{0xC4,1,{0xD2}},	
{0xC5,1,{0xE4}},	
{0xC6,1,{0xF7}},	
{0xC7,1,{0xFC}},	
{0xC8,1,{0xFC}},	
{0xB0,1,{0x0C}},	
{0xB2,1,{0x04}},	
{0xB3,1,{0x0C}},	
{0xB4,1,{0x1C}},	
{0xB5,1,{0x2C}},	
{0xB6,1,{0x3C}},	
{0xB7,1,{0x5C}},	
{0xB8,1,{0x7C}},	
{0xB9,1,{0xB7}},	
{0xBA,1,{0xFD}},	
{0xBB,1,{0x83}},	
{0xBC,1,{0x05}},	
{0xBD,1,{0x07}},	
{0xBE,1,{0x86}},	
{0xBF,1,{0x00}},	
{0xCB,1,{0x06}},	
{0xCC,1,{0xAF}},	
{0xC0,1,{0x40}},	
{0xC1,1,{0x87}},	
{0xC2,1,{0xA2}},	
{0xC3,1,{0xBC}},	
{0xC4,1,{0xCC}},	
{0xC5,1,{0xDD}},	
{0xC6,1,{0xF2}},	
{0xC7,1,{0xF8}},	
{0xC8,1,{0xFA}},
    {0x11,1,{0x00}},
    {REGFLAG_DELAY,120,{}},
    
    {0x29,1,{0x00}},
    {REGFLAG_DELAY,20,{}},
    {REGFLAG_END_OF_TABLE,0x00,{}}
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

    params->dsi.mode    = BURST_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;//LCM_THREE_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 


    params->dsi.vertical_sync_active                            = 4;
    params->dsi.vertical_backporch                              = 16;
    params->dsi.vertical_frontporch                             = 16;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 24; 
    params->dsi.horizontal_backporch                            = 80; 
    params->dsi.horizontal_frontporch                           = 60; 
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 520;


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

static void lcm_init(void)
{

	avdd_enable(1);
	lcd_power_en(1);
	MDELAY(100);
	lcd_reset(1);
	MDELAY(5);
	lcd_reset(0);	
	MDELAY(10);
	lcd_reset(1);
	MDELAY(50);//Must > 5ms
//	init_lcm_registers();
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
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


struct LCM_DRIVER us716_hsd_hx8279_m101031bi40a1_wuxganl_101_lcm_drv = 
{
    .name			= "us716_hsd_hx8279_m101031bi40a1_wuxganl_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

