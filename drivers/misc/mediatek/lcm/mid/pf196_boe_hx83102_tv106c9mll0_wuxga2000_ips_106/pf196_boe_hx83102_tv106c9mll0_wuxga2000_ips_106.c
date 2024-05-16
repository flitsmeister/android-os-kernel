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

#define FRAME_WIDTH												(1200)
#define FRAME_HEIGHT											(2000)

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

#define UDELAY(n)												(lcm_util.udelay(n))
#define MDELAY(n)												(lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)


#define OCP2138_BIAS_I2C_ENABLE   0

/*************kzhkzh add for bias ic-i2c-control interface start****************************/
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
		//printk("Kzhkzh %s:%d i2c->addr:0x%x\n",__func__,__LINE__,ocp2138_client->addr);
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
		printk("Kzhkzh [0x%x]=0x%x ", reg_index, ocp2138_reg[reg_index]);
	}
	return;
}
static int ocp2138_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{

	ocp2138_client = i2c;
	
	printk("Kzhkzh %s:%d i2c->addr:0x%x\n",__func__,__LINE__,ocp2138_client->addr);
	if(0x3e == ocp2138_client->addr){
		printk("Kzhkzh %s:%d enter ocp2138_dump_register\n",__func__,__LINE__);
		ocp2138_dump_register();
	}
	return 0;
}

