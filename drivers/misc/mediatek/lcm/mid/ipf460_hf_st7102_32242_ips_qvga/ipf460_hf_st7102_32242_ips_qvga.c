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

    params->dsi.vertical_sync_active                            = 2;
    params->dsi.vertical_backporch                              = 18;
    params->dsi.vertical_frontporch                             = 280;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 2;
    params->dsi.horizontal_backporch                            = 40;
    params->dsi.horizontal_frontporch                           = 40;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 120;
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
	{0x99,3,{0x71,0x02,0xa2}},
	{0x99,3,{0x71,0x02,0xa3}},
	{0x99,3,{0x71,0x02,0xa4}},
	{0xA4,1,{0x31}},
	{0x78,1,{0x21}},
	{0x79,1,{0xEF}},
	{0xB0,7,{0x22,0x61,0x1E,0x61,0x2F,0x39,0x39}},
	{0xB7,2,{0x64,0x64}},
	{0xBF,2,{0x4E,0x4E}},
	{0xD7,6,{0x00,0x14,0x88,0x08,0xF0,0xF0}},
	{0xA3,32,{0x40,0x03,0x88,0x30,0x44,
			0x00,0x00,0x00,0x00,0x05,
			0x00,0x68,0x00,0x02,0x00,
			0x45,0x05,0x00,0x00,0x00,
			0x00,0x46,0x00,0x00,0x02,
			0x20,0x52,0x00,0x05,0x00,
			0x00,0xFF}},
	{0xA6,44,{0x55,0x00,0x24,0x55,0x38,
			0x00,0x40,0x7F,0x4E,0x4E,
			0x00,0x24,0x55,0x39,0x00,
			0x40,0x7E,0x4E,0x4E,0x00,
			0x2C,0x55,0x3A,0xC0,0x40,
			0x7D,0x4E,0x4E,0x00,0x2C,
			0x55,0x3B,0x00,0x40,0x7C,
			0x4E,0x4E,0x00,0x00,0x06,
			0x00,0x00,0x00,0x00}},
	{0xA7,48,{0x1A,0x1A,0x00,0x64,0x40,
			0x03,0x12,0x40,0x00,0x41,
			0x40,0x4E,0x4E,0x00,0x64,
			0x40,0x47,0x56,0x02,0x03,
			0x45,0x44,0x4E,0x4E,0x00,
			0x64,0x40,0x00,0x00,0x00,
			0x00,0x40,0x40,0x4E,0x4E,
			0x00,0x24,0x40,0x00,0x00,
			0x00,0x00,0x40,0x40,0x4E,
			0x4E,0x00,0x53}},
	{0xAC,37,{0x05,0x12,0x01,0x0B,0x1A,
			0x10,0x04,0x09,0x06,0x02,
			0x19,0x06,0x19,0x02,0x1A,
			0x1A,0x07,0x13,0x03,0x0A,
			0x1A,0x11,0x04,0x08,0x06,
			0x00,0x19,0x04,0x19,0x04,
			0x18,0x1A,0xAF,0x8A,0xBF,
			0x0A,0x00}},
	{0xAD,7,{0xCC,0x40,0x46,0x00,0x06,0x5E,0x2F}},
	{0xE8,14,{0x30,0x07,0x05,0x6A,0x6A,
			0x9C,0x00,0xE2,0x04,0x00,
			0x00,0x00,0x00,0xEF}},
	{0x75,2,{0x03,0x04}},
	{0xE7,33,{0x8B,0x3E,0x00,0x0C,0xF0,
			0x5D,0x00,0x5D,0x00,0x5D,
			0x00,0x5D,0x00,0xFF,0x00,
			0x08,0x7B,0x00,0x00,0xC8,
			0x6A,0x5A,0x08,0x1A,0x3C,
			0x00,0xA1,0x01,0x8C,0x01,
			0x7F,0xF0,0x22}},
	{0xE9,9,{0x3C,0x7F,0x08,0x0C,0x1A,0x7A,0x22,0x1A,0x33}},
	
	{0xA9,3,{0x00,0x00,0xF3}},
	{0xC8,37,{0x00,0x00,0x09,0x13,0x21,
			0x00,0x3F,0x03,0x6E,0x04,
			0x10,0xB6,0x07,0x19,0x01,
			0x11,0x66,0x01,0xA5,0x01,
			0x21,0xE5,0x0C,0x31,0x08,
			0x32,0xA6,0x08,0x0D,0x0B,
			0xF3,0x65,0x0A,0xAC,0xE2,
			0x03,0xFF}},
	{0xC9,37,{0x00,0x00,0x09,0x13,0x21,
			0x00,0x3F,0x03,0x6E,0x04,
			0x10,0xB6,0x07,0x19,0x01,
			0x11,0x66,0x01,0xA5,0x01,
			0x21,0xE5,0x0C,0x31,0x08,
			0x32,0xA6,0x08,0x0D,0x0B,
			0xF3,0x65,0x0A,0xAC,0xE2,
			0x03,0xFF}},
	{0x11,01,{0x00}},
	{REGFLAG_MDELAY, 120, {}},  
	{0x35,1,{0x00}},
	{0x29,01,{0x00}},	
	{REGFLAG_END_OF_TABLE, 0x00, {}} 
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

struct LCM_DRIVER ipf460_hf_st7102_32242_ips_qvga_lcm_drv =
{
    .name            = "ipf460_hf_st7102_32242_ips_qvga",
    .set_util_funcs  = lcm_set_util_funcs,
    .get_params      = lcm_get_params,
    .init            = lcm_init,
    .suspend         = lcm_suspend,
    .resume          = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
