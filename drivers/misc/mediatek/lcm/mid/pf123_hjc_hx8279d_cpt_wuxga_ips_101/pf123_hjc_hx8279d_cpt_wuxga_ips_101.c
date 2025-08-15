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

#define FRAME_WIDTH  										(1200)
#define FRAME_HEIGHT 										(1920)

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

/*static void avdd_enable(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ZERO);
    }
}*/


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
#if 1
struct LCM_setting_table {
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
};
#endif

/* static void init_lcm_registers(void)
{ 
	unsigned int data_array[16];

data_array[0]=0x00B01500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x50B21500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x01B01500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00C01500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x17C11500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x01C21500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x26C31500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00C41500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x23C51500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x11C61500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x05C71500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x07C81500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x09C91500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0BCA1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x1BCB1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x1DCC1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x1FCD1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x21CE1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0FCF1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0DD01500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00D11500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00D21500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00D31500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00D41500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x18D51500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x02D61500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x26D71500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00D81500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x23D91500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x11DA1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x06DB1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x08DC1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0ADD1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0CDE1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x1CDF1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x1EE01500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x20E11500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x22E21500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x10E31500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0EE41500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00E51500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00E61500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00E71500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x03B01500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x04BE1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x40B91500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x88CC1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0CC81500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x07C91500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x01CD1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x40CA1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x1ACE1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x60CF1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x08D21500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x08D31500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x01DB1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x06D91500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00D41500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x01D51500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x04D61500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x03D71500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00C21500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0EC31500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00C41500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0EC51500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00DD1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0EDE1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00E61500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0EE71500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00C21500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0EC31500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00C41500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0EC51500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00DD1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0EDE1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00E61500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x0EE71500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x06B01500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0xA5C01500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x1CD51500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00C01500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00B01500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x8FBA1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x5CF91500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x14C21500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x14C41500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x1ABF1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x11C01500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x1DBD1500;
dsi_set_cmdq(data_array,1,1);
MDELAY(1);

data_array[0]=0x00110500;
dsi_set_cmdq(data_array,1,1);
MDELAY(120);

data_array[0] = 0x00290500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(50);
}*/
#if 1
static struct LCM_setting_table lcm_initialization_setting[] = 
{
	{0xB0, 1, {0x01}},
{0xC0, 1, {0x26}},
{0xC1, 1, {0x10}},
{0xC2, 1, {0x0E}},
{0xC3, 1, {0x00}},
{0xC4, 1, {0x00}},
{0xC5, 1, {0x23}},
{0xC6, 1, {0x11}},
{0xC7, 1, {0x22}},
{0xC8, 1, {0x20}},
{0xC9, 1, {0x1E}},
{0xCA, 1, {0x1C}},
{0xCB, 1, {0x0C}},
{0xCC, 1, {0x0A}},
{0xCD, 1, {0x08}},
{0xCE, 1, {0x06}},
{0xCF, 1, {0x18}},
{0xD0, 1, {0x02}},
{0xD1, 1, {0x00}},
{0xD2, 1, {0x00}},
{0xD3, 1, {0x00}},
{0xD4, 1, {0x26}},
{0xD5, 1, {0x0F}},
{0xD6, 1, {0x0D}},
{0xD7, 1, {0x00}},
{0xD8, 1, {0x00}},
{0xD9, 1, {0x23}},
{0xDA, 1, {0x11}},
{0xDB, 1, {0x21}},
{0xDC, 1, {0x1F}},
{0xDD, 1, {0x1D}},
{0xDE, 1, {0x1B}},
{0xDF, 1, {0x0B}},
{0xE0, 1, {0x09}},
{0xE1, 1, {0x07}},
{0xE2, 1, {0x05}},
{0xE3, 1, {0x17}},
{0xE4, 1, {0x01}},
{0xE5, 1, {0x00}},
{0xE6, 1, {0x00}},
{0xE7, 1, {0x00}},
{0xB0, 1, {0x03}},
{0xBE, 1, {0x04}},
{0xB9, 1, {0x40}},
{0xCC, 1, {0x88}},
{0xC8, 1, {0x0C}},
{0xC9, 1, {0x07}},
{0xCD, 1, {0x01}},
{0xCA, 1, {0x40}},
{0xCE, 1, {0x1A}},
{0xCF, 1, {0x60}},
{0xD2, 1, {0x08}},
{0xD3, 1, {0x08}},
{0xDB, 1, {0x01}},
{0xD9, 1, {0x06}},
{0xD4, 1, {0x00}},
{0xD5, 1, {0x01}},
{0xD6, 1, {0x04}},
{0xD7, 1, {0x03}},
{0xC2, 1, {0x00}},
{0xC3, 1, {0x0E}},
{0xC4, 1, {0x00}},
{0xC5, 1, {0x0E}},
{0xDD, 1, {0x00}},
{0xDE, 1, {0x0E}},
{0xE6, 1, {0x00}},
{0xE7, 1, {0x0E}},
{0xC2, 1, {0x00}},
{0xC3, 1, {0x0E}},
{0xC4, 1, {0x00}},
{0xC5, 1, {0x0E}},
{0xDD, 1, {0x00}},
{0xDE, 1, {0x0E}},
{0xE6, 1, {0x00}},
{0xE7, 1, {0x0E}},
{0xB0, 1, {0x06}},
{0xC0, 1, {0xA5}},
{0xD5, 1, {0x1C}},
{0xC0, 1, {0x00}},
{0xB0, 1, {0x00}},
{0xBA, 1, {0x8F}},
{0xBD, 1, {0x17}},
{0xF9, 1, {0x5C}},
{0xC2, 1, {0x14}},
{0xC4, 1, {0x14}},
{0xBF, 1, {0x1A}},
{0xC0, 1, {0x11}},
	
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
#endif
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

	params->physical_height = 217;//216
	params->physical_width 	= 137;//135
	
	params->dsi.mode = SYNC_EVENT_VDO_MODE;

	/* DSI */
	/* Command mode setting */
	/* Three lane or Four lane */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;

	/* The following defined the format for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;

	/* Video mode setting */
	params->dsi.intermediat_buffer_num = 0;

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count = FRAME_WIDTH * 3;

	params->dsi.vertical_sync_active                = 1;//10;//4
	params->dsi.vertical_backporch                  = 25;//20;//16
	params->dsi.vertical_frontporch                 = 35;//20;/*modified by mtk*///20
	params->dsi.vertical_active_line                = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active              = 1;//20;//10
	params->dsi.horizontal_backporch                = 60;//80;//40
	params->dsi.horizontal_frontporch               = 80;//80;//80
	params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

	params->dsi.ssc_disable = 1;
	params->dsi.PLL_CLOCK = 480;  //161MHz //455;//520
	//params->dsi.cont_clock = 1;

}


static void lcm_init(void)
{
	
	lcd_power_en(1);
	MDELAY(30);  
	//avdd_enable(1);
	lcd_reset(1);
	MDELAY(5);
	lcd_reset(0);	
	MDELAY(10);
	lcd_reset(1);
	MDELAY(20);//Must > 5ms
	//init_lcm_registers();
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
//	MDELAY(10);
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00280500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
	data_array[0] = 0x00100500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(200);
	lcd_reset(0);
    MDELAY(100);
	lcd_power_en(0);
	MDELAY(100);

}

static void lcm_resume(void)
{
  lcm_init();
}

struct LCM_DRIVER pf123_hjc_hx8279d_cpt_wuxga_ips_101_lcm_drv =
{
    .name			= "pf123_hjc_hx8279d_cpt_wuxga_ips_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
};

