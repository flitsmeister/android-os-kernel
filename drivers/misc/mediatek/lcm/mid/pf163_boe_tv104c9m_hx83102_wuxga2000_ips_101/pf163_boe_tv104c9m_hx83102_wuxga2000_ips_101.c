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
#define FRAME_HEIGHT 										(2000)


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

extern unsigned int GPIO_TP_RST;

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

static void tp_reset(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_TP_RST, 1);
    }
    else
    {
        lcm_set_gpio_output(GPIO_TP_RST, 0);
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
	{0xB9, 3, {0x83,0x10,0x2E} },
	{0xE9, 1, {0xCD} },
	{0xBB, 1, {0x01} },
	{0xE9, 1, {0x00} },
	{0xD1, 4, {0x67,0x2C,0xFF,0x05} },
	{0xBE, 3, {0x11,0x96,0x89} },
	{0xD9, 3, {0x3F,0x00,0x02} },
	{0xB1, 17, {0x10,0xFA,0xAF,0xAF,0x2B,0x2B,0xA2,0x43,0x57,0x36,0x36,0x36,0x36,0x22,0x21,0x15,0x00} },
	{0xB2, 16, {0x00,0xB0,0x47,0xD0,0x00,0x2C,0x2C,0x2C,0x00,0x00,0x00,0x00,0x15,0x20,0xD7,0x00} },
	{0xB4, 16, {0x80,0x60,0x01,0x01,0x80,0x60,0x68,0x50,0x01,0x8E,0x01,0x58,0x00,0xFF,0x00,0xFF} },
	{0xBF, 3, {0xFC,0x85,0x80} },
	{0xD2, 2, {0x2B,0x2B} },
	{0xD3, 43, {0x00,0x00,0x00,0x00,0x01,0x04,0x00,0x08,0x08,0x27,0x27,0x22,0x2F,0x23,0x23,0x04,0x04,0x32,0x10,0x21,0x00,0x21,0x32,0x10,0x1F,0x00,0x02,0x32,0x17,0xFD,0x00,0x10,0x00,0x00,0x20,0x30,0x01,0x55,0x21,0x38,0x01,0x55,0x0F} },
	{0xE0, 46, {0x00,0x06,0x10,0x18,0x20,0x39,0x51,0x59,0x61,0x5F,0x79,0x7F,0x86,0x95,0x92,0x98,0xA0,0xB3,0xB1,0x57,0x5F,0x6A,0x70,0x00,0x06,0x11,0x19,0x20,0x39,0x51,0x59,0x5F,0x5B,0x73,0x79,0x7C,0x89,0x86,0x8C,0x94,0xA5,0xA3,0x51,0x59,0x64,0x76} },
	{0xBD, 1, {0x01} },
	{0xB1, 4, {0x01,0x9B,0x01,0x31} },
	{0xCB, 10, {0xF4,0x36,0x12,0x16,0xC0,0x28,0x6C,0x85,0x3F,0x04} },
	{0xD3, 11, {0x01,0x00,0x7C,0x00,0x00,0x11,0x10,0x00,0x0E,0x00,0x01} },
	{0xBD, 1, {0x02} },
	{0xB4, 6, {0x4E,0x00,0x33,0x11,0x33,0x88} },
	{0xBF, 3, {0xF2,0x00,0x02} },
	{0xBD, 1, {0x00} },
	{0xC0, 14, {0x23,0x23,0x22,0x11,0xA2,0x17,0x00,0x80,0x00,0x00,0x08,0x00,0x63,0x63} },
	{0xC6, 1, {0xF9} },
	{0xC7, 1, {0x30} },
	{0xC8, 6, {0x00,0x04,0x04,0x00,0x00,0x82} },
	{0xD0, 3, {0x07,0x04,0x05} },
	{0xD5, 44, {0x03,0x02,0x01,0x00,0x07,0x06,0x05,0x04,0x18,0x18,0x24,0x24,0x19,0x19,0x18,0x18,0x19,0x19,0x21,0x20,0x23,0x22,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18} },
	{0xD6, 44, {0x04,0x05,0x06,0x07,0x00,0x01,0x02,0x03,0x18,0x18,0x24,0x24,0x19,0x19,0x19,0x19,0x18,0x18,0x22,0x23,0x20,0x21,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18} },
	{0xE7, 23, {0x12,0x13,0x02,0x02,0x53,0x53,0x0E,0x0E,0x00,0x2A,0x27,0x75,0x2A,0x73,0x01,0x27,0x00,0x00,0x00,0x00,0x17,0x00,0x68} },
	{0xBD, 1, {0x01} },
	{0xE7, 7, {0x02,0x34,0x01,0x91,0x0D,0xD8,0x0E} },
	{0xBD, 1, {0x02} },
	{0xE7, 28, {0xFF,0x01,0xFD,0x01,0x00,0x00,0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x81,0x01,0x02,0x40} },
	{0xBD, 1, {0x00} },
	{0xBD, 1, {0x02} },
	{0xD8, 12, {0xFF,0xFF,0xFF,0xFF,0xFF,0xF0,0xFF,0xFF,0xFF,0xFF,0xFF,0xF0} },
	{0xBD, 1, {0x03} },
	{0xB2, 1, {0x80} },
	{0xBD, 1, {0x00} },
	{0xCC, 1, {0x02} },
	{0xE1, 2, {0x01,0x00} },
	{0xBD, 1, {0x03} },
	{0xD8, 24, {0xAA,0xAA,0xA8,0x00,0x00,0x00,0xAA,0xAA,0xA8,0x00,0x00,0x00,0x55,0x55,0x54,0x00,0x00,0x00,0x55,0x55,0x54,0x00,0x00,0x00} },
	{0xBD, 1, {0x00} },
	{0xB9, 4, {0x00,0x00,0x00,0x00} },
	{0x11, 0, {}},
	{REGFLAG_DELAY, 120, {}},
	//sleep out
	{0x29, 0, {}},
	{REGFLAG_DELAY, 20, {}},
//	{REGFLAG_END_OF_TABLE, 0x00, {}},
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
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

		params->dsi.packet_size = 256;
		params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
		params->dsi.vertical_sync_active			= 8;
		params->dsi.vertical_backporch				= 38;
		params->dsi.vertical_frontporch 			= 52;
		params->dsi.vertical_active_line			= FRAME_HEIGHT;//2098
	
		params->dsi.horizontal_sync_active			= 8;	
		params->dsi.horizontal_backporch			= 23;
		params->dsi.horizontal_frontporch			= 40;
		params->dsi.horizontal_active_pixel 		= FRAME_WIDTH;//1271
		params->dsi.ssc_disable = 1;
		params->dsi.PLL_CLOCK = 520;//503;500;--59.13 520-60
}


