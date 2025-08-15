

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

#define FRAME_WIDTH                                     (1024)
#define FRAME_HEIGHT                                    (600)
#define PHYSICAL_WIDTH                                  (154)
#define PHYSICAL_HEIGHT                                 (86)

#define LCM_DSI_CMD_MODE                                    0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util;

extern unsigned int GPIO_LCM_3V3_EN;
extern unsigned int GPIO_LCM_1V8_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_BIAS_EN;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
#define MDELAY(n)        (lcm_util.mdelay(n))
#define UDELAY(n)        (lcm_util.udelay(n))
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
    {0x30, 1, {0x00}},
    {0xf7, 4, {0x49,0x61,0x02,0x00}},

    {0x30, 1, {0x01}},
    {0x04, 1, {0x0C}},
    {0x0B, 1, {0x10}},
    {0x1F, 1, {0x05}},
    {0x23, 1, {0x38}},
    {0x28, 1, {0x18}},
    {0x29, 1, {0x29}},
    {0x2A, 1, {0x01}},
    {0x2B, 1, {0x29}},
    {0x2C, 1, {0x01}},

    {0x30, 1, {0x02}},
    {0x00, 1, {0x05}},
    {0x01, 1, {0x22}},
    {0x02, 1, {0x08}},
    {0x03, 1, {0x12}},
    {0x04, 1, {0x16}},
    {0x05, 1, {0x64}},
    {0x06, 1, {0x00}},
    {0x07, 1, {0x00}},
    {0x08, 1, {0x78}},
    {0x09, 1, {0x00}},
    {0x0A, 1, {0x04}},

    {0x0B,11, {0x16,0x17,0x0B,0x0D,0x0D,0x0D,0x11,0x10,0x07,0x07,0x09}},
    {0x0C,11, {0x09,0x1E,0x1E,0x1C,0x1C,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D}},
    {0x0D,11, {0x0A,0x05,0x0B,0x0D,0x0D,0x0D,0x11,0x10,0x06,0x06,0x08}},
    {0x0E,11, {0x08,0x1F,0x1F,0x1D,0x1D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D}},
    {0x0F,11, {0x0A,0x05,0x0D,0x0B,0x0D,0x0D,0x11,0x10,0x1D,0x1D,0x1F}},
    {0x10,11, {0x1F,0x08,0x08,0x06,0x06,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D}},
    {0x11,11, {0x16,0x17,0x0D,0x0B,0x0D,0x0D,0x11,0x10,0x1C,0x1C,0x1E}},
    {0x12, 1, {0x1E,0x09,0x09,0x07,0x07,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D}},
    {0x13, 4, {0x00,0x00,0x00,0x00}},
    {0x14, 4, {0x00,0x00,0x41,0x41}},
    {0x15, 4, {0x00,0x00,0x00,0x00}},

    {0x17, 1, {0x00}},
    {0x18, 1, {0x85}},
    {0x19, 1, {0x06,0x09}},
    {0x1a, 1, {0x05,0x08}},
    {0x1b, 1, {0x0A,0x04}},
    {0x26, 1, {0x00}},
    {0x27, 1, {0x00}},

    {0x30, 1, {0x06}},
    {0x12,14, {0x3F,0x27,0x28,0x35,0x1B,0x17,0x16,0x13,0x10,0x01,0x23,0x1B,0x10,0x30}},
    {0x13,14, {0x3F,0x27,0x28,0x35,0x1D,0x18,0x16,0x13,0x10,0x02,0x24,0x1B,0x10,0x30}},

    {0x30, 1, {0x0a}},
    {0x02, 1, {0x4F}},
    {0x0B, 1, {0x40}},

    {0x30, 1, {0x0d}},
    {0x10, 1, {0x05}},
    {0x11, 1, {0x0c}},
    {0x12, 1, {0x05}},
    {0x13, 1, {0x0c}},
    {0x30, 1, {0x00}},

    {0x11,1,{0x00}},
    {REGFLAG_DELAY, 120, {}},
    {0x29,1,{0x00}},
    {REGFLAG_DELAY, 20, {}},
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
    params->physical_width = PHYSICAL_WIDTH;
    params->physical_height = PHYSICAL_HEIGHT;

    params->physical_width = 136;
    params->physical_height = 217;

    params->dsi.mode    = SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;//LCM_THREE_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 

    params->dsi.vertical_sync_active                            = 2;
    params->dsi.vertical_backporch                              = 23;
    params->dsi.vertical_frontporch                             = 12;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 24;
    params->dsi.horizontal_backporch                            = 160;
    params->dsi.horizontal_frontporch                           = 160;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 154;
}


static void lcd_power_en(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_3V3_EN, GPIO_OUT_ONE);
        lcm_set_gpio_output(GPIO_LCM_1V8_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_3V3_EN, GPIO_OUT_ZERO);
        lcm_set_gpio_output(GPIO_LCM_1V8_EN, GPIO_OUT_ZERO);
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

static void lcd_avdd_en(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_EN, 1);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_EN, 0);
    }
}

static void lcm_init(void)
{
    lcd_power_en(1);
    MDELAY(100);

    lcd_reset(1);
    MDELAY(5);
    lcd_reset(0);
    MDELAY(10);
    lcd_reset(1);
    MDELAY(50);//Must > 5ms

    lcd_avdd_en(1);
    MDELAY(20);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    MDELAY(180);
}

static void lcm_suspend(void)
{
    unsigned int data_array[16];

    data_array[0]=0x00280500; // Display Off
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(20);

    data_array[0] = 0x00100500; // Sleep In
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(30);

    lcd_avdd_en(0);
    MDELAY(50);

    lcd_reset(0);
    MDELAY(100);

    lcd_power_en(0);
    MDELAY(150);
}

static void lcm_resume(void)
{
    lcm_init();
}

struct LCM_DRIVER us717u12d_lc_jd9165a_boe_wsvga_7_lcm_drv =
{
    .name            = "us717u12d_lc_jd9165a_boe_wsvga_7",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

