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

#define FRAME_WIDTH  										(1200)
#define FRAME_HEIGHT 										(1920)


extern unsigned int GPIO_LCM_3V3_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_1V8_EN;
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
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                      lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                  lcm_util.dsi_write_regs(addr, pdata, byte_nums)
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
        lcm_set_gpio_output(GPIO_LCM_3V3_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_3V3_EN, GPIO_OUT_ZERO);
    }
}

static void lcd_vdd_enable(unsigned char enabled)
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
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

    params->dsi.vertical_sync_active                            = 4; 
    params->dsi.vertical_backporch                              = 16; 
    params->dsi.vertical_frontporch                             = 16;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 24; 
    params->dsi.horizontal_backporch                            = 80; 
    params->dsi.horizontal_frontporch                           = 120; 
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 480;
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
#define REGFLAG_END_OF_TABLE    0xFFFD
#define REGFLAG_RESET_LOW        0xFFFE
#define REGFLAG_RESET_HIGH        0xFFFF
    unsigned int i;
    unsigned int cmd;

    for (i = 0; i < count; i++) {
        cmd = table[i].cmd;
        switch (cmd) {
            // case REGFLAG_MDELAY:
                // MDELAY(table[i].count}},
                // break;
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
                break;
        }
    }
}


static __attribute__((unused)) struct LCM_setting_table init_setting[] = {
/*
{0x10, 0, {} },
{REGFLAG_MDELAY, 20, {} },
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
{0xC1, 1, {0x02}},
{0xC2, 1, {0x06}},
{0xC3, 1, {0x16}},
{0xC4, 1, {0x0E}},
{0xC5, 1, {0x18}},
{0xC6, 1, {0x26}},
{0xC7, 1, {0x32}},
{0xC8, 1, {0x3F}},
{0xC9, 1, {0x3F}},
{0xCA, 1, {0x3F}},
{0xCB, 1, {0x3F}},
{0xCC, 1, {0x3D}},
{0xCD, 1, {0x2F}},
{0xCE, 1, {0x2F}},
{0xCF, 1, {0x2F}},
{0xD0, 1, {0x07}},
{0xD2, 1, {0x00}},
{0xD3, 1, {0x02}},
{0xD4, 1, {0x06}},
{0xD5, 1, {0x12}},
{0xD6, 1, {0x0A}},
{0xD7, 1, {0x14}},
{0xD8, 1, {0x22}},
{0xD9, 1, {0x2E}},
{0xDA, 1, {0x3D}},
{0xDB, 1, {0x3F}},
{0xDC, 1, {0x3F}},
{0xDD, 1, {0x3F}},
{0xDE, 1, {0x3D}},
{0xDF, 1, {0x2F}},
{0xE0, 1, {0x2F}},
{0xE1, 1, {0x2F}},
{0xE2, 1, {0x07}},

{0xB0, 1, {0x07}},//Page 07
{0xB1, 1, {0x18}},
{0xB2, 1, {0x19}},
{0xB3, 1, {0x2E}},
{0xB4, 1, {0x52}},
{0xB5, 1, {0x72}},
{0xB6, 1, {0x8C}},
{0xB7, 1, {0xBD}},
{0xB8, 1, {0xEB}},
{0xB9, 1, {0x47}},
{0xBA, 1, {0x96}},
{0xBB, 1, {0x1E}},
{0xBC, 1, {0x90}},
{0xBD, 1, {0x93}},
{0xBE, 1, {0xFA}},
{0xBF, 1, {0x56}},
{0xC0, 1, {0x8C}},
{0xC1, 1, {0xB7}},
{0xC2, 1, {0xCC}},
{0xC3, 1, {0xDF}},
{0xC4, 1, {0xE8}},
{0xC5, 1, {0xF0}},
{0xC6, 1, {0xF8}},
{0xC7, 1, {0xFA}},
{0xC8, 1, {0xFC}},
{0xC9, 1, {0x00}},
{0xCA, 1, {0x00}},
{0xCB, 1, {0x5A}},
{0xCC, 1, {0xAF}},
{0xCD, 1, {0xFF}},
{0xCE, 1, {0xFF}},

{0xB0, 1, {0x08}},//Page 08
{0xB1, 1, {0x04}},
{0xB2, 1, {0x15}},
{0xB3, 1, {0x2D}},
{0xB4, 1, {0x51}},
{0xB5, 1, {0x72}},
{0xB6, 1, {0x8D}},
{0xB7, 1, {0xBE}},
{0xB8, 1, {0xED}},
{0xB9, 1, {0x4A}},
{0xBA, 1, {0x9A}},
{0xBB, 1, {0x23}},
{0xBC, 1, {0x95}},
{0xBD, 1, {0x98}},
{0xBE, 1, {0xFF}},
{0xBF, 1, {0x59}},
{0xC0, 1, {0x8E}},
{0xC1, 1, {0xB9}},
{0xC2, 1, {0xCD}},
{0xC3, 1, {0xDF}},
{0xC4, 1, {0xE8}},
{0xC5, 1, {0xF0}},
{0xC6, 1, {0xF8}},
{0xC7, 1, {0xFA}},
{0xC8, 1, {0xFC}},
{0xC9, 1, {0x00}},
{0xCA, 1, {0x00}},
{0xCB, 1, {0x5A}},
{0xCC, 1, {0xAF}},
{0xCD, 1, {0xFF}},
{0xCE, 1, {0xFF}},

{0xB0, 1, {0x09}},//Page 09
{0xB1, 1, {0x04}},
{0xB2, 1, {0x2C}},
{0xB3, 1, {0x36}},
{0xB4, 1, {0x53}},
{0xB5, 1, {0x73}},
{0xB6, 1, {0x8E}},
{0xB7, 1, {0xC0}},
{0xB8, 1, {0xEF}},
{0xB9, 1, {0x4C}},
{0xBA, 1, {0x9D}},
{0xBB, 1, {0x25}},
{0xBC, 1, {0x96}},
{0xBD, 1, {0x9A}},
{0xBE, 1, {0x01}},
{0xBF, 1, {0x59}},
{0xC0, 1, {0x8E}},
{0xC1, 1, {0xB9}},
{0xC2, 1, {0xCD}},
{0xC3, 1, {0xDF}},
{0xC4, 1, {0xE8}},
{0xC5, 1, {0xF0}},
{0xC6, 1, {0xF8}},
{0xC7, 1, {0xFA}},
{0xC8, 1, {0xFC}},
{0xC9, 1, {0x00}},
{0xCA, 1, {0x00}},
{0xCB, 1, {0x5A}},
{0xCC, 1, {0xBF}},
{0xCD, 1, {0xFF}},
{0xCE, 1, {0xFF}},

{0xB0, 1, {0x0A}},//Page 0A
{0xB1, 1, {0x18}},
{0xB2, 1, {0x19}},
{0xB3, 1, {0x2E}},
{0xB4, 1, {0x52}},
{0xB5, 1, {0x72}},
{0xB6, 1, {0x8C}},
{0xB7, 1, {0xBD}},
{0xB8, 1, {0xEB}},
{0xB9, 1, {0x47}},
{0xBA, 1, {0x96}},
{0xBB, 1, {0x1E}},
{0xBC, 1, {0x90}},
{0xBD, 1, {0x93}},
{0xBE, 1, {0xFA}},
{0xBF, 1, {0x56}},
{0xC0, 1, {0x8C}},
{0xC1, 1, {0xB7}},
{0xC2, 1, {0xCC}},
{0xC3, 1, {0xDF}},
{0xC4, 1, {0xE8}},
{0xC5, 1, {0xF0}},
{0xC6, 1, {0xF8}},
{0xC7, 1, {0xFA}},
{0xC8, 1, {0xFC}},
{0xC9, 1, {0x00}},
{0xCA, 1, {0x00}},
{0xCB, 1, {0x5A}},
{0xCC, 1, {0xAF}},
{0xCD, 1, {0xFF}},
{0xCE, 1, {0xFF}},

{0xB0, 1, {0x0B}},//Page 0B
{0xB1, 1, {0x04}},
{0xB2, 1, {0x15}},
{0xB3, 1, {0x2D}},
{0xB4, 1, {0x51}},
{0xB5, 1, {0x72}},
{0xB6, 1, {0x8D}},
{0xB7, 1, {0xBE}},
{0xB8, 1, {0xED}},
{0xB9, 1, {0x4A}},
{0xBA, 1, {0x9A}},
{0xBB, 1, {0x23}},
{0xBC, 1, {0x95}},
{0xBD, 1, {0x98}},
{0xBE, 1, {0xFF}},
{0xBF, 1, {0x59}},
{0xC0, 1, {0x8E}},
{0xC1, 1, {0xB9}},
{0xC2, 1, {0xCD}},
{0xC3, 1, {0xDF}},
{0xC4, 1, {0xE8}},
{0xC5, 1, {0xF0}},
{0xC6, 1, {0xF8}},
{0xC7, 1, {0xFA}},
{0xC8, 1, {0xFC}},
{0xC9, 1, {0x00}},
{0xCA, 1, {0x00}},
{0xCB, 1, {0x5A}},
{0xCC, 1, {0xAF}},
{0xCD, 1, {0xFF}},
{0xCE, 1, {0xFF}},

{0xB0, 1, {0x0C}},//Page 0C
{0xB1, 1, {0x04}},
{0xB2, 1, {0x2C}},
{0xB3, 1, {0x36}},
{0xB4, 1, {0x53}},
{0xB5, 1, {0x73}},
{0xB6, 1, {0x8E}},
{0xB7, 1, {0xC0}},
{0xB8, 1, {0xEF}},
{0xB9, 1, {0x4C}},
{0xBA, 1, {0x9D}},
{0xBB, 1, {0x25}},
{0xBC, 1, {0x96}},
{0xBD, 1, {0x9A}},
{0xBE, 1, {0x01}},
{0xBF, 1, {0x59}},
{0xC0, 1, {0x8E}},
{0xC1, 1, {0xB9}},
{0xC2, 1, {0xCD}},
{0xC3, 1, {0xDF}},
{0xC4, 1, {0xE8}},
{0xC5, 1, {0xF0}},
{0xC6, 1, {0xF8}},
{0xC7, 1, {0xFA}},
{0xC8, 1, {0xFC}},
{0xC9, 1, {0x00}},
{0xCA, 1, {0x00}},
{0xCB, 1, {0x5A}},
{0xCC, 1, {0xBF}},
{0xCD, 1, {0xFF}},
{0xCE, 1, {0xFF}},

{0xB0, 1, {0x04}},//Page 04
{0xB5, 1, {0x02}},
{0xB6, 1, {0x01}},
*/
    {0x11, 0, {} },
    {REGFLAG_MDELAY, 180, {} },
    {0x29, 0, {} },
    {REGFLAG_MDELAY, 100, {} },

    {REGFLAG_END_OF_TABLE, 0x00, {} }

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
    MDELAY(120);
}

static void lcm_init(void)
{
    lcd_power_en(1);
    MDELAY(50);
    lcd_vdd_enable(1);
    MDELAY(50);
    lcd_reset(1);
    MDELAY(20);
    lcd_reset(0);
    MDELAY(20);
    lcd_reset(1);
    MDELAY(120);//Must > 5ms
    init_lcm_registers();

}

static void lcm_suspend(void)
{
    unsigned int data_array[16];

    data_array[0]=0x00280500; // Display Off
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10); 

    data_array[0] = 0x00100500; // Sleep In
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);

    MDELAY(150);
    lcd_reset(0);
    MDELAY(100);
    lcd_vdd_enable(0);
    MDELAY(50);
    lcd_power_en(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
    lcm_init();
}

struct LCM_DRIVER us720i20d_dyg_hx8279d_dyg08021039f_wuxganl_ips_8_lcm_drv =
{
    .name            = "us720i20d_dyg_hx8279d_dyg08021039f_wuxganl_ips_8",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
