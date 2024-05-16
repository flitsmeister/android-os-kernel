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
extern unsigned int GPIO_LCM_BL_EN;
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

static void backlight_enable(unsigned char enabled)
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
	params->dsi.LANE_NUM				= LCM_THREE_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

	params->dsi.vertical_sync_active = 3;
	params->dsi.vertical_backporch =  6;
	params->dsi.vertical_frontporch = 31;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 20;
	params->dsi.horizontal_backporch =	20; 
	params->dsi.horizontal_frontporch = 20;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.PLL_CLOCK=260; 
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

	{0xee,1,{0x50}},				
	{0xea,2,{0x85,0x55}},    
	{0x64,1,{0x44}},        
	{0xb4,1,{0x08}},		    
	{0x9a,1,{0x10}},		    
		{0x9b,1,{0xa4}},		    //  n	ot use power ic  a4/  use power ic  a0
	{0x6c,3,{0x02,0x07,0x10}}, 
	{0xa2,1,{0x80}},		   
	{0x90,5,{0x31,0xf0,0x40,0x44,0x41}},   
	{0xa5,3,{0x09,0x00,0x00}},	
	{0x81,1,{0x55}},        
	{0x95,1,{0x71}},        
	{0x3b,1,{0x33}},        
	{0x3c,1,{0x22}},        
	{0x3d,1,{0x33}},        
	{0x80,1,{0x0b}},        
	{0x82,1,{0x00}},        
	{0x84,1,{0x0b}},        
	{0x27,1,{0x00}},        
	{0x28,1,{0x20}},        
	{0xa0,1,{0xfe}},        
	{0xee,1,{0x60}},     
	{0x50,1,{0xcd}},     
	{0x52,1,{0x77}},     
	{0x60,1,{0xc4}},     
	{0x61,1,{0x0d}},     
	{0xc0,1,{0x00}},     
	{0x6a,1,{0x60}},	   
	{0x69,1,{0x60}},	   
		{0x57,1,{0x28}},	   //VCOM SET	 1f->28-18
	{0xc2,1,{0x01}},	   
	{0xc4,3,{0x07,0x33,0x33}},     
	{0xdc,1,{0x10}},	   
	{0xc3,1,{0x2f}},	   
	{0xc8,1,{0x1a}},	   
	{0x5d,2,{0x52,0x14}}, 
	{0x5b,2,{0x20,0x1f}}, 
	{0xda,2,{0x00,0x71}}, 
	{0x6b,1,{0x05}},	     
	{0x80,5,{0x10,0x2f,0x3a,0x44,0x44}},  
	{0x85,5,{0x50,0x48,0x58,0x38,0x38}},  
	{0x8a,5,{0x3a,0x21,0x38,0x35,0x43}},  
		{0x8f,4,{0x44,0x4e,0x5c,0x71}},  //243.247.251.255
	{0xa0,5,{0x10,0x2f,0x3a,0x44,0x44}},    
	{0xa5,5,{0x50,0x48,0x58,0x38,0x38}}, 
	{0xaa,5,{0x3a,0x21,0x38,0x35,0x43}},
		{0xaf,4,{0x44,0x4e,0x5c,0x71}},  //243.247.251.255
	{0xee,1,{0x70}},     
	{0xb3,7,{0x40,0x00,0x03,0xb2,0xb2,0xb2,0xb2}},     
	{0xee,1,{0x80}},  
	{0x70,5,{0x00,0x00,0x00,0x00,0x10}},
		{0x75,3,{0x00,0x00,0x80}},	
	{0x80,5,{0x00,0x04,0x00,0x00,0x50}},
	{0x85,3,{0x00,0x00,0x80}},
	{0x00,5,{0x03,0x06,0x00,0x12,0x8d}},  
	{0x05,5,{0x08,0x05,0x08,0x00,0x12}},  
	{0x0a,5,{0x8d,0x08,0x08,0x00,0x00}},  
	{0x0f,5,{0x00,0x00,0x00,0x08,0x05}}, 
	{0xa0,5,{0x70,0x70,0x01,0x01,0x71}},  
	{0xa5,5,{0x71,0x71,0x71,0x71,0x71}},  
	{0xaa,5,{0x70,0x70,0x71,0x33,0x33}},  
	{0xaf,5,{0x31,0x31,0x37,0x37,0x35}},
	{0xb4,2,{0x35,0x71}},
	{0xc0,5,{0x70,0x70,0x00,0x00,0x71}},  
	{0xc5,5,{0x71,0x71,0x71,0x71,0x71}},  
	{0xca,5,{0x70,0x70,0x71,0x32,0x32}},  
	{0xcf,5,{0x30,0x30,0x36,0x36,0x34}},
		{0xd4,2,{0x34,0x71}},
	{0xea,2,{0x00,0x00}},    
	{0xee,1,{0x00}},		

	{0x11,1,{0x00}},
	{REGFLAG_MDELAY, 120, {}},
	{0x29,1,{0x00}},
	{REGFLAG_MDELAY, 20, {}},
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
	MDELAY(100);
	lcd_reset(0);
	MDELAY(20);
	lcd_reset(1);
	MDELAY(20);//Must > 5ms
	init_lcm_registers();
    backlight_enable(1);
}

static void lcm_suspend(void)
{
    backlight_enable(0);
    MDELAY(150);
    lcd_reset(0);
    MDELAY(100);
	lcd_power_en(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
	lcm_init();
}

struct LCM_DRIVER us717_cc_gh8555al_boe_wuxga_ips_101_lcm_drv =
{
    .name			= "us717_cc_gh8555al_boe_wuxga_ips_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
