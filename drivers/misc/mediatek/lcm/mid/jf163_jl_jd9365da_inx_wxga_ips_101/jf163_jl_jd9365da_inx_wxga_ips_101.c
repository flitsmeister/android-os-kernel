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
#define FRAME_WIDTH												(800)
#define FRAME_HEIGHT											(1280)

extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;

#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0

#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
static struct LCM_UTIL_FUNCS lcm_util = {0};

#define UDELAY(n)												(lcm_util.udelay(n))
#define MDELAY(n)												(lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
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

	params->dsi.vertical_sync_active				= 4;
	params->dsi.vertical_backporch					= 8;
	params->dsi.vertical_frontporch 				= 30;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 
	
	params->dsi.horizontal_sync_active				= 20;	
	params->dsi.horizontal_backporch				= 20;
	params->dsi.horizontal_frontporch				= 20;
	params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;
	
	params->dsi.ssc_disable = 1;
	params->dsi.PLL_CLOCK = 210;
}

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static void __attribute__((unused)) push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
#define REGFLAG_MDELAY			0xFFFC
#define REGFLAG_UDELAY			0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW		0xFFFE
#define REGFLAG_RESET_HIGH		0xFFFF
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
	{0xE0,1,{0x00}},
	{0xE1,1,{0x93}},
	{0xE2,1,{0x65}},
	{0xE3,1,{0xF8}},
	{0x80,1,{0x03}},

	{0xE0,1,{0x01}},
	{0x00,1,{0x00}},
	{0x01,1,{0x24}},
	{0x03,1,{0x00}},
	{0x04,1,{0x30}},
	{0x0C,1,{0x74}},

	{0x17,1,{0x00}},
	{0x18,1,{0xD7}},
	{0x19,1,{0x01}},
	{0x1A,1,{0x00}},
	{0x1B,1,{0xD7}},
	{0x1C,1,{0x01}},

	{0x24,1,{0xFE}},
	{0x37,1,{0x19}},
	{0x35,1,{0x28}},

	{0x38,1,{0x05}},
	{0x39,1,{0x08}},
	{0x3A,1,{0x12}},
	{0x3C,1,{0x78}},
	{0x3D,1,{0xFF}},
	{0x3E,1,{0xFF}},
	{0x3F,1,{0x7F}},

	{0x40,1,{0x06}},
	{0x41,1,{0xA0}},
	{0x43,1,{0x1E}},
	{0x44,1,{0x0B}},

	{0x55,1,{0x02}},
	{0x57,1,{0x6A}},
	{0x59,1,{0x0A}},
	{0x5A,1,{0x2E}},
	{0x5B,1,{0x1A}},
	{0x5C,1,{0x15}},

	{0x5D,1,{0x7F}},
	{0x5E,1,{0x56}},
	{0x5F,1,{0x45}},
	{0x60,1,{0x37}},
	{0x61,1,{0x34}},
	{0x62,1,{0x27}},
	{0x63,1,{0x2B}},
	{0x64,1,{0x16}},
	{0x65,1,{0x30}},
	{0x66,1,{0x2F}},
	{0x67,1,{0x2F}},
	{0x68,1,{0x4E}},
	{0x69,1,{0x3D}},
	{0x6A,1,{0x45}},
	{0x6B,1,{0x36}},
	{0x6C,1,{0x32}},
	{0x6D,1,{0x24}},
	{0x6E,1,{0x13}},
	{0x6F,1,{0x02}},
	{0x70,1,{0x7F}},
	{0x71,1,{0x56}},
	{0x72,1,{0x45}},
	{0x73,1,{0x37}},
	{0x74,1,{0x34}},
	{0x75,1,{0x27}},
	{0x76,1,{0x2B}},
	{0x77,1,{0x16}},
	{0x78,1,{0x30}},
	{0x79,1,{0x2F}},
	{0x7A,1,{0x2F}},
	{0x7B,1,{0x4E}},
	{0x7C,1,{0x3D}},
	{0x7D,1,{0x45}},
	{0x7E,1,{0x36}},
	{0x7F,1,{0x32}},
	{0x80,1,{0x24}},
	{0x81,1,{0x13}},
	{0x82,1,{0x02}},

	{0xE0,1,{0x02}},

	{0x00,1,{0x52}},
	{0x01,1,{0x5F}},
	{0x02,1,{0x5F}},
	{0x03,1,{0x50}},
	{0x04,1,{0x77}},
	{0x05,1,{0x57}},
	{0x06,1,{0x5F}},
	{0x07,1,{0x4E}},
	{0x08,1,{0x4C}},
	{0x09,1,{0x5F}},
	{0x0A,1,{0x4A}},
	{0x0B,1,{0x48}},
	{0x0C,1,{0x5F}},
	{0x0D,1,{0x46}},
	{0x0E,1,{0x44}},
	{0x0F,1,{0x40}},
	{0x10,1,{0x5F}},
	{0x11,1,{0x5F}},
	{0x12,1,{0x5F}},
	{0x13,1,{0x5F}},
	{0x14,1,{0x5F}},
	{0x15,1,{0x5F}},

	{0x16,1,{0x53}},
	{0x17,1,{0x5F}},
	{0x18,1,{0x5F}},
	{0x19,1,{0x51}},
	{0x1A,1,{0x77}},
	{0x1B,1,{0x57}},
	{0x1C,1,{0x5F}},
	{0x1D,1,{0x4F}},
	{0x1E,1,{0x4D}},
	{0x1F,1,{0x5F}},
	{0x20,1,{0x4B}},
	{0x21,1,{0x49}},
	{0x22,1,{0x5F}},
	{0x23,1,{0x47}},
	{0x24,1,{0x45}},
	{0x25,1,{0x41}},
	{0x26,1,{0x5F}},
	{0x27,1,{0x5F}},
	{0x28,1,{0x5F}},
	{0x29,1,{0x5F}},
	{0x2A,1,{0x5F}},
	{0x2B,1,{0x5F}},

	{0x2C,1,{0x13}},
	{0x2D,1,{0x1F}},
	{0x2E,1,{0x1F}},
	{0x2F,1,{0x01}},
	{0x30,1,{0x17}},
	{0x31,1,{0x17}},
	{0x32,1,{0x1F}},
	{0x33,1,{0x0D}},
	{0x34,1,{0x0F}},
	{0x35,1,{0x1F}},
	{0x36,1,{0x05}},
	{0x37,1,{0x07}},
	{0x38,1,{0x1F}},
	{0x39,1,{0x09}},
	{0x3A,1,{0x0B}},
	{0x3B,1,{0x11}},
	{0x3C,1,{0x1F}},
	{0x3D,1,{0x1F}},
	{0x3E,1,{0x1F}},
	{0x3F,1,{0x1F}},
	{0x40,1,{0x1F}},
	{0x41,1,{0x1F}},

	{0x42,1,{0x12}},
	{0x43,1,{0x1F}},
	{0x44,1,{0x1F}},
	{0x45,1,{0x00}},
	{0x46,1,{0x17}},
	{0x47,1,{0x17}},
	{0x48,1,{0x1F}},
	{0x49,1,{0x0C}},
	{0x4A,1,{0x0E}},
	{0x4B,1,{0x1F}},
	{0x4C,1,{0x04}},
	{0x4D,1,{0x06}},
	{0x4E,1,{0x1F}},
	{0x4F,1,{0x08}},
	{0x50,1,{0x0A}},
	{0x51,1,{0x10}},
	{0x52,1,{0x1F}},
	{0x53,1,{0x1F}},
	{0x54,1,{0x1F}},
	{0x55,1,{0x1F}},
	{0x56,1,{0x1F}},
	{0x57,1,{0x1F}},

	{0x58,1,{0x40}},
	{0x5B,1,{0x10}},
	{0x5C,1,{0x06}},
	{0x5D,1,{0x40}},
	{0x5E,1,{0x00}},
	{0x5F,1,{0x00}},
	{0x60,1,{0x40}},
	{0x61,1,{0x03}},
	{0x62,1,{0x04}},
	{0x63,1,{0x58}}, //STV_ON
	{0x64,1,{0x58}}, //STV_OFF
	{0x65,1,{0x75}},
	{0x66,1,{0x08}},
	{0x67,1,{0xB4}},
	{0x68,1,{0x08}},
	{0x69,1,{0x58}}, //CKV_ON
	{0x6A,1,{0x58}}, //CKV_OFF(0x4C~0x58~0x64)
	{0x6B,1,{0x0C}},
	{0x6D,1,{0x00}},
	{0x6E,1,{0x00}},
	{0x6F,1,{0x88}},
	{0x75,1,{0xBB}},
	{0x76,1,{0x00}},
	{0x77,1,{0x05}},
	{0x78,1,{0x2A}},

	{0xE0,1,{0x04}},
	{0x00,1,{0x0E}},
	{0x02,1,{0xB3}},
	{0x09,1,{0x61}},
	{0x0E,1,{0x48}},

	{0xE0,1,{0x00}},
	{0x35,1,{0x00}},
	{0xE0,1,{0x00}},

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
	MDELAY(5);

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

	MDELAY(10);
	lcd_power_en(0);
	MDELAY(20);
}

static void lcm_resume(void)
{
	lcm_init();
}

struct LCM_DRIVER jf163_jl_jd9365da_inx_wxga_ips_101_lcm_drv =
{
	.name			= "jf163_jl_jd9365da_inx_wxga_ips_101",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params		= lcm_get_params,
	.init			= lcm_init,
	.suspend		= lcm_suspend,
	.resume			= lcm_resume,
};
