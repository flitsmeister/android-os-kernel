

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
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

#define FRAME_WIDTH  (1920)
#define FRAME_HEIGHT (1080)

#define REGFLAG_DELAY 0xFD
//#define GPIO_LCM_PWREN                                      (GPIO173 |0x80000000)//GPIO_LCM_PWR2_EN//GPIO_LCM_PWR2_EN
//#define GPIO_LCM_BL_EN                                      (GPIO174 | 0x80000000)//GPIO_LCM_PWR_EN//GPIO_LCM_PWR_EN
//#define GPIO_LCM_RST                                        (GPIO45 | 0x80000000)
#define REGFLAG_END_OF_TABLE 0xFE

#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util;

//extern unsigned int GPIO_LCM_3v3_EN;
extern unsigned int GPIO_LCM_RST;
//extern unsigned int GPIO_LCM_BL_EN;
//extern unsigned int GPIO_LCM_1v8_EN;

extern unsigned int EDP_LCM_LED_EN1;
extern unsigned int GPIO_LCM_VLCM33_EN;
extern unsigned int GPIO_LCM_LT8911B_VDD18_EN;
extern unsigned int GPIO_LCM_LT8911B_RST_EN;
extern unsigned int GPIO_LCM_GPIO5_IRQO2;


#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define MDELAY(n) 											(lcm_util.mdelay(n))
#define UDELAY(n) 											(lcm_util.udelay(n))
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
		
    #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
    #else
		params->dsi.mode   = SYNC_PULSE_VDO_MODE;//SYNC_EVENT_VDO_MODE;		//SYNC_EVENT_VDO_MODE;
    #endif
	printk("dsy log : [kernel/LCM] %s %d \n",__FUNCTION__,__LINE__);
		// DSI
		/* Command mode setting */
		// Three lane or Four lane
		params->dsi.LANE_NUM								= LCM_FOUR_LANE;
		
		//The following defined the fomat for data coming from LCD engine.
		//params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		//params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		//params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Highly depends on LCD driver capability.
		// Not support in MT6573
		//params->dsi.packet_size=256;

		// Video mode setting		
		//params->dsi.intermediat_buffer_num = 0;

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		//params->dsi.word_count=FRAME_WIDTH*3;
		
		
	params->dsi.vertical_sync_active = 5;
	params->dsi.vertical_backporch = 23;
	params->dsi.vertical_frontporch = 3;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 32;
	params->dsi.horizontal_backporch = 80;
	params->dsi.horizontal_frontporch = 48;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	/* video mode timing */
	//params->dsi.word_count = FRAME_WIDTH * 3;

	params->dsi.PLL_CLOCK = 420;
	params->dsi.ssc_disable = 1;
	params->dsi.cont_clock = 1;
	params->dsi.clk_lp_per_line_enable = 0;	
		
    //	params->dsi.ssc_disable							= 1;
	
	//	params->dsi.cont_clock= 1;
		
}
// static void lcd_power_en(unsigned int enabled)
// {
//     if (enabled)
//     {
//         lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ONE);
//     }
//     else
//     {
//         lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ZERO);
//     }
// }

// static void avdd_enable(unsigned int enabled)
// {
//     if (enabled)
//     {
//         lcm_set_gpio_output(GPIO_LCM_3v3_EN, GPIO_OUT_ONE);
//     }
//     else
//     {
//         lcm_set_gpio_output(GPIO_LCM_3v3_EN, GPIO_OUT_ZERO);
//     }
// }

// static void dvdd_enable(unsigned int enabled)
// {
//     if (enabled)
//     {
//         lcm_set_gpio_output(GPIO_LCM_1v8_EN, GPIO_OUT_ONE);
//     }
//     else
//     {
//         lcm_set_gpio_output(GPIO_LCM_1v8_EN, GPIO_OUT_ZERO);
//     }
// }

static void lcd_reset(unsigned int enabled)
{
	printk("dsy log : [kernel/LCM] %s %d \n",__FUNCTION__,__LINE__);
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




static void lcm_init(void)
{

	// avdd_enable(0);
	// dvdd_enable(0);
	// lcd_reset(0);
	// lcd_power_en(0);
	// MDELAY(10);
printk("dsy log : [kernel/LCM] %s %d start\n",__FUNCTION__,__LINE__);
	lcm_set_gpio_output(EDP_LCM_LED_EN1, 0);//bl
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCM_LT8911B_RST_EN, 1);//en
	
	lcm_set_gpio_output(GPIO_LCM_LT8911B_VDD18_EN, 1);//en
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCM_VLCM33_EN, 1); //3.3
	 MDELAY(10);
	// avdd_enable(1);
	// MDELAY(10);
	lcm_set_gpio_output(GPIO_LCM_GPIO5_IRQO2, 1); 
	MDELAY(10);


	// lcd_reset(1);
	// MDELAY(20);
	lcd_reset(0);	
	MDELAY(100);
	lcd_reset(1);
	MDELAY(100);
#if defined(CONFIG_MID_LT8911EXB_SUPPORT)
	LT8911EXB_config();
	MDELAY(50);
#endif
	lcm_set_gpio_output(EDP_LCM_LED_EN1, 1);//bl
	MDELAY(10);
	printk("dsy log : [kernel/LCM] %s %d end\n",__FUNCTION__,__LINE__);
}

static void lcm_suspend(void)
{


	printk("dsy log : [kernel/LCM] %s %d start\n",__FUNCTION__,__LINE__);
	// lcd_power_en(0);
	// MDELAY(20);	
	lcm_set_gpio_output(EDP_LCM_LED_EN1, 0);
	MDELAY(10);

    lcd_reset(0);
    MDELAY(100);
      	lcm_set_gpio_output(GPIO_LCM_LT8911B_VDD18_EN, 0);;//en
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCM_VLCM33_EN, 0); 
	MDELAY(10);
	 
	lcm_set_gpio_output(GPIO_LCM_LT8911B_RST_EN, 1);//en
	MDELAY(10);
	// avdd_enable(1);
	// MDELAY(10);
	lcm_set_gpio_output(GPIO_LCM_GPIO5_IRQO2, 0); 
	MDELAY(10);
 //    avdd_enable(0);
	// MDELAY(20);
printk("dsy log : [kernel/LCM] %s %d end\n",__FUNCTION__,__LINE__);
	// dvdd_enable(0);
	// MDELAY(20);

}

static void lcm_resume(void)
{
	printk("dsy log : [kernel/LCM] %s %d start\n",__FUNCTION__,__LINE__);
	lcm_init();
}


struct LCM_DRIVER us145u_ag_lt8911exb_wuxga_ips_101_lcm_drv = 
{
  .name						= "us145u_ag_lt8911exb_wuxga_ips_101",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if (LCM_DSI_CMD_MODE)
  .update         = lcm_update,
#endif
};
