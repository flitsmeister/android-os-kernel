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


extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_1v8_EN;
extern unsigned int GPIO_LCM_RST;
/* extern unsigned int GPIO_LCM_BL_EN;
extern unsigned int GPIO_LCM_BIAS_EN; */
#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0


#define LCM_DSI_CMD_MODE									0

#define REGFLAG_MDELAY          0xFFFC
#define REGFLAG_UDELAY          0xFFFB
#define REGFLAG_END_OF_TABLE    0xFFFD
#define REGFLAG_RESET_LOW       0xFFFE
#define REGFLAG_RESET_HIGH      0xFFFF

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util = {0};

#define UDELAY(n)											(lcm_util.udelay(n))
#define MDELAY(n)											(lcm_util.mdelay(n))

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
        lcm_set_gpio_output(GPIO_LCM_PWR_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_PWR_EN, GPIO_OUT_ZERO);
    }
}

/* static void avdd_enable(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_EN, GPIO_OUT_ZERO);
    }
} */

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

/* static void lcd_ldo_1v8(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_1v8_EN, 0);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_1v8_EN, 1);
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
    params->dsi.mode					= SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

    params->dsi.vertical_sync_active = 4;
    params->dsi.vertical_backporch = 8;
    params->dsi.vertical_frontporch = 30;
    params->dsi.vertical_active_line = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active = 20;
    params->dsi.horizontal_backporch = 20;
    params->dsi.horizontal_frontporch = 20;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 210;

}

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static void __attribute__((unused)) push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{

	unsigned int i;
    unsigned int cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;
		switch (cmd) {
            case REGFLAG_MDELAY :
				MDELAY(table[i].count);
				break;
            case REGFLAG_END_OF_TABLE :
                break;
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);

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
	{0x01,1,{0x26}},
	{0x17,1,{0x00}},
	{0x18,1,{0xD7}},//4.8V
	{0x19,1,{0x01}},//0.3V
	{0x1A,1,{0x00}},
	{0x1B,1,{0xD7}},  //VGMN=0
	{0x1C,1,{0x01}},

	{0x1F,1,{0x70}},	//VGH_REG=16.2V
	{0x20,1,{0x2D}},	//VGL_REG=-12V
	{0x21,1,{0x2D}},	//VGL_REG2=-12V
	{0x22,1,{0x7E}},
	{0x24,1,{0xFD}},

	{0x37,1,{0x19}},	//SS=1,BGR=1

	{0x35,1,{0x28}},	//SAP

	{0x38,1,{0x05}},	//JDT=101 zigzag inversion
	{0x39,1,{0x08}},
	{0x3A,1,{0x12}},
	{0x3C,1,{0x7E}},	//SET EQ3 for TE_H
	{0x3D,1,{0xFF}},	//SET CHGEN_ON, modify 20140806
	{0x3E,1,{0xFF}},	//SET CHGEN_OFF, modify 20140806
	{0x3F,1,{0x7F}},	//SET CHGEN_OFF2, modify 20140806

	{0x40,1,{0x06}},	//RSO=
	{0x41,1,{0xA0}},	//LN=640->1280 line
	{0x43,1,{0x1E}},	//VFP=
	{0x44,1,{0x0B}},	//VBP

	{0x55,1,{0x02}},	//DCDCM=1111
	{0x56,1,{0x01}},
	{0x57,1,{0x6A}},
	{0x58,1,{0x09}},
	{0x59,1,{0x0A}},	//VCL = -2.5V
	{0x5A,1,{0x2E}},	//VGH = 16.2V
	{0x5B,1,{0x1A}},	//VGL = -12V
	{0x5C,1,{0x15}},	//pump clk

	{0x5D,1,{0x7F}},
	{0x5E,1,{0x5D}},
	{0x5F,1,{0x4B}},
	{0x60,1,{0x3E}},
	{0x61,1,{0x3A}},
	{0x62,1,{0x2B}},
	{0x63,1,{0x2F}},
	{0x64,1,{0x19}},
	{0x65,1,{0x32}},
	{0x66,1,{0x31}},
	{0x67,1,{0x31}},
	{0x68,1,{0x4F}},
	{0x69,1,{0x3E}},
	{0x6A,1,{0x47}},
	{0x6B,1,{0x36}},
	{0x6C,1,{0x31}},
	{0x6D,1,{0x24}},
	{0x6E,1,{0x12}},
	{0x6F,1,{0x02}},
	{0x70,1,{0x7F}},
	{0x71,1,{0x5D}},
	{0x72,1,{0x4B}},
	{0x73,1,{0x3E}},
	{0x74,1,{0x3A}},
	{0x75,1,{0x2B}},
	{0x76,1,{0x2F}},
	{0x77,1,{0x19}},
	{0x78,1,{0x32}},
	{0x79,1,{0x31}},
	{0x7A,1,{0x31}},
	{0x7B,1,{0x4F}},
	{0x7C,1,{0x3E}},
	{0x7D,1,{0x47}},
	{0x7E,1,{0x36}},
	{0x7F,1,{0x31}},
	{0x80,1,{0x24}},
	{0x81,1,{0x12}},
	{0x82,1,{0x02}},

	{0xE0,1,{0x02}},

	{0x00,1,{0x52}},//RESET_EVEN
	{0x01,1,{0x55}},//VSSG_EVEN
	{0x02,1,{0x55}},//VSSG_EVEN
	{0x03,1,{0x50}},//STV2_ODD
	{0x04,1,{0x77}},//VDD2_ODD
	{0x05,1,{0x57}},//VDD1_ODD
	{0x06,1,{0x55}},//x
	{0x07,1,{0x4E}},//CK11
	{0x08,1,{0x4C}},//CK9
	{0x09,1,{0x5F}},//x
	{0x0A,1,{0x4A}},//CK7
	{0x0B,1,{0x48}},//CK5
	{0x0C,1,{0x55}},//x
	{0x0D,1,{0x46}},//CK3
	{0x0E,1,{0x44}},//CK1
	{0x0F,1,{0x40}},//STV1_ODD
	{0x10,1,{0x55}},//x
	{0x11,1,{0x55}},//x
	{0x12,1,{0x55}},//x
	{0x13,1,{0x55}},//x
	{0x14,1,{0x55}},//x
	{0x15,1,{0x55}},//x

	{0x16,1,{0x53}},//RESET__EVEN
	{0x17,1,{0x55}},//VSSG_EVEN
	{0x18,1,{0x55}},//VSSG_EVEN
	{0x19,1,{0x51}},//STV2_EVEN
	{0x1A,1,{0x77}},//VDD2_EVEN
	{0x1B,1,{0x57}},//VDD1_EVEN
	{0x1C,1,{0x55}},//x
	{0x1D,1,{0x4F}},//CK12
	{0x1E,1,{0x4D}},//CK10
	{0x1F,1,{0x5F}},//x
	{0x20,1,{0x4B}},//CK8
	{0x21,1,{0x49}},//CK6
	{0x22,1,{0x55}},//x
	{0x23,1,{0x47}},//CK4
	{0x24,1,{0x45}},//CK2
	{0x25,1,{0x41}},//STV1_EVEN
	{0x26,1,{0x55}},//x
	{0x27,1,{0x55}},//x
	{0x28,1,{0x55}},//x
	{0x29,1,{0x55}},//x
	{0x2A,1,{0x55}},//x
	{0x2B,1,{0x55}},//x

	{0x2C,1,{0x13}},//RESET_EVEN
	{0x2D,1,{0x15}},//VSSG_EVEN
	{0x2E,1,{0x15}},//VSSG_EVEN
	{0x2F,1,{0x01}},//STV2_ODD
	{0x30,1,{0x37}},//VDD2_ODD
	{0x31,1,{0x17}},//VDD1_ODD
	{0x32,1,{0x15}},//x
	{0x33,1,{0x0D}},//CK11
	{0x34,1,{0x0F}},//CK9
	{0x35,1,{0x15}},//x
	{0x36,1,{0x05}},//CK7
	{0x37,1,{0x07}},//CK5
	{0x38,1,{0x15}},//x
	{0x39,1,{0x09}},//CK3
	{0x3A,1,{0x0B}},//CK1
	{0x3B,1,{0x11}},//STV1_ODD
	{0x3C,1,{0x15}},//x
	{0x3D,1,{0x15}},//x
	{0x3E,1,{0x15}},//x
	{0x3F,1,{0x15}},//x
	{0x40,1,{0x15}},//x
	{0x41,1,{0x15}},//x

	{0x42,1,{0x12}},//RESET__EVEN
	{0x43,1,{0x15}},//VSSG_EVEN
	{0x44,1,{0x15}},//VSSG_EVEN
	{0x45,1,{0x00}},//STV2_EVEN
	{0x46,1,{0x37}},//VDD2_EVEN
	{0x47,1,{0x17}},//VDD1_EVEN
	{0x48,1,{0x15}},//x
	{0x49,1,{0x0C}},//CK12
	{0x4A,1,{0x0E}},//CK10
	{0x4B,1,{0x15}},//x
	{0x4C,1,{0x04}},//CK8
	{0x4D,1,{0x06}},//CK6
	{0x4E,1,{0x15}},//x
	{0x4F,1,{0x08}},//CK4
	{0x50,1,{0x0A}},//CK2
	{0x51,1,{0x10}},//STV1_EVEN
	{0x52,1,{0x15}},//x
	{0x53,1,{0x15}},//x
	{0x54,1,{0x15}},//x
	{0x55,1,{0x15}},//x
	{0x56,1,{0x15}},//x
	{0x57,1,{0x15}},//x

	{0x58,1,{0x40}},
	{0x5B,1,{0x10}},
	{0x5C,1,{0x06}},//STV_S0
	{0x5D,1,{0x40}},
	{0x5E,1,{0x00}},
	{0x5F,1,{0x00}},
	{0x60,1,{0x40}},//ETV_W
	{0x61,1,{0x03}},
	{0x62,1,{0x04}},
	{0x63,1,{0x6C}},//CKV_ON
	{0x64,1,{0x6C}},//CKV_OFF
	{0x65,1,{0x75}},
	{0x66,1,{0x08}},//ETV_S0
	{0x67,1,{0xB4}}, //ckv_num/ckv_w
	{0x68,1,{0x08}}, //CKV_S0
	{0x69,1,{0x6C}},//CKV_ON
	{0x6A,1,{0x6C}},//CKV_OFF
	{0x6B,1,{0x0C}}, //dummy
	{0x6D,1,{0x00}},//GGND1
	{0x6E,1,{0x00}},//GGND2
	{0x6F,1,{0x88}},
	{0x75,1,{0xBB}},//FLM_EN
	{0x76,1,{0x00}},
	{0x77,1,{0x05}},
	{0x78,1,{0x2A}},//FLM_OFF

	{0xE0,1,{0x04}},
	{0x09,1,{0x11}},
	{0x0E,1,{0x48}},	//Source EQ option
	{0x2D,1,{0x03}},//defult 0x01

	{0xE0,1,{0x00}},
	{0xE6,1,{0x02}},//WD_Timer
	{0xE7,1,{0x06}},//WD_Timer
	
	{0x11,1,{0x00}},
	{REGFLAG_MDELAY, 120, {}},
	{0x29,1,{0x00}},
	{REGFLAG_MDELAY, 20, {}},
	{0x35,1,{0x00}},
    {REGFLAG_MDELAY,20,{}},
	{0xE0,1,{0x00}},
    {REGFLAG_END_OF_TABLE,0x00,{}}
	
};

/* static void init_lcm_registers(void)
{
    unsigned int data_array[16];
	

    data_array[0] = 0x00110500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);

    data_array[0] = 0x00290500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);

} */

static void lcm_init(void)
{

    lcd_power_en(1);
	MDELAY(30);

/* 	lcd_ldo_1v8(1);
    MDELAY(30); */

    lcd_reset(1);
    MDELAY(30);
    lcd_reset(0);
    MDELAY(20);
    lcd_reset(1);
    MDELAY(120);//Must > 5ms
    //init_lcm_registers();
	push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
    MDELAY(50);

}

static void lcm_suspend(void)
{
    unsigned int data_array[16];
    data_array[0]=0x00280500; // Display Off
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(5);

    data_array[0] = 0x00100500; // Sleep In
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(5);

    //avdd_enable(0);
    MDELAY(30);
    lcd_power_en(0);
    MDELAY(30);

    lcd_reset(0);
    MDELAY(30);
}

static void lcm_resume(void)
{
	lcm_init();
}

struct LCM_DRIVER us869_sat_jd9365da_h3_boe_wxga_101_lcm_drv =
{
    .name			= "us869_sat_jd9365da_h3_boe_wxga_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
