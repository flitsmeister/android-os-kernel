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
extern unsigned int GPIO_LCM_1v8_EN;
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

static void lcd_ldo_1v8(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_1v8_EN, 0);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_1v8_EN, 1);
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

	{0xFF,3,{0x98,0x81,0x03}},

	//GIP_1

	{0x01,1,{0x00}},
	{0x02,1,{0x00}},
	{0x03,1,{0x73}},        //STVA
	{0x04,1,{0xD7}},        //STVB
	{0x05,1,{0x00}},        //STVC
	{0x06,1,{0x08}},        //STVA_Rise
	{0x07,1,{0x11}},        //STVB_Rise
	{0x08,1,{0x00}},        //STVC_Rise
	{0x09,1,{0x3F}},    // STV宽度跟CLK调整一样宽  //FTI1R(A)
	{0x0a,1,{0x00}},        //FTI2R(B)
	{0x0b,1,{0x00}},        //FTI3R(C)
	{0x0c,1,{0x00}},        //FTI1F(A)
	{0x0d,1,{0x00}},        //FTI2F(B)
	{0x0e,1,{0x00}},        //FTI2F(C)
	{0x0f,1,{0x3F}},   //26  Duty=42%  //CLW1(ALR) Duty=45%
	{0x10,1,{0x3F}},   // 26         //CLW2(ARR) Duty=45%
	{0x11,1,{0x00}},           
	{0x12,1,{0x00}},        
	{0x13,1,{0x00}},        //CLWX(ATF)
	{0x14,1,{0x00}},
	{0x15,1,{0x00}},        //GPMRi(ALR)
	{0x16,1,{0x00}},        //GPMRii(ARR)
	{0x17,1,{0x00}},        //GPMFi(ALF)
	{0x18,1,{0x00}},        //GPMFii(AFF)
	{0x19,1,{0x00}},
	{0x1a,1,{0x00}},
	{0x1b,1,{0x00}},   
	{0x1c,1,{0x00}},
	{0x1d,1,{0x00}},
	{0x1e,1,{0x40}},        //CLKA 40笆は C0も笆は(X8把σCLKB)
	{0x1f,1,{0x80}},        //C0
	{0x20,1,{0x06}},        //CLKA_Rise
	{0x21,1,{0x01}},        //CLKA_Fall
	{0x22,1,{0x00}},        //CLKB_Rise(keep toggle惠砞CLK A)
	{0x23,1,{0x00}},        //CLKB_Fall
	{0x24,1,{0x00}},        //CLK keep toggle(AL) 8X┕オ
	{0x25,1,{0x00}},        //CLK keep toggle(AR) 8X┕オ
	{0x26,1,{0x00}},
	{0x27,1,{0x00}},
	{0x28,1,{0x33}},       //CLK Phase
	{0x29,1,{0x33}},       //CLK overlap
	{0x2a,1,{0x00}},  
	{0x2b,1,{0x00}},
	{0x2c,1,{0x00}},       //GCH R
	{0x2d,1,{0x00}},       //GCL R 
	{0x2e,1,{0x00}},       //GCH F        
	{0x2f,1,{0x00}},       //GCL F
	{0x30,1,{0x00}},
	{0x31,1,{0x00}},
	{0x32,1,{0x00}},       //GCH/L ext2/1︽  5E 01:31   5E 00:42
	{0x33,1,{0x00}},
	{0x34,1,{0x00}},       //VDD1&2 non-overlap 04:2.62us
	{0x35,1,{0x00}},       //GCH/L 跋丁 00:VS玡 01:VS 10:阁VS 11:frameい       
	{0x36,1,{0x00}},
	{0x37,1,{0x00}},       //GCH/L
	{0x38,1,{0x00}},	//VDD1&2 toggle 1sec
	{0x39,1,{0x00}},
	{0x3a,1,{0x00}}, 
	{0x3b,1,{0x00}},
	{0x3c,1,{0x00}},
	{0x3d,1,{0x00}},
	{0x3e,1,{0x00}},
	{0x3f,1,{0x00}},
	{0x40,1,{0x00}},
	{0x41,1,{0x00}},
	{0x42,1,{0x00}},
	{0x43,1,{0x00}},       //GCH/L
	{0x44,1,{0x00}},


	//GIP_2
	{0x50,1,{0x01}},
	{0x51,1,{0x23}},
	{0x52,1,{0x44}},
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

	//GIP_3
	{0x5e,1,{0x00}},
	{0x5f,1,{0x0C}},     //FW_CGOUT_L[1]    CLK6
	{0x60,1,{0x0C}},     //FW_CGOUT_L[2]    CLK6
	{0x61,1,{0x0F}},     //FW_CGOUT_L[3]    CLK4
	{0x62,1,{0x0F}},     //FW_CGOUT_L[4]    CLK4
	{0x63,1,{0x0E}},     //FW_CGOUT_L[5]    CLK2
	{0x64,1,{0x0E}},     //FW_CGOUT_L[6]    CLK2
	{0x65,1,{0x06}},     //FW_CGOUT_L[7]    STV2
	{0x66,1,{0x07}},     //FW_CGOUT_L[8]    STV4
	{0x67,1,{0x0D}},     //FW_CGOUT_L[9]    CLK8
	{0x68,1,{0x02}},     //FW_CGOUT_L[10]   
	{0x69,1,{0x02}},     //FW_CGOUT_L[11]     
	{0x6a,1,{0x02}},     //FW_CGOUT_L[12]  
	{0x6b,1,{0x02}},     //FW_CGOUT_L[13]   
	{0x6c,1,{0x02}},     //FW_CGOUT_L[14]      
	{0x6d,1,{0x02}},     //FW_CGOUT_L[15]   
	{0x6e,1,{0x0D}},     //FW_CGOUT_L[16]   CLK8    
	{0x6f,1,{0x02}},     //FW_CGOUT_L[17]   VGL
	{0x70,1,{0x02}},     //FW_CGOUT_L[18]   VGL
	{0x71,1,{0x05}},     //FW_CGOUT_L[19]   VDD
	{0x72,1,{0x01}},     //FW_CGOUT_L[20]   VDS
	{0x73,1,{0x08}},     //FW_CGOUT_L[21]   STV0
	{0x74,1,{0x00}},     //FW_CGOUT_L[22]   VSD
	  
	{0x75,1,{0x0C}},     //BW_CGOUT_L[1]   
	{0x76,1,{0x0C}},     //BW_CGOUT_L[2]    
	{0x77,1,{0x0F}},     //BW_CGOUT_L[3]    
	{0x78,1,{0x0F}},     //BW_CGOUT_L[4]    
	{0x79,1,{0x0E}},     //BW_CGOUT_L[5]     
	{0x7a,1,{0x0E}},     //BW_CGOUT_L[6]     
	{0x7b,1,{0x06}},     //BW_CGOUT_L[7]   
	{0x7c,1,{0x07}},     //BW_CGOUT_L[8]    
	{0x7d,1,{0x0D}},     //BW_CGOUT_L[9]      
	{0x7e,1,{0x02}},     //BW_CGOUT_L[10]
	{0x7f,1,{0x02}},     //BW_CGOUT_L[11]    
	{0x80,1,{0x02}},     //BW_CGOUT_L[12]   
	{0x81,1,{0x02}},     //BW_CGOUT_L[13] 
	{0x82,1,{0x02}},     //BW_CGOUT_L[14]      
	{0x83,1,{0x02}},     //BW_CGOUT_L[15]   
	{0x84,1,{0x0D}},     //BW_CGOUT_L[16]      
	{0x85,1,{0x02}},     //BW_CGOUT_L[17]
	{0x86,1,{0x02}},     //BW_CGOUT_L[18]
	{0x87,1,{0x05}},     //BW_CGOUT_L[19]
	{0x88,1,{0x01}},     //BW_CGOUT_L[20]   
	{0x89,1,{0x08}},     //BW_CGOUT_L[21]   
	{0x8A,1,{0x00}},     //BW_CGOUT_L[22]   



	//CMD_Page 4
	{0xFF,3,{0x98,0x81,0x04}},

	{0x6E,1,{0x3B}},           //VGH 18V
	{0x6F,1,{0x57}},           // reg vcl + pumping ratio VGH=4x VGL=-3x
	{0x3A,1,{0x24}},        //A4     //POWER SAVING
	{0x8D,1,{0x1F}},           //VGL -12V
	{0x87,1,{0xBA}},           //ESD
	{0xB2,1,{0xD1}},
	{0x88,1,{0x0B}},
	{0x38,1,{0x01}},      
	{0x39,1,{0x00}},
	{0xB5,1,{0x07}},           //gamma bias
	{0x31,1,{0x75}},
	{0x3B,1,{0x98}},  			
				
	//CMD_Page 1
	{0xFF,3,{0x98,0x81,0x01}},
	{0x22,1,{0x0A}},         //BGR, SS
	{0x31,1,{0x09}},         //Zig-zag
	{0x35,1,{0x07}},         //
	{0x53,1,{0x7B}},         //VCOM1
	{0x55,1,{0x40}},         //VCOM2 
	{0x50,1,{0x86}},  // 4.3 95          //VREG1OUT 4.5V
	{0x51,1,{0x82}},   //4.3 90          //VREG2OUT -4.5V
	{0x60,1,{0x27}},  //  SDT=2.8 
	{0x62,1,{0x20}},

	//============Gamma START=============

	{0xA0,1,{0x00}},
	{0xA1,1,{0x12}},
	{0xA2,1,{0x20}},
	{0xA3,1,{0x13}},
	{0xA4,1,{0x14}},
	{0xA5,1,{0x27}},
	{0xA6,1,{0x1D}},
	{0xA7,1,{0x1F}},
	{0xA8,1,{0x7C}},
	{0xA9,1,{0x1D}},
	{0xAA,1,{0x2A}},
	{0xAB,1,{0x6B}},
	{0xAC,1,{0x1A}},
	{0xAD,1,{0x18}},
	{0xAE,1,{0x4E}},
	{0xAF,1,{0x24}},
	{0xB0,1,{0x2A}},
	{0xB1,1,{0x4D}},
	{0xB2,1,{0x5B}},
	{0xB3,1,{0x23}},



	//Neg Register
	{0xC0,1,{0x00}},
	{0xC1,1,{0x13}},
	{0xC2,1,{0x20}},
	{0xC3,1,{0x12}},
	{0xC4,1,{0x15}},
	{0xC5,1,{0x28}},
	{0xC6,1,{0x1C}},
	{0xC7,1,{0x1E}},
	{0xC8,1,{0x7B}},
	{0xC9,1,{0x1E}},
	{0xCA,1,{0x29}},
	{0xCB,1,{0x6C}},
	{0xCC,1,{0x1A}},
	{0xCD,1,{0x19}},
	{0xCE,1,{0x4D}},
	{0xCF,1,{0x22}},
	{0xD0,1,{0x2A}},
	{0xD1,1,{0x4D}},
	{0xD2,1,{0x5B}},
	{0xD3,1,{0x23}},
	//============ Gamma END===========			
			
	//CMD_Page 0			
	{0xFF,3,{0x98,0x81,0x00}},
	
    {0x11,1,{0x00}},
	{REGFLAG_MDELAY, 120, {}},
    
    {0x29,1,{0x00}},
	{REGFLAG_MDELAY, 20, {}},

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

    lcd_power_en(1);
	MDELAY(30);
	
	lcd_ldo_1v8(1);
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


struct LCM_DRIVER us717_lhm_ili9881c_inx_wxga_8_lcm_drv = 
{
    .name			= "us717_lhm_ili9881c_inx_wxga_8",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

