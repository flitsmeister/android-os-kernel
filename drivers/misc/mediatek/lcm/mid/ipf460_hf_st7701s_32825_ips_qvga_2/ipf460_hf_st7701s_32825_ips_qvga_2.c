#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#else
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#endif

#include <lcm_drv.h>

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH                                             (480)
#define FRAME_HEIGHT                                            (480)

extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_3V3_EN;
extern unsigned int GPIO_LCM_1V8_EN;
#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0


#define LCM_DSI_CMD_MODE                                    0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util = {0};

#define UDELAY(n)                                                (lcm_util.udelay(n))
#define MDELAY(n)                                                (lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)     lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                       lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                   lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                             lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)                lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
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
static void lcd_3v3_en(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_3V3_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_3V3_EN, GPIO_OUT_ZERO);
    }
}

static void lcd_reset(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_RST, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_RST, GPIO_OUT_ZERO);
    }
}

static void lcd_1v8_en(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_1V8_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_1V8_EN, GPIO_OUT_ZERO);
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
    params->dsi.mode               = BURST_VDO_MODE;
    params->dsi.LANE_NUM           = LCM_TWO_LANE;
    params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
    params->dsi.PS                 = LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

    params->dsi.vertical_sync_active                            = 8;
    params->dsi.vertical_backporch                              = 16;
    params->dsi.vertical_frontporch                             = 24;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 30;
    params->dsi.horizontal_backporch                            = 50;
    params->dsi.horizontal_frontporch                           = 50;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 90;
    //params->dsi.ssc_disable = 1;
}

struct LCM_setting_table {
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static void __attribute__((unused)) push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
#define REGFLAG_MDELAY            0xFFFC
#define REGFLAG_UDELAY            0xFFFB
#define REGFLAG_END_OF_TABLE    0xFFFD
#define REGFLAG_RESET_LOW        0xFFFE
#define REGFLAG_RESET_HIGH        0xFFFF
    unsigned int i;
    unsigned int cmd;

    for (i = 0; i < count; i++) {
        cmd = table[i].cmd;
        switch (cmd) {
            case REGFLAG_MDELAY:
                MDELAY(table[i].count);
                break;
            case REGFLAG_END_OF_TABLE:
                break;
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
                break;
        }
    }
}

static __attribute__((unused)) struct LCM_setting_table init_setting[] = {
    {0xFF, 5, {0x77,0x01,0x00,0x00,0x13}},
    {0xEF, 1, {0x08}},
    {0xFF, 5, {0x77,0x01,0x00,0x00,0x10}},
    {0xC0, 2, {0x3B,0x00}},
    {0xC1, 2, {0x10,0x0C}},
    {0xC2, 2, {0x07,0x0A}},
    //{0xC7, 1, {0x04}},
    {0xCC, 1, {0x10}},
    {0xCD, 1, {0x08}},
    {0xB0,16, {0x05,0x12,0x98,0x0E,0x0F,0x07,0x07,0x09,0x09,0x23,0x05,0x52,0x0F,0x67,0x2C,0x11}},
    {0xB1,16, {0x0B,0x11,0x97,0x0C,0x12,0x06,0x06,0x08,0x08,0x22,0x03,0x51,0x11,0x66,0x2B,0x0F}},
    {0xFF, 5, {0x77,0x01,0x00,0x00,0x11}},
    {0xB0, 1, {0x5D}},
    {0xB1, 1, {0x3E}},
    {0xB2, 1, {0x81}},
    {0xB3, 1, {0x80}},
    {0xB5, 1, {0x4E}},
    {0xB7, 1, {0x85}},
    {0xB8, 1, {0x20}},
    {0xC1, 1, {0x78}},
    {0xC2, 1, {0x78}},
    {0xD0, 1, {0x88}},
    {0xE0, 3, {0x00,0x00,0x02}},
    {0xE1,11, {0x06,0x30,0x08,0x30,0x05,0x30,0x07,0x30,0x00,0x33,0x33}},
    {0xE2,12, {0x11,0x11,0x33,0x33,0xF4,0x00,0x00,0x00,0xF4,0x00,0x00,0x00}},
    {0xE3, 4, {0x00,0x00,0x11,0x11}},
    {0xE4, 2, {0x44,0x44}},
    {0xE5,16, {0x0D,0xF5,0x30,0xF0,0x0F,0xF7,0x30,0xF0,0x09,0xF1,0x30,0xF0,0x0B,0xF3,0x30,0xF0}},
    {0xE6, 4, {0x00,0x00,0x11,0x11}},
    {0xE7, 2, {0x44,0x44}},
    {0xE8,16, {0x0C,0xF4,0x30,0xF0,0x0E,0xF6,0x30,0xF0,0x08,0xF0,0x30,0xF0,0x0A,0xF2,0x30,0xF0}},
    {0xE9, 2, {0x36,0x01}},
    {0xEB, 7, {0x00,0x01,0xE4,0xE4,0x44,0x88,0x40}},
    {0xED,16, {0xFF,0x10,0xAF,0x76,0x54,0x2B,0xCF,0xFF,0xFF,0xFC,0xB2,0x45,0x67,0xFA,0x01,0xFF}},
    {0xEF, 6, {0x08,0x08,0x08,0x45,0x3F,0x54}},
    {0xFF, 5, {0x77,0x01,0x00,0x00,0x00}},
    //{0x21, 0 ,{}},
    {0x11, 0 ,{}},
    {REGFLAG_MDELAY, 120, {}},

    {0x3A, 1, {0x77}},
    //{0x36, 1, {0x00}},

    {0x35, 1, {0x00}},

    {0x29, 0, {}},
    {REGFLAG_END_OF_TABLE, 0, {}}
};

static void lcm_init(void)
{
    lcd_3v3_en(1);
    MDELAY(30);

    lcd_1v8_en(1);
    MDELAY(10);

    lcd_reset(1);
    MDELAY(20);
    lcd_reset(0);
    MDELAY(50);
    lcd_reset(1);

    MDELAY(120);//Must > 5ms
    push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
    unsigned int data_array[16];
    data_array[0] = 0x00280500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(20);
    data_array[0] = 0x00100500;
    dsi_set_cmdq(data_array, 1, 1);

    MDELAY(120);
    lcd_reset(0);
    MDELAY(50);
    lcd_1v8_en(0);
    MDELAY(50);

    lcd_3v3_en(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
    lcm_init();
}

struct LCM_DRIVER ipf460_hf_st7701s_32825_ips_qvga_2_lcm_drv =
{
    .name            = "ipf460_hf_st7701s_32825_ips_qvga_2",
    .set_util_funcs  = lcm_set_util_funcs,
    .get_params      = lcm_get_params,
    .init            = lcm_init,
    .suspend         = lcm_suspend,
    .resume          = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
