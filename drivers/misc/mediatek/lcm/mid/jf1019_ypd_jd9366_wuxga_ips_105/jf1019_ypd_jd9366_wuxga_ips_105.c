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
#define FRAME_HEIGHT											(1340)


extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_BIAS_EN;
extern unsigned int GPIO_TP_RST;

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

static void tp_reset(unsigned char enabled)
{
	if (enabled)
	{
		lcm_set_gpio_output(GPIO_TP_RST, 1);
	}
	else
	{
		lcm_set_gpio_output(GPIO_TP_RST, 0);
	}
}
static void display_bias_set(unsigned char enabled)
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

    params->dsi.vertical_sync_active                            = 8;
    params->dsi.vertical_backporch                              = 20;
    params->dsi.vertical_frontporch                             = 140;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 8;
    params->dsi.horizontal_backporch                            = 28;
    params->dsi.horizontal_frontporch                           = 40;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 249;
	params->dsi.ssc_disable = 1;
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

	{0x30, 1, {0x01}},
	{0x78, 4, {0x49,0x61,0x02,0x00}},
	{0x30, 1, {0x02}},
	{0x31, 1, {0x22}},
	{0x32, 1, {0x08}},
	{0x33, 1, {0x22}},
	{0x3d, 1, {0x64}},
	{0x3e, 1, {0xc0}},
	{0x3f, 1, {0x61}},
	{0x3c, 1, {0x04}},
	{0x42, 1, {0xa2}},
	{0x43, 1, {0xb0}},
	{0x44, 1, {0x21}},
	{0x49, 1, {0xe0}},
	{0x41, 12, {0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03}},
	{0x5a, 11, {0x23,0x00,0x22,0x23,0x20,0x21,0x23,0x0a,0x08,0x0e,0x0c}},
	{0x5b, 11, {0x04,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23}},
	{0x5c, 11, {0x23,0x00,0x22,0x23,0x20,0x21,0x23,0x0b,0x09,0x0f,0x0d}},
	{0x5d, 11, {0x05,0x2c,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23}},
	{0x5e, 11, {0x23,0x00,0x23,0x22,0x20,0x21,0x23,0x09,0x0b,0x0d,0x0f}},
	{0x5f, 11, {0x05,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23}},
	{0x60, 11, {0x23,0x00,0x23,0x22,0x20,0x21,0x23,0x08,0x0a,0x0c,0x0e}},
	{0x61, 11, {0x04,0x2c,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23}},
	{0x4c, 2, {0x20,0x20}},
	{0x4e, 2, {0x7f,0x7f}},
	{0x50, 2, {0x00,0x00}},
	{0x55, 2, {0x80,0x80}},
	{0x56, 2, {0x80,0x80}},
	{0x6f, 3, {0x02,0x00,0x00}},
	{0x70, 3, {0x02,0x00,0x00}},
	{0x71, 3, {0x00,0x00,0x00}},
	{0x72, 3, {0x00,0x00,0x00}},
	{0x30, 1, {0x08}},
	{0x33, 1, {0x05}},
	{0x40, 1, {0x50}},
	{0x41, 1, {0x80}},
	{0x42, 1, {0x10}},
	{0x47, 1, {0x0a}},
	{0x48, 1, {0x0c}},
	{0x50, 1, {0x04}},
	{0x5a, 1, {0x20}},
	{0x5b, 1, {0x3c}},
	{0x5c, 1, {0x53}},
	{0x62, 1, {0x04}},
	{0x65, 1, {0x5f}},
	{0x5e, 1, {0x00}},
	{0x73, 1, {0x01}},
	{0x30, 1, {0x0a}},
	{0x33, 1, {0x30}},
	{0x3f, 1, {0x53}},
	{0x47, 1, {0x20}},
	{0x48, 1, {0x80}},
	{0x49, 1, {0x03}},
	{0x30, 1, {0x0b}},
	{0x33, 2, {0x00,0x83}},
	{0x3c, 2, {0x00,0x83}},
	{0x43, 1, {0xb3}},
	{0x44, 1, {0x33}},
	{0x3e, 5, {0x00,0x05,0x0e,0x15,0x1b}},
	{0x3f, 14, {0x2e,0x45,0x49,0x4e,0x4a,0x60,0x64,0x68,0x77,0x75,0x7a,0x84,0xa0,0xa1}},
	{0x40, 9, {0x4f,0x53,0x64,0x73,0x00,0x05,0x0e,0x15,0x1b}},
	{0x41, 14, {0x2e,0x45,0x49,0x4e,0x4a,0x60,0x64,0x68,0x77,0x75,0x7a,0x84,0xa0,0xa1}},
	{0x42, 4, {0x4f,0x53,0x64,0x73}},
	{0x45, 1, {0x43}},
	{0x46, 1, {0x5b}},
	{0x48, 1, {0x29}},
	{0x49, 1, {0x2c}},
	{0x4a, 1, {0x1b}},
	{0x64, 1, {0x07}},
	{0x30, 1, {0x0c}},
	{0x32, 1, {0x62}},
	{0x62, 1, {0x41}},
	{0x71, 1, {0x77}},
	{0x30, 1, {0x0d}},
	{0x4c, 1, {0x74}},
	{0x30, 1, {0x08}},
	{0x3C, 1, {0x5B}},//bist
	{0x52, 1, {0xc3}},
	{0x54, 1, {0x20}},
	{0x30, 1, {0x00}},

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
	display_bias_set(1);
	MDELAY(10);

	lcd_reset(1);
	MDELAY(20);
	lcd_reset(0);
	MDELAY(50);
	lcd_reset(1);

	MDELAY(120);//Must > 5ms
	init_lcm_registers();
	tp_reset(1);
	MDELAY(10);	
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
	tp_reset(0);
	MDELAY(20);
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

struct LCM_DRIVER jf1019_ypd_jd9366_wuxga_ips_105_lcm_drv =
{
	.name			= "jf1019_ypd_jd9366_wuxga_ips_105",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params		= lcm_get_params,
	.init			= lcm_init,
	.suspend		= lcm_suspend,
	.resume			= lcm_resume,
	//.compare_id	= lcm_compare_id,
};
