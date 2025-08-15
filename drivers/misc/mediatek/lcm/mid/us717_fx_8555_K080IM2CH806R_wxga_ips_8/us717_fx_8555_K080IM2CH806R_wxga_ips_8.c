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

    params->dsi.horizontal_sync_active = 20;
    params->dsi.horizontal_backporch =  20; 
    params->dsi.horizontal_frontporch = 120;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;

    params->dsi.PLL_CLOCK=220; 

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

	{0xee, 1, {0x50}},
	{0xea, 2, {0x85, 0x55}}, 
	{0x30, 1, {0x00}}, 	 	 

 	{0x39, 3, {0x02, 0x07, 0x10}}, 	
 	{0x90, 2, {0xa0, 0x11}}, 
	{0x93, 1, {0x65}},//NEW ADD
	{0x24, 1, {0x20}}, 
	{0x99, 1, {0x00}}, 
	{0x95, 1, {0x74}}, 
 	{0x97, 1, {0x06}},
	{0x79, 1, {0x04}}, 
	{0x7b, 1, {0x00}}, 
 	{0x7a, 1, {0x20}},
    //{0x7d, 1, {0x00}},//NEW 20210601	
	{0x32, 1, {0xd9}}, 

	{0xee, 1, {0x60}},
    {0x21, 1, {0x01}},//NEW ADD
	//{0x25, 1, {0x70}},
    {0x27, 1, {0x22}},//NEW ADD
    {0x28, 1, {0x12}},//NEW ADD
    {0x29, 2, {0x8C, 0x25}},//  
    {0x2C, 1, {0xF9}},//NEW ADD	
	{0x91, 1, {0x11}},//33 
 	{0x8b, 1, {0x90}}, 
	{0x86, 1, {0x20}}, 
 	{0x89, 1, {0x20}}, 
 	{0x8d, 1, {0x44}}, 
 	{0x9a, 1, {0x00}},
 	{0x9c, 1, {0x80}},
 	{0x30, 1, {0x01}}, 
    {0x32, 1, {0xF9}},//NEW ADD
	{0x3B, 1, {0x00}},//NEW ADD
 	{0x42, 1, {0x55}},
 	{0x43, 1, {0x55}},
 	{0x3c, 1, {0x3a}},
 	{0x3d, 1, {0x11}},
 	{0x3e, 1, {0x93}},
    {0x42, 2, {0x55, 0x55}},//NEW ADD	
 	{0x92, 1, {0x11}},
 	{0x93, 1, {0x93}},

 	{0x5a, 5, {0x10, 0x20, 0x28, 0x2f, 0x2d}},
	{0x47, 5, {0x10, 0x20, 0x28, 0x2f, 0x2d}},

 	{0x4c, 5, {0x3c, 0x2d, 0x40, 0x20, 0x21}},
 	{0x5f, 5, {0x3c, 0x2d, 0x40, 0x20, 0x21}}, 

	{0x64, 5, {0x21, 0x06, 0x1b, 0x15, 0x25}},
	{0x51, 5, {0x21, 0x06, 0x1b, 0x15, 0x25}},

	{0x69, 4, {0x2a, 0x38, 0x4c, 0x7f}},
 	{0x56, 4, {0x2a, 0x38, 0x4c, 0x7f}},

	{0xee, 1, {0x70}},  
 	{0x00, 4, {0x01, 0x04, 0x00, 0x01}},  
 	{0x04, 4, {0x08, 0x0b, 0x55, 0x01}},  
 	{0x0c, 2, {0x02,0x90}},//60 20201103
	
  	{0x10, 5, {0x04, 0x07, 0x00, 0x00, 0x00}}, 
 	{0x15, 4, {0x00, 0x04, 0x0d, 0x08}}, 	
	{0x29, 2, {0x02, 0x90}},//60 20201103

	{0x30, 6, {0x12, 0x13, 0x55, 0x1d, 0x1d, 0x02}},
	{0x36, 6, {0x12, 0x13, 0x55, 0x1d, 0x1d, 0x02}},

	{0x60, 5, {0x00, 0x10, 0x14, 0x11, 0x15}},  
 	{0x65, 5, {0x12, 0x16, 0x13, 0x17, 0x3c}},  
	{0x6a, 5, {0x3c, 0x3c, 0x3c, 0x3c, 0x3c}},  
	{0x6f, 5, {0x20, 0x60, 0x04, 0x3c, 0x3c}},
	{0x74, 2, {0x3c, 0x3c}},

	{0x80, 5, {0x00, 0x10, 0x14, 0x11, 0x15}},  
 	{0x85, 5, {0x12, 0x16, 0x13, 0x17, 0x3c}},  
	{0x8a, 5, {0x3c, 0x3c, 0x3c, 0x3c, 0x3c}},  
	{0x8f, 5, {0x20, 0x60, 0x04, 0x3c, 0x3c}},
	{0x94, 2, {0x3c, 0x3c}},

	{0xea, 2, {0x00, 0x00}},
	{0xee, 1, {0x00}},
	//{REGFLAG_DELAY,10,{}},
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

struct LCM_DRIVER us717_fx_8555_K080IM2CH806R_wxga_ips_8_lcm_drv =
{
    .name			= "us717_fx_8555_K080IM2CH806R_wxga_ips_8",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
