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

#define FRAME_WIDTH											(800)
#define FRAME_HEIGHT										(1280)


extern unsigned int GPIO_LCM_3V3_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_1V8_EN;
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
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
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

    params->dsi.vertical_sync_active                            = 4; //2; //4;
    params->dsi.vertical_backporch                              = 11; //10; //16;
    params->dsi.vertical_frontporch                             = 17;//5;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 30; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 40; //60; //60; //80;
    params->dsi.horizontal_frontporch                           = 40; //60;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 228;
	params->dsi.ssc_disable = 1;
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
                // MDELAY(table[i].count}},
                // break;
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
                break;
		}
    }
}


static __attribute__((unused)) struct LCM_setting_table init_setting[] = {
//page1

	{0xB9,3,{0xF1,0x12,0x84}},	
	{0xBA,27,{0x33,0x81,0x05,0xF9,0x0E,0x0E,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x44,0x25,0x00,0x91,0x0A,0x00,0x00,0x02,0x4F,0x01,0x00,0x00,0x37}},
	{0xB2,2,{0x40,0x08}},
	{0xCC,1,{0x0B}},
	{0xB8,2,{0xA8,0x03}},
	{0xB3,8,{0x00,0x00,0x00,0x00,0x28,0x28,0x28,0x28}},
	{0xC0,9,{0x73,0x73,0x50,0x50,0x80,0x00,0x08,0x70,0x00}},	
	{0xBF,3,{0x00,0x11,0x82}},	
	{0xBC,1,{0x46}},	
	{0xB4,2,{0x80}},	
	{0xE3,11,{0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x00,0xC0,0x00}},	
	{0xB1,10,{0x23,0x53,0x23,0x2F,0x2F,0x33,0x77,0x04,0xDB,0x0C}},	
	{0xB5,2,{0x09,0x09}},	
	{0xB6,2,{0x84,0x88}},	
	{0xE9,63,{0x02,0x00,0x09,0x05,0x0E,0x80,0x81,0x12,0x31,0x23,0x47,0x0B,0x80,0x36,0x47,0x00,0x00,0x81,0x00,0x00,0x00,0x00,0x00,0x81,0x00,0x00,0x00,0x00,0xF8,0xAB,0x02,0x46,0x08,0x88,0x88,0x84,0x88,0x88,0x88,0xF8,0xAB,0x13,0x57,0x18,0x88,0x88,0x85,0x88,0x88,0x88,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},	
	{0xEA,63,{0x96,0x12,0x01,0x01,0x02,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x8F,0xAB,0x75,0x31,0x58,0x88,0x88,0x81,0x88,0x88,0x88,0x8F,0xAB,0x64,0x20,0x48,0x88,0x88,0x80,0x88,0x88,0x88,0x23,0x10,0x00,0x00,0x01,0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x80,0x81,0x00,0x00,0x00,0x00,0x01,0x0F}},	
	{0xE0,34,{0x00,0x09,0x0D,0x25,0x3B,0x3F,0x36,0x33,0x06,0x0B,0x0B,0x0D,0x10,0x0E,0x11,0x14,0x1B,0x00,0x09,0x0D,0x25,0x3B,0x3F,0x36,0x33,0x06,0x0B,0x0B,0x0D,0x10,0x0E,0x11,0x14,0x1B}},


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

struct LCM_DRIVER us720_sat_sc7705_wxga_ips_8_lcm_drv =
{
    .name			= "us720_sat_sc7705_wxga_ips_8",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
