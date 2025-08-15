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

#define FRAME_WIDTH  										(800)
#define FRAME_HEIGHT 										(1280)

extern unsigned int GPIO_LCM_PWR_EN;//3.3V
extern unsigned int GPIO_LCM_1V8_EN;////1.8 en pin
extern unsigned int GPIO_LCM_RST;
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

    params->dsi.vertical_sync_active = 4;
    params->dsi.vertical_backporch = 12;
    params->dsi.vertical_frontporch = 30;
    params->dsi.vertical_active_line = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active = 20;
    params->dsi.horizontal_backporch = 20;
    params->dsi.horizontal_frontporch = 40;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 220;
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
	{0xE0,1,{0x00}},
	{0xE1,1,{0x93}},
	{0xE2,1,{0x65}},
	{0xE3,1,{0xF8}},
	{0x80,1,{0x03}},

	{0xE0,1,{0x01}},
	{0x00,1,{0x00}},
	{0x01,1,{0x7E}},// 7E VCOM 最佳
	{0x03,1,{0x00}},
	{0x04,1,{0x65}},

	{0x0C,1,{0x74}},

	{0x17,1,{0x00}},
	{0x18,1,{0xB7}},
	{0x19,1,{0x00}},
	{0x1A,1,{0x00}},
	{0x1B,1,{0xB7}},
	{0x1C,1,{0x00}},

	{0x24,1,{0xFE}},

	{0x37,1,{0x19}},

	{0x38,1,{0x05}},
	{0x39,1,{0x00}},
	{0x3A,1,{0x01}},
	{0x3B,1,{0x01}},
	{0x3C,1,{0x70}},
	{0x3D,1,{0xFF}},
	{0x3E,1,{0xFF}},
	{0x3F,1,{0xFF}},

	{0x40,1,{0x06}},
	{0x41,1,{0xA0}},
	{0x43,1,{0x1E}},
	{0x44,1,{0x0F}},
	{0x45,1,{0x28}},
	{0x4B,1,{0x04}},

	{0x55,1,{0x02}},
	{0x56,1,{0x01}},
	{0x57,1,{0xA9}},
	{0x58,1,{0x0A}},
	{0x59,1,{0x0A}},
	{0x5A,1,{0x37}},
	{0x5B,1,{0x19}},

	{0x5D,1,{0x78}},
	{0x5E,1,{0x63}},
	{0x5F,1,{0x54}},
	{0x60,1,{0x49}},
	{0x61,1,{0x45}},
	{0x62,1,{0x38}},
	{0x63,1,{0x3D}},
	{0x64,1,{0x28}},
	{0x65,1,{0x43}},
	{0x66,1,{0x41}},
	{0x67,1,{0x43}},
	{0x68,1,{0x62}},
	{0x69,1,{0x50}},
	{0x6A,1,{0x57}},
	{0x6B,1,{0x49}},
	{0x6C,1,{0x44}},
	{0x6D,1,{0x37}},
	{0x6E,1,{0x23}},
	{0x6F,1,{0x10}},
	{0x70,1,{0x78}},
	{0x71,1,{0x63}},
	{0x72,1,{0x54}},
	{0x73,1,{0x49}},
	{0x74,1,{0x45}},
	{0x75,1,{0x38}},
	{0x76,1,{0x3D}},
	{0x77,1,{0x28}},
	{0x78,1,{0x43}},
	{0x79,1,{0x41}},
	{0x7A,1,{0x43}},
	{0x7B,1,{0x62}},
	{0x7C,1,{0x50}},
	{0x7D,1,{0x57}},
	{0x7E,1,{0x49}},
	{0x7F,1,{0x44}},
	{0x80,1,{0x37}},
	{0x81,1,{0x23}},
	{0x82,1,{0x10}},

	{0xE0,1,{0x02}},
	{0x00,1,{0x47}},
	{0x01,1,{0x47}},
	{0x02,1,{0x45}},
	{0x03,1,{0x45}},
	{0x04,1,{0x4B}},
	{0x05,1,{0x4B}},
	{0x06,1,{0x49}},
	{0x07,1,{0x49}},
	{0x08,1,{0x41}},
	{0x09,1,{0x1F}},
	{0x0A,1,{0x1F}},
	{0x0B,1,{0x1F}},
	{0x0C,1,{0x1F}},
	{0x0D,1,{0x1F}},
	{0x0E,1,{0x1F}},
	{0x0F,1,{0x5F}},
	{0x10,1,{0x5F}},
	{0x11,1,{0x57}},
	{0x12,1,{0x77}},
	{0x13,1,{0x35}},
	{0x14,1,{0x1F}},
	{0x15,1,{0x1F}},

	{0x16,1,{0x46}},
	{0x17,1,{0x46}},
	{0x18,1,{0x44}},
	{0x19,1,{0x44}},
	{0x1A,1,{0x4A}},
	{0x1B,1,{0x4A}},
	{0x1C,1,{0x48}},
	{0x1D,1,{0x48}},
	{0x1E,1,{0x40}},
	{0x1F,1,{0x1F}},
	{0x20,1,{0x1F}},
	{0x21,1,{0x1F}},
	{0x22,1,{0x1F}},
	{0x23,1,{0x1F}},
	{0x24,1,{0x1F}},
	{0x25,1,{0x5F}},
	{0x26,1,{0x5F}},
	{0x27,1,{0x57}},
	{0x28,1,{0x77}},
	{0x29,1,{0x35}},
	{0x2A,1,{0x1F}},
	{0x2B,1,{0x1F}},

	{0x58,1,{0x40}},
	{0x59,1,{0x00}},
	{0x5A,1,{0x00}},
	{0x5B,1,{0x10}},
	{0x5C,1,{0x06}},
	{0x5D,1,{0x40}},
	{0x5E,1,{0x01}},
	{0x5F,1,{0x02}},
	{0x60,1,{0x30}},
	{0x61,1,{0x01}},
	{0x62,1,{0x02}},
	{0x63,1,{0x03}},
	{0x64,1,{0x6B}},
	{0x65,1,{0x05}},
	{0x66,1,{0x0C}},
	{0x67,1,{0x73}},
	{0x68,1,{0x09}},
	{0x69,1,{0x03}},
	{0x6A,1,{0x56}},
	{0x6B,1,{0x08}},
	{0x6C,1,{0x00}},
	{0x6D,1,{0x04}},
	{0x6E,1,{0x04}},
	{0x6F,1,{0x88}},
	{0x70,1,{0x00}},
	{0x71,1,{0x00}},
	{0x72,1,{0x06}},
	{0x73,1,{0x7B}},
	{0x74,1,{0x00}},
	{0x75,1,{0xF8}},
	{0x76,1,{0x00}},
	{0x77,1,{0xD5}},
	{0x78,1,{0x2E}},
	{0x79,1,{0x12}},
	{0x7A,1,{0x03}},
	{0x7B,1,{0x00}},
	{0x7C,1,{0x00}},
	{0x7D,1,{0x03}},
	{0x7E,1,{0x7B}},

	{0xE0,1,{0x04}},
	{0x00,1,{0x0E}},
	{0x02,1,{0xB3}},
	{0x09,1,{0x60}},
	{0x0E,1,{0x2A}},
	{0x36,1,{0x59}},

	{0xE0,1,{0x00}},

	{0x35,1,{0x00}},
	{REGFLAG_END_OF_TABLE,0x00,{}}

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
	init_lcm_registers();
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

struct LCM_DRIVER us868_jl_jd9635da_jlm80b011p21wx_wxga_8_lcm_drv =
{
    .name			= "us868_jl_jd9635da_jlm80b011p21wx_wxga_8",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
