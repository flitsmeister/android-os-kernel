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

/* static void avdd_enable(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ZERO);
    }
} */


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
	{0xB0, 1, {0x5A}},
	{0xB1, 1, {0x00}},
	{0x89, 1, {0x01}},
	{0x91, 1, {0x3F}},
	{0xB1, 1, {0x03}},
	{0x2C, 1, {0x28}},
	{0x00, 1, {0xF1}},
	{0x01, 1, {0x78}},
	{0x02, 1, {0x3C}},
	{0x03, 1, {0x1E}},
	{0x04, 1, {0x8F}},
	{0x05, 1, {0x01}},
	{0x06, 1, {0x00}},
	{0x07, 1, {0x00}},
	{0x08, 1, {0x00}},
	{0x09, 1, {0x00}},
	{0x0A, 1, {0x01}},
	{0x0B, 1, {0x3C}},
	{0x0C, 1, {0x00}},
	{0x0D, 1, {0x00}},
	{0x0E, 1, {0x24}},
	{0x0F, 1, {0x1C}},
	{0x10, 1, {0xC8}},
	{0x11, 1, {0x60}},
	{0x12, 1, {0x70}},
	{0x13, 1, {0x01}},
	{0x14, 1, {0xE3}},
	{0x15, 1, {0xFF}},
	{0x16, 1, {0x3D}},
	{0x17, 1, {0x0E}},
	{0x18, 1, {0x01}},
	{0x19, 1, {0x00}},
	{0x1A, 1, {0x00}},
	{0x1B, 1, {0xFC}},
	{0x1C, 1, {0x0B}},
	{0x1D, 1, {0xA0}},
	{0x1E, 1, {0x03}},
	{0x1F, 1, {0x04}},
	{0x20, 1, {0x0C}},
	{0x21, 1, {0x00}},
	{0x22, 1, {0x04}},
	{0x23, 1, {0x81}},
	{0x24, 1, {0x1F}},
	{0x25, 1, {0x10}},
	{0x26, 1, {0x9B}},
	{0x2D, 1, {0x01}},
	{0x2E, 1, {0x84}},
	{0x2F, 1, {0x00}},
	{0x30, 1, {0x02}},
	{0x31, 1, {0x08}},
	{0x32, 1, {0x01}},
	{0x33, 1, {0x1C}},
	{0x34, 1, {0x70}},
	{0x35, 1, {0xFF}},
	{0x36, 1, {0xFF}},
	{0x37, 1, {0xFF}},
	{0x38, 1, {0xFF}},
	{0x39, 1, {0xFF}},
	{0x3A, 1, {0x05}},
	{0x3B, 1, {0x00}},
	{0x3C, 1, {0x00}},
	{0x3D, 1, {0x00}},
	{0x3E, 1, {0x0F}},
	{0x3F, 1, {0xA4}},
	{0x40, 1, {0x28}},
	{0x41, 1, {0xFC}},
	{0x42, 1, {0x01}},
	{0x43, 1, {0x00}}, //0x43=08 change to 00 by Focal at 20190314 
	{0x44, 1, {0x05}},
	{0x45, 1, {0xF0}},
	{0x46, 1, {0x01}},
	{0x47, 1, {0x02}},
	{0x48, 1, {0x00}},
	{0x49, 1, {0x58}},
	{0x4A, 1, {0x00}},
	{0x4B, 1, {0x05}},
	{0x4C, 1, {0x03}},
	{0x4D, 1, {0xD0}},
	{0x4E, 1, {0x13}},
	{0x4F, 1, {0xFF}},
	{0x50, 1, {0x0A}},
	{0x51, 1, {0x53}},
	{0x52, 1, {0x26}},
	{0x53, 1, {0x22}},
	{0x54, 1, {0x09}},
	{0x55, 1, {0x22}},
	{0x56, 1, {0x00}},
	{0x57, 1, {0x1C}},
	{0x58, 1, {0x03}},
	{0x59, 1, {0x3F}},
	{0x5A, 1, {0x28}},
	{0x5B, 1, {0x01}},
	{0x5C, 1, {0xCC}},
	{0x5D, 1, {0x21}},
	{0x5E, 1, {0x04}},
	{0x5F, 1, {0x13}},
	{0x60, 1, {0x42}},
	{0x61, 1, {0x08}},
	{0x62, 1, {0x64}},
	{0x63, 1, {0xEB}},
	{0x64, 1, {0x10}},
	{0x65, 1, {0xA8}},
	{0x66, 1, {0x84}},
	{0x67, 1, {0x8E}},
	{0x68, 1, {0x29}},
	{0x69, 1, {0x11}},
	{0x6A, 1, {0x42}},
	{0x6B, 1, {0x38}},
	{0x6C, 1, {0x21}},
	{0x6D, 1, {0x84}},
	{0x6E, 1, {0x50}},
	{0x6F, 1, {0xB6}},
	{0x70, 1, {0x0E}},
	{0x71, 1, {0xA1}},
	{0x72, 1, {0xCE}},
	{0x73, 1, {0xF8}},
	{0x74, 1, {0xDA}},
	{0x75, 1, {0x1A}},
	{0x76, 1, {0x00}},
	{0x77, 1, {0x00}},
	{0x78, 1, {0x5F}},
	{0x79, 1, {0xE0}},
	{0x7A, 1, {0x01}},
	{0x7C, 1, {0xFF}},
	{0x7D, 1, {0xFF}},//00
	{0x7E, 1, {0xFF}},//46
	{0x7F, 1, {0xCE}},
	{0xB1, 1, {0x02}},
	{0x00, 1, {0xFF}},
	{0x01, 1, {0x01}},
	{0x02, 1, {0x00}},
	{0x03, 1, {0x00}},
	{0x04, 1, {0x00}},
	{0x05, 1, {0x00}},
	{0x06, 1, {0x00}},
	{0x07, 1, {0x00}},
	{0x08, 1, {0xC0}},
	{0x09, 1, {0x00}},
	{0x0A, 1, {0x00}},
	{0x0B, 1, {0x04}},
	{0x0C, 1, {0xE6}},
	{0x0D, 1, {0x0D}},
	{0x0F, 1, {0x08}},
	{0x10, 1, {0xE5}},
	{0x11, 1, {0xA8}},
	{0x12, 1, {0xF4}},
	{0x13, 1, {0x74}},
	{0x14, 1, {0x6A}},
	{0x15, 1, {0xD9}},
	{0x16, 1, {0x64}},
	{0x17, 1, {0x21}},
	{0x18, 1, {0x31}},
	{0x19, 1, {0x93}},
	{0x1A, 1, {0xA6}},
	{0x1B, 1, {0x0F}},
	{0x1C, 1, {0xFF}},
	{0x1D, 1, {0xFF}},
	{0x1E, 1, {0xFF}},
	{0x1F, 1, {0xFF}},
	{0x20, 1, {0xFF}},
	{0x21, 1, {0xFF}},
	{0x22, 1, {0xFF}},
	{0x23, 1, {0xFF}},
	{0x24, 1, {0xFF}},
	{0x25, 1, {0xFF}},
	{0x26, 1, {0xFF}},
	{0x27, 1, {0x1F}},
	{0x2D, 1, {0x03}},
	{0x33, 1, {0x09}},
	{0x35, 1, {0x7F}},
	{0x36, 1, {0x0C}},
	{0x38, 1, {0x7F}},
	{0x3A, 1, {0x80}},
	{0x3B, 1, {0x55}},
	{0x3C, 1, {0xE2}},
	{0x3D, 1, {0x32}},
	{0x3E, 1, {0x00}},
	{0x3F, 1, {0x58}},
	{0x40, 1, {0x06}},
	{0x41, 1, {0x80}},
	{0x42, 1, {0xCB}},
	{0x43, 1, {0x2C}},
	{0x44, 1, {0x61}},
	{0x45, 1, {0x39}},
	{0x46, 1, {0x00}},
	{0x47, 1, {0x00}},
	{0x48, 1, {0x8B}},
	{0x49, 1, {0xD2}},
	{0x4A, 1, {0x01}},
	{0x4B, 1, {0x00}},
	{0x4C, 1, {0x10}},
	{0x4D, 1, {0xC0}},
	{0x4E, 1, {0x0F}},
	{0x4F, 1, {0xF1}},
	{0x50, 1, {0x78}},
	{0x51, 1, {0x7A}},
	{0x52, 1, {0x34}},
	{0x53, 1, {0x99}},
	{0x54, 1, {0xA2}},
	{0x55, 1, {0x03}},
	{0x56, 1, {0x6C}},
	{0x57, 1, {0x1A}},
	{0x58, 1, {0x05}},
	{0x59, 1, {0x30}},
	{0x5A, 1, {0x1E}},
	{0x5B, 1, {0x8F}},
	{0x5C, 1, {0xC7}},
	{0x5D, 1, {0xE3}},
	{0x5E, 1, {0xF1}},
	{0x5F, 1, {0x78}},
	{0x60, 1, {0x3C}},
	{0x61, 1, {0x36}},
	{0x62, 1, {0x1E}},
	{0x63, 1, {0x1B}},
	{0x64, 1, {0x8F}},
	{0x65, 1, {0xC7}},
	{0x66, 1, {0xE3}},
	{0x67, 1, {0x31}},
	{0x68, 1, {0x14}},
	{0x69, 1, {0x89}},
	{0x6A, 1, {0x70}},
	{0x6B, 1, {0x8C}},
	{0x6C, 1, {0x8D}},
	{0x6D, 1, {0x8D}},
	{0x6E, 1, {0x8D}},
	{0x6F, 1, {0x8D}},
	{0x70, 1, {0xC7}},
	{0x71, 1, {0xE3}},
	{0x72, 1, {0xF1}},
	{0x73, 1, {0xD8}},
	{0x74, 1, {0xD8}},
	{0x75, 1, {0xD8}},
	{0x76, 1, {0x18}},
	{0x77, 1, {0x00}},
	{0x78, 1, {0x00}},
	{0x79, 1, {0x00}},
	{0x7A, 1, {0xC6}},
	{0x7B, 1, {0xC6}},
	{0x7C, 1, {0xC6}},
	{0x7D, 1, {0xC6}},
	{0x7E, 1, {0xC6}},
	{0x7F, 1, {0xE3}},
	{0x0B, 1, {0x04}},
	{0xB1, 1, {0x03}},
	{0x2C, 1, {0x2C}},               
	{0xB1, 1, {0x00}},
	{0x89, 1, {0x03}}, //display on
	
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
		params->dsi.vertical_sync_active			= 4;
		params->dsi.vertical_backporch				= 10;
		params->dsi.vertical_frontporch 			= 20;
		params->dsi.vertical_active_line			= FRAME_HEIGHT;//1920   1954
	
		params->dsi.horizontal_sync_active			= 18;	
		params->dsi.horizontal_backporch			= 18;
		params->dsi.horizontal_frontporch			= 18;
		params->dsi.horizontal_active_pixel 		= FRAME_WIDTH;//1200  1254

		params->dsi.PLL_CLOCK = 456;
}


static void lcm_init(void)
{
	//avdd_enable(1);
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
   // avdd_enable(0);
    MDELAY(50);
    lcd_reset(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
  lcm_init();
}


struct LCM_DRIVER pf163_jlt_ota7290b_inx_wuxga_101_lcm_drv = 
{
    .name			= "pf163_jlt_ota7290b_inx_wuxga_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
};

