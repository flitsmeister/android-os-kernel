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

#define FRAME_WIDTH                                             (1200)
#define FRAME_HEIGHT                                            (2000)

extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_VDD;
extern unsigned int GPIO_TP_RST;

extern unsigned int GPIO_LCM_BIAS_EN;
#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0


#define LCM_DSI_CMD_MODE                                    0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util = {0};

#define UDELAY(n)                                                (lcm_util.udelay(n))
#define MDELAY(n)                                                (lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                        lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                    lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)                lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define OCP2138_BIAS_I2C_ENABLE   1

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

    params->dsi.vertical_sync_active                            = 2;
    params->dsi.vertical_backporch                              = 14;
    params->dsi.vertical_frontporch                             = 16;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 10;
    params->dsi.horizontal_backporch                            = 70;
    params->dsi.horizontal_frontporch                           = 30;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 479;
    params->dsi.ssc_disable = 1;
}

struct LCM_setting_table {
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static void __attribute__((unused)) push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
#define REGFLAG_MDELAY            0xFFFC
#define REGFLAG_UDELAY            0xFFFB
#define REGFLAG_END_OF_TABLE    0xFFFD
#define REGFLAG_RESET_LOW        0xFFFE
#define REGFLAG_RESET_HIGH        0xFFFF
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
    {0xB0, 1, {0x05}},    //    Page5
    {0xB3, 1, {0x52}},    //    Data Settle
    {0xB8, 1, {0x7F}},    //    Master OSC FIX
    {0xBC, 1, {0x20}},    //    Disable ERR
    {0xD6, 1, {0x7F}},    //    Slave OSC FIX

    {0xB0, 1, {0x01}},    //    Page1
    {0xC0, 1, {0x00}},    //    GIP MUX
    {0xC1, 1, {0x00}},    //    GIP MUX
    {0xC2, 1, {0x00}},    //    GIP MUX
    {0xC3, 1, {0x00}},    //    GIP MUX
    {0xC4, 1, {0x26}},    //    GIP MUX
    {0xC5, 1, {0x00}},    //    GIP MUX
    {0xC6, 1, {0x11}},    //    GIP MUX
    {0xC7, 1, {0x12}},    //    GIP MUX
    {0xC8, 1, {0x06}},    //    GIP MUX
    {0xC9, 1, {0x08}},    //    GIP MUX
    {0xCA, 1, {0x0A}},    //    GIP MUX
    {0xCB, 1, {0x0C}},    //    GIP MUX
    {0xCC, 1, {0x02}},    //    GIP MUX
    {0xCD, 1, {0x10}},    //    GIP MUX
    {0xCE, 1, {0x00}},    //    GIP MUX
    {0xCF, 1, {0x00}},    //    GIP MUX
    {0xD0, 1, {0x00}},    //    GIP MUX
    {0xD1, 1, {0x00}},    //    GIP MUX
    {0xD2, 1, {0x00}},    //    GIP MUX
    {0xD3, 1, {0x00}},    //    GIP MUX
    {0xD4, 1, {0x00}},    //    GIP MUX
    {0xD5, 1, {0x00}},    //    GIP MUX
    {0xD6, 1, {0x00}},    //    GIP MUX
    {0xD7, 1, {0x00}},    //    GIP MUX
    {0xD8, 1, {0x26}},    //    GIP MUX
    {0xD9, 1, {0x00}},    //    GIP MUX
    {0xDA, 1, {0x11}},    //    GIP MUX
    {0xDB, 1, {0x12}},    //    GIP MUX
    {0xDC, 1, {0x05}},    //    GIP MUX
    {0xDD, 1, {0x07}},    //    GIP MUX
    {0xDE, 1, {0x09}},    //    GIP MUX
    {0xDF, 1, {0x0B}},    //    GIP MUX
    {0xE0, 1, {0x01}},    //    GIP MUX
    {0xE1, 1, {0x0F}},    //    GIP MUX
    {0xE2, 1, {0x00}},    //    GIP MUX
    {0xE3, 1, {0x00}},    //    GIP MUX
    {0xE4, 1, {0x00}},    //    GIP MUX
    {0xE5, 1, {0x00}},    //    GIP MUX
    {0xE6, 1, {0x00}},    //    GIP MUX
    {0xE7, 1, {0x00}},    //    GIP MUX

    {0xB0, 1, {0x07}},    //    Page7

