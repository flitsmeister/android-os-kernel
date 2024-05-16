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


extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;
//extern unsigned int GPIO_LCM_BL_EN; 
extern unsigned int GPIO_LCM_BIAS_EN;
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

static void avdd_enable(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_EN, GPIO_OUT_ZERO);
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
    params->dsi.mode    				= SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

    params->dsi.vertical_sync_active = 4;
    params->dsi.vertical_backporch =  8;
    params->dsi.vertical_frontporch = 8;
    params->dsi.vertical_active_line = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active = 4;
    params->dsi.horizontal_backporch =  132; 
    params->dsi.horizontal_frontporch = 24;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;

    params->dsi.PLL_CLOCK=225; 

   //  params->dsi.vertical_sync_active                            =  16; //2; //4;
   //  params->dsi.vertical_backporch                              = 18; //10; //16;
   //  params->dsi.vertical_frontporch                             = 20;//5; 
   //  params->dsi.vertical_active_line                            = FRAME_HEIGHT; 

   //  params->dsi.horizontal_sync_active                          = 20; // 10; //5;//6;
   //  params->dsi.horizontal_backporch                            = 20; //60; //60; //80;
   //  params->dsi.horizontal_frontporch                           = 120; //60; 
   //  params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

   // // params->dsi.PLL_CLOCK = 231;
   // params->dsi.PLL_CLOCK = 220;

    //params->dsi.cont_clock    = 1;//1
    //params->dsi.ssc_disable = 0;//0

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

 {0xFF, 3, {0x98, 0x81, 0x03} },
    {0x01, 1, {0x00} },
    {0x02, 1, {0x00} },
    {0x03, 1, {0x73} },
    {0x04, 1, {0x00} },
    {0x05, 1, {0x00} },
    {0x06, 1, {0x08} },
    {0x07, 1, {0x00} },
    {0x08, 1, {0x00} },
    {0x09, 1, {0x00} },
    {0x0A, 1, {0x01} },
    {0x0B, 1, {0x01} },
    {0x0C, 1, {0x00} },
    {0x0D, 1, {0x01} },
    {0x0E, 1, {0x01} },
    {0x0F, 1, {0x00} },
    {0x10, 1, {0x00} },
    {0x11, 1, {0x00} },
    {0x12, 1, {0x00} },
    {0x13, 1, {0x1F} },
    {0x14, 1, {0x1F} },
    {0x15, 1, {0x00} },
    {0x16, 1, {0x00} },
    {0x17, 1, {0x00} },
    {0x18, 1, {0x00} },
    {0x19, 1, {0x00} },
    {0x1A, 1, {0x00} },
    {0x1B, 1, {0x00} },
    {0x1C, 1, {0x00} },
    {0x1D, 1, {0x00} },
    {0x1E, 1, {0x40} },
    {0x1F, 1, {0xC0} },
    {0x20, 1, {0x06} },
    {0x21, 1, {0x01} },
    {0x22, 1, {0x06} },
    {0x23, 1, {0x01} },
    {0x24, 1, {0x88} },
    {0x25, 1, {0x88} },
    {0x26, 1, {0x00} },
    {0x27, 1, {0x00} },
    {0x28, 1, {0x3B} },
    {0x29, 1, {0x03} },
    {0x2A, 1, {0x00} },
    {0x2B, 1, {0x00} },
    {0x2C, 1, {0x00} },
    {0x2D, 1, {0x00} },
    {0x2E, 1, {0x00} },
    {0x2F, 1, {0x00} },
    {0x30, 1, {0x00} },
    {0x31, 1, {0x00} },
    {0x32, 1, {0x00} },
    {0x33, 1, {0x00} },
    {0x34, 1, {0x00} },
    {0x35, 1, {0x00} },
    {0x36, 1, {0x00} },
    {0x37, 1, {0x00} },
    {0x38, 1, {0x00} },
    {0x39, 1, {0x00} },
    {0x3A, 1, {0x00} },
    {0x3B, 1, {0x00} },
    {0x3C, 1, {0x00} },
    {0x3D, 1, {0x00} },
    {0x3E, 1, {0x00} },
    {0x3F, 1, {0x00} },
    {0x40, 1, {0x00} },
    {0x41, 1, {0x00} },
    {0x42, 1, {0x00} },
    {0x43, 1, {0x00} },
    {0x44, 1, {0x00} },
    {0x50, 1, {0x01} },
    {0x51, 1, {0x23} },
    {0x52, 1, {0x45} },
    {0x53, 1, {0x67} },
    {0x54, 1, {0x89} },
    {0x55, 1, {0xAB} },
    {0x56, 1, {0x01} },
    {0x57, 1, {0x23} },
    {0x58, 1, {0x45} },
    {0x59, 1, {0x67} },
    {0x5A, 1, {0x89} },
    {0x5B, 1, {0xAB} },
    {0x5C, 1, {0xCD} },
    {0x5D, 1, {0xEF} },
    {0x5E, 1, {0x00} },
    {0x5F, 1, {0x01} },
    {0x60, 1, {0x01} },
    {0x61, 1, {0x06} },
    {0x62, 1, {0x06} },
    {0x63, 1, {0x07} },
    {0x64, 1, {0x07} },
    {0x65, 1, {0x00} },
    {0x66, 1, {0x00} },
    {0x67, 1, {0x02} },
    {0x68, 1, {0x02} },
    {0x69, 1, {0x05} },
    {0x6A, 1, {0x05} },
    {0x6B, 1, {0x02} },
    {0x6C, 1, {0x0D} },
    {0x6D, 1, {0x0D} },
    {0x6E, 1, {0x0C} },
    {0x6F, 1, {0x0C} },
    {0x70, 1, {0x0F} },
    {0x71, 1, {0x0F} },
    {0x72, 1, {0x0E} },
    {0x73, 1, {0x0E} },
    {0x74, 1, {0x02} },
    {0x75, 1, {0x01} },
    {0x76, 1, {0x01} },
    {0x77, 1, {0x06} },
    {0x78, 1, {0x06} },
    {0x79, 1, {0x07} },
    {0x7A, 1, {0x07} },
    {0x7B, 1, {0x00} },
    {0x7C, 1, {0x00} },
    {0x7D, 1, {0x02} },
    {0x7E, 1, {0x02} },
    {0x7F, 1, {0x05} },
    {0x80, 1, {0x05} },
    {0x81, 1, {0x02} },
    {0x82, 1, {0x0D} },
    {0x83, 1, {0x0D} },
    {0x84, 1, {0x0C} },
    {0x85, 1, {0x0C} },
    {0x86, 1, {0x0F} },
    {0x87, 1, {0x0F} },
    {0x88, 1, {0x0E} },
    {0x89, 1, {0x0E} },
    {0x8A, 1, {0x02} },
    {0xFF, 3, {0x98, 0x81, 0x04} },
  //  {0x00, 1, {0x00} }, //3lane
    {0x6C, 1, {0x15} },
    {0x6E, 1, {0x2A} },
    {0x6F, 1, {0x33} },
    {0x8D, 1, {0x1B} },
    {0x87, 1, {0xBA} },
    {0x3A, 1, {0x24} },
    {0x26, 1, {0x76} },
    {0xB2, 1, {0xD1} },
   
    {0xFF, 3, {0x98, 0x81, 0x01} },
    {0x22, 1, {0x0A} },
    {0x31, 1, {0x00} },

    {0x43, 1, {0x66} },
    {0x53, 1, {0x40} },
    {0x50, 1, {0x87} },
    {0x51, 1, {0x82} },
    {0x60, 1, {0x15} },
    {0x61, 1, {0x01} },
    {0x62, 1, {0x0C} },
    {0x63, 1, {0x00} },
    {0xA0, 1, {0x00} },
    {0xA1, 1, {0x13} },
    {0xA2, 1, {0x23} },
    {0xA3, 1, {0x14} },
    {0xA4, 1, {0x16} },
    {0xA5, 1, {0x29} },
    {0xA6, 1, {0x1E} },
    {0xA7, 1, {0x1D} },
    {0xA8, 1, {0x86} },
    {0xA9, 1, {0x1E} },
    {0xAA, 1, {0x29} },
    {0xAB, 1, {0x74} },
    {0xAC, 1, {0x19} },
    {0xAD, 1, {0x17} },
    {0xAE, 1, {0x4B} },
    {0xAF, 1, {0x20} },
    {0xB0, 1, {0x26} },
    {0xB1, 1, {0x4C} },
    {0xB2, 1, {0x5D} },
    {0xB3, 1, {0x3F} },
    {0xC0, 1, {0x00} },
    {0xC1, 1, {0x13} },
    {0xC2, 1, {0x23} },
    {0xC3, 1, {0x14} },
    {0xC4, 1, {0x16} },
    {0xC5, 1, {0x29} },
    {0xC6, 1, {0x1E} },
    {0xC7, 1, {0x1D} },
    {0xC8, 1, {0x86} },
    {0xC9, 1, {0x1E} },
    {0xCA, 1, {0x29} },
    {0xCB, 1, {0x74} },
    {0xCC, 1, {0x19} },
    {0xCD, 1, {0x17} },
    {0xCE, 1, {0x4B} },
    {0xCF, 1, {0x20} },
    {0xD0, 1, {0x26} },
    {0xD1, 1, {0x4C} },
    {0xD2, 1, {0x5D} },
    {0xD3, 1, {0x3F} },
    {0xFF, 3, {0x98, 0x81, 0x00} },

	// {0x11,1,{0x00}},
	// {REGFLAG_MDELAY, 120, {}},
	// {0x29,1,{0x00}},
	// {REGFLAG_MDELAY, 20, {}},
	// {0x35, 1, {0x00} },
	//  {REGFLAG_END_OF_TABLE,0x00,{}}
};

static void init_lcm_registers(void)
{
    unsigned int data_array[16];
	push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
    data_array[0] = 0x00351500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(1); 
    
    data_array[0] = 0x00110500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
    data_array[0] = 0x00290500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);

}

static void lcm_init(void)
{
	// lcd_power_en(1);
	// MDELAY(100);
	// lcd_reset(0);
	// MDELAY(20);
	// lcd_reset(1);
	// MDELAY(20);//Must > 5ms
	// init_lcm_registers();
 //    backlight_enable(1);
    lcd_power_en(0);
    lcd_reset(0);
    avdd_enable(0);
    MDELAY(30);

    lcd_power_en(1);
    MDELAY(30);

    avdd_enable(1);
    MDELAY(30);  

    lcd_reset(1);
    MDELAY(30);    
    lcd_reset(0);
    MDELAY(50);
    lcd_reset(1);
    MDELAY(120);//Must > 5ms

    init_lcm_registers();
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

    avdd_enable(0);
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

struct LCM_DRIVER us717_fx_ili9881_k101_im2byl02_l_wxga_ips_101_lcm_drv =
{
    .name			= "us717_fx_ili9881_k101_im2byl02_l_wxga_ips_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
