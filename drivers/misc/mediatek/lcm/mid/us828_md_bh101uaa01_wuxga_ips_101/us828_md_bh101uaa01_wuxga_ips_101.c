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


extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_BL_EN;
extern unsigned int GPIO_CTP_TP_RST_EN; //tp reset
extern unsigned int GPIO_CTP_LDO; //TP_VDD1_8  TP_3_3V
#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0


#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util = {0};

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
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

static void avdd_enable(unsigned char enabled)
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


static void tp_rst_en(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_CTP_TP_RST_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_CTP_TP_RST_EN, GPIO_OUT_ZERO);
    }
}

static void tp_pwr_en(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_CTP_LDO, 1);
    }
    else
    {
        lcm_set_gpio_output(GPIO_CTP_LDO, 0);
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

    // params->dsi.vertical_sync_active                            = 8; //2; //4;
    // params->dsi.vertical_backporch                              = 32; //10; //16;
    // params->dsi.vertical_frontporch                             = 179;//5;
    // params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    // params->dsi.horizontal_sync_active                          = 8; // 10; //5;//6;
    // params->dsi.horizontal_backporch                            = 12; //60; //60; //80;
    // params->dsi.horizontal_frontporch                           = 12; //60;
    // params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;
    params->dsi.vertical_sync_active                            = 6; //2; //4;
    params->dsi.vertical_backporch                              = 85; //10; //16;
    params->dsi.vertical_frontporch                             = 26;//5;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 8; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 64; //60; //60; //80;
    params->dsi.horizontal_frontporch                           = 64; //60;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;


    params->dsi.PLL_CLOCK = 520;//467 520
}

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static void __attribute__((unused)) push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
#define REGFLAG_MDELAY		    0xFFFC
#define REGFLAG_UDELAY	        0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW	    0xFFFE
#define REGFLAG_RESET_HIGH	    0xFFFF
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

    {0xFF,1,{0x10}},

    {0xB9,1,{0x01}},

    {0xFF,1,{0x20}},

    {0x18,1,{0x40}},

    {0xFF,1,{0x10}},

    {0xB9,1,{0x02}},

    {0xFF,1,{0xF0}},

    {0xFB,1,{0x01}},

    {0x3A,1,{0x08}},

    {0xFF,1,{0x20}},

    {0xFB,1,{0x01}},

    {0x05,1,{0xC9}},
    {0x06,1,{0xC0}},

    {0x07,1,{0x6E}},
    {0x08,1,{0x50}},

    {0x0D,1,{0x63}},

    {0x0E,1,{0x87}},
    {0x0F,1,{0x69}},

    {0x30,1,{0x10}},

    {0x58,1,{0x40}},
    {0x75,1,{0xA2}},
    {0x88,1,{0x00}},
    {0x89,1,{0x87}},
    {0x8A,1,{0x87}},
    {0x94,1,{0x00}},

    {0x95,1,{0xD7}},
    {0x96,1,{0xD7}},
    {0xF2,1,{0x51}},

    {0xFF,1,{0x24}},

    {0xFB,1,{0x01}},

    {0x00,1,{0x00}},
    {0x01,1,{0x05}},
    {0x02,1,{0x22}},
    {0x03,1,{0x23}},
    {0x04,1,{0x0C}},
    {0x05,1,{0x0C}},
    {0x06,1,{0x0D}},
    {0x07,1,{0x0D}},
    {0x08,1,{0x0E}},
    {0x09,1,{0x0E}},
    {0x0A,1,{0x0F}},
    {0x0B,1,{0x0F}},
    {0x0C,1,{0x10}},
    {0x0D,1,{0x10}},
    {0x0E,1,{0x11}},
    {0x0F,1,{0x11}},
    {0x10,1,{0x12}},
    {0x11,1,{0x12}},
    {0x12,1,{0x13}},
    {0x13,1,{0x13}},
    {0x14,1,{0x04}},
    {0x15,1,{0x00}},
    {0x16,1,{0x00}},
    {0x17,1,{0x05}},
    {0x18,1,{0x24}},
    {0x19,1,{0x25}},
    {0x1A,1,{0x0C}},
    {0x1B,1,{0x0C}},
    {0x1C,1,{0x0D}},
    {0x1D,1,{0x0D}},
    {0x1E,1,{0x0E}},
    {0x1F,1,{0x0E}},
    {0x20,1,{0x0F}},
    {0x21,1,{0x0F}},
    {0x22,1,{0x10}},
    {0x23,1,{0x10}},
    {0x24,1,{0x11}},
    {0x25,1,{0x11}},
    {0x26,1,{0x12}},
    {0x27,1,{0x12}},
    {0x28,1,{0x13}},
    {0x29,1,{0x13}},
    {0x2A,1,{0x04}},
    {0x2B,1,{0x00}},

    {0x2D,1,{0x00}},
    {0x2F,1,{0x0D}},
    {0x30,1,{0x40}},
    {0x33,1,{0x40}},
    {0x34,1,{0x0D}},
    {0x37,1,{0x77}},
    {0x39,1,{0x00}},
    {0x3A,1,{0x01}},
    {0x3B,1,{0x63}},
    {0x3D,1,{0x92}},
    {0xAB,1,{0x77}},

    {0x4D,1,{0x15}},
    {0x4E,1,{0x26}},
    {0x4F,1,{0x37}},
    {0x50,1,{0x48}},
    {0x51,1,{0x84}},
    {0x52,1,{0x73}},
    {0x53,1,{0x62}},
    {0x54,1,{0x51}},
    {0x55,2,{0x8E,0x0E}},
    {0x56,1,{0x08}},
    {0x58,1,{0x21}},
    {0x59,1,{0x70}},
    {0x5A,1,{0x01}},
    {0x5B,1,{0x63}},
    {0x5C,1,{0x00}},
    {0x5D,1,{0x00}},
    {0x5E,2,{0x00,0x10}},

    {0x60,1,{0x96}},
    {0x61,1,{0x80}},
    {0x63,1,{0x70}},

    {0x91,1,{0x44}},
    {0x92,1,{0x78}},
    {0x93,1,{0x1A}},
    {0x94,1,{0x5B}},

    {0x97,1,{0xC2}},

    {0xB6,12,{0x05,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x05,0x05,0x00,0x00}},

    {0xC2,1,{0xC6}},

    {0xC3,1,{0x0A}},
    {0xC4,1,{0x23}},
    {0xC5,1,{0x03}},
    {0xC6,1,{0x59}},
    {0xC7,1,{0x0F}},
    {0xC8,1,{0x17}},
    {0xC9,1,{0x0F}},
    {0xCA,1,{0x17}},

    {0xDB,1,{0x01}},
    {0xDC,1,{0x79}},
    {0xDF,1,{0x01}},
    {0xE0,1,{0x79}},
    {0xE1,1,{0x01}},
    {0xE2,1,{0x79}},
    {0xE3,1,{0x01}},
    {0xE4,1,{0x79}},
    {0xE5,1,{0x01}},
    {0xE6,1,{0x79}},
    {0xEF,1,{0x01}},
    {0xF0,1,{0x79}},

    {0xFF,1,{0x25}},

    {0xFB,1,{0x01}},

    {0x05,1,{0x00}},

    {0x13,1,{0x03}},
    {0x14,1,{0x11}},

    {0x19,1,{0x07}},
    {0x1B,1,{0x11}},

    {0x1E,1,{0x00}},
    {0x1F,1,{0x01}},
    {0x20,1,{0x63}},
    {0x25,1,{0x00}},
    {0x26,1,{0x01}},
    {0x27,1,{0x63}},
    {0x3F,1,{0x80}},
    {0x40,1,{0x08}},
    {0x43,1,{0x00}},
    {0x44,1,{0x01}},
    {0x45,1,{0x63}},
    {0x48,1,{0x01}},
    {0x49,1,{0x63}},

    {0x5B,1,{0x80}},
    {0x5C,1,{0x00}},
    {0x5D,1,{0x01}},
    {0x5E,1,{0x63}},
    {0x61,1,{0x01}},
    {0x62,1,{0x63}},
    {0x67,1,{0x00}},
    {0x68,1,{0x0F}},

    {0xC2,1,{0x40}},

    {0xFF,1,{0x26}},

    {0xFB,1,{0x01}},

    {0x00,1,{0xA1}},

    {0x04,1,{0x28}},

    {0x0A,1,{0xF3}},

    {0x0C,1,{0x13}},
    {0x0D,1,{0x0A}},
    {0x0F,1,{0x0A}},
    {0x11,1,{0x00}},
    {0x12,1,{0x50}},
    {0x13,1,{0x4D}},
    {0x14,1,{0x61}},
    {0x16,1,{0x10}},
    {0x19,1,{0x0A}},
    {0x1A,1,{0x80}},
    {0x1B,1,{0x09}},
    {0x1C,1,{0xC0}},
    {0x1D,1,{0x00}},
    {0x1E,1,{0x78}},
    {0x1F,1,{0x78}},
    {0x20,1,{0x00}},
    {0x21,1,{0x00}},
    {0x24,1,{0x00}},
    {0x25,1,{0x78}},
    {0x2A,1,{0x0A}},
    {0x2B,1,{0x80}},
    {0x2F,1,{0x08}},
    {0x30,1,{0x78}},
    {0x33,1,{0x77}},
    {0x34,1,{0x77}},
    {0x35,1,{0x77}},
    {0x36,1,{0x67}},
    {0x37,1,{0x11}},
    {0x38,1,{0x01}},

    {0x39,1,{0x10}},
    {0x3A,1,{0x78}},

    {0x02,1,{0x31}},
    {0x31,1,{0x08}},
    {0x32,1,{0x90}},

    {0xC9,1,{0x86}},
    {0xA9,1,{0x4C}},
    {0xAA,1,{0x4B}},
    {0xAB,1,{0x4A}},
    {0xAC,1,{0x49}},
    {0xAD,1,{0x48}},
    {0xAE,1,{0x00}},
    {0xAF,1,{0x00}},
    {0xB0,1,{0x00}},
    {0xB1,1,{0x00}},
    {0xB2,1,{0x00}},

    {0xFF,1,{0x27}},

    {0xFB,1,{0x01}},

    {0x56,1,{0x06}},

    {0x58,1,{0x80}},
    {0x59,1,{0x46}},
    {0x5A,1,{0x00}},
    {0x5B,1,{0x13}},
    {0x5C,1,{0x00}},
    {0x5D,1,{0x01}},
    {0x5E,1,{0x20}},
    {0x5F,1,{0x10}},
    {0x60,1,{0x00}},
    {0x61,1,{0x11}},
    {0x62,1,{0x00}},
    {0x63,1,{0x01}},
    {0x64,1,{0x21}},
    {0x65,1,{0x0F}},
    {0x66,1,{0x00}},
    {0x67,1,{0x01}},
    {0x68,1,{0x22}},

    {0xC0,1,{0x00}},

    {0xFF,1,{0x2A}},

    {0xFB,1,{0x01}},

    {0x22,1,{0x2F}},
    {0x23,1,{0x08}},

    {0x24,1,{0x00}},

    {0x25,1,{0x76}},
    {0x27,1,{0x00}},
    {0x28,1,{0x1A}},
    {0x29,1,{0x00}},
    {0x2A,1,{0x1A}},
    {0x2B,1,{0x00}},
    {0x2D,1,{0x1A}},

    {0x64,1,{0x41}},
    {0x6A,1,{0x41}},
    {0x79,1,{0x41}},
    {0x7C,1,{0x41}},
    {0x7F,1,{0x41}},
    {0x82,1,{0x41}},
    {0x85,1,{0x41}},

    {0x97,1,{0x3C}},
    {0x98,1,{0x02}},
    {0x99,1,{0x95}},
    {0x9A,1,{0x06}},
    {0x9B,1,{0x00}},
    {0x9C,1,{0x0B}},
    {0x9D,1,{0x0A}},
    {0x9E,1,{0x90}},

    {0xA2,1,{0x33}},
    {0xA3,1,{0xF0}},
    {0xA4,1,{0x30}},

    {0xE8,1,{0x2A}},

    {0xFF,1,{0x23}},

    {0xFB,1,{0x01}},

    {0x00,1,{0x80}},
    {0x07,1,{0x00}},
    {0x08,1,{0x01}},
    {0x09,1,{0x2C}},
    {0x11,1,{0x01}},
    {0x12,1,{0x68}},
    {0x15,1,{0xE9}},
    {0x16,1,{0x0C}},

    {0xFF,1,{0x2A}},
    {0xFB,1,{0x01}},
    {0xF1,1,{0x03}},

    {0xFF,1,{0xD0}},
    {0xFB,1,{0x01}},
    {0x00,1,{0x33}},


    {0xFF,1,{0x20}},



    {0XFB,1,{0X01}},
    {0XAF,1,{0x02}},

    {0xB0,16,{0x00,0x1E,0x00,0x24,0x00,0x43,0x00,0x5E,0x00,0x76,0x00,0x8B,0x00,0x9E,0x00,0xAF}},
    {0xB1,16,{0x00,0xBF,0x00,0xF5,0x01,0x1C,0x01,0x5A,0x01,0x87,0x01,0xCF,0x02,0x02,0x02,0x04}},
    {0xB2,16,{0x02,0x39,0x02,0x7A,0x02,0xAB,0x02,0xEB,0x03,0x15,0x03,0x49,0x03,0x5B,0x03,0x6D}},
    {0xB3,12,{0x03,0x81,0x03,0x96,0x03,0xAB,0x03,0xBC,0x03,0xBE,0x03,0xC1}},

    {0XFF,1,{0X21}},
    {0XFB,1,{0X01}},

    {0xB0,16,{0x00,0x05,0x00,0x1C,0x00,0x3B,0x00,0x56,0x00,0x6E,0x00,0x83,0x00,0x96,0x00,0xA7}},
    {0xB1,16,{0x00,0xB7,0x00,0xED,0x01,0x14,0x01,0x52,0x01,0x7F,0x01,0xC7,0x01,0xFA,0x01,0xFC}},
    {0xB2,16,{0x02,0x31,0x02,0x72,0x02,0xA3,0x02,0xE3,0x03,0x0D,0x03,0x41,0x03,0x53,0x03,0x65}},
    {0xB3,12,{0x03,0x79,0x03,0x8E,0x03,0xA3,0x03,0xB4,0x03,0xB6,0x03,0xC4}},




    {0xFF,1,{0x10}},
    {0xFB,1,{0x01}},

    {0x35,1,{0x00}},
    {0x51,2,{0x0F,0xFF}},
    {0x53,1,{0x2C}},
    {0x55,1,{0x00}},
    {0xBB,1,{0x13}},
    {0x3B,5,{0x03,0x5B,0x1A,0x04,0x04}},

    {0xFF,1,{0x27}},
    {0xFB,1,{0x01}},
    {0xD1,1,{0x54}},
    {0xD2,1,{0x34}},
    {0xFF,1,{0x10}},
    {0xFF,1,{0x10}},
    {0xFB,1,{0x01}},
 


   

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
    tp_pwr_en(1);
    lcd_power_en(0);
    avdd_enable(0);
    MDELAY(30);

	lcd_power_en(1);
    avdd_enable(1);
	display_bias_enable();
	MDELAY(30);

    tp_rst_en(1);
    MDELAY(10); 
	lcd_reset(1);
	MDELAY(10);

        // tp_rst_en(0);
    // MDELAY(30);
	lcd_reset(0);
	MDELAY(30);

    // tp_rst_en(1);
    // MDELAY(150);
    
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
    tp_rst_en(0);
    lcd_reset(0);
    MDELAY(100);

	display_bias_disable();
	lcd_power_en(0);

    tp_pwr_en(0);
    MDELAY(30);
    avdd_enable(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
	lcm_init();
}

struct LCM_DRIVER us828_md_bh101uaa01_wuxga_ips_101_lcm_drv =
{
    .name			= "us828_md_bh101uaa01_wuxga_ips_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
