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

#define FRAME_WIDTH                                     (1200)
#define FRAME_HEIGHT                                    (1920)
#define PHYSICAL_WIDTH                                  (135)
#define PHYSICAL_HIGHT                                  (216)

extern unsigned int GPIO_LCM_PWR_EN;
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

static void lcd_bias_gpio_set(unsigned char enabled)
{
    printk("lcd_bias_gpio_set:%d\r\n",enabled);
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_EN, 1);
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

    params->dsi.vertical_sync_active                            = 2;
    params->dsi.vertical_backporch                              = 10;
    params->dsi.vertical_frontporch                             = 14;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 24;
    params->dsi.horizontal_backporch                            = 80;
    params->dsi.horizontal_frontporch                           = 60;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 478;
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
    {0xB0, 1, {0x0F}},//Page F
    {0xCD, 1, {0xAA}},
    {0x30, 1, {0x00}},//Page0
    {0x42, 1, {0x15}},
    {0x44, 1, {0x15}},
    {0x32, 1, {0x00}},//0x01:Bist,0x00:normal
    {0x3F, 1, {0x05}},
    {0x40, 1, {0x00}},
    {0x3A, 1, {0x4F}},
    {0x38, 1, {0x00}},
    {0x3D, 1, {0x06}},//VCOM
    {0x30, 1, {0x01}},//Page1
    {0x31, 1, {0x02}},
    {0x32, 1, {0x24}},
    {0x33, 1, {0x20}},
    {0x46, 1, {0x80}},
    {0x3F, 1, {0xA6}},
    {0x35, 1, {0x03}},
    {0x38, 1, {0x86}},
    {0x39, 1, {0x00}},
    {0x3A, 12, {0x3E,0x3E,0x00,0x00,0x08,0x08,0x00,0x00,0x20,0x20,0x3F,0x3F}},
    {0x3B, 11, {0x0B,0x0B,0x0A,0x0A,0x09,0x09,0x08,0x08,0x23,0x23,0x1E}},
    {0x3C, 9, {0x1E,0x23,0x23,0x22,0x22,0x1E,0x1E,0x03,0x03}},
    {0x3D, 11, {0x0B,0x0B,0x0A,0x0A,0x09,0x09,0x08,0x08,0x23,0x23,0x1E}},
    {0x3E, 9, {0x1E,0x23,0x23,0x22,0x22,0x1E,0x1E,0x03,0x03}},
    {0x30, 1, {0x07}},//Page7
    {0x35, 1, {0xB8}},
    {0x36, 1, {0x08}},
    {0x30, 1, {0x08}},//Page8
    {0x33, 1, {0x12}},
    {0x43, 1, {0x1D}},
    {0x45, 1, {0x16}},
    {0x5C, 1, {0x20}},
    {0x30, 1, {0x0A}},//PageA
    {0x4E, 1, {0x02}},
    {0x39, 1, {0x05}},
    {0x3A, 1, {0x1D}},
    {0x30, 1, {0x02}},//Page2
    {0x3A, 10, {0x3F,0x1B,0x15,0x12,0x04,0x06,0x08,0x0F,0x1D,0x28}},
    {0x3B, 7, {0x2D,0x30,0x2F,0x2F,0x20,0x15,0x0C}},
    {0x3C, 10, {0x3F,0x1B,0x15,0x12,0x04,0x06,0x08,0x0F,0x1D,0x28}},
    {0x3D, 7, {0x2D,0x30,0x2F,0x2F,0x20,0x15,0x0C}},
    {0x30, 1, {0x08}},//Page8
    {0x5C, 1, {0x20}},
    {0x5D, 1, {0x00}},
    {0x30, 1, {0x0A}},//PageA
    {0x4C, 1, {0x04}},
    {0x30, 1, {0x0F}},//PageF
    {0x4D, 1, {0x00}},

    {0x11, 0,{}},
    {REGFLAG_MDELAY, 120, {}},
    {0x29, 0,{}},
    {REGFLAG_MDELAY, 10, {}},
};

static void lcm_init(void)
{
    lcd_power_en(1);
    MDELAY(5);
    display_bias_set(1);
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

    display_bias_set(0);
    MDELAY(10);
    lcd_power_en(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
    lcm_init();
}

struct LCM_DRIVER jf868_ol_ek79208ac_wuxga_ips_101_lcm_drv =
{
    .name           = "jf868_ol_ek79208ac_wuxga_ips_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
