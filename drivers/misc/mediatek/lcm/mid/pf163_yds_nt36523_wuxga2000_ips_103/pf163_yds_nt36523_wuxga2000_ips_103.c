#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#else
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#endif
#endif
#include <linux/i2c.h>
#include <linux/fs.h>

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(1200)
#define FRAME_HEIGHT 										(2000)

extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_VDD;
extern unsigned int GPIO_TP_RST;
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

#define OCP2138_BIAS_I2C_ENABLE   0//外部i2c偏压

/************* add for bias ic-i2c-control interface start****************************/
#if OCP2138_BIAS_I2C_ENABLE
static struct i2c_client *ocp2138_client = NULL;
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
	struct i2c_client *client = ocp2138_client;
		buf = addr;
		//printk(" %s:%d i2c->addr:0x%x\n",__func__,__LINE__,ocp2138_client->addr);
		ret = i2c_master_send(client, (const char *)&buf, 1);
		if (ret != 1) {
			printk("send command error!!\n");
			return -EFAULT;
		}
		ret = i2c_master_recv(client, (char *)&buf, 1);
		if (ret != 1) {
			printk("reads data error!!\n");
			return -EFAULT;
		}
		else
			printk("%s(0x%02X) = 0x%02X\n", __func__, addr, buf);
		*data = buf;
		return 0;
}
static int ocp2138_write(u8 reg, unsigned char value)
{
        int ret;
        u8 write_cmd[2] = {0};

        write_cmd[0] = reg;
        write_cmd[1] = value;

        ret = i2c_master_send(ocp2138_client, write_cmd, 2);
        if (ret != 2) {
                printk("ocp2138_write error->[REG=0x%02x,val=0x%02x,ret=%d]\n", reg, value, ret);
                return -1;
        }
		 //printk("ocp2138_write success->[REG=0x%02x,val=0x%02x,ret=%d]\n", reg, value, ret);
        return 0;
}
static void ocp2138_dump_register(void)
{
	unsigned char ocp2138_reg[4];
	int reg_index;
	for (reg_index = 0; reg_index < 4; reg_index++) {
		ocp2138_read(reg_index, &ocp2138_reg[reg_index]);
		printk(" [0x%x]=0x%x ", reg_index, ocp2138_reg[reg_index]);
	}
	return;
}
static int ocp2138_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{

	ocp2138_client = i2c;
	
	printk(" %s:%d i2c->addr:0x%x\n",__func__,__LINE__,ocp2138_client->addr);
	if(0x3e == ocp2138_client->addr){
		printk(" %s:%d enter ocp2138_dump_register\n",__func__,__LINE__);
		ocp2138_dump_register();
	}
	return 0;
}

static int __init ocp2138_init(void)
{
	printk("******** ocp2138_init! ********\n");
	if (i2c_add_driver(&ocp2138_driver) != 0) {
		printk("[ocp2138_init] failed to register ocp2138 i2c driver.\n");
	} else {
		printk("[ocp2138_init] Success to register ocp2138 i2c driver.\n");
	}
	
	return 0;
}



static void __exit ocp2138_exit(void)
{
	i2c_del_driver(&ocp2138_driver);
    printk("[ocp2138] ocp2138_exit !!!! \n");	
	
}

module_init(ocp2138_init);
module_exit(ocp2138_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C ocp2138 bias avdd avee Driver");
MODULE_AUTHOR("mid@szroco.com");

#endif
/************* add for bias ic-i2c-control interface end****************************/

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
#if OCP2138_BIAS_I2C_ENABLE
static void lcd_bias_en(unsigned char enabled)
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
#endif
static void vdd_enable(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_VDD, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_VDD, GPIO_OUT_ZERO);
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
	params->dsi.mode               = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM           = LCM_FOUR_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

    params->dsi.vertical_sync_active                            = 4; //2; //4;
    params->dsi.vertical_backporch                              = 4; //10; //16;
    params->dsi.vertical_frontporch                             = 8;//5;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 10; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 10; //60; //60; //80;
    params->dsi.horizontal_frontporch                           = 10; //60;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 468;
	params->dsi.ssc_disable = 1;
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
            // case REGFLAG_MDELAY:
                // MDELAY(table[i].count);
                // break;
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
                break;
		}
    }
}

