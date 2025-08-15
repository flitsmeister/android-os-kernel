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

#define FRAME_WIDTH                                     (800)
#define FRAME_HEIGHT                                    (1280)
#define PHYSICAL_WIDTH                                  (108)
#define PHYSICAL_HIGHT                                  (172)

extern unsigned int GPIO_LCM_BL_EN;
extern unsigned int GPIO_LCM_3V3_EN;
extern unsigned int GPIO_LCM_1V8_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_BIAS_EN;

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

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                      lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                  lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
extern int _lcm_i2c_write_bytes(unsigned char addr, unsigned char value);
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
static void lcd_bl_en(unsigned char enabled)
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

static void lcd_reset(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_RST, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_RST, GPIO_OUT_ONE);
    }
}
/* 
static void lcd_bias_gpio_set(unsigned char enabled)
{
    printk("lcd_bias_gpio_set:%d\r\n",enabled);
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_EN, GPIO_OUT_ONE);
        MDELAY(10);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_EN, 0);
    }
}

static void display_bias_set(unsigned char enabled)
{
    if(enabled){
        lcd_bias_gpio_set(1);
        _lcm_i2c_write_bytes(0x0, 0x12);
        _lcm_i2c_write_bytes(0x1, 0x12);
    }else{        
        _lcm_i2c_write_bytes(0x0, 0x0);
        _lcm_i2c_write_bytes(0x1, 0x0);
        lcd_bias_gpio_set(0);
    }
}
 */

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

    params->dsi.vertical_sync_active                            = 4;
    params->dsi.vertical_backporch                              = 12;
    params->dsi.vertical_frontporch                             = 30;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 20;
    params->dsi.horizontal_backporch                            = 20;
    params->dsi.horizontal_frontporch                           = 40;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 210;
    params->dsi.ssc_disable = 1;
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
#define REGFLAG_END_OF_TABLE      0xFFFD
#define REGFLAG_RESET_LOW         0xFFFE
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
    {0xFF, 3, {0x98,0x81,0x03}},
    {0x01, 1, {0x00}},
    {0x02, 1, {0x00}},
    {0x03, 1, {0x53}},
    {0x04, 1, {0x53}},
    {0x05, 1, {0x13}},
    {0x06, 1, {0x04}},
    {0x07, 1, {0x02}},
    {0x08, 1, {0x02}},
    {0x09, 1, {0x00}},
    {0x0a, 1, {0x00}},
    {0x0b, 1, {0x00}},
    {0x0c, 1, {0x00}},
    {0x0d, 1, {0x00}},
    {0x0e, 1, {0x00}},
    {0x0f, 1, {0x00}},
    {0x10, 1, {0x00}},
    {0x11, 1, {0x00}},
    {0x12, 1, {0x00}},
    {0x13, 1, {0x00}},
    {0x14, 1, {0x00}},
    {0x15, 1, {0x00}},
    {0x16, 1, {0x00}},
    {0x17, 1, {0x00}},
    {0x18, 1, {0x00}},
    {0x19, 1, {0x00}},
    {0x1a, 1, {0x00}},
    {0x1b, 1, {0x00}},
    {0x1c, 1, {0x00}},
    {0x1d, 1, {0x00}},
    {0x1e, 1, {0xc0}},
    {0x1f, 1, {0x80}},
    {0x20, 1, {0x02}},
    {0x21, 1, {0x09}},
    {0x22, 1, {0x00}},
    {0x23, 1, {0x00}},
    {0x24, 1, {0x00}},
    {0x25, 1, {0x00}},
    {0x26, 1, {0x00}},
    {0x27, 1, {0x00}},
    {0x28, 1, {0x55}},
    {0x29, 1, {0x03}},
    {0x2a, 1, {0x00}},
    {0x2b, 1, {0x00}},
    {0x2c, 1, {0x00}},
    {0x2d, 1, {0x00}},
    {0x2e, 1, {0x00}},
    {0x2f, 1, {0x00}},
    {0x30, 1, {0x00}},
    {0x31, 1, {0x00}},
    {0x32, 1, {0x00}},
    {0x33, 1, {0x00}},
    {0x34, 1, {0x00}},
    {0x35, 1, {0x00}},
    {0x36, 1, {0x00}},
    {0x37, 1, {0x00}},
    {0x38, 1, {0x3C}},
    {0x39, 1, {0x00}},
    {0x3a, 1, {0x00}},
    {0x3b, 1, {0x00}},
    {0x3c, 1, {0x00}},
    {0x3d, 1, {0x00}},
    {0x3e, 1, {0x00}},
    {0x3f, 1, {0x00}},
    {0x40, 1, {0x00}},
    {0x41, 1, {0x00}},
    {0x42, 1, {0x00}},
    {0x43, 1, {0x00}},
    {0x44, 1, {0x00}},
    {0x50, 1, {0x01}},
    {0x51, 1, {0x23}},
    {0x52, 1, {0x45}},
    {0x53, 1, {0x67}},
    {0x54, 1, {0x89}},
    {0x55, 1, {0xab}},
    {0x56, 1, {0x01}},
    {0x57, 1, {0x23}},
    {0x58, 1, {0x45}},
    {0x59, 1, {0x67}},
    {0x5a, 1, {0x89}},
    {0x5b, 1, {0xab}},
    {0x5c, 1, {0xcd}},
    {0x5d, 1, {0xef}},
    {0x5e, 1, {0x01}},
    {0x5f, 1, {0x08}},
    {0x60, 1, {0x02}},
    {0x61, 1, {0x02}},
    {0x62, 1, {0x0A}},
    {0x63, 1, {0x15}},
    {0x64, 1, {0x14}},
    {0x65, 1, {0x02}},
    {0x66, 1, {0x11}},
    {0x67, 1, {0x10}},
    {0x68, 1, {0x02}},
    {0x69, 1, {0x0F}},
    {0x6a, 1, {0x0E}},
    {0x6b, 1, {0x02}},
    {0x6c, 1, {0x0D}},
    {0x6d, 1, {0x0C}},
    {0x6e, 1, {0x06}},
    {0x6f, 1, {0x02}},
    {0x70, 1, {0x02}},
    {0x71, 1, {0x02}},
    {0x72, 1, {0x02}},
    {0x73, 1, {0x02}},
    {0x74, 1, {0x02}},
    {0x75, 1, {0x06}},
    {0x76, 1, {0x02}},
    {0x77, 1, {0x02}},
    {0x78, 1, {0x0A}},
    {0x79, 1, {0x15}},
    {0x7a, 1, {0x14}},
    {0x7b, 1, {0x02}},
    {0x7c, 1, {0x10}},
    {0x7d, 1, {0x11}},
    {0x7e, 1, {0x02}},
    {0x7f, 1, {0x0C}},
    {0x80, 1, {0x0D}},
    {0x81, 1, {0x02}},
    {0x82, 1, {0x0E}},
    {0x83, 1, {0x0F}},
    {0x84, 1, {0x08}},
    {0x85, 1, {0x02}},
    {0x86, 1, {0x02}},
    {0x87, 1, {0x02}},
    {0x88, 1, {0x02}},
    {0x89, 1, {0x02}},
    {0x8A, 1, {0x02}},

    {0xFF, 3, {0x98,0x81,0x04}},
    {0x6C, 1, {0x15}},
    {0x6E, 1, {0x30}},
    {0x6F, 1, {0x37}},
    {0x8D, 1, {0x1F}},
    {0x87, 1, {0xBA}},
    {0x26, 1, {0x76}},
    {0xB2, 1, {0xD1}},
    {0xB5, 1, {0x07}},
    {0x35, 1, {0x17}},
    {0x33, 1, {0x14}},
    {0x31, 1, {0x75}},
    {0x3A, 1, {0x85}},
    {0x3B, 1, {0x98}},
    {0x38, 1, {0x01}},
    {0x39, 1, {0x00}},
    {0x7A, 1, {0x10}},

    {0xFF, 3, {0x98,0x81,0x01}},
    {0x22, 1, {0x0A}},
    {0x31, 1, {0x00}},
    {0x50, 1, {0xCF}},
    {0x51, 1, {0xCA}},
    {0x53, 1, {0x47}},
    {0x55, 1, {0x48}},

    {0x56, 1, {0x00}},
    {0x60, 1, {0x28}},
    {0x2E, 1, {0xC8}},
    {0x34, 1, {0x01}},

    {0xA0, 1, {0x00}},
    {0xA1, 1, {0x06}},
    {0xA2, 1, {0x15}},
    {0xA3, 1, {0x16}},
    {0xA4, 1, {0x19}},
    {0xA5, 1, {0x2C}},
    {0xA6, 1, {0x20}},
    {0xA7, 1, {0x21}},
    {0xA8, 1, {0x73}},
    {0xA9, 1, {0x1A}},
    {0xAA, 1, {0x27}},
    {0xAB, 1, {0x68}},
    {0xAC, 1, {0x19}},
    {0xAD, 1, {0x1A}},
    {0xAE, 1, {0x4E}},
    {0xAF, 1, {0x21}},
    {0xB0, 1, {0x25}},
    {0xB1, 1, {0x52}},
    {0xB2, 1, {0x63}},
    {0xB3, 1, {0x3F}},

    {0xC0, 1, {0x00}},
    {0xC1, 1, {0x18}},
    {0xC2, 1, {0x24}},
    {0xC3, 1, {0x10}},
    {0xC4, 1, {0x12}},
    {0xC5, 1, {0x27}},
    {0xC6, 1, {0x1C}},
    {0xC7, 1, {0x1D}},
    {0xC8, 1, {0x78}},
    {0xC9, 1, {0x1D}},
    {0xCA, 1, {0x28}},
    {0xCB, 1, {0x64}},
    {0xCC, 1, {0x1C}},
    {0xCD, 1, {0x1B}},
    {0xCE, 1, {0x4F}},
    {0xCF, 1, {0x26}},
    {0xD0, 1, {0x2D}},
    {0xD1, 1, {0x50}},
    {0xD2, 1, {0x62}},
    {0xD3, 1, {0x3F}},

    {0xFF, 3, {0x98,0x81,0x02}},
    {0x04, 1, {0x17}},
    {0x05, 1, {0x12}},
    {0x06, 1, {0x40}},
    {0x07, 1, {0x0B}},
    {0xFF, 3, {0x98,0x81,0x00}},
    {0x51, 2, {0x0F,0xF0}},
    {0x53, 1, {0x2C}},
    {0x55, 1, {0x00}},

    {0x35, 1, {0x00}},

    {0x11, 0,{}},
    {REGFLAG_MDELAY, 120, {} },
    {0x29, 0,{}},
    {REGFLAG_MDELAY, 10, {} },
    {0xBD, 7, {0xED,0x23,0x42,0x52,0x52,0x1F,0x00}},
    {0xAC, 1, {0x05}},
};

static void lcm_init(void)
{
    lcd_bl_en(1);
    MDELAY(10);

    lcd_3v3_en(1);
    MDELAY(5);
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
    MDELAY(10);
    lcd_3v3_en(0);
    MDELAY(20);

    lcd_bl_en(0);
}

static void lcm_resume(void)
{
    lcm_init();
}

struct LCM_DRIVER jf717_jl_ili9881c_inx_wxga_ips_8_lcm_drv =
{
    .name           = "jf717_jl_ili9881c_inx_wxga_ips_8",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
