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

{0xee, 1, {0x50}},		// page 1
{0xea, 2, {0x85,0x55}},    // 
{0x20, 1, {0x00}},        //
{0x30, 1, {0x00}},        // bist
{0x24, 1, {0xa0}},        // rgb  TE	
{0x56, 1, {0x83}},  			
{0x79, 1, {0x05}},        // zigzag
{0x7a, 1, {0x20}},        // 
{0x7d, 1, {0x00}},        // 
{0x80, 1, {0x10}},        // te v width
{0x90, 2, {0x25,0x40}},   // ss_tp
{0x93, 1, {0xf8}},   // 
{0x95, 1, {0x74}},      //inv 
{0x97, 1, {0x08}},   //   smart gip
{0x99, 1, {0x10}},        // ss_delay

{0xee, 1, {0x60}},
{0x21, 1, {0x00}},	
{0x27, 1, {0x62}},				// vddd
{0x2c, 1, {0xf9}},
{0x29, 1, {0x8a}}, 
{0x30, 1, {0x01}},
{0x31, 1, {0xAf}},
{0x32, 1, {0xdA}},				// vrs_tldo
{0x33, 1, {0x83}},				// dsi_rts<1:0>=10  
{0x34, 1, {0x2f}}, 
//{0x35, 1, {0x10}},				// vrs_tldo
//{0x36, 1, {0x00}},				// dsi_rts<1:0>=10  
//{0x37, 1, {0x00}}, 
{0x3a, 1, {0x24}},				// gas off
{0x3b, 1, {0x00}},				// gip_s3s
{0x3C, 1, {0x29}},				// VCOM  -0.911V 
{0x3d, 2, {0x11,0x93}},		// VGH=16.05V   VGL=-10.39V 
{0x42, 2, {0x5f,0x5f}},	
{0x86, 1, {0x20}},
{0x89, 1, {0x00}},
{0x8a, 1, {0xAA}},
{0x91, 1, {0x44}},
{0x92, 1, {0x33}},
{0x93, 1, {0x9B}},                   // vcsw1=1 vcsw2=0
{0x9a, 1, {0x00}},				// 800 
{0x9b, 2, {0x02,0x80}},			// 1280

//gamma2.5--2022/12/02
{0x47, 5, {0x00,0x05,0x1a,0x2a,0x29}},	//gamma P0.4.8.12.20
{0x5a, 5, {0x00,0x05,0x1a,0x2a,0x29}},	//gamma n 0.4.8.12.20

{0x4c, 5, {0x3a,0x30,0x42,0x20,0x1f}},	//28.44.64.96.128. 
{0x5f, 5, {0x3a,0x30,0x42,0x20,0x1f}},	//28.44.64.96.128. 

{0x51, 5, {0x1d,0x01,0x13,0x0c,0x18}},	//159.191.211.227.235 
{0x64, 5, {0x1d,0x01,0x13,0x0c,0x18}},	//159.191.211.227.235  

{0x56, 4, {0x17,0x29,0x35,0x7f}},	//243.247.251.255
{0x69, 4, {0x17,0x29,0x35,0x7f}},	//243.247.251.255

/*	
//gamma2.2--2022/12/02
{0x47, 5, {0x00,0x05,0x1a,0x2a,0x29}},	//gamma P0.4.8.12.20
{0x5a, 5, {0x00,0x05,0x1a,0x2a,0x29}},	//gamma n 0.4.8.12.20

{0x4c, 5, {0x3f,0x35,0x48,0x26,0x25}},	//28.44.64.96.128. 
{0x5f, 5, {0x3f,0x35,0x48,0x26,0x25}},	//28.44.64.96.128. 

{0x51, 5, {0x22,0x05,0x17,0x0f,0x1d}},	//159.191.211.227.235 
{0x64, 5, {0x22,0x05,0x17,0x0f,0x1d}},	//159.191.211.227.235  

{0x56, 4, {0x1d,0x2b,0x35,0x7f}},	//243.247.251.255
{0x69, 4, {0x1d,0x2b,0x35,0x7f}},	//243.247.251.255
*/

{0xee, 1, {0x70}},  
//STV0 stv1 stv2
{0x00, 4, {0x01,0x07,0x00,0x01}},  
{0x04, 4, {0x05,0x0b,0x55,0x01}},  
{0x08, 4, {0x08,0x0c,0x55,0x01}},  
{0x0c, 2, {0x02,0x02}},//0202

// CYC0 cyc1
{0x10, 5, {0x03,0x07,0x00,0x00,0x00}}, 
{0x15, 4, {0x00,0x15,0x0d,0x08}},   
{0x29, 2, {0x02,0x02}},//d9   02 d0

//gip0-gip21=gipL1-gipL22 (forward scan)
{0x60, 5, {0x3c,0x3c,0x08,0x05,0x04}},
{0x65, 5, {0x17,0x16,0x15,0x14,0x13}},
{0x6a, 5, {0x12,0x11,0x10,0x00,0x01}},
{0x6f, 5, {0x3c,0x3c,0x3c,0x3c,0x3c}},
{0x74, 2, {0x3c,0x3c}},

//gip22-gip43=gipR1-gipR22 (forward scan)
{0x80, 5, {0x3c,0x3c,0x08,0x05,0x04}},
{0x85, 5, {0x17,0x16,0x15,0x14,0x13}},
{0x8a, 5, {0x12,0x11,0x10,0x00,0x01}},
{0x8f, 5, {0x3c,0x3c,0x3c,0x3c,0x3c}},
{0x94, 2, {0x3c,0x3c}},

//{0xee, 1, {0x80}},		// page 4
//{0x23, 4, {0x85,0x55,0x05,0x00}}, // 855504

{0xea, 2, {0x00,0x00}},    // write enable
{0xee, 1, {0x00}},		// ENTER PAGE0

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

struct LCM_DRIVER us868_tyzn_k101_im2kgh04_ips_wxga_101_lcm_drv =
{
    .name            = "us868_tyzn_k101_im2kgh04_ips_wxga_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
