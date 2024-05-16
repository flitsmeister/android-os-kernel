#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
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

#define REGFLAG_DELAY             							0xFC
#define REGFLAG_END_OF_TABLE      							0xFE   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util;

extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_BL_EN;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))
#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

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

	//=========GIP_1===========//  
	{0x01, 1, {0x00}},
	{0x02, 1, {0x00}},
	{0x03, 1, {0x73}},
	{0x04, 1, {0x00}},
	{0x05, 1, {0x00}},
	{0x06, 1, {0x08}},
	{0x07, 1, {0x00}},
	{0x08, 1, {0x00}},
	{0x09, 1, {0x00}},
	{0x0a, 1, {0x01}},
	{0x0b, 1, {0x01}},
	{0x0c, 1, {0x00}},
	{0x0d, 1, {0x01}},
	{0x0e, 1, {0x01}},
	{0x0f, 1, {0x00}},
	{0x10, 1, {0x00}},
	{0x11, 1, {0x00}},
	{0x12, 1, {0x00}},
	{0x13, 1, {0x1F}},
	{0x14, 1, {0x1F}},
	{0x15, 1, {0x00}},
	{0x16, 1, {0x00}},
	{0x17, 1, {0x00}},
	{0x18, 1, {0x00}},
	{0x19, 1, {0x00}},
	{0x1a, 1, {0x00}},
	{0x1b, 1, {0x00}},
	{0x1c, 1, {0x00}},
	{0x1d, 1, {0x00}},
	{0x1e, 1, {0x40}},
	{0x1f, 1, {0xC0}},
	{0x20, 1, {0x06}},
	{0x21, 1, {0x01}},
	{0x22, 1, {0x06}},
	{0x23, 1, {0x01}},
	{0x24, 1, {0x88}},
	{0x25, 1, {0x88}},
	{0x26, 1, {0x00}},
	{0x27, 1, {0x00}},
	{0x28, 1, {0x3B}},
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
	{0x34, 1, {0x00}},  //GPWR1/2 non overlap time 2.62us
	{0x35, 1, {0x00}},   
	{0x36, 1, {0x00}},   
	{0x37, 1, {0x00}},   
	{0x38, 1, {0x00}},
	{0x39, 1, {0x00}},   
	{0x3a, 1, {0x00}},   
	{0x3b, 1, {0x00}},   
	{0x3c, 1, {0x00}},   
	{0x3d, 1, {0x00}},   
	{0x3e, 1, {0x00}},   
	{0x3f, 1, {0x00}}, 
	{0x40, 1, {0x00}},   
	{0x41, 1, {0x00}},   
	{0x42, 1, {0x00}},   
	{0x43, 1, {0x00}},   
	{0x44, 1, {0x00}},

	//=========GIP_2===========//   
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

	//=========GIP_3===========//      
	{0x5e, 1, {0x00}},
	{0x5f, 1, {0x01}},  //FW_GOUT_L1  FW
	{0x60, 1, {0x01}},    //FW_GOUT_L2  BW
	{0x61, 1, {0x06}},    //FW_GOUT_L3  GPWR1
	{0x62, 1, {0x06}},    //FW_GOUT_L4  GPWR2
	{0x63, 1, {0x07}},    //FW_GOUT_L5  CLK1_R
	{0x64, 1, {0x07}},    //FW_GOUT_L6  CLK2_R
	{0x65, 1, {0x00}},    //FW_GOUT_L7  CLK3_R
	{0x66, 1, {0x00}},    //FW_GOUT_L8  CLK4_R
	{0x67, 1, {0x02}},    //FW_GOUT_L9  STV1_R
	{0x68, 1, {0x02}},   
	{0x69, 1, {0x05}},   
	{0x6a, 1, {0x05}},   
	{0x6b, 1, {0x02}},   
	{0x6c, 1, {0x0d}},  
	{0x6d, 1, {0x0d}},     
	{0x6e, 1, {0x0c}},     //FW_GOUT_L  STV2_R
	{0x6f, 1, {0x0c}},   //FW_GOUT_L  VGL
	{0x70, 1, {0x0f}},     //FW_GOUT_L  VGL
	{0x71, 1, {0x0f}},     //FW_GOUT_L  VGL
	{0x72, 1, {0x0e}},   
	{0x73, 1, {0x0e}},   
	{0x74, 1, {0x02}},
	{0x75, 1, {0x01}},     //BW_GOUT_L1  FW
	{0x76, 1, {0x01}},     //BW_GOUT_L2  BW
	{0x77, 1, {0x06}},     //BW_GOUT_L3  GPWR1
	{0x78, 1, {0x06}},     //BW_GOUT_L4  GPWR2
	{0x79, 1, {0x07}},     //BW_GOUT_L5  CLK1_R
	{0x7a, 1, {0x07}},     //BW_GOUT_L6  CLK2_R
	{0x7b, 1, {0x00}},     //BW_GOUT_L7  CLK3_R
	{0x7c, 1, {0x00}},   //BW_GOUT_L8  CLK4_R
	{0x7D, 1, {0x02}},     //BW_GOUT_L9  STV1_R
	{0x7E, 1, {0x02}},     
	{0x7F, 1, {0x05}},     
	{0x80, 1, {0x05}},     
	{0x81, 1, {0x02}},     
	{0x82, 1, {0x0d}},     
	{0x83, 1, {0x0d}},      
	{0x84, 1, {0x0c}},      //BW_GOUT_L  STV2_R
	{0x85, 1, {0x0c}},      //BW_GOUT_L  VGL
	{0x86, 1, {0x0f}},      //BW_GOUT_L  VGL
	{0x87, 1, {0x0f}},      //BW_GOUT_L  VGL
	{0x88, 1, {0x0e}},   
	{0x89, 1, {0x0e}},   
	{0x8A, 1, {0x02}},

	//CMD_Page 4
	{0xFF, 3, {0x98, 0x81, 0x04}},
	{0x3B, 1, {0xC0}},
	{0x6C, 1, {0x15}},       //Set VCORE voltage =1.5V
	{0x6E, 1, {0x2A}},       //di_pwr_reg=0 for power mode 2A //VGH clamp 15V
	{0x6F, 1, {0x33}},       //reg vcl + pumping ratio VGH=4x VGL=-2x
	{0x8D, 1, {0x1B}},       //VGL clamp -10V
	{0x87, 1, {0xBA}},       //ESD
	{0x3A, 1, {0x24}},       //POWER SAVING             
	{0x26, 1, {0x76}},               
	{0xB2, 1, {0xD1}},

	//CMD_Page 1
	{0xFF, 3, {0x98, 0x81, 0x01}},
	{0x22, 1, {0x0A}},		//BGR, SS
	{0x31, 1, {0x00}},		//Zig-Zag inversion
	{0x40, 1, {0x53}},
	{0x43, 1, {0x66}},
	{0x53, 1, {0x40}},
	{0x50, 1, {0x87}},
	{0x51, 1, {0x82}},
	{0x60, 1, {0x15}},
	{0x61, 1, {0x01}},
	{0x62, 1, {0x0C}},
	{0x63, 1, {0x00}},
	
	//CMD_Page 0			
	{0xFF, 3, {0x98, 0x81, 0x00}},
	{0x35, 1, {0x00}},	//TE on
	
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
		params->dsi.vertical_sync_active			= 6;
		params->dsi.vertical_backporch				= 15;
		params->dsi.vertical_frontporch 			= 16;
		params->dsi.vertical_active_line			= FRAME_HEIGHT; 
	
		params->dsi.horizontal_sync_active			= 8;	
		params->dsi.horizontal_backporch			= 48;//62;	
		params->dsi.horizontal_frontporch			= 52;//80;
		params->dsi.horizontal_active_pixel 		= FRAME_WIDTH;

		params->dsi.PLL_CLOCK = 220;
}


static void lcm_init(void)
{
	avdd_enable(1);
	lcd_power_en(1);
	MDELAY(50);  
	lcd_reset(1);
	MDELAY(10);
	lcd_reset(0);	
	MDELAY(10);
	lcd_reset(1);
	MDELAY(10);//Must > 5ms
	//init_lcm_registers();
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	MDELAY(10);
}


static void lcm_suspend(void)
{
	lcd_power_en(0);
    avdd_enable(0);
    MDELAY(50);
    lcd_reset(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
  lcm_init();
}


struct LCM_DRIVER us163_fx_ili9881c_im2byl03a_boe_wxga_101_lcm_drv = 
{
    .name			= "us163_fx_ili9881c_im2byl03a_boe_wxga_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
};

