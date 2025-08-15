

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

#define FRAME_WIDTH                                         (1024)
#define FRAME_HEIGHT                                        (600)
#define PHYSICAL_WIDTH                                      (154)
#define PHYSICAL_HIGHT                                      (85)

#define LCM_DSI_CMD_MODE                                    0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util;

extern unsigned int GPIO_LCM_3V3_EN;
extern unsigned int GPIO_LCM_1V8_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_BL_EN;
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
    params->physical_height = PHYSICAL_HIGHT;

    params->dsi.mode    = BURST_VDO_MODE;//SYNC_EVENT_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;//LCM_THREE_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 

    params->dsi.vertical_sync_active = 1;
    params->dsi.vertical_backporch = 23;
    params->dsi.vertical_frontporch = 12;
    params->dsi.vertical_active_line = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active = 10;
    params->dsi.horizontal_backporch = 160;
    params->dsi.horizontal_frontporch = 160;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 165;

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

static void avdd_enable(unsigned char enabled)
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

/* static void backlight_enable(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ZERO);
    }
} */

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

static void init_lcm_registers(void)
{
    unsigned int data_array[16];

   data_array[0] = 0xAC801500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(1);

    data_array[0] = 0xB8811500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(1);

    data_array[0] = 0x09821500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(1);

    data_array[0] = 0x78831500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(1);

    data_array[0] = 0x7F841500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(1);

    data_array[0] = 0xBB851500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(1);

    data_array[0] = 0x70861500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(1);

    data_array[0] = 0x00110500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);

    data_array[0] = 0x00290500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(5);
}

static void lcm_init(void)
{
    lcd_power_en(1);
    MDELAY(10);

    lcd_reset(1);
    MDELAY(20);
    lcd_reset(0);
    MDELAY(50);
    lcd_reset(1);
    MDELAY(120);//Must > 5ms

    avdd_enable(1);
    MDELAY(10);

    init_lcm_registers();
    MDELAY(10);

    /*backlight_enable(1);*/
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

    avdd_enable(0);
    /*backlight_enable(0);*/
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

struct LCM_DRIVER us717u12d_sq_ek79007_wsvga_7_lcm_drv =
{
    .name           = "us717u12d_sq_ek79007_wsvga_7",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