static __attribute__((unused)) struct LCM_setting_table init_setting[] = {

	{0xFF,1,{0x10}},
	{0xB9,1,{0x01}},
	{0xFF,1,{0x20}},
	{0x18,1,{0x40}},
	{0xFF,1,{0x10}},
	{0xB9,1,{0x02}},
	{0xFF,1,{0xF0}},
	{0xFB,1,{0x01}},
	{0x3A,1,{0x08}},
	{0xFF,1,{0x20}},
	{0xFB,1,{0x01}},

	{0x05,1,{0x01}},
	{0x06,1,{0xC0}},
	{0x07,1,{0x37}},
	{0x08,1,{0x1E}},
	{0x0D,1,{0x23}},
	{0x0E,1,{0x55}},
	{0x0F,1,{0x32}},
	{0x30,1,{0x10}},
	{0x58,1,{0x60}},
	{0x75,1,{0xB1}},
	{0x88,1,{0x00}},
	{0x89,1,{0x29}},
	{0x8A,1,{0x29}},
	{0x94,1,{0x00}},
	{0x95,1,{0xD7}},
	{0x96,1,{0xD7}},
	{0xF2,1,{0x51}},

	{0xFF,1,{0x24}},
	{0xFB,1,{0x01}},

	{0x00,1,{0x00}},
	{0x01,1,{0x2A}},
	{0x02,1,{0x0F}},
	{0x03,1,{0x0F}},
	{0x04,1,{0x0E}},
	{0x05,1,{0x0E}},
	{0x06,1,{0x0D}},
	{0x07,1,{0x0D}},
	{0x08,1,{0x0C}},
	{0x09,1,{0x0C}},
	{0x0A,1,{0x26}},
	{0x0B,1,{0x26}},
	{0x0C,1,{0x29}},
	{0x0D,1,{0x01}},
	{0x0E,1,{0x08}},
	{0x0F,1,{0x08}},
	{0x10,1,{0x23}},
	{0x11,1,{0x23}},
	{0x12,1,{0x22}},
	{0x13,1,{0x22}},
	{0x14,1,{0x05}},
	{0x15,1,{0x04}},
	{0x16,1,{0x00}},
	{0x17,1,{0x2A}},
	{0x18,1,{0x0F}},
	{0x19,1,{0x0F}},
	{0x1A,1,{0x0E}},
	{0x1B,1,{0x0E}},
	{0x1C,1,{0x0D}},
	{0x1D,1,{0x0D}},
	{0x1E,1,{0x0C}},
	{0x1F,1,{0x0C}},
	{0x20,1,{0x26}},
	{0x21,1,{0x26}},
	{0x22,1,{0x29}},
	{0x23,1,{0x01}},
	{0x24,1,{0x08}},
	{0x25,1,{0x08}},
	{0x26,1,{0x23}},
	{0x27,1,{0x23}},
	{0x28,1,{0x22}},
	{0x29,1,{0x22}},
	{0x2A,1,{0x05}},
	{0x2B,1,{0x04}},

	{0x2D,1,{0x00}},
	{0x2F,1,{0x03}},
	{0x30,1,{0x42}},
	{0x33,1,{0x42}},
	{0x34,1,{0x03}},
	{0x37,1,{0x33}},
	{0x39,1,{0x00}},
	{0x3A,1,{0x07}},
	{0x3B,1,{0xBA}},
	{0x3D,1,{0x02}},
	{0xAB,1,{0x33}},

	{0x3F,1,{0x42}},
	{0x43,1,{0x42}},
	{0x47,1,{0x60}},
	{0x49,1,{0x00}},
	{0x4A,1,{0x07}},
	{0x4B,1,{0xBA}},
	{0x4C,1,{0x01}},

	{0x4D,1,{0x21}},
	{0x4E,1,{0x43}},
	{0x4F,1,{0x00}},
	{0x50,1,{0x00}},
	{0x51,1,{0x34}},
	{0x52,1,{0x12}},
	{0x53,1,{0x00}},
	{0x54,1,{0x00}},
	{0x55,2,{0x84,0x04}},
	{0x56,1,{0x04}},
	{0x58,1,{0x10}},
	{0x59,1,{0x10}},
	{0x5A,1,{0x07}},
	{0x5B,1,{0xBA}},
	{0x5C,1,{0x00}},
	{0x5D,1,{0x00}},
	{0x5E,2,{0x00,0x04}},

	{0x60,1,{0x96}},
	{0x61,1,{0xD0}},
	{0x63,1,{0x70}},

	{0x91,1,{0x40}},
	{0x92,1,{0xD9}},
	{0x93,1,{0x1A}},
	{0x94,1,{0x08}},

	{0x96,1,{0x00}},
	{0x97,1,{0xC2}},
	{0xB6,12,{0x05,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x05,0x05,0x00,0x00}},

	{0xC2,1,{0xC6}},
	{0xC3,1,{0x00}},
	{0xC4,1,{0x20}},
	{0xC5,1,{0x00}},
	{0xC6,1,{0x00}},
	{0xC7,1,{0x00}},
	{0xC8,1,{0x00}},
	{0xC9,1,{0x00}},
	{0xCA,1,{0x00}},
	{0xCF,1,{0x0D}},
	{0xD0,1,{0x0D}},
	{0xD1,1,{0x0D}},
	{0xD2,1,{0x0D}},
	{0xD3,1,{0x00}},
	{0xD4,1,{0xFF}},
	{0xD5,1,{0x0F}},
	{0xD6,1,{0x77}},

	{0xDB,1,{0x71}},
	{0xDC,1,{0xDA}},
	{0xDF,1,{0x01}},
	{0xE0,1,{0xDA}},
	{0xE1,1,{0x01}},
	{0xE2,1,{0xDA}},
	{0xE3,1,{0x01}},
	{0xE4,1,{0xDA}},
	{0xE5,1,{0x01}},
	{0xE6,1,{0xDA}},
	{0xEF,1,{0x01}},
	{0xF0,1,{0xDA}},

	{0xFF,1,{0x25}},
	{0xFB,1,{0x01}},
	{0x05,1,{0x00}},  ///////00
	{0x13,1,{0x03}},
	{0x14,1,{0x12}},
	{0x19,1,{0x07}},
	{0x1B,1,{0x11}},

	{0x1E,1,{0x00}},
	{0x1F,1,{0x07}},
	{0x20,1,{0xBA}},
	{0x25,1,{0x00}},
	{0x26,1,{0x07}},
	{0x27,1,{0xBA}},

	//no dummy gck
	{0x3F,1,{0x80}},
	{0x40,1,{0x00}},
	{0x43,1,{0x00}},
	{0x44,1,{0x07}},
	{0x45,1,{0xBA}},
	{0x48,1,{0x07}},
	{0x49,1,{0xBA}},

	{0x6B,1,{0x62}},
	{0x6C,1,{0x0D}},
	{0x6D,1,{0x0D}},
	{0x6E,1,{0x0D}},
	{0x6F,1,{0x0D}},
	{0x70,1,{0x0D}},
	{0x71,1,{0x0D}},
	{0x72,1,{0x0D}},
	{0x73,1,{0x0D}},
	{0x78,1,{0x00}},
	{0x79,1,{0xFF}},
	{0x7A,1,{0x0F}},
	{0x7B,1,{0x77}},
	{0x7C,1,{0x00}},
	{0x7D,1,{0xFF}},
	{0x7E,1,{0x0F}},
	{0x7F,1,{0x77}},
	{0x86,1,{0x02}},
	{0x89,1,{0x04}},
	{0x8A,1,{0x13}},

	{0x5B,1,{0x00}},
	{0x5C,1,{0x00}},
	{0x5D,1,{0x07}},
	{0x5E,1,{0xBA}},
	{0x61,1,{0x07}},
	{0x62,1,{0xBA}},
	{0x67,1,{0x00}},
	{0x68,1,{0x04}},

	{0xC2,1,{0x40}},
	{0xFF,1,{0x26}},
	{0xFB,1,{0x01}},
	{0x00,1,{0xA1}},
	{0x04,1,{0x28}},
	{0x06,1,{0x32}},
	{0x0A,1,{0xF3}},

	{0x0C,1,{0x11}},
	{0x0D,1,{0x00}},
	{0x0F,1,{0x09}},
	{0x11,1,{0x00}},
	{0x12,1,{0x50}},
	{0x13,1,{0x71}},
	{0x14,1,{0x6F}},
	{0x16,1,{0x90}},
	{0x19,1,{0x0F}},
	{0x1A,1,{0xFF}},
	{0x1B,1,{0x0E}},
	{0x1C,1,{0xEC}},
	{0x1D,1,{0x00}},
	{0x1E,1,{0xD9}},
	{0x1F,1,{0xD9}},
	{0x20,1,{0x00}},
	{0x21,1,{0x00}},
	{0x24,1,{0x00}},
	{0x25,1,{0xD9}},
	{0x2A,1,{0x0F}},
	{0x2B,1,{0xFF}},
	{0x2F,1,{0x06}},
	{0x30,1,{0xD9}},
	{0x33,1,{0x66}},
	{0x34,1,{0x66}},
	{0x35,1,{0x66}},
	{0x36,1,{0x11}},
	{0x37,1,{0x11}},
	{0x38,1,{0x01}},

	{0x39,1,{0x00}},
	{0x3A,1,{0xDA}},
	{0xC9,1,{0x00}},
	{0xFF,1,{0x27}},
	{0xFB,1,{0x01}},
	{0x56,1,{0x06}},

	{0x58,1,{0x00}},
	{0x59,1,{0x46}},
	{0x5A,1,{0x00}},
	{0x5B,1,{0x13}},
	{0x5C,1,{0x00}},
	{0x5D,1,{0x01}},
	{0x5E,1,{0x20}},
	{0x5F,1,{0x10}},
	{0x60,1,{0x00}},
	{0x61,1,{0x11}},
	{0x62,1,{0x00}},
	{0x63,1,{0x01}},
	{0x64,1,{0x21}},
	{0x65,1,{0x0F}},
	{0x66,1,{0x00}},
	{0x67,1,{0x01}},
	{0x68,1,{0x22}},

	{0xC0,1,{0x00}},
	{0xFF,1,{0x2A}},
	{0xFB,1,{0x01}},
	{0x22,1,{0x0F}},
	{0x23,1,{0x08}},

	{0x24,1,{0x00}},
	{0x25,1,{0xD4}},
	{0x27,1,{0x00}},
	{0x28,1,{0x1A}},
	{0x29,1,{0x00}},
	{0x2A,1,{0x1A}},
	{0x2B,1,{0x00}},
	{0x2D,1,{0x1A}},

	{0x64,1,{0x41}},
	{0x67,1,{0x41}},
	{0x68,1,{0x23}},
	{0x6A,1,{0x41}},
	{0x79,1,{0x41}},
	{0x7C,1,{0x41}},
	{0x7F,1,{0x41}},
	{0x88,1,{0x41}},
	{0x8B,1,{0xC9}},

	{0x97,1,{0x3C}},
	{0x98,1,{0x02}},
	{0x99,1,{0x95}},
	{0x9A,1,{0x06}},
	{0x9B,1,{0x00}},
	{0x9C,1,{0x0B}},
	{0x9D,1,{0x0A}},
	{0x9E,1,{0x90}},

	{0xA2,1,{0x3F}},
	{0xA3,1,{0xF0}},
	{0xA4,1,{0x03}},
	{0xA5,1,{0x0F}},

	{0xE8,1,{0x2A}},
	{0xFF,1,{0x23}},
	{0xFB,1,{0x01}},

	{0x00,1,{0x80}},
	{0x07,1,{0x00}},
	{0x08,1,{0x01}},
	{0x09,1,{0x2C}},
	{0x11,1,{0x01}},
	{0x12,1,{0x77}},
	{0x15,1,{0x07}},
	{0x16,1,{0x07}},

	{0xFF,1,{0x27}},
	{0xD1,1,{0x45}},
	{0xD2,1,{0x67}},
	{0xFF,1,{0x20}},
	{0x30,1,{0x00}},
	//{0xFF,1,{0x}},2A
	//{0xF1,1,{0x}},04
	{0xFF,1,{0x10}},
	{0xFB,1,{0x01}},
	{0x35,1,{0x00}},
	{0x51,2,{0x0F,0xFF}},
	{0x53,1,{0x2C}},

	{0x55,1,{0x00}},
	{0xBB,1,{0x13}},
	{0x3B,5,{0x03,0x08,0x1A,0x04,0x04}},

};

