

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


#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util;

extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_BL_EN;

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
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
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 


    params->dsi.vertical_sync_active                            = 2; //4;
    params->dsi.vertical_backporch                              = 10; //16;
    params->dsi.vertical_frontporch                             = 12; 
    params->dsi.vertical_active_line                            = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active                          = 25; //5;//6;
    params->dsi.horizontal_backporch                            = 25; //60; //80;
    params->dsi.horizontal_frontporch                           = 25; 
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    //params->dsi.cont_clock    = 1;//1
    //params->dsi.ssc_disable = 0;//0
    params->dsi.PLL_CLOCK = 205;
	params->dsi.cont_clock = 1;
}

static void init_lcm_registers(void)
{
    unsigned int data_array[16];
    
#ifdef BUILD_LK
    printf("[BND][LK/LCM] %s() enter\n", __func__);
#else
    printk("[BND][Kernel/LCM] %s() enter\n", __func__);
#endif

	data_array[0]=0x00023902;
	data_array[1]=0x0000AACD;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00000030;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00000133; //Pmode=10:R33H=0X01;Pmode=11:R33H=0X11;Pmode=10,?a?y0x21
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00000032;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00004136;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000003A;
	dsi_set_cmdq(data_array,2,1);	
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00008267;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00002769;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000016D;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00001668;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00093902;
	data_array[1]=0x000F0055;
	data_array[2]=0x000F000F;
	data_array[3]=0x0000000F;
	dsi_set_cmdq(data_array,4,1);
	MDELAY(2);
	MDELAY(2);
	
	data_array[0]=0x00113902;
	data_array[1]=0x000F0056;
	data_array[2]=0x000F000F;
	data_array[3]=0x000F000F;
	data_array[4]=0x000F000F;
	data_array[5]=0x0000000F;
	dsi_set_cmdq(data_array,6,1);
	MDELAY(2);
	MDELAY(2);
	
	data_array[0]=0x00033902;
	data_array[1]=0x00800072;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00033902;
	data_array[1]=0x00102073;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000035E;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00005E41;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000A461;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000387E;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00001074;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000203F;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00001447;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00006648;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000504F;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00004F4E;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00001139;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00001060;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000D050;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00003476;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000807C;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000042E;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00143902;
	data_array[1]=0x181A1F53;
	data_array[2]=0x16141414;
	data_array[3]=0x10131718;
	data_array[4]=0x0B0E0E0F;
	data_array[5]=0x01030609;
	dsi_set_cmdq(data_array,6,1);
	MDELAY(2);
	
	data_array[0]=0x00143902;
	data_array[1]=0x181A1F54;
	data_array[2]=0x16141414;
	data_array[3]=0x12151918;
	data_array[4]=0x0B0E0E0F;
	data_array[5]=0x01030609;
	dsi_set_cmdq(data_array,6,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00002A5F;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00000463; //0x04 4lanes  //0x24 3lanes
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00003128;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00000029;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000FC34;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000312D;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00006778;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000C41D;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000F41D;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000004D;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(20);

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
   
    MDELAY(10);
    avdd_enable(1);
    MDELAY(20);

    lcd_reset(1);
    MDELAY(20);
    lcd_reset(0);
    MDELAY(20);
    lcd_reset(1);
	MDELAY(50);//Must > 5ms
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


struct LCM_DRIVER us716_fx_ek79029_im2b8017r_boe_wxga_ips_8_lcm_drv = 
{
    .name			= "us716_fx_ek79029_im2b8017r_boe_wxga_ips_8",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

