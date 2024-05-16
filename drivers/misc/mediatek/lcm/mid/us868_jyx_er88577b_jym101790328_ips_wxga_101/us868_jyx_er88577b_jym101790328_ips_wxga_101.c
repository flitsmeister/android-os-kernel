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

#define FRAME_WIDTH                                          (800)
#define FRAME_HEIGHT                                         (1280)

extern unsigned int GPIO_LCM_PWR_EN;//3.3V
extern unsigned int GPIO_LCM_1V8_EN;////1.8 en pin
extern unsigned int GPIO_LCM_RST;
#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0

#define LCM_DSI_CMD_MODE                                    0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util = {0};

#define UDELAY(n)                                             (lcm_util.udelay(n))
#define MDELAY(n)                                             (lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                        lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                    lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#ifdef BUILD_LK
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
    mt_set_gpio_mode(GPIO, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO, (output>0)? GPIO_OUT_ONE: GPIO_OUT_ZERO);
}
#else
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
    if (GPIO == 0xFFFFFFFF) {
        return;
    }
    gpio_set_value(GPIO, (output > 0) ? GPIO_OUT_ONE : GPIO_OUT_ZERO);
}
#endif

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

static void lcm_1v8_en(unsigned char enabled)
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
    params->dsi.PS                 = LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

    params->dsi.vertical_sync_active = 8;
    params->dsi.vertical_backporch = 8;
    params->dsi.vertical_frontporch = 20;
    params->dsi.vertical_active_line = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active = 20;
    params->dsi.horizontal_backporch = 20;
    params->dsi.horizontal_frontporch = 80;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 218;
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
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
                break;
        }
    }
}

static __attribute__((unused)) struct LCM_setting_table init_setting[] = {

	{0xE0,2,{0xAB,0xBA}},
	{0xE1,2,{0xBA,0xAB}},
	{0xB1,4,{0x10,0x01,0x47,0xFF}},
	{0xB2,6,{0x0C,0x10,0x04,0x50,0x50,0x14}},
	{0xB3,3,{0x56,0xD3,0x00}},
	{0xB4,3,{0x33,0x30,0x04}},
	{0xB6,7,{0xB0,0x00,0x00,0x10,0x00,0x10,0x00}},
	{0xB8,7,{0x36,0x12,0x29,0x49,0x48,0x00,0x00}},
	{0xB9,38,{0x7F,0x28,0x1D,0x12,0x13,0x07,0x10,0x00,0x1C,0x1E,0x21,0x42,0x33,0x3F,0x34,0x35,0x2B,0x1B,0x00,0x7F,0x28,0x1D,0x12,0x13,0x07,0x10,0x00,0x1C,0x1E,0x21,0x42,0x33,0x3F,0x34,0x35,0x2B,0x1B,0x00}},
	{0xC0,16,{0x32,0x45,0x54,0x45,0x66,0x66,0x44,0x44,0x85,0x04,0x85,0x04,0x3F,0x00,0x00,0xC1}},
	{0xC1,10,{0x34,0x94,0x02,0x8F,0x85,0x04,0x85,0x04,0x54,0x00}},
	{0xC2,12,{0x33,0x09,0x08,0x89,0x08,0x11,0x22,0x20,0x44,0xBB,0x18,0x00}},
	{0xC3,22,{0x80,0x40,0x08,0x07,0x06,0x13,0x12,0x11,0x10,0x0F,0x0E,0x0D,0x0C,0x04,0x05,0x02,0x02,0x02,0x02,0x02,0x02,0x00}},
	{0xC4,22,{0x00,0x00,0x08,0x07,0x06,0x13,0x12,0x11,0x10,0x0F,0x0E,0x0D,0x0C,0x04,0x05,0x02,0x02,0x02,0x02,0x02,0x02,0x00}},
	{0xC5,3,{0xE8,0x85,0x77}},
	{0xC6,2,{0x1F,0x22}},
	{0xC8,6,{0x21,0x00,0x31,0x42,0x34,0x16}},
	{0xCA,2,{0x4B,0x43}},
	{0xCD,8,{0x0E,0x64,0x64,0x20,0x19,0x6B,0x06,0xB3}},
	{0xD2,4,{0xE3,0x2B,0x38,0x00}},
	{0xD4,11,{0x00,0x01,0x00,0x0E,0x04,0x44,0x08,0x10,0x00,0x00,0x00}},
	{0xE6,8,{0x80,0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}},
	{0xF0,5,{0x12,0x03,0x20,0x00,0xFF}},
	{0xF3,1,{0x00}},

    {0x11,0,{}},
    {REGFLAG_MDELAY,120,{}},
    {0x29,0,{}},
    {REGFLAG_MDELAY,5,{}},

};

/* static void init_lcm_registers(void)
{
    unsigned int data_array[16];
    push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
    data_array[0] = 0x00110500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
    data_array[0] = 0x00290500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
} */

static void lcm_init(void)
{
    lcm_1v8_en(1);
    MDELAY(30);
    lcd_power_en(1);
    MDELAY(30);

    lcd_reset(1);
    MDELAY(30);
    lcd_reset(0);
    MDELAY(30);
    lcd_reset(1);
    MDELAY(30);//Must > 5ms
    //init_lcm_registers();
    push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
    unsigned int data_array[16];
    data_array[0] = 0x00280500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(30);
    data_array[0] = 0x00100500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(20);

    lcd_reset(0);
    MDELAY(30);

    lcd_power_en(0);//3.3
    MDELAY(30);

    lcm_1v8_en(0);//1.8
    MDELAY(30);
}

static void lcm_resume(void)
{
    lcm_init();
}

struct LCM_DRIVER us868_jyx_er88577b_jym101790328_ips_wxga_101_lcm_drv =
{
    .name            = "us868_jyx_er88577b_jym101790328_ips_wxga_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
