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
    params->dsi.LANE_NUM           = LCM_FOUR_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

    params->dsi.vertical_sync_active                            = 6;
    params->dsi.vertical_backporch                              = 15;
    params->dsi.vertical_frontporch                             = 25;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 8; 
    params->dsi.horizontal_backporch                            = 48; 
    params->dsi.horizontal_frontporch                           = 50; 
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 235;
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
{0xFF,3,{0x98,0x81,0x03}},    
//========GIP_1===========//  
{0x01,1,{0x00}},
{0x02,1,{0x00}},
{0x03,1,{0x53}},   //STVA Width 4H
{0x04,1,{0x13}},   //STVB Width 4H
{0x05,1,{0x00}},   
{0x06,1,{0x04}},   //STVA Rise start
{0x07,1,{0x00}},   //STVB Rise start
{0x08,1,{0x00}},   
{0x09,1,{0x22}},   //Detail A&B 4.0us
{0x0a,1,{0x22}},   //Detail A&B 4.0us
{0x0b,1,{0x00}},   
{0x0c,1,{0x01}},   
{0x0d,1,{0x00}},   
{0x0e,1,{0x00}},   
{0x0f,1,{0x23}},   //Detail A&B 4.0us
{0x10,1,{0x23}},   //Detail A&B 4.0us
{0x11,1,{0x00}},
{0x12,1,{0x00}},
{0x13,1,{0x00}},
{0x14,1,{0x00}},
{0x15,1,{0x00}},
{0x16,1,{0x00}},
{0x17,1,{0x00}},
{0x18,1,{0x00}},
{0x19,1,{0x00}},
{0x1a,1,{0x00}},
{0x1b,1,{0x00}},
{0x1c,1,{0x00}},
{0x1d,1,{0x00}},
{0x1e,1,{0x44}},
{0x1f,1,{0x80}},
{0x20,1,{0x02}},   //CLKA Rise STA
{0x21,1,{0x03}},   //CLKA FALL END
{0x22,1,{0x00}},
{0x23,1,{0x00}},
{0x24,1,{0x00}},
{0x25,1,{0x00}},
{0x26,1,{0x00}},
{0x27,1,{0x00}},
{0x28,1,{0x33}},   //CLK_x_Numb[2:0]  Phase_CLK[2:0]
{0x29,1,{0x03}},   //Overlap_CLK[3:0]
{0x2a,1,{0x00}},    
{0x2b,1,{0x00}},    
{0x2c,1,{0x00}},    
{0x2d,1,{0x00}},    
{0x2e,1,{0x00}},    
{0x2f,1,{0x00}},    
{0x30,1,{0x00}},    
{0x31,1,{0x00}},    
{0x32,1,{0x00}},    
{0x33,1,{0x00}},    
{0x34,1,{0x04}},   //GPWR1/2 non overlap time 2.62us
{0x35,1,{0x00}},    
{0x36,1,{0x00}},    
{0x37,1,{0x00}},    
{0x38,1,{0x3C}},   //FOR GPWR1/2 cycle 2 s  
{0x39,1,{0x00}},    
{0x3a,1,{0x40}},    
{0x3b,1,{0x40}},    
{0x3c,1,{0x00}},    
{0x3d,1,{0x00}},    
{0x3e,1,{0x00}},    
{0x3f,1,{0x00}},    
{0x40,1,{0x00}},    
{0x41,1,{0x00}},    
{0x42,1,{0x00}},    
{0x43,1,{0x00}},    
{0x44,1,{0x00}}, 
//========GIP_2===========//   
{0x50,1,{0x01}},    
{0x51,1,{0x23}},    
{0x52,1,{0x45}},    
{0x53,1,{0x67}},    
{0x54,1,{0x89}},     
{0x55,1,{0xab}},    
{0x56,1,{0x01}},    
{0x57,1,{0x23}},    
{0x58,1,{0x45}},    
{0x59,1,{0x67}},    
{0x5a,1,{0x89}},    
{0x5b,1,{0xab}},    
{0x5c,1,{0xcd}},    
{0x5d,1,{0xef}},
//========GIP_3===========//       
{0x5e,1,{0x11}}, 
{0x5f,1,{0x01}},     //FW_GOUT_L1  FW
{0x60,1,{0x00}},     //FW_GOUT_L2  BW
{0x61,1,{0x15}},     //FW_GOUT_L3  GPWR1
{0x62,1,{0x14}},     //FW_GOUT_L4  GPWR2
{0x63,1,{0x0C}},     //FW_GOUT_L5  CLK1_R
{0x64,1,{0x0D}},     //FW_GOUT_L6  CLK2_R
{0x65,1,{0x0E}},     //FW_GOUT_L7  CLK3_R
{0x66,1,{0x0F}},     //FW_GOUT_L8  CLK4_R
{0x67,1,{0x06}},     //FW_GOUT_L9  STV1_R
{0x68,1,{0x02}},    
{0x69,1,{0x02}},    
{0x6a,1,{0x02}},    
{0x6b,1,{0x02}},    
{0x6c,1,{0x02}},    
{0x6d,1,{0x02}},      
{0x6e,1,{0x08}},      //FW_GOUT_L  STV2_R
{0x6f,1,{0x02}},      //FW_GOUT_L  VGL
{0x70,1,{0x02}},      //FW_GOUT_L  VGL
{0x71,1,{0x02}},      //FW_GOUT_L  VGL
{0x72,1,{0x02}},    
{0x73,1,{0x02}},    
{0x74,1,{0x02}},
{0x75,1,{0x01}},      //BW_GOUT_L1  FW
{0x76,1,{0x00}},      //BW_GOUT_L2  BW
{0x77,1,{0x15}},      //BW_GOUT_L3  GPWR1
{0x78,1,{0x14}},      //BW_GOUT_L4  GPWR2
{0x79,1,{0x0C}},      //BW_GOUT_L5  CLK1_R
{0x7a,1,{0x0D}},      //BW_GOUT_L6  CLK2_R
{0x7b,1,{0x0E}},      //BW_GOUT_L7  CLK3_R
{0x7c,1,{0x0F}},      //BW_GOUT_L8  CLK4_R
{0x7D,1,{0x08}},      //BW_GOUT_L9  STV1_R
{0x7E,1,{0x02}},      
{0x7F,1,{0x02}},      
{0x80,1,{0x02}},      
{0x81,1,{0x02}},      
{0x82,1,{0x02}},      
{0x83,1,{0x02}},       
{0x84,1,{0x06}},       //BW_GOUT_L  STV2_R
{0x85,1,{0x02}},       //BW_GOUT_L  VGL
{0x86,1,{0x02}},       //BW_GOUT_L  VGL
{0x87,1,{0x02}},       //BW_GOUT_L  VGL
{0x88,1,{0x02}},    
{0x89,1,{0x02}},    
{0x8A,1,{0x02}},
//CMD_age 4  
{0xFF,3,{0x98,0x81,0x04}},
{0x6C,1,{0x15}},        //Set VCORE voltage =1.5V
{0x6E,1,{0x3B}},        //di_pwr_reg=0 for power mode 2A //VGH clamp 15V
{0x6F,1,{0x43}},        //reg vcl + pumping ratio VGH=4x VGL=-2x 
{0x3A,1,{0x24}},        //POWER SAVING
{0x8D,1,{0x14}},        //-10V 
{0x87,1,{0xBA}},        //ESD               
{0x26,1,{0x76}},                
{0xB2,1,{0xD1}},        
{0xB5,1,{0x27}},        //GMA BIAS
{0x31,1,{0x75}},        //SRC BIAS
{0x30,1,{0x03}},        //SRC OUTPUT BIAS
{0x3B,1,{0x98}},        //PUMP SHIFT CLK
{0x35,1,{0x1f}},        //HZ_opt 0426
{0x33,1,{0x14}},        //Blanking frame ÔO¶¨žéGND        
{0x7A,1,{0x0F}},
{0x38,1,{0x02}},
{0x39,1,{0x00}},
//CMD_age 1 
{0xFF,3,{0x98,0x81,0x01}},
{0x22,1,{0x0A}},		//BGR, SS
{0x31,1,{0x0A}},		//Zig-Zag inversion
{0x35,1,{0x07}},		//Chopper
{0x52,1,{0x00}},		//VCOM1
{0x53,1,{0x54}},		//VCOM1  
{0x54,1,{0x00}},		//VCOM2
{0x55,1,{0x59}},		//VCOM2
{0x50,1,{0x83}},		//VREG1OUT=4.3
{0x51,1,{0x80}},		//VREG2OUT=-4.3
{0x60,1,{0x20}},	        //SDT=2.5
{0x61,1,{0x01}},
{0x62,1,{0x07}},		//Source EQ
{0x63,1,{0x00}},
//GammP     
{0xA0,1,{0x08}},
{0xA1,1,{0x11}},
{0xA2,1,{0x18}},
{0xA3,1,{0x10}},
{0xA4,1,{0x10}},
{0xA5,1,{0x1F}},
{0xA6,1,{0x13}},
{0xA7,1,{0x17}},
{0xA8,1,{0x44}},
{0xA9,1,{0x17}},
{0xAA,1,{0x23}},
{0xAB,1,{0x4B}},
{0xAC,1,{0x21}},
{0xAD,1,{0x23}},
{0xAE,1,{0x58}},
{0xAF,1,{0x2A}},
{0xB0,1,{0x2D}},
{0xB1,1,{0x51}},
{0xB2,1,{0x62}},
{0xB3,1,{0x33}},
//GammN     
{0xC0,1,{0x08}},
{0xC1,1,{0x11}},
{0xC2,1,{0x18}},
{0xC3,1,{0x10}},
{0xC4,1,{0x10}},
{0xC5,1,{0x1F}},
{0xC6,1,{0x13}},
{0xC7,1,{0x17}},
{0xC8,1,{0x44}},
{0xC9,1,{0x17}},
{0xCA,1,{0x23}},
{0xCB,1,{0x4B}},
{0xCC,1,{0x21}},
{0xCD,1,{0x23}},
{0xCE,1,{0x58}},
{0xCF,1,{0x2A}},
{0xD0,1,{0x2D}},
{0xD1,1,{0x51}},
{0xD2,1,{0x62}},
{0xD3,1,{0x33}},
//CMD_Page 0
{0xFF,3,{0x98,0x81,0x00}},
{0x11,1,{0x00}},//sleep out
{REGFLAG_MDELAY, 120, {}},
{0x29,1,{0x00}},	//display on
//{0x35,1,{0x00}},	//TE on
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

struct LCM_DRIVER us717_zy_ili9881c_hsd_wxganl_ips_101_lcm_drv =
{
    .name			= "us717_zy_ili9881c_hsd_wxganl_ips_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