    {0xB1, 1, {0x00}},    //    V255_7:0
    {0xB2, 1, {0xBC}},    //    V254_7:0
    {0xB3, 1, {0xC6}},    //    V252_7:0
    {0xB4, 1, {0xDA}},    //    V248_7:0
    {0xB5, 1, {0xE9}},    //    V244_7:0
    {0xB6, 1, {0xFB}},    //    V240_7:0
    {0xB7, 1, {0x17}},    //    V232_7:0
    {0xB8, 1, {0x31}},    //    V224_7:0
    {0xB9, 1, {0x62}},    //    V208_7:0
    {0xBA, 1, {0x91}},    //    V192_7:0
    {0xBB, 1, {0xF0}},    //    V160_7:0
    {0xBC, 1, {0x52}},    //    V128_7:0
    {0xBD, 1, {0x56}},    //    V127_7:0
    {0xBE, 1, {0xCC}},    //    V95_7:0
    {0xBF, 1, {0x46}},    //    V63_7:0
    {0xC0, 1, {0x84}},    //    V47_7:0
    {0xC1, 1, {0xAF}},    //    V31_7:0
    {0xC2, 1, {0xC5}},    //    V23_7:0
    {0xC3, 1, {0xDB}},    //    V15_7:0
    {0xC4, 1, {0xE6}},    //    V11_7:0
    {0xC5, 1, {0xF0}},    //    V7_7:0
    {0xC6, 1, {0xF8}},    //    V3_7:0
    {0xC7, 1, {0xFC}},    //    V1_7:0
    {0xC8, 1, {0xFC}},    //    V0_7:0

    {0xC9, 1, {0x00}},
    {0xCA, 1, {0x00}},
    {0xCB, 1, {0x05}},
    {0xCC, 1, {0x6B}},
    {0xCD, 1, {0xFF}},
    {0xCE, 1, {0xFF}},

    {0xB0, 1, {0x0A}},    //    Page10

    {0xB1, 1, {0x00}},    //    V255_7:0
    {0xB2, 1, {0xBC}},    //    V254_7:0
    {0xB3, 1, {0xC6}},    //    V252_7:0
    {0xB4, 1, {0xDA}},    //    V248_7:0
    {0xB5, 1, {0xE9}},    //    V244_7:0
    {0xB6, 1, {0xFB}},    //    V240_7:0
    {0xB7, 1, {0x17}},    //    V232_7:0
    {0xB8, 1, {0x31}},    //    V224_7:0
    {0xB9, 1, {0x62}},    //    V208_7:0
    {0xBA, 1, {0x91}},    //    V192_7:0
    {0xBB, 1, {0xF0}},    //    V160_7:0
    {0xBC, 1, {0x52}},    //    V128_7:0
    {0xBD, 1, {0x56}},    //    V127_7:0
    {0xBE, 1, {0xCC}},    //    V95_7:0
    {0xBF, 1, {0x46}},    //    V63_7:0
    {0xC0, 1, {0x84}},    //    V47_7:0
    {0xC1, 1, {0xAF}},    //    V31_7:0
    {0xC2, 1, {0xC5}},    //    V23_7:0
    {0xC3, 1, {0xDB}},    //    V15_7:0
    {0xC4, 1, {0xE6}},    //    V11_7:0
    {0xC5, 1, {0xF0}},    //    V7_7:0
    {0xC6, 1, {0xF8}},    //    V3_7:0
    {0xC7, 1, {0xFC}},    //    V1_7:0
    {0xC8, 1, {0xFC}},    //    V0_7:0

    {0xC9, 1, {0x00}},
    {0xCA, 1, {0x00}},
    {0xCB, 1, {0x05}},
    {0xCC, 1, {0x6B}},
    {0xCD, 1, {0xFF}},
    {0xCE, 1, {0xFF}},

    {0xB0, 1, {0x08}},    //    Page8

    {0xB1, 1, {0x00}},    //    V255_7:0
    {0xB2, 1, {0xBC}},    //    V254_7:0
    {0xB3, 1, {0xC6}},    //    V252_7:0
    {0xB4, 1, {0xDA}},    //    V248_7:0
    {0xB5, 1, {0xE9}},    //    V244_7:0
    {0xB6, 1, {0xFB}},    //    V240_7:0
    {0xB7, 1, {0x17}},    //    V232_7:0
    {0xB8, 1, {0x31}},    //    V224_7:0
    {0xB9, 1, {0x62}},    //    V208_7:0
    {0xBA, 1, {0x91}},    //    V192_7:0
    {0xBB, 1, {0xF0}},    //    V160_7:0
    {0xBC, 1, {0x52}},    //    V128_7:0
    {0xBD, 1, {0x56}},    //    V127_7:0
    {0xBE, 1, {0xCC}},    //    V95_7:0
    {0xBF, 1, {0x46}},    //    V63_7:0
    {0xC0, 1, {0x84}},    //    V47_7:0
    {0xC1, 1, {0xAF}},    //    V31_7:0
    {0xC2, 1, {0xC5}},    //    V23_7:0
    {0xC3, 1, {0xDB}},    //    V15_7:0
    {0xC4, 1, {0xE6}},    //    V11_7:0
    {0xC5, 1, {0xF0}},    //    V7_7:0
    {0xC6, 1, {0xF8}},    //    V3_7:0
    {0xC7, 1, {0xFC}},    //    V1_7:0
    {0xC8, 1, {0xFC}},    //    V0_7:0

