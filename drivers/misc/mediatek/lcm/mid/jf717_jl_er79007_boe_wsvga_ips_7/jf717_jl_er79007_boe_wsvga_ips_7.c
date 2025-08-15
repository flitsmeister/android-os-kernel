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

#define FRAME_WIDTH                                     (1024)
#define FRAME_HEIGHT                                    (600)
#define PHYSICAL_WIDTH                                  (154)
#define PHYSICAL_HIGHT                                  (86)

extern unsigned int GPIO_LCM_BL_EN;
extern unsigned int GPIO_LCM_3V3_EN;
extern unsigned int GPIO_LCM_1V8_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_BIAS_EN;
extern unsigned int GPIO_LCM_DSI_TE_EN;
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

/* static void lcd_3v3_en(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_3V3_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_3V3_EN, GPIO_OUT_ZERO);
    }
} */

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
        lcm_set_gpio_output(GPIO_LCM_RST, GPIO_OUT_ZERO);
    }
}

static void lcd_te_en(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_DSI_TE_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_DSI_TE_EN, GPIO_OUT_ZERO);
    }
}

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
        lcm_set_gpio_output(GPIO_LCM_BIAS_EN, GPIO_OUT_ZERO);
    }
}
/* 
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

    params->dsi.mode               = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM           = LCM_FOUR_LANE;
    params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

    params->dsi.vertical_sync_active                            = 1;
    params->dsi.vertical_backporch                              = 23;
    params->dsi.vertical_frontporch                             = 12;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 10;
    params->dsi.horizontal_backporch                            = 160;
    params->dsi.horizontal_frontporch                           = 160;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 160;
    // params->dsi.ssc_disable = 1;
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
    {0x80, 1, {0x8B}},
    {0x81, 1, {0xFF}},
    {0x82, 1, {0xAF}},
    {0x83, 1, {0xDF}},
    {0x84, 1, {0x97}},
    {0x85, 1, {0x9C}},
    {0x86, 1, {0xB9}},

    {0x11, 0,{}},
    {REGFLAG_MDELAY, 120, {} },
    {0x29, 0,{}},
    {REGFLAG_MDELAY, 10, {} },
};

static void lcm_init(void)
{
    lcd_bl_en(1);
    MDELAY(10);

    lcd_1v8_en(1);
    MDELAY(10);

    lcd_te_en(1);
    MDELAY(10);

    lcd_reset(1);
    MDELAY(20);
    lcd_reset(0);
    MDELAY(30);
    lcd_reset(1);

    MDELAY(30);//Must > 5ms
    push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);

    MDELAY(20);
    lcd_bias_gpio_set(1);
    MDELAY(20);
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

    lcd_bias_gpio_set(0);
    MDELAY(20);

    lcd_reset(0);
    MDELAY(20);

    lcd_te_en(0);
    MDELAY(20);

    lcd_1v8_en(0);
    MDELAY(10);

    lcd_bl_en(0);
}

static void lcm_resume(void)
{
    lcm_init();
}

struct LCM_DRIVER jf717_jl_er79007_boe_wsvga_ips_7_lcm_drv =
{
    .name           = "jf717_jl_er79007_boe_wsvga_ips_7",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