static int __init ocp2138_init(void)
{
	printk("******** ocp2138_init! ********\n");
	if (i2c_add_driver(&ocp2138_driver) != 0) {
		printk(
			    "[ocp2138_init] failed to register ocp2138 i2c driver.\n");
	} else {
		printk(
			    "[ocp2138_init] Success to register ocp2138 i2c driver.\n");
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

	params->dsi.vertical_sync_active                            = 8;
	params->dsi.vertical_backporch                              = 38;
	params->dsi.vertical_frontporch                             = 80;
	params->dsi.vertical_active_line                            = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active                          = 8;
	params->dsi.horizontal_backporch                            = 28;
	params->dsi.horizontal_frontporch                           = 40;
	params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;//1200

	params->dsi.PLL_CLOCK = 500;
	params->dsi.ssc_disable = 1;
}

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static void __attribute__((unused)) push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
#define REGFLAG_MDELAY			0xFFFC
#define REGFLAG_UDELAY			0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW		0xFFFE
#define REGFLAG_RESET_HIGH		0xFFFF
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

	{0xB9,3,{0x83,0x10,0x2E}},
	{0xE9,1,{0xCD}},
	{0xBB,1,{0x01}},
	{0xE9,1,{0x00}},
	{0xD1,4,{0x67,0x0C,0xFF,0x05}},
	{0xB1,17,{0x10,0xFA,0xAF,0xAF,0x2B,0x2B,0xC1,0x75,0x39,0x36,0x36,0x36,0x36,0x22,0x21,0x15,0x00}},
	{0xB2,16,{0x00,0xB0,0x47,0xD0,0x00,0x2C,0x50,0x2C,0x00,0x00,0x00,0x00,0x15,0x20,0x57,0x00}},
	{0xB4,16,{0x38,0x47,0x38,0x47,0x66,0x4E,0x00,0x00,0x01,0x72,0x01,0x58,0x00,0xFF,0x00,0xFF}},
	{0xBF,3,{0xFC,0x85,0x80}},
	{0xD2,2,{0x2B,0x2B}},
	{0xD3,43,{0x00,0x00,0x00,0x00,0x78,0x04,0x00,0x14,0x00,0x27,0x00,0x44,0x4F,0x29,0x29,0x00,0x00,0x32,0x10,0x25,0x00,0x25,0x32,0x10,0x1F,0x00,0x1F,0x32,0x18,0x10,0x08,0x10,0x00,0x00,0x20,0x30,0x01,0x55,0x21,0x2E,0x01,0x55,0x0F}},
	{0xE0,46,{0x00,0x03,0x0B,0x11,0x18,0x26,0x3E,0x45,0x4E,0x4B,0x67,0x6E,0x77,0x89,0x89,0x94,0x9E,0xB1,0xB1,0x57,0x5E,0x68,0x70,0x00,0x03,0x0B,0x11,0x18,0x26,0x3E,0x45,0x4E,0x4B,0x67,0x6E,0x77,0x89,0x89,0x94,0x9E,0xB1,0xB1,0x57,0x5E,0x68,0x70}},
	{0xC1,1,{0x01}},
	{0xBD,1,{0x01}},
	{0xC1,58,{0x00,0x04,0x08,0x0C,0x10,0x14,0x18,0x1C,0x20,0x24,0x28,0x2C,0x30,0x34,0x38,0x3C,0x40,0x45,0x49,0x4D,0x55,0x5D,0x65,0x6D,0x75,0x7D,0x85,0x8D,0x95,0x9C,0xA4,0xAC,0xB5,0xBC,0xC4,0xCD,0xD5,0xDC,0xE4,0xEC,0xF4,0xF8,0xFA,0xFC,0xFE,0xFF,0x00,0x54,0x56,0x5F,0xF5,0xBA,0xA4,0xFB,0x68,0xFF,0xA5,0x00}},
	{0xBD,1,{0x02}},
	{0xC1,58,{0x00,0x04,0x08,0x0C,0x10,0x14,0x18,0x1C,0x20,0x24,0x28,0x2C,0x30,0x34,0x38,0x3C,0x40,0x44,0x48,0x4C,0x54,0x5C,0x64,0x6C,0x74,0x7C,0x84,0x8C,0x93,0x9B,0xA3,0xAB,0xB4,0xBB,0xC3,0xCC,0xD4,0xDB,0xE4,0xEC,0xF4,0xF8,0xFA,0xFC,0xFE,0xFF,0x00,0x54,0x56,0x5F,0xF5,0xBA,0xA4,0xFB,0x68,0x30,0x00,0x00}},
	{0xBD,1,{0x03}},
	{0xC1,58,{0x00,0x04,0x08,0x0C,0x10,0x14,0x18,0x1C,0x20,0x24,0x28,0x2C,0x30,0x34,0x38,0x3C,0x40,0x44,0x48,0x4C,0x54,0x5C,0x64,0x6B,0x73,0x7B,0x83,0x8B,0x92,0x9A,0xA2,0xAA,0xB3,0xBA,0xC2,0xCB,0xD3,0xDA,0xE3,0xEB,0xF3,0xF7,0xF9,0xFC,0xFE,0xFF,0x00,0x54,0x56,0x5F,0xF5,0xBA,0xA4,0xFB,0x68,0x3A,0xFC,0x00}},
	{0xBD,1,{0x00}},
	{0xCB,5,{0x00,0x13,0x08,0x02,0x34}},
	{0xBD,1,{0x01}},
	{0xB1,4,{0x01,0x9B,0x01,0x31}},
	{0xCB,10,{0xF4,0x36,0x12,0x16,0xC0,0x28,0x6C,0x85,0x3F,0x04}},
	{0xD3,11,{0x01,0x00,0x3C,0x00,0x00,0x11,0x10,0x00,0x0E,0x00,0x01}},
	{0xBD,1,{0x02}},
	{0xB4,6,{0x4E,0x00,0x33,0x11,0x33,0x88}},
	{0xBF,3,{0xF2,0x00,0x02}},
	{0xBD,1,{0x00}},
	{0xC0,14,{0x23,0x23,0x22,0x11,0xA2,0x17,0x00,0x80,0x00,0x00,0x08,0x00,0x63,0x63}},
	{0xC6,1,{0xF9}},
	{0xC7,1,{0x30}},
	{0xC8,8,{0x00,0x04,0x04,0x00,0x00,0x85,0x43,0xFF}},
	{0xD0,3,{0x07,0x04,0x05}},
	{0xD5,44,{0x21,0x20,0x21,0x20,0x25,0x24,0x25,0x24,0x18,0x18,0x18,0x18,0x1A,0x1A,0x1A,0x1A,0x1B,0x1B,0x1B,0x1B,0x03,0x02,0x03,0x02,0x01,0x00,0x01,0x00,0x07,0x06,0x07,0x06,0x05,0x04,0x05,0x04,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}},
	{0xE7,23,{0x12,0x13,0x02,0x02,0x57,0x57,0x0E,0x0E,0x1B,0x28,0x29,0x74,0x28,0x74,0x01,0x07,0x00,0x00,0x00,0x00,0x17,0x00,0x68}},
	{0xBD,1,{0x01}},
	{0xE7,7,{0x02,0x38,0x01,0x93,0x0D,0xD9,0x0E}},
	{0xBD,1,{0x02}},
	{0xE7,28,{0xFF,0x01,0xFF,0x01,0x00,0x00,0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x81,0x00,0x02,0x40}},
	{0xBD,1,{0x00}},
	{0xBA,8,{0x70,0x03,0xA8,0x83,0xF2,0x00,0xC0,0x0D}},
	{0xBD,1,{0x02}},
	{0xD8,12,{0xAF,0xFF,0xFF,0xFF,0xF0,0x00,0xAF,0xFF,0xFF,0xFF,0xF0,0x00}},
	{0xBD,1,{0x03}},
	{0xD8,24,{0xAA,0xAA,0xAA,0xAA,0xA0,0x00,0xAA,0xAA,0xAA,0xAA,0xA0,0x00,0x55,0x55,0x55,0x55,0x50,0x00,0x55,0x55,0x55,0x55,0x50,0x00}},
	{0xBD,1,{0x00}},
	{0xE1,2,{0x01,0x05}},
	{0xCC,1,{0x02}},
	{0xBD,1,{0x03}},
	{0xB2,1,{0x80}},
	{0xBD,1,{0x00}},

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
	//kzhkzh add for bias interface--5.8
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
		//kzhkzh add for bias interface-disable
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

struct LCM_DRIVER pf196_boe_hx83102_tv106c9mll0_wuxga2000_ips_106_lcm_drv =
{
	.name			= "pf196_boe_hx83102_tv106c9mll0_wuxga2000_ips_106",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params		= lcm_get_params,
	.init			= lcm_init,
	.suspend		= lcm_suspend,
	.resume			= lcm_resume,
	//.compare_id	= lcm_compare_id,
};
