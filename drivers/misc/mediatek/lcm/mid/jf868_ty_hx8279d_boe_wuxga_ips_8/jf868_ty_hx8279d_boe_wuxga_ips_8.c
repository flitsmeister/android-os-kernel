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
#define PHYSICAL_WIDTH                                  (108)
#define PHYSICAL_HIGHT                                  (172)

extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;
// extern unsigned int GPIO_LCM_BIAS_EN;

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

/* static void lcd_bias_gpio_set(unsigned char enabled)
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
} */

/* static void display_bias_set(unsigned char enabled)
{
    if(enabled){
        lcd_bias_gpio_set(1);
        _lcm_i2c_write_bytes(0x0, 0x14);
        _lcm_i2c_write_bytes(0x1, 0x14);
    }else{        
        _lcm_i2c_write_bytes(0x0, 0x0);
        _lcm_i2c_write_bytes(0x1, 0x0);
        lcd_bias_gpio_set(0);
    }
} */


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

    params->dsi.vertical_sync_active                            = 10;
    params->dsi.vertical_backporch                              = 25;
    params->dsi.vertical_frontporch                             = 35;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 10;
    params->dsi.horizontal_backporch                            = 60;
    params->dsi.horizontal_frontporch                           = 80;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 484;
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
            // case REGFLAG_MDELAY:
                // MDELAY(table[i].count);
                // break;
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
                break;
        }
    }
}

