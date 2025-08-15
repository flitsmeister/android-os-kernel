

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#else
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#endif

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(800)
#define FRAME_HEIGHT 										(1280)

#define REGFLAG_DELAY             							0xFFE
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util;

//#define GPIO_LCM_PWREN                                      (GPIO122 |0x80000000)
//#define GPIO_LCM_EN                                         (GPIO26 | 0x80000000)
//#define GPIO_LCM_RST                                        (GPIO83 | 0x80000000)
extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_BL_EN;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))
#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
/* #include <linux/delay.h> */
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#endif



static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	if (GPIO == 0xFFFFFFFF) {		
		return;
	}
	gpio_direction_output(GPIO, output);
	gpio_set_value(GPIO, (output > 0) ? GPIO_OUT_ONE : GPIO_OUT_ZERO);
}

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
struct LCM_setting_table {
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = 
{
	{0xFF, 3, {0x98, 0x81, 0x03}},
	
	//GIP_1
	{0x01, 1, {0x00}},
	{0x02, 1, {0x00}},
	{0x03, 1, {0x53}},        
	{0x04, 1, {0x13}},        
	{0x05, 1, {0x00}},        
	{0x06, 1, {0x04}},        
	{0x07, 1, {0x00}},      
	{0x08, 1, {0x00}},       
	{0x09, 1, {0x22}},   
	{0x0a, 1, {0x22}},       
	{0x0b, 1, {0x00}},        
	{0x0c, 1, {0x01}},      
	{0x0d, 1, {0x00}},        
	{0x0e, 1, {0x00}},       
	{0x0f, 1, {0x25}},
	{0x10, 1, {0x25}},
	{0x11, 1, {0x00}},           
	{0x12, 1, {0x00}},        
	{0x13, 1, {0x00}},      
	{0x14, 1, {0x00}},
	{0x15, 1, {0x00}},        
	{0x16, 1, {0x00}},       
	{0x17, 1, {0x00}},        
	{0x18, 1, {0x00}},       
	{0x19, 1, {0x00}},
	{0x1a, 1, {0x00}},
	{0x1b, 1, {0x00}},   
	{0x1c, 1, {0x00}},
	{0x1d, 1, {0x00}},
	{0x1e, 1, {0x44}},        
	{0x1f, 1, {0x80}},       
	{0x20, 1, {0x02}},        //CLKA_Rise
	{0x21, 1, {0x03}},        //CLKA_Fall
	{0x22, 1, {0x00}},        
	{0x23, 1, {0x00}},        
	{0x24, 1, {0x00}},
	{0x25, 1, {0x00}},
	{0x26, 1, {0x00}},
	{0x27, 1, {0x00}},
	{0x28, 1, {0x33}},      
	{0x29, 1, {0x03}},       
	{0x2a, 1, {0x00}},  
	{0x2b, 1, {0x00}},
	{0x2c, 1, {0x00}},      
	{0x2d, 1, {0x00}},       
	{0x2e, 1, {0x00}},            
	{0x2f, 1, {0x00}},    
	{0x30, 1, {0x00}},
	{0x31, 1, {0x00}},
	{0x32, 1, {0x00}},      
	{0x33, 1, {0x00}},
	{0x34, 1, {0x04}},       //GPWR1/2 non overlap time 2.62us
	{0x35, 1, {0x00}},             
	{0x36, 1, {0x00}},
	{0x37, 1, {0x00}},       
	{0x38, 1, {0x3C}},	//FOR GPWR1/2 cycle 2 s  
	{0x39, 1, {0x00}},
	{0x3a, 1, {0x40}}, 
	{0x3b, 1, {0x40}},
	{0x3c, 1, {0x00}},
	{0x3d, 1, {0x00}},
	{0x3e, 1, {0x00}},
	{0x3f, 1, {0x00}},
	{0x40, 1, {0x00}},
	{0x41, 1, {0x00}},
	{0x42, 1, {0x00}},
	{0x43, 1, {0x00}},      
	{0x44, 1, {0x00}},

	//GIP_2
	{0x50, 1, {0x01}},
	{0x51, 1, {0x23}},
	{0x52, 1, {0x45}},
	{0x53, 1, {0x67}},
	{0x54, 1, {0x89}},
	{0x55, 1, {0xab}},
	{0x56, 1, {0x01}},
	{0x57, 1, {0x23}},
	{0x58, 1, {0x45}},
	{0x59, 1, {0x67}},
	{0x5a, 1, {0x89}},
	{0x5b, 1, {0xab}},
	{0x5c, 1, {0xcd}},
	{0x5d, 1, {0xef}},
	
	//GIP_3
	{0x5e, 1, {0x11}},
	{0x5f, 1, {0x01}},    //GOUT_L1  FW
	{0x60, 1, {0x00}},    //GOUT_L2  BW
	{0x61, 1, {0x15}},    //GOUT_L3  GPWR1
	{0x62, 1, {0x14}},     //GOUT_L4  GPWR2
	{0x63, 1, {0x0C}},     //GOUT_L5  CLK1_R
	{0x64, 1, {0x0D}},    //GOUT_L6  CLK2_R
	{0x65, 1, {0x0E}},    //GOUT_L7  CLK3_R
	{0x66, 1, {0x0F}},     //GOUT_L8  CLK4_R
	{0x67, 1, {0x06}},    //GOUT_L9  STV1_R
	{0x68, 1, {0x02}},        
	{0x69, 1, {0x02}},      
	{0x6a, 1, {0x02}},      
	{0x6b, 1, {0x02}},       
	{0x6c, 1, {0x02}},          
	{0x6d, 1, {0x02}},       
	{0x6e, 1, {0x08}},     //GOUT_L16  STV2_R   
	{0x6f, 1, {0x02}},    //GOUT_L17  VGL
	{0x70, 1, {0x02}},     //GOUT_L18  VGL
	{0x71, 1, {0x02}},    //GOUT_L19  VGL
	{0x72, 1, {0x02}},     
	{0x73, 1, {0x02}},    
	{0x74, 1, {0x02}},    
	
	{0x75, 1, {0x01}},       
	{0x76, 1, {0x00}},        
	{0x77, 1, {0x15}},     //BW_CGOUT_L[3]    
	{0x78, 1, {0x14}},     //BW_CGOUT_L[4]    
	{0x79, 1, {0x0C}},     //BW_CGOUT_L[5]     
	{0x7a, 1, {0x0D}},     //BW_CGOUT_L[6]     
	{0x7b, 1, {0x0E}},     //BW_CGOUT_L[7]   
	{0x7c, 1, {0x0F}},    //BW_CGOUT_L[8]    
	{0x7d, 1, {0x08}},     //BW_CGOUT_L[9]      
	{0x7e, 1, {0x02}},     //BW_CGOUT_L[10]
	{0x7f, 1, {0x02}},    //BW_CGOUT_L[11]    
	{0x80, 1, {0x02}},     //BW_CGOUT_L[12]   
	{0x81, 1, {0x02}},     //BW_CGOUT_L[13] 
	{0x82, 1, {0x02}},     //BW_CGOUT_L[14]      
	{0x83, 1, {0x02}},     //BW_CGOUT_L[15]   
	{0x84, 1, {0x06}},     //BW_CGOUT_L[16]      
	{0x85, 1, {0x02}},     //BW_CGOUT_L[17]
	{0x86, 1, {0x02}},     //BW_CGOUT_L[18]
	{0x87, 1, {0x02}},     //BW_CGOUT_L[19]
	{0x88, 1, {0x02}},     //BW_CGOUT_L[20]   
	{0x89, 1, {0x02}},     //BW_CGOUT_L[21]   
	{0x8A, 1, {0x02}},     //BW_CGOUT_L[22]   
	
	//CMD_Page 4
	{0xFF, 3, {0x98, 0x81, 0x04}},
	
	{0x6C, 1, {0x15}},
	{0x6E, 1, {0x2A}},           //VGH 15V
	{0x6F, 1, {0x35}},           // reg vcl + pumping ratio VGH=3x VGL=-2.5x
	{0x3A, 1, {0x24}},        //A4     //POWER SAVING
	{0x8D, 1, {0x14}},           //VGL -10V
	{0x87, 1, {0xBA}},           //ESD
	{0x26, 1, {0x76}},
	{0xB2, 1, {0xD1}},
	{0x35, 1, {0x1F}},      
	{0x39, 1, {0x00}},
	{0xB5, 1, {0x27}},           //gamma bias
	{0x31, 1, {0x75}},
	{0x3B, 1, {0x98}},  			
	{0x30, 1, {0x03}},
	{0x33, 1, {0x14}},
	{0x38, 1, {0x02}},
	{0x7A, 1, {0x00}},

	//CMD_Page 1
	{0xFF, 3, {0x98, 0x81, 0x01}},
	{0x22, 1, {0x0A}},          //BGR, SS
	{0x31, 1, {0x00}},          //Column inversion
	{0x35, 1, {0x07}},
	{0x53, 1, {0x6E}},          //VCOM1
	{0x55, 1, {0x40}},          //VCOM2 
	{0x50, 1, {0x85}},   // 4.3 95          //VREG1OUT 4.5V
	{0x51, 1, {0x85}},    //4.3 90          //VREG2OUT -4.5V
	{0x60, 1, {0x1F}},   //  SDT=2.8 
	{0x62, 1, {0x07}},
	{0x63, 1, {0x00}},
	//============Gamma START=============
	
	{0xA0, 1, {0x08}},
	{0xA1, 1, {0x14}},
	{0xA2, 1, {0x1E}},
	{0xA3, 1, {0x12}},
	{0xA4, 1, {0x13}},
	{0xA5, 1, {0x24}},
	{0xA6, 1, {0x18}},
	{0xA7, 1, {0x1A}},
	{0xA8, 1, {0x56}},
	{0xA9, 1, {0x18}},
	{0xAA, 1, {0x25}},
	{0xAB, 1, {0x57}},
	{0xAC, 1, {0x22}},
	{0xAD, 1, {0x24}},
	{0xAE, 1, {0x58}},
	{0xAF, 1, {0x2B}},
	{0xB0, 1, {0x2E}},
	{0xB1, 1, {0x4D}},
	{0xB2, 1, {0x5B}},
	{0xB3, 1, {0x3F}},
	
	//Neg Register
	{0xC0, 1, {0x08}},
	{0xC1, 1, {0x14}},
	{0xC2, 1, {0x1E}},
	{0xC3, 1, {0x12}},
	{0xC4, 1, {0x13}},
	{0xC5, 1, {0x24}},
	{0xC6, 1, {0x17}},
	{0xC7, 1, {0x1A}},
	{0xC8, 1, {0x56}},
	{0xC9, 1, {0x18}},
	{0xCA, 1, {0x25}},
	{0xCB, 1, {0x56}},
	{0xCC, 1, {0x22}},
	{0xCD, 1, {0x24}},
	{0xCE, 1, {0x58}},
	{0xCF, 1, {0x2B}},
	{0xD0, 1, {0x2D}},
	{0xD1, 1, {0x4E}},
	{0xD2, 1, {0x5C}},
	{0xD3, 1, {0x3F}},

//============ Gamma END===========			

//CMD_Page 0			
	{0xFF, 3, {0x98, 0x81, 0x00}},
	
	{0x11, 0, {}},
	{REGFLAG_DELAY, 120, {}},
	//sleep out
	{0x29, 0, {}},
	{REGFLAG_DELAY, 20, {}},
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;
    unsigned int cmd;

	for (i = 0; i < count; i++) {

		cmd = table[i].cmd;

		switch (cmd) {

			case REGFLAG_DELAY :
				MDELAY(table[i].count);
				break;

			case REGFLAG_END_OF_TABLE :
				break;

			default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
                break;
		}
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
	
		params->type = LCM_TYPE_DSI;
		params->width = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;

		params->dsi.mode = BURST_VDO_MODE;
	
		/* DSI */
		/* Command mode setting */
		params->dsi.LANE_NUM = LCM_FOUR_LANE;
		
		params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	
		/* Highly depends on LCD driver capability. */
		/* Not support in MT6573 */
		params->dsi.packet_size = 256;
		params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
		params->dsi.vertical_sync_active				= 4;
		params->dsi.vertical_backporch				= 12;
		params->dsi.vertical_frontporch 				= 20;
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 
	
		params->dsi.horizontal_sync_active			= 24;	
		params->dsi.horizontal_backporch				= 24;//62;	
		params->dsi.horizontal_frontporch			= 30;//80;
		params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;
	
		params->dsi.ssc_disable = 1;
		params->dsi.cont_clock	= 0;
		params->dsi.PLL_CLOCK = 220;//400
}


static void lcm_init(void)
{

	avdd_enable(1);
	lcd_power_en(1);
	MDELAY(100);
	lcd_reset(1);
	MDELAY(5);
	lcd_reset(0);	
	MDELAY(20);
	lcd_reset(1);
	MDELAY(50);//Must > 5ms
	//init_lcm_registers();
	MDELAY(180);
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	lcd_power_en(0);
    avdd_enable(0);
    MDELAY(150);
    lcd_reset(0);
    MDELAY(100);
    MDELAY(20);
}

static void lcm_resume(void)
{
  lcm_init();
}


struct LCM_DRIVER us828_bns_ili9881c_h030hd981d31b_hsd_wxga_8_lcm_drv = 
{
    .name			= "us828_bns_ili9881c_h030hd981d31b_hsd_wxga_8",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
};

