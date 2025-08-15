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

#define FRAME_WIDTH												(1200)
#define FRAME_HEIGHT											(1920)


extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_TP_RST;
extern unsigned int GPIO_LCM_BIAS_EN;
extern unsigned int GPIO_LCM_BL_EN;
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

static void display_bias_set(unsigned char enabled)
{
	if(enabled){
		lcd_bias_gpio_set(1);
		_lcm_i2c_write_bytes(0x0, 0x11);
		_lcm_i2c_write_bytes(0x1, 0x11);
	}else{		
		_lcm_i2c_write_bytes(0x0, 0x0);
		_lcm_i2c_write_bytes(0x1, 0x0);
		lcd_bias_gpio_set(0);
	}
}

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
	params->dsi.mode               = SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM           = LCM_FOUR_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

    params->dsi.vertical_sync_active                            = 8;
    params->dsi.vertical_backporch                              = 38;
    params->dsi.vertical_frontporch                             = 124;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 8; 
    params->dsi.horizontal_backporch                            = 28;
    params->dsi.horizontal_frontporch                           = 40;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 510;
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

   {0xB9, 3, {0x83,0x10,0x2E}},
    {0xD1, 4, {0x67,0x0C,0xFF,0x05}},
    {0xB1,17, {0x10,0xFA,0xAF,0xAF,0x2B,0x2B,0xC1,0x75,0x39,0x36,0x36,0x36,0x36,0x22,0x21,0x15,0x00}},
    {0xD2, 2, {0x2B,0x2B}},
    {0xB2,16, {0x00,0xB0,0x47,0x80,0x00,0x2C,0x7E,0x2D,0x00,0x00,0x00,0x00,0x15,0x20,0xD7,0x00}},
    {0xBD, 1, {0x03}},
    {0xB2, 1, {0x80}},
    {0xBD, 1, {0x00}},
    {0xB4,16, {0x7E,0x64,0x7E,0x64,0x7E,0x64,0x68,0x50,0x01,0x8E,0x01,0x58,0x00,0xFF,0x00,0xFF}},
    {0xE9, 1, {0xCD}},
    {0xBB, 1, {0x01}},
    {0xE9, 1, {0x00}},
    {0xBF, 3, {0xFC,0x85,0x80}},
    {0xBA, 8, {0x70,0x03,0xA8,0x83,0xF2,0x00,0xC0,0x0D}},
    {0xD3,43, {0x00,0x00,0x00,0x00,0x01,0x04,0x00,0x14,0x0C,0x27,0x27,0x22,0x29,0x29,0x29,0x04,0x04,0x32,0x10,0x27,0x00,0x27,0x32,0x17,0xC0,0x07,0xC0,0x32,0x10,0x24,0x00,0x24,0x00,0x00,0x23,0x44,0x7A,0x68,0x24,0x43,0x7A,0x68,0x0F}},
    {0xE0,46, {0x00,0x04,0x0C,0x13,0x19,0x2A,0x41,0x47,0x4F,0x4C,0x68,0x70,0x79,0x8B,0x8B,0x95,0x9F,0xB2,0xB2,0x57,0x5F,0x69,0x76,0x00,0x04,0x0C,0x13,0x19,0x2A,0x41,0x47,0x4F,0x4C,0x68,0x70,0x79,0x8B,0x8B,0x95,0x9F,0xB2,0xB2,0x57,0x5F,0x69,0x76}},
    {0xCB, 5, {0x00,0x13,0x08,0x02,0x3E}},
    {0xBD, 1, {0x01}},
    {0xB1, 4, {0x01,0x9B,0x01,0x31}},
    {0xCB,10, {0x80,0x36,0x12,0x16,0xC0,0x28,0x40,0x84,0x02,0x34}},
    {0xD3,11, {0x01,0x00,0x7C,0x00,0x00,0x11,0x10,0x00,0x0E,0x00,0x01}},
    {0xBD, 1, {0x02}},
    {0xCB, 5, {0x00,0x03,0x00,0x01,0x8F}},
    {0xB4, 6, {0x4E,0x00,0x33,0x11,0x33,0x88}},
    {0xBF, 3, {0xF2,0x00,0x02}},
    {0xBD, 1, {0x00}},
    {0xC0,14, {0x23,0x23,0x22,0x11,0xA2,0x1F,0x00,0x80,0x00,0x00,0x08,0x00,0x63,0x63}},
    {0xC6, 1, {0xF9}},
    {0xC7, 1, {0x30}},
    {0xC8, 6, {0x00,0x04,0x04,0x00,0x00,0x82}},
    {0xD0, 3, {0x07,0x04,0x05}},
    {0xD5,44, {0x18,0x18,0x18,0x18,0x2D,0x2D,0x21,0x20,0x25,0x24,0x18,0x18,0x39,0x39,0x2E,0x2E,0x03,0x02,0x03,0x02,0x01,0x00,0x01,0x00,0x07,0x06,0x07,0x06,0x05,0x04,0x05,0x04,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}},
    {0xD6,44, {0x18,0x18,0x18,0x18,0x18,0x18,0x20,0x21,0x24,0x25,0x2D,0x2D,0x39,0x39,0x2E,0x2E,0x04,0x05,0x04,0x05,0x06,0x07,0x06,0x07,0x00,0x01,0x00,0x01,0x02,0x03,0x02,0x03,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}},
    {0xE7,23, {0x12,0x13,0x02,0x02,0x4B,0x4B,0x0F,0x0F,0x19,0x1C,0x26,0x74,0x28,0x6B,0x01,0x27,0x00,0x00,0x00,0x00,0x17,0x00,0x68}},
    {0xBD, 1, {0x01}},
    {0xE7, 7, {0x02,0x0C,0x01,0x88,0x0E,0xBC,0x0F}},
    {0xBD, 1, {0x02}},
    {0xE7,28, {0xFF,0x01,0xFD,0x01,0x00,0x00,0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x81,0x00,0x02,0x40}},
    {0xBD, 1, {0x00}},
    {0xBD, 1, {0x02}},
    {0xD8,12, {0xFF,0xFF,0xFF,0xFF,0xFF,0xF0,0xFF,0xFF,0xFF,0xFF,0xFF,0xF0}},
    {0xBD, 1, {0x03}},
    {0xD8,24, {0xAA,0xAA,0xAA,0xAA,0x00,0x00,0xAA,0xAA,0xAA,0xAA,0x00,0x00,0x55,0x55,0x55,0x55,0x00,0x00,0x55,0x55,0x55,0x55,0x00,0x00}},
    {0xBD, 1, {0x00}},
    {0xCC, 1, {0x02}},
    {0xE1, 2, {0x06,0x02}},
    {0xB9, 3, {0x00,0x00,0x00}},

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

	MDELAY(10);
	tp_reset(1);
	MDELAY(10);
	lcd_reset(1);
	MDELAY(20);
	lcd_reset(0);
	MDELAY(50);
	lcd_reset(1);

	MDELAY(120);//Must > 5ms
	init_lcm_registers();
	lcd_bl_en(1);
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
    lcd_bl_en(0);
    MDELAY(20);
	lcd_reset(0);
	MDELAY(50);
	tp_reset(0);
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

struct LCM_DRIVER jf196_boe_hx83102e_tv101wum_wuxga_ips_101_lcm_drv =
{
	.name			= "jf196_boe_hx83102e_tv101wum_wuxga_ips_101",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params		= lcm_get_params,
	.init			= lcm_init,
	.suspend		= lcm_suspend,
	.resume			= lcm_resume,
	//.compare_id	= lcm_compare_id,
};