    {0xC9, 1, {0x00}},
    {0xCA, 1, {0x00}},
    {0xCB, 1, {0x05}},
    {0xCC, 1, {0x6B}},
    {0xCD, 1, {0xFF}},
    {0xCE, 1, {0xFF}},

    {0xB0, 1, {0x0B}},    //    Page11

    {0xB1, 1, {0x00}},    //    V255_7:0
    {0xB2, 1, {0xBC}},    //    V254_7:0
    {0xB3, 1, {0xC6}},    //    V252_7:0
    {0xB4, 1, {0xDA}},    //    V248_7:0
    {0xB5, 1, {0xE9}},    //    V244_7:0
    {0xB6, 1, {0xFB}},    //    V240_7:0
    {0xB7, 1, {0x17}},    //    V232_7:0
    {0xB8, 1, {0x31}},    //    V224_7:0
    {0xB9, 1, {0x62}},    //    V208_7:0
    {0xBA, 1, {0x91}},    //    V192_7:0
    {0xBB, 1, {0xF0}},    //    V160_7:0
    {0xBC, 1, {0x52}},    //    V128_7:0
    {0xBD, 1, {0x56}},    //    V127_7:0
    {0xBE, 1, {0xCC}},    //    V95_7:0
    {0xBF, 1, {0x46}},    //    V63_7:0
    {0xC0, 1, {0x84}},    //    V47_7:0
    {0xC1, 1, {0xAF}},    //    V31_7:0
    {0xC2, 1, {0xC5}},    //    V23_7:0
    {0xC3, 1, {0xDB}},    //    V15_7:0
    {0xC4, 1, {0xE6}},    //    V11_7:0
    {0xC5, 1, {0xF0}},    //    V7_7:0
    {0xC6, 1, {0xF8}},    //    V3_7:0
    {0xC7, 1, {0xFC}},    //    V1_7:0
    {0xC8, 1, {0xFC}},    //    V0_7:0

    {0xC9, 1, {0x00}},
    {0xCA, 1, {0x00}},
    {0xCB, 1, {0x05}},
    {0xCC, 1, {0x6B}},
    {0xCD, 1, {0xFF}},
    {0xCE, 1, {0xFF}},

    {0xB0, 1, {0x09}},    //    Page9

    {0xB1, 1, {0x00}},    //    V255_7:0
    {0xB2, 1, {0xBC}},    //    V254_7:0
    {0xB3, 1, {0xC6}},    //    V252_7:0
    {0xB4, 1, {0xDA}},    //    V248_7:0
    {0xB5, 1, {0xE9}},    //    V244_7:0
    {0xB6, 1, {0xFB}},    //    V240_7:0
    {0xB7, 1, {0x17}},    //    V232_7:0
    {0xB8, 1, {0x31}},    //    V224_7:0
    {0xB9, 1, {0x62}},    //    V208_7:0
    {0xBA, 1, {0x91}},    //    V192_7:0
    {0xBB, 1, {0xF0}},    //    V160_7:0
    {0xBC, 1, {0x52}},    //    V128_7:0
    {0xBD, 1, {0x56}},    //    V127_7:0
    {0xBE, 1, {0xCC}},    //    V95_7:0
    {0xBF, 1, {0x46}},    //    V63_7:0
    {0xC0, 1, {0x84}},    //    V47_7:0
    {0xC1, 1, {0xAF}},    //    V31_7:0
    {0xC2, 1, {0xC5}},    //    V23_7:0
    {0xC3, 1, {0xDB}},    //    V15_7:0
    {0xC4, 1, {0xE6}},    //    V11_7:0
    {0xC5, 1, {0xF0}},    //    V7_7:0
    {0xC6, 1, {0xF8}},    //    V3_7:0
    {0xC7, 1, {0xFC}},    //    V1_7:0
    {0xC8, 1, {0xFC}},    //    V0_7:0

    {0xC9, 1, {0x00}},
    {0xCA, 1, {0x00}},
    {0xCB, 1, {0x05}},
    {0xCC, 1, {0x6B}},
    {0xCD, 1, {0xFF}},
    {0xCE, 1, {0xFF}},

    {0xB0, 1, {0x0C}},    //    Page12

