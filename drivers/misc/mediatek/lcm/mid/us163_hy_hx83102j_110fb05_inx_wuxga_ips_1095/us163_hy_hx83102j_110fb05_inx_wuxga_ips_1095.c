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

#define REGFLAG_DELAY             							0xFFFC
#define REGFLAG_END_OF_TABLE      							0xFFFE   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util;

extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_1v8_EN;
extern unsigned int GPIO_TP_RST;
extern unsigned int GPIO_LCM_BIAS_ENN;
extern unsigned int GPIO_LCM_BIAS_ENP;
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

#define OCP2138_BIAS_I2C_ENABLE 1
/*************kzhkzh add for bias ic-i2c-control interface start****************************/
#if OCP2138_BIAS_I2C_ENABLE
static struct i2c_client *_ocp2138_client = NULL;
static const struct i2c_device_id ocp2138_i2c_id[] = { {"ocp2138", 0}, {} };

static int ocp2138_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);

#ifdef CONFIG_OF
static const struct of_device_id ocp2138_of_match[] = {
	{.compatible = "mid,ocp2138",},
	{},
};

MODULE_DEVICE_TABLE(i2c, ocp2138_of_match);
#endif

static struct i2c_driver ocp2138_driver = {
	.driver = {
		   .name = "ocp2138",
#ifdef CONFIG_OF
		   .of_match_table = ocp2138_of_match,
#endif
	},
	.probe = ocp2138_i2c_probe,
	.id_table = ocp2138_i2c_id,
};

static int ocp2138_read(u8 addr, u8 *data)
{
	u8 buf;
	int ret = 0;
	struct i2c_client *client = _ocp2138_client;
		buf = addr;

	if(client == NULL)
		return -1;
		 
	ret = i2c_master_send(client, (const char *)&buf, 1);
	if (ret != 1) {
		pr_info("send command error!!\n");
		return -EFAULT;
	}
	ret = i2c_master_recv(client, (char *)&buf, 1);
	if (ret != 1) {
		printk("reads data error!!\n");
		return -EFAULT;
	}
	else
		pr_info("%s(0x%02X) = 0x%02X\n", __func__, addr, buf);
	*data = buf;
	return 0;
}
static int ocp2138_write(u8 reg, unsigned char value)
{
        int ret;
        u8 write_cmd[2] = {0};
		struct i2c_client *client = _ocp2138_client;

        write_cmd[0] = reg;
        write_cmd[1] = value;

		if(client == NULL)
			 return -1;
		 
        ret = i2c_master_send(client, write_cmd, 2);
        if (ret != 2) {
                pr_info("ocp2138_write error->[REG=0x%02x,val=0x%02x,ret=%d]\n", reg, value, ret);
                return -1;
        }
        return 0;
}
static void ocp2138_dump_register(void)
{
	unsigned char ocp2138_reg[4];
	int reg_index;
	for (reg_index = 0; reg_index < 4; reg_index++) {
		ocp2138_read(reg_index, &ocp2138_reg[reg_index]);
		pr_info("[0x%x]=0x%x ", reg_index, ocp2138_reg[reg_index]);
	}
	return;
}
static int ocp2138_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{

	_ocp2138_client = i2c;
	
	pr_info("%s:%d i2c->addr:0x%x\n",__func__,__LINE__,_ocp2138_client->addr);
	
	if(0x3e == _ocp2138_client->addr){
		pr_info("%s:%d enter ocp2138_dump_register\n",__func__,__LINE__);
		ocp2138_dump_register();
	}
	return 0;
}

static int __init ocp2138_init(void)
{
	pr_info("******** ocp2138_init! ********\n");
	
	if (i2c_add_driver(&ocp2138_driver) != 0) {
		pr_info("[ocp2138_init] failed to register ocp2138 i2c driver.\n");
	} else {
		pr_info("[ocp2138_init] Success to register ocp2138 i2c driver.\n");
	}
	
	return 0;
}

static void __exit ocp2138_exit(void)
{
	i2c_del_driver(&ocp2138_driver);
    pr_info("[ocp2138] ocp2138_exit !!!! \n");	
}