static void init_lcm_registers(void)
{
	unsigned int data_array[16];
	push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
}

static void lcm_init(void)
{
	vdd_enable(1);
	MDELAY(30);
#if OCP2138_BIAS_I2C_ENABLE
	lcd_bias_en(1);
	MDELAY(10);
	ocp2138_write(0x00,0x11);
	ocp2138_write(0x01,0x11);

#else
	#if defined(CONFIG_MFD_MT6370_PMU)
	// add for bias interface--5.8
	pmu_reg_write(0xB2, 0xE5);
    pmu_reg_write(0xB3, 0x24);//0x65
    pmu_reg_write(0xB4, 0x24);
    pmu_reg_write(0xB1, 0x7A);
	#endif

#endif

	MDELAY(10);
	tp_reset(1);
	MDELAY(10);

	lcd_reset(1);
	MDELAY(20);
	lcd_reset(0);
	MDELAY(50);
	lcd_reset(1);

	MDELAY(120);//Must > 5ms
	init_lcm_registers();
}

static void lcm_suspend(void)
{
	unsigned int data_array[16];
	data_array[0] = 0x00280500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);
	data_array[0] = 0x00100500;
	dsi_set_cmdq(data_array, 1, 1);

	MDELAY(120);
	lcd_reset(0);
	MDELAY(50);
	tp_reset(0);
	MDELAY(50);

#if OCP2138_BIAS_I2C_ENABLE	
	lcd_bias_en(0);
	MDELAY(30);
#else

	#if defined(CONFIG_MFD_MT6370_PMU)
		// add for bias interface-disable
		pmu_reg_write(0xB1, 0x36);
		MDELAY(30);
	#endif
#endif
	vdd_enable(0);
	MDELAY(20);
}

static void lcm_resume(void)
{
	lcm_init();
}

struct LCM_DRIVER pf163_yds_nt36523_wuxga2000_ips_103_lcm_drv =
{
	.name			= "pf163_yds_nt36523_wuxga2000_ips_103",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params		= lcm_get_params,
	.init			= lcm_init,
	.suspend		= lcm_suspend,
	.resume			= lcm_resume,
	//.compare_id	= lcm_compare_id,
};
