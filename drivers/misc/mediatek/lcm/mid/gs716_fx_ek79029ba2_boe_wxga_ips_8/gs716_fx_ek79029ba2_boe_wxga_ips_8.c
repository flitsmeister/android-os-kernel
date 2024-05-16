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

#define FRAME_WIDTH  (800)//1200
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

    params->dsi.vertical_sync_active                            = 2; //2; //4;
    params->dsi.vertical_backporch                              = 10; //10; //16;
    params->dsi.vertical_frontporch                             = 12;//5; 
    params->dsi.vertical_active_line                            = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active                          = 25; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 25; //60; //60; //80;
    params->dsi.horizontal_frontporch                           = 25; //60; 
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
{0xCD, 1, {0xAA}},
{0x30, 1, {0x00}},
{0x33, 1, {0x01}},
{0x32, 1, {0x00}},
{0x36, 1, {0x41}},
{0x3A, 1, {0x00}},
{0x67, 1, {0x82}},
{0x69, 1, {0x27}},
{0x6D, 1, {0x01}},
{0x68, 1, {0x16}},
{0x55, 8, {0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F}},
{0x56, 16, {0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F}},
{0x72, 2, {0x00,0x80}},
{0x73, 2, {0x20,0x10}},
{0x5E, 1, {0x03}},
{0x41, 1, {0x5E}},
{0x61, 1, {0xA4}},
{0x7E, 1, {0x38}},
{0x74, 1, {0x10}},
{0x3F, 1, {0x20}},
{0x47, 1, {0x14}},
{0x48, 1, {0x66}},
{0x4F, 1, {0x50}},
{0x4E, 1, {0x4F}},
{0x39, 1, {0x11}},
{0x60, 1, {0x10}},
{0x50, 1, {0xD0}},
{0x76, 1, {0x34}},
{0x7C, 1, {0x80}},
{0x2E, 1, {0x04}},
{0x53, 19, {0x1F,0x1A,0x18,0x14,0x14,0x14,0x16,0x18,0x17,0x13,0x10,0x0F,0x0E,0x0E,0x0B,0x09,0x06,0x03,0x01}},
{0x54, 19, {0x1F,0x1A,0x18,0x14,0x14,0x14,0x16,0x18,0x19,0x15,0x12,0x0F,0x0E,0x0E,0x0B,0x09,0x06,0x03,0x01}},
{0x5F, 1, {0x2A}},
{0x63, 1, {0x04}},//0x24:3-Lane; 0x04:4-lane;
{0x28, 1, {0x31}},
{0x29, 1, {0x00}},
{0x34, 1, {0xFC}},
{0x2D, 1, {0x31}},
{0x78, 1, {0x67}},
{0x1D, 1, {0xC4}},
{0x1D, 1, {0xF4}},
{0x4D, 1, {0x00}}, 
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
    MDELAY(5);
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

struct LCM_DRIVER gs716_fx_ek79029ba2_boe_wxga_ips_8_lcm_drv =
{
    .name			= "gs716_fx_ek79029ba2_boe_wxga_ips_8",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