module_init(ocp2138_init);
module_exit(ocp2138_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C ocp2138 bias avdd avee Driver");
MODULE_AUTHOR("kuangzenghui@szroco.com");

#endif
/*************kzhkzh add for bias ic-i2c-control interface end****************************/

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
        lcm_set_gpio_output(GPIO_LCM_1v8_EN, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_1v8_EN, GPIO_OUT_ZERO);
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

static void lcd_bias_en(unsigned char enabled)
{
	if (enabled)
	{
		lcm_set_gpio_output(GPIO_LCM_BIAS_ENN, GPIO_OUT_ONE);  //+6.0
		lcm_set_gpio_output(GPIO_LCM_BIAS_ENP, GPIO_OUT_ONE);  //-6.0
		MDELAY(5);
		#if OCP2138_BIAS_I2C_ENABLE
		ocp2138_write(0x00,0x14);
	    ocp2138_write(0x01,0x14);
		#endif
	}
	else
	{
		lcm_set_gpio_output(GPIO_LCM_BIAS_ENN, GPIO_OUT_ZERO);  //+6.0
		lcm_set_gpio_output(GPIO_LCM_BIAS_ENP, GPIO_OUT_ZERO);  //-6.0
		MDELAY(5);
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
	{0xB9,5 ,{0x83,0x10,0x21,0x55,0x00}},																																					
	{0xE9,1 ,{0xC4}},																																									
	{0xD9,1 ,{0xD2}},																																										
	{0xE9,1 ,{0x3F}},																																									
	{0xB1,26,{0x2C,0xAB,0xAB,0x27,0xE7,0x42,0xE1,0x43,0x36,0x36,0x36,0x36,0x1A,0x8B,0x11,0x65,0x00,0x88,0xFA,0xFF,0xFF,0x8F,0xFF,0x08,0x9A,0x33}},																
	{0xB2,15,{0x00,0x47,0xB0,0x80,0x00,0x12,0x61,0x3C,0xA4,0x00,0x00,0x00,0x00,0x88,0xF5}},																										
	{0xB4,12,{0x80,0x80,0x80,0x80,0x80,0x80,0x98,0x7F,0x58,0x4C,0x01,0x9B}},																													
	{0xB6,3 ,{0x5C,0x5C,0x03}},																																						
	{0xE9,1 ,{0xCD}},																																										
	{0xBA,1 ,{0x84}},																																										
	{0xE9,1 ,{0x3F}},																																										
	{0xE9,1 ,{0xD1}},																																										
	{0xB4,1 ,{0x00}},																																										
	{0xE9,1 ,{0x3F}},																																										
	{0xBC,2 ,{0x1B,0x04}},																																								
	{0xBE,1 ,{0x20}},																																										
	{0xBF,8 ,{0xFC,0xC4,0x80,0x9C,0x36,0x00,0x0D,0x04}},																																		
	{0xC0,10,{0x34,0x34,0x22,0x11,0x22,0xA0,0x61,0x08,0xF5,0x03}},																																
	{0xC2,3 ,{0x43,0xFF,0x00}},																																								
	{0xC7,20,{0x0D,0x83,0x14,0xAF,0x24,0x89,0x00,0x00,0xBE,0x00,0x60,0x80,0x03,0xB2,0x04,0x60,0x10,0x80,0x22,0x40}},																					
	{0xE9,1 ,{0xC6}},																																									
	{0xC8,1 ,{0x97}},																																										
	{0xE9,1 ,{0x3F}},																																										
	{0xC9,5 ,{0x00,0x1E,0x13,0x88,0x01}},																																				
	{0xCB,6 ,{0x08,0x13,0x07,0x00,0x0F,0x53}},																																				
	{0xCC,3 ,{0x0E,0x03,0x44}},	//02 正扫 03 反扫																																		
	{0xE9,1 ,{0xC4}},																																										
	{0xD0,1 ,{0x03}},																																										
	{0xE9,1 ,{0x3F}},																																										
	{0xD1,7 ,{0x07,0x06,0x00,0x02,0x04,0x2C,0xFF}},																																			
	{0xD3,34,{0x06,0x00,0x00,0x00,0x00,0x10,0x08,0x10,0x08,0x6F,0x47,0x94,0x37,0x10,0x10,0x03,0x03,0x54,0x10,0x0D,0x00,0x0D,0x32,0x17,0xA3,0x07,0xA3,0x32,0x10,0x08,0x00,0x08,0x00,0x00}},								
	{0xD5,44,{0x26,0x27,0x24,0x25,0x19,0x19,0x18,0x18,0x18,0x18,0x1E,0x1E,0x1F,0x1F,0x18,0x18,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x22,0x23,0x20,0x21,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}},
	{0xD6,44,{0x21,0x20,0x23,0x22,0x18,0x18,0x19,0x19,0x18,0x18,0x1E,0x1E,0x1F,0x1F,0x40,0x40,0x0F,0x0E,0x0D,0x0C,0x0B,0x0A,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00,0x25,0x24,0x27,0x26,0x40,0x40,0x18,0x18,0x18,0x18,0x18,0x18}},
	{0xD8,36 ,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},					
	{0xE7,25 ,{0x10,0x0E,0x0E,0x23,0x2D,0x9F,0x01,0x32,0xA1,0x14,0x14,0x00,0x00,0x00,0x00,0xF0,0x07,0x01,0x02,0x00,0x33,0x03,0x84,0x18,0x01}},															
	{0xBD,1 ,{0x01}},																																										
	{0xB1,4 ,{0x01,0x7F,0x11,0xFD}},																																					
	{0xCB,1 ,{0x86}},																																										
	{0xE9,1 ,{0xC5}},																																										
	{0xD3,7 ,{0x00,0x00,0x00,0x80,0x80,0x0C,0x81}},																																			
	{0xE9,1 ,{0x3F}},																																										
	{0xE7,13,{0x02,0x00,0x2A,0x01,0x8D,0x0D,0xCD,0x0E,0xA0,0x00,0x08,0x20,0x40}},																													
	{0xBD,1 ,{0x02}},																																									
	{0xCB,5 ,{0x03,0x07,0x00,0x10,0x9A}},																																				
	{0xBF,1 ,{0xF2}},																																										
	{0xD8,12,{0xFF,0xFE,0xFF,0xFF,0xFA,0xB0,0xFF,0xFE,0xFF,0xFF,0xFA,0xB0}},																															
	{0xE7,35,{0xFB,0x02,0xFB,0x02,0xFB,0x02,0x01,0x01,0x01,0x27,0x00,0x27,0x81,0x02,0x40,0x00,0x20,0x9B,0x06,0x05,0x04,0x03,0x02,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02}},							
	{0xBD,1 ,{0x03}},																																										
	{0xE9,1 ,{0xC6}},																																										
	{0xB4,3 ,{0x03,0xFF,0xF8}},																																							
	{0xE9,1 ,{0x3F}},																																										
	{0xD8,24,{0xAA,0xAA,0xAA,0xAA,0xAA,0xA0,0xAA,0xAA,0xAA,0xAA,0xAA,0xA0,0xFF,0xFE,0xFF,0xFF,0xFA,0xB0,0xFF,0xFE,0xFF,0xFF,0xFA,0xB0}},																	
	{0xE1,1 ,{0x00}},																																										
	{0xBD,1 ,{0x00}},																																										
	{0xE9,1 ,{0xC4}},																																										
	{0xBA,1 ,{0x96}},																																										
	{0xE9,1 ,{0x3F}},																																										
	{0xBD,1 ,{0x01}},																																										
	{0xE9,1 ,{0xC5}},																																										
	{0xBA,1 ,{0x4F}},																																										
	{0xE9,1 ,{0x3F}},																																										
	{0xBD,1 ,{0x00}},																																										
	{0xB9,4 ,{0x00,0x00,0x00,0x00}},
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
		params->dsi.vertical_sync_active				= 8;
		params->dsi.vertical_backporch				= 12;
		params->dsi.vertical_frontporch 				= 99;
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 
	
		params->dsi.horizontal_sync_active			= 20;	
		params->dsi.horizontal_backporch				= 40;	
		params->dsi.horizontal_frontporch			= 60;
		params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;

		params->dsi.PLL_CLOCK = 514;
		params->dsi.ssc_disable=1;
}

static void lcm_init(void)
{
	avdd_enable(1);
	lcd_power_en(1);
	MDELAY(100); 
	lcd_bias_en(1);
	MDELAY(5);	
	lcd_reset(1);
	tp_reset(1);
	MDELAY(20);
	lcd_reset(0);	
	MDELAY(20);
	lcd_reset(1);
	MDELAY(50);//Must > 5ms
	
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	MDELAY(100);
}


static void lcm_suspend(void)
{
	lcd_power_en(0);
	MDELAY(10);
	lcd_bias_en(0);
	MDELAY(10);
	avdd_enable(0);
	MDELAY(50);
	tp_reset(0);
	lcd_reset(0);
	MDELAY(20);
}

static void lcm_resume(void)
{
  lcm_init();
}


struct LCM_DRIVER us163_hy_hx83102j_110fb05_inx_wuxga_ips_1095_lcm_drv = 
{
    .name			= "us163_hy_hx83102j_110fb05_inx_wuxga_ips_1095",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
};