    {0xB1, 1, {0x00}},    //    V255_7:0
    {0xB2, 1, {0xBC}},    //    V254_7:0
    {0xB3, 1, {0xC6}},    //    V252_7:0
    {0xB4, 1, {0xDA}},    //    V248_7:0
    {0xB5, 1, {0xE9}},    //    V244_7:0
    {0xB6, 1, {0xFB}},    //    V240_7:0
    {0xB7, 1, {0x17}},    //    V232_7:0
    {0xB8, 1, {0x31}},    //    V224_7:0
    {0xB9, 1, {0x62}},    //    V208_7:0
    {0xBA, 1, {0x91}},    //    V192_7:0
    {0xBB, 1, {0xF0}},    //    V160_7:0
    {0xBC, 1, {0x52}},    //    V128_7:0
    {0xBD, 1, {0x56}},    //    V127_7:0
    {0xBE, 1, {0xCC}},    //    V95_7:0
    {0xBF, 1, {0x46}},    //    V63_7:0
    {0xC0, 1, {0x84}},    //    V47_7:0
    {0xC1, 1, {0xAF}},    //    V31_7:0
    {0xC2, 1, {0xC5}},    //    V23_7:0
    {0xC3, 1, {0xDB}},    //    V15_7:0
    {0xC4, 1, {0xE6}},    //    V11_7:0
    {0xC5, 1, {0xF0}},    //    V7_7:0
    {0xC6, 1, {0xF8}},    //    V3_7:0
    {0xC7, 1, {0xFC}},    //    V1_7:0
    {0xC8, 1, {0xFC}},    //    V0_7:0

    {0xC9, 1, {0x00}},
    {0xCA, 1, {0x00}},
    {0xCB, 1, {0x05}},
    {0xCC, 1, {0x6B}},
    {0xCD, 1, {0xFF}},
    {0xCE, 1, {0xFF}},

    {0xB0, 1, {0x03}},    //    Page3
    {0xC5, 1, {0x35}},    //    GIP timing
    {0xC8, 1, {0x05}},    //    GIP timing
    {0xC9, 1, {0x03}},    //    GIP timing
    {0xCA, 1, {0x40}},    //    GIP timing
    {0xCD, 1, {0x3C}},    //    GIP timing//64
    {0xCE, 1, {0x08}},    //    GIP timing
    {0xCF, 1, {0x60}},    //    GIP timing  60
    {0xD3, 1, {0x04}},    //    GIP timing
    {0xD6, 1, {0x00}},    //    GIP timing 01
    {0xD7, 1, {0x01}},    //    GIP timing  00
    {0xD9, 1, {0x00}},    //    GIP timing 06
    {0xDB, 1, {0x02}},    //    GIP timing 
    {0xDE, 1, {0x35}},    //    GIP timing
    {0xE3, 1, {0x0F}},    //    GIP timing

    {0xB0, 1, {0x00}},    //    Page0
    //{0xB2, 1, {0x42}},    //    NORMAL WHITE(TN)
    {0xB3, 1, {0x00}},    //    1200x2000
    {0xB4, 1, {0xFA}},    //    1200x2000

    {0xBF, 1, {0x1F}},    //    VGH=18V
    //{0xC0, 1, {0x0F}},    //    VGL=-11.2V
    {0xC0, 1, {0x0B}},    //    VGL=-10V

    {0xC2, 1, {0x0A}},    //    VGPH=4.5V
    {0xC4, 1, {0x0A}},    //    VGNH=-4.5V
    {0xBA, 1, {0x8F}},    //    column
    //{0xBD, 1, {0x29}},    //    Vcom

    {0xB0, 1, {0x06}},    //    Page6
    {0xB8, 1, {0xA5}},    //    Password enable
    {0xC0, 1, {0xA5}},    //    Password enable
    {0xBC, 1, {0x11}},    //    Gamma chopper
    {0xC7, 1, {0x0F}},    //    VCCIF/VCC
    //{0xD5, 1, {0x48}},    //     GOE=2.5us
    {0xD5, 1, {0x18}},    //     GOE=4.5us
    {0xB8, 1, {0x00}},    //    Password disable
    {0xC0, 1, {0x00}},    //    Password disable

    {0xB7, 2, {0x50,0x02}}, //(03)(orise:02)
    {0xBC, 2, {0x00,0x00}},
    {0x21, 1, {0x00}},

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
    MDELAY(5);

#if OCP2138_BIAS_I2C_ENABLE
	lcd_bias_en(1);
	MDELAY(10);
	ocp2138_write(0x00,0x13);
	ocp2138_write(0x01,0x13);

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
    MDELAY(10);
    vdd_enable(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
    lcm_init();
}

struct LCM_DRIVER pf196_dj_hx8279_d01_wuxga2000_ips_1036_lcm_drv =
{
    .name            = "pf196_dj_hx8279_d01_wuxga2000_ips_1036",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params        = lcm_get_params,
    .init            = lcm_init,
    .suspend        = lcm_suspend,
    .resume            = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