static void lcm_init(void)
{
	avdd_enable(1);
	MDELAY(30);
	lcd_power_en(1);
	MDELAY(30);

#if defined(CONFIG_MFD_MT6370_PMU)
	//kzhkzh add for bias interface--6V
    pmu_reg_write(0xB2, 0xE8);
    pmu_reg_write(0xB3, 0x28);
    pmu_reg_write(0xB4, 0x28);
    pmu_reg_write(0xB1, 0x5A); 

	//kzhkzh add for bias interface--5.8
/*	pmu_reg_write(0xB2, 0xE5);
    pmu_reg_write(0xB3, 0x24);//0x65
    pmu_reg_write(0xB4, 0x24);
    pmu_reg_write(0xB1, 0x7A);*/

	MDELAY(30);
#endif

	tp_reset(1);
	MDELAY(30);
	
	lcd_reset(1);
	MDELAY(10);
	lcd_reset(0);	
	MDELAY(10);
	lcd_reset(1);
	MDELAY(100);//Must > 5ms
	//init_lcm_registers();
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	MDELAY(10);
}


static void lcm_suspend(void)
{
    unsigned int data_array[16];

	data_array[0]=0x00280500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20); 

	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(30); 

    lcd_reset(0);
    MDELAY(20);
    tp_reset(0);
    MDELAY(20);	
	
#if defined(CONFIG_MFD_MT6370_PMU)
	//kzhkzh add for bias interface-disable
    pmu_reg_write(0xB1, 0x36);
    MDELAY(30);
#endif
	
	lcd_power_en(0);
	MDELAY(20);
    avdd_enable(0);
    MDELAY(20);

}

static void lcm_resume(void)
{
  lcm_init();
}


struct LCM_DRIVER pf163_boe_tv104c9m_hx83102_wuxga2000_ips_101_lcm_drv = 
{
    .name			= "pf163_boe_tv104c9m_hx83102_wuxga2000_ips_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
};

