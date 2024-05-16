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

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(800)
#define FRAME_HEIGHT 										(1280)


#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util;

extern unsigned int GPIO_LCM_PWR_EN;//3.3V
extern unsigned int GPIO_LCM_RST;//reset
extern unsigned int GPIO_LCM_BL_EN;//bias en pin

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

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

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{

    memset(params, 0, sizeof(struct LCM_PARAMS));

    params->type   = LCM_TYPE_DSI;

    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    params->dsi.mode    = BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_THREE_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 


	params->dsi.vertical_sync_active				= 6;
    params->dsi.vertical_backporch					= 20;
    params->dsi.vertical_frontporch 				= 16;
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active				= 20;
    params->dsi.horizontal_backporch				= 60;
    params->dsi.horizontal_frontporch				= 60;
    params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 310;
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
#define REGFLAG_DELAY           0XFE
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

			{0xFF,3,{0x98,0x81,0x03}},//PAGE3
	{0x01,1,{0x00}},
	{0x02,1,{0x00}},
	{0x03,1,{0x53}},	
	{0x04,1,{0x53}},	
	{0x05,1,{0x13}},
	{0x06,1,{0x04}},
	{0x07,1,{0x02}},
	{0x08,1,{0x02}},
	{0x09,1,{0x00}},
	{0x0a,1,{0x00}},
	{0x0b,1,{0x00}},
	{0x0c,1,{0x00}},
	{0x0d,1,{0x00}},
	{0x0e,1,{0x00}},
	{0x0f,1,{0x00}},
	{0x10,1,{0x00}},
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
	{0x1e,1,{0xc0}},
	{0x1f,1,{0x00}},
	{0x20,1,{0x02}},
	{0x21,1,{0x09}},
	{0x22,1,{0x00}},
	{0x23,1,{0x00}},
	{0x24,1,{0x00}},
	{0x25,1,{0x00}},
	{0x26,1,{0x00}},
	{0x27,1,{0x00}},
	{0x28,1,{0x55}},
	{0x29,1,{0x03}},
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
	{0x34,1,{0x00}},
	{0x35,1,{0x00}},
	{0x36,1,{0x00}},
	{0x37,1,{0x00}},
	{0x38,1,{0x3C}},
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
	{0x43,1,{0x00}},
	{0x44,1,{0x00}},
	{0x45,1,{0x00}},
	
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
	
	{0x5e,1,{0x01}},
	{0x5f,1,{0x0A}},	 //FW_CGOUT_L[1] RESE_ODD
	{0x60,1,{0x02}},	 //FW_CGOUT_L[2] VSSG_ODD
	{0x61,1,{0x02}},	 //FW_CGOUT_L[3] VSSG_ODD
	{0x62,1,{0x08}},	 //FW_CGOUT_L[4] STV2_ODD
	{0x63,1,{0x15}},	 //FW_CGOUT_L[5] VDD2_ODD
	{0x64,1,{0x14}},	 //FW_CGOUT_L[6] VDD1_ODD
	{0x65,1,{0x02}},	 //FW_CGOUT_L[7]
	{0x66,1,{0x11}},	 //FW_CGOUT_L[8] CK11
	{0x67,1,{0x10}},	 //FW_CGOUT_L[9] CK9
	{0x68,1,{0x02}},	 //FW_CGOUT_L[10]
	{0x69,1,{0x0F}},	 //FW_CGOUT_L[11] CK7
	{0x6a,1,{0x0E}},	 //FW_CGOUT_L[12] CK5
	{0x6b,1,{0x02}},	 //FW_CGOUT_L[13]	
	{0x6c,1,{0x0D}},	 //FW_CGOUT_L[14] CK3  
	{0x6d,1,{0x0C}},	 //FW_CGOUT_L[15] CK1  
	{0x6e,1,{0x06}},	 //FW_CGOUT_L[16] STV1_ODD	
	{0x6f,1,{0x02}},	 //FW_CGOUT_L[17]	
	{0x70,1,{0x02}},	 //FW_CGOUT_L[18]	
	{0x71,1,{0x02}},	 //FW_CGOUT_L[19]	
	{0x72,1,{0x02}},	 //FW_CGOUT_L[20]	
	{0x73,1,{0x02}},	 //FW_CGOUT_L[21]	
	{0x74,1,{0x02}},	 //FW_CGOUT_L[22] 
	  
	{0x75,1,{0x0A}},	 //BW_CGOUT_L[1]   RESE_ODD
	{0x76,1,{0x02}},	 //BW_CGOUT_L[2]   VSSG_ODD 
	{0x77,1,{0x02}},	 //BW_CGOUT_L[3]   VSSG_ODD  
	{0x78,1,{0x06}},	 //BW_CGOUT_L[4]   STV2_ODD 
	{0x79,1,{0x15}},	 //BW_CGOUT_L[5]   VDD2_ODD 
	{0x7a,1,{0x14}},	 //BW_CGOUT_L[6]   VDD1_ODD 
	{0x7b,1,{0x02}},	 //BW_CGOUT_L[7]	
	{0x7c,1,{0x10}},	 //BW_CGOUT_L[8]   CK11 
	{0x7d,1,{0x11}},	 //BW_CGOUT_L[9]   CK9 
	{0x7e,1,{0x02}},	 //BW_CGOUT_L[10]	
	{0x7f,1,{0x0C}},	 //BW_CGOUT_L[11]  CK7
	{0x80,1,{0x0D}},	 //BW_CGOUT_L[12]  CK5 
	{0x81,1,{0x02}},	 //BW_CGOUT_L[13]	
	{0x82,1,{0x0E}},	 //BW_CGOUT_L[14]  CK3 
	{0x83,1,{0x0F}},	 //BW_CGOUT_L[15]  CK1 
	{0x84,1,{0x08}},	//BW_CGOUT_L[16]  STV1_ODD 
	{0x85,1,{0x02}},	 //BW_CGOUT_L[17]	
	{0x86,1,{0x02}},	 //BW_CGOUT_L[18]	
	{0x87,1,{0x02}},	 //BW_CGOUT_L[19]	
	{0x88,1,{0x02}},	 //BW_CGOUT_L[20]	
	{0x89,1,{0x02}},	 //BW_CGOUT_L[21]	
	{0x8A,1,{0x02}},	 //BW_CGOUT_L[22]	
	
	{0xFF,3,{0x98,0x81,0x04}},//PAGE4
	{0x00,1,{0x00}},//3L
	{0x3B,1,{0xC0}},	 // ILI4003D sel 
	{0x6C,1,{0x15}},
	{0x6E,1,{0x30}},	 //VGH 16V
	{0x6F,1,{0x55}},	 //Pump ratio VGH=VSPX4 VGL=VSNX4
	{0x3A,1,{0x24}},
	{0x8D,1,{0x1F}},
	{0x87,1,{0xBA}},
	{0x26,1,{0x76}},
	{0xB2,1,{0xD1}},
	{0xB5,1,{0x07}},
	{0x35,1,{0x1F}},
	{0x88,1,{0x0B}},
	{0x21,1,{0x30}},
	
	{0xFF,3,{0x98,0x81,0x01}},//PAGE1
	{0x22,1,{0x0A}},
	{0x31,1,{0x09}},
	{0x40,1,{0x53}},
	{0x53,1,{0x37}},
	{0x55,1,{0x88}},
	{0x50,1,{0x95}},
	{0x51,1,{0x95}},
	{0x60,1,{0x30}},

	{0xA0,1,{0x0F}},
	{0xA1,1,{0x17}},
	{0xA2,1,{0x22}},
	{0xA3,1,{0x19}},
	{0xA4,1,{0x15}},
	{0xA5,1,{0x28}},
	{0xA6,1,{0x1C}},
	{0xA7,1,{0x1C}},
	{0xA8,1,{0x78}},
	{0xA9,1,{0x1C}},
	{0xAA,1,{0x28}},
	{0xAB,1,{0x69}},
	{0xAC,1,{0x1A}},
	{0xAD,1,{0x19}},
	{0xAE,1,{0x4B}},
	{0xAF,1,{0x22}},
	{0xB0,1,{0x2A}},
	{0xB1,1,{0x4B}},
	{0xB2,1,{0x6B}},
	{0xB3,1,{0x3F}},

	{0xC0,1,{0x01}},
	{0xC1,1,{0x17}},
	{0xC2,1,{0x22}},
	{0xC3,1,{0x19}},
	{0xC4,1,{0x15}},
	{0xC5,1,{0x28}},
	{0xC6,1,{0x1C}},
	{0xC7,1,{0x1D}},
	{0xC8,1,{0x78}},
	{0xC9,1,{0x1C}},
	{0xCA,1,{0x28}},
	{0xCB,1,{0x69}},
	{0xCC,1,{0x1A}},
	{0xCD,1,{0x19}},
	{0xCE,1,{0x4B}},
	{0xCF,1,{0x22}},
	{0xD0,1,{0x2A}},
	{0xD1,1,{0x4B}},
	{0xD2,1,{0x6B}},
	{0xD3,1,{0x3F}},
	
	{0xFF,3,{0x98,0x81,0x00}},//PAGE0
	//{0x35,1,{0x00}},				 //TE OUT
};

static void init_lcm_registers(void)
{
    unsigned int data_array[16];
	
	push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
	
    data_array[0] = 0x00110500;  	// SLPOUT
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);


    data_array[0] = 0x00290500;  	// DSPON
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(5);


}

static void lcm_init(void)
{

	lcd_power_en(1);
	MDELAY(100);
	lcd_reset(0);	
	MDELAY(20);
	lcd_reset(1);
	MDELAY(50);//Must > 5ms
	avdd_enable(1);
	init_lcm_registers();
	MDELAY(180); 
}


static void lcm_suspend(void)
{
	lcd_power_en(0);
    MDELAY(150);
    lcd_reset(0);
    MDELAY(100);
    avdd_enable(0);
   // DSI_clk_HS_mode(0);
    MDELAY(20);
}


static void lcm_resume(void)
{
  lcm_init();
}


struct LCM_DRIVER us717_hx_ili9881c_inx_wxganl_ips_101_lcm_drv = 
{
    .name			= "us717_hx_ili9881c_inx_wxganl_ips_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