static __attribute__((unused)) struct LCM_setting_table init_setting[] = {
    {0xB0, 1, {0x05}},//Page 05
    {0xB3, 1, {0x52}},

    {0xB0, 1, {0x01}},//Page 01
    {0xC8, 1, {0x00}},
    {0xC9, 1, {0x00}},
    {0xCC, 1, {0x26}},
    {0xCD, 1, {0x26}},
    {0xDC, 1, {0x00}},
    {0xDD, 1, {0x00}},
    {0xE0, 1, {0x26}},
    {0xE1, 1, {0x26}},

    {0xB0, 1, {0x03}},//Page 03
    {0xC3, 1, {0x2A}},
    {0xE7, 1, {0x2A}},
    {0xC5, 1, {0x2A}},
    {0xDE, 1, {0x2A}},

    {0xB0, 1, {0x00}},//Page 00
    {0xB6, 1, {0x03}},
    {0xBA, 1, {0x8B}},
    {0xBD, 1, {0x75}},
    {0xBF, 1, {0x15}},
    {0xC0, 1, {0x18}},
    {0xC2, 1, {0x14}},
    {0xC3, 1, {0x02}},
    {0xC4, 1, {0x14}},
    {0xC5, 1, {0x02}},

    {0xB0, 1, {0x06}},//Page 06
    {0xC0, 1, {0xA5}},
    {0xD5, 1, {0x20}},
    {0xC0, 1, {0x00}},

    {0xB0, 1, {0x02}},//Page 02
    {0xC0, 1, {0x00}},
    {0xC1, 1, {0x0E}},
    {0xC2, 1, {0x1D}},
    {0xC3, 1, {0x39}},
    {0xC4, 1, {0x3F}},
    {0xC5, 1, {0x3F}},
    {0xC6, 1, {0x3F}},
    {0xC7, 1, {0x3F}},
    {0xC8, 1, {0x3F}},
    {0xC9, 1, {0x3F}},
    {0xCA, 1, {0x3F}},
    {0xCB, 1, {0x3F}},
    {0xCC, 1, {0x3F}},
    {0xCD, 1, {0x3F}},
    {0xCE, 1, {0x3F}},
    {0xCF, 1, {0x3E}},
    {0xD0, 1, {0x07}},
    {0xD2, 1, {0x00}},
    {0xD3, 1, {0x0E}},
    {0xD4, 1, {0x1D}},
    {0xD5, 1, {0x39}},
    {0xD6, 1, {0x3F}},
    {0xD7, 1, {0x3F}},
    {0xD8, 1, {0x3F}},
    {0xD9, 1, {0x3F}},
    {0xDA, 1, {0x3F}},
    {0xDB, 1, {0x3F}},
    {0xDC, 1, {0x3F}},
    {0xDD, 1, {0x3F}},
    {0xDE, 1, {0x3F}},
    {0xDF, 1, {0x3F}},
    {0xE0, 1, {0x3F}},
    {0xE1, 1, {0x3E}},
    {0xE2, 1, {0x07}},

    {0xB0, 1, {0x04}},//Page 04
    {0xB5, 1, {0x02}},
    {0xB6, 1, {0x01}},

    //----gamma 2.2
    {0xB0, 1, {0x07}},//Page 07
    {0xB1, 1, {0x00}},
    {0xB2, 1, {0x06}},
    {0xB3, 1, {0x13}},
    {0xB4, 1, {0x20}},
    {0xB5, 1, {0x2C}},
    {0xB6, 1, {0x36}},
    {0xB7, 1, {0x72}},
    {0xB8, 1, {0xA7}},
    {0xB9, 1, {0x17}},
    {0xBA, 1, {0x7D}},
    {0xBB, 1, {0x25}},
    {0xBC, 1, {0x97}},
    {0xBD, 1, {0x9A}},
    {0xBE, 1, {0xFE}},
    {0xBF, 1, {0x5B}},
    {0xC0, 1, {0x8B}},
    {0xC1, 1, {0xA6}},
    {0xC2, 1, {0xB3}},
    {0xC3, 1, {0xC3}},
    {0xC4, 1, {0xD1}},
    {0xC5, 1, {0xDC}},
    {0xC6, 1, {0xE9}},
    {0xC7, 1, {0xEF}},
    {0xC8, 1, {0xFC}},
    {0xC9, 1, {0x00}},
    {0xCA, 1, {0x00}},
    {0xCB, 1, {0x5A}},
    {0xCC, 1, {0xAF}},
    {0xCD, 1, {0xFF}},
    {0xCE, 1, {0xFF}},


    {0xB0, 1, {0x08}},//Page 08
    {0xB1, 1, {0x00}},
    {0xB2, 1, {0x06}},
    {0xB3, 1, {0x13}},
    {0xB4, 1, {0x20}},
    {0xB5, 1, {0x2C}},
    {0xB6, 1, {0x36}},
    {0xB7, 1, {0x72}},
    {0xB8, 1, {0xA7}},
    {0xB9, 1, {0x17}},
    {0xBA, 1, {0x7D}},
    {0xBB, 1, {0x25}},
    {0xBC, 1, {0x97}},
    {0xBD, 1, {0x9A}},
    {0xBE, 1, {0xFE}},
    {0xBF, 1, {0x5B}},
    {0xC0, 1, {0x8B}},
    {0xC1, 1, {0xA6}},
    {0xC2, 1, {0xB3}},
    {0xC3, 1, {0xC3}},
    {0xC4, 1, {0xD1}},
    {0xC5, 1, {0xDC}},
    {0xC6, 1, {0xE9}},
    {0xC7, 1, {0xEF}},
    {0xC8, 1, {0xFC}},
    {0xC9, 1, {0x00}},
    {0xCA, 1, {0x00}},
    {0xCB, 1, {0x5A}},
    {0xCC, 1, {0xAF}},
    {0xCD, 1, {0xFF}},
    {0xCE, 1, {0xFF}},

    {0xB0, 1, {0x09}},//Page 09
    {0xB1, 1, {0x00}},
    {0xB2, 1, {0x06}},
    {0xB3, 1, {0x13}},
    {0xB4, 1, {0x20}},
    {0xB5, 1, {0x2C}},
    {0xB6, 1, {0x36}},
    {0xB7, 1, {0x72}},
    {0xB8, 1, {0xA7}},
    {0xB9, 1, {0x17}},
    {0xBA, 1, {0x7D}},
    {0xBB, 1, {0x25}},
    {0xBC, 1, {0x97}},
    {0xBD, 1, {0x9A}},
    {0xBE, 1, {0xFE}},
    {0xBF, 1, {0x5B}},
    {0xC0, 1, {0x8B}},
    {0xC1, 1, {0xA6}},
    {0xC2, 1, {0xB3}},
    {0xC3, 1, {0xC3}},
    {0xC4, 1, {0xD1}},
    {0xC5, 1, {0xDC}},
    {0xC6, 1, {0xE9}},
    {0xC7, 1, {0xEF}},
    {0xC8, 1, {0xFC}},
    {0xC9, 1, {0x00}},
    {0xCA, 1, {0x00}},
    {0xCB, 1, {0x5A}},
    {0xCC, 1, {0xAF}},
    {0xCD, 1, {0xFF}},
    {0xCE, 1, {0xFF}},

    {0xB0, 1, {0x0A}},//Page 0A
    {0xB1, 1, {0x00}},
    {0xB2, 1, {0x06}},
    {0xB3, 1, {0x13}},
    {0xB4, 1, {0x20}},
    {0xB5, 1, {0x2C}},
    {0xB6, 1, {0x36}},
    {0xB7, 1, {0x72}},
    {0xB8, 1, {0xA7}},
    {0xB9, 1, {0x17}},
    {0xBA, 1, {0x7D}},
    {0xBB, 1, {0x25}},
    {0xBC, 1, {0x97}},
    {0xBD, 1, {0x9A}},
    {0xBE, 1, {0xFE}},
    {0xBF, 1, {0x5B}},
    {0xC0, 1, {0x8B}},
    {0xC1, 1, {0xA6}},
    {0xC2, 1, {0xB3}},
    {0xC3, 1, {0xC3}},
    {0xC4, 1, {0xD1}},
    {0xC5, 1, {0xDC}},
    {0xC6, 1, {0xE9}},
    {0xC7, 1, {0xEF}},
    {0xC8, 1, {0xFC}},
    {0xC9, 1, {0x00}},
    {0xCA, 1, {0x00}},
    {0xCB, 1, {0x5A}},
    {0xCC, 1, {0xAF}},
    {0xCD, 1, {0xFF}},
    {0xCE, 1, {0xFF}},


    {0xB0, 1, {0x0B}},//Page 0B
    {0xB1, 1, {0x00}},
    {0xB2, 1, {0x06}},
    {0xB3, 1, {0x13}},
    {0xB4, 1, {0x20}},
    {0xB5, 1, {0x2C}},
    {0xB6, 1, {0x36}},
    {0xB7, 1, {0x72}},
    {0xB8, 1, {0xA7}},
    {0xB9, 1, {0x17}},
    {0xBA, 1, {0x7D}},
    {0xBB, 1, {0x25}},
    {0xBC, 1, {0x97}},
    {0xBD, 1, {0x9A}},
    {0xBE, 1, {0xFE}},
    {0xBF, 1, {0x5B}},
    {0xC0, 1, {0x8B}},
    {0xC1, 1, {0xA6}},
    {0xC2, 1, {0xB3}},
    {0xC3, 1, {0xC3}},
    {0xC4, 1, {0xD1}},
    {0xC5, 1, {0xDC}},
    {0xC6, 1, {0xE9}},
    {0xC7, 1, {0xEF}},
    {0xC8, 1, {0xFC}},
    {0xC9, 1, {0x00}},
    {0xCA, 1, {0x00}},
    {0xCB, 1, {0x5A}},
    {0xCC, 1, {0xAF}},
    {0xCD, 1, {0xFF}},
    {0xCE, 1, {0xFF}},

    {0xB0, 1, {0x0C}},//Page 0C
    {0xB1, 1, {0x00}},
    {0xB2, 1, {0x06}},
    {0xB3, 1, {0x13}},
    {0xB4, 1, {0x20}},
    {0xB5, 1, {0x2C}},
    {0xB6, 1, {0x36}},
    {0xB7, 1, {0x72}},
    {0xB8, 1, {0xA7}},
    {0xB9, 1, {0x17}},
    {0xBA, 1, {0x7D}},
    {0xBB, 1, {0x25}},
    {0xBC, 1, {0x97}},
    {0xBD, 1, {0x9A}},
    {0xBE, 1, {0xFE}},
    {0xBF, 1, {0x5B}},
    {0xC0, 1, {0x8B}},
    {0xC1, 1, {0xA6}},
    {0xC2, 1, {0xB3}},
    {0xC3, 1, {0xC3}},
    {0xC4, 1, {0xD1}},
    {0xC5, 1, {0xDC}},
    {0xC6, 1, {0xE9}},
    {0xC7, 1, {0xEF}},
    {0xC8, 1, {0xFC}},
    {0xC9, 1, {0x00}},
    {0xCA, 1, {0x00}},
    {0xCB, 1, {0x5A}},
    {0xCC, 1, {0xAF}},
    {0xCD, 1, {0xFF}},
    {0xCE, 1, {0xFF}},

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
    MDELAY(5);
    // display_bias_set(1);
    MDELAY(10);

    lcd_reset(1);
    MDELAY(20);
    lcd_reset(0);
    MDELAY(50);
    lcd_reset(1);

    MDELAY(120);//Must > 5ms
    init_lcm_registers();
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

    // display_bias_set(0);
    // MDELAY(10);
    lcd_power_en(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
    lcm_init();
}

struct LCM_DRIVER jf868_ty_hx8279d_boe_wuxga_ips_8_lcm_drv =
{
    .name           = "jf868_ty_hx8279d_boe_wuxga_ips_8",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
