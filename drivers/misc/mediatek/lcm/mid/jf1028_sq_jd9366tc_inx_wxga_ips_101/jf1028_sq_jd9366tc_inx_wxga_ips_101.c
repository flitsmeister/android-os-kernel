#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#else
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#endif

#include <lcm_drv.h>

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH												(800)
#define FRAME_HEIGHT											(1280)


extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_BIAS_ENN;
extern unsigned int GPIO_LCM_BIAS_ENP;

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
extern int _lcm_i2c_write_bytes(unsigned char addr, unsigned char value);
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

static void lcd_bias_gpio_set(unsigned char enabled)
{
	printk("lcd_bias_gpio_set:%d\r\n",enabled);
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_BIAS_ENP, 1);
		MDELAY(10);
        lcm_set_gpio_output(GPIO_LCM_BIAS_ENN, 1);
        MDELAY(10);
    }
    else
    {
		lcm_set_gpio_output(GPIO_LCM_BIAS_ENN, 0);
        MDELAY(10);
        lcm_set_gpio_output(GPIO_LCM_BIAS_ENP, 0);
    }
}

static void display_bias_set(unsigned char enabled)
{
	if(enabled){
		lcd_bias_gpio_set(1);
		_lcm_i2c_write_bytes(0x0, 0x14);
		_lcm_i2c_write_bytes(0x1, 0x14);
	}else{		
		//_lcm_i2c_write_bytes(0x0, 0x0);
		//_lcm_i2c_write_bytes(0x1, 0x0);
		lcd_bias_gpio_set(0);
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

	params->dsi.vertical_sync_active				= 4;
	params->dsi.vertical_backporch					= 38;
	params->dsi.vertical_frontporch 				= 140;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 
	
	params->dsi.horizontal_sync_active				= 8;	
	params->dsi.horizontal_backporch				= 28;
	params->dsi.horizontal_frontporch				= 40;
	params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;
	
	params->dsi.ssc_disable = 1;
	params->dsi.PLL_CLOCK = 242;
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

    {0x30,1,{0x01}},
    {0x78,4,{0x49,0x61,0x02,0x00}},
    {0x30,1,{0x02}},
    {0x31,1,{0x22}},  
    {0x32,1,{0x0c}},   
    {0x33,1,{0x30}},
    {0x3d,1,{0x42}},
    {0x3f,1,{0x61}},
    {0x42,1,{0x82}},
    {0x44,1,{0x21}},
    {0x49,1,{0xC2}}, 
    {0x4A,1,{0x3F}},  
    {0x41,12,{0x0C,0x0C,0x2C,0x2C,0x0C,0x0C,0x2C,0x2C,0x0C,0x0C,0x00,0x00}},  
    {0x5a,11,{0x23,0x23,0x1C,0x18,0x20,0x21,0x12,0x10,0x0E,0x0C,0x0A}}, 
    {0x5b,11,{0x08,0x00,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23}},
    {0x5c,11,{0x23,0x23,0x1D,0x19,0x20,0x21,0x13,0x11,0x0F,0x0D,0x0B}},
    {0x5d,11,{0x09,0x01,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23}},
    {0x5e,11,{0x23,0x23,0x1D,0x01,0x20,0x21,0x11,0x13,0x09,0x0B,0x0D}},
    {0x5f,11,{0x0F,0x19,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23}},
    {0x60,11,{0x23,0x23,0x1C,0x00,0x20,0x21,0x10,0x12,0x08,0x0A,0x0C}},
    {0x61,11,{0x0E,0x18,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23}}, 
    {0x4E,2,{0x07,0x07}}, 
    {0x50,2,{0x01,0x01}},
    {0x55,2,{0xF8,0xF8}},
    {0x56,2,{0xF8,0xF8}},
    {0x57,2,{0xFF,0xFF}},
    {0x58,2,{0xFF,0xFF}},
    {0x6F,3,{0x00,0x00,0x00}},
    {0x70,3,{0x00,0x00,0x00}},
    {0x71,3,{0xFF,0xFF,0x3F}},
    {0x72,3,{0xFF,0xFF,0x3F}},
    {0x30,1,{0x05}},
    {0x31,1,{0x00}},
    {0x40,3,{0x6F,0x39 ,0x39}}, 
    {0x30,1,{0x07}},
    {0x31,1,{0xc0}},
    {0x30,1,{0x08}}, 
    {0x42,1,{0x1C}},       
    {0x47,1,{0x0A}}, 
    {0x50,1,{0x0A}},       
    {0x5A,1,{0x20}},       
    {0x5B,1,{0x00}},      
    {0x5C,1,{0x53}},     
    {0x62,1,{0x04}},    
    {0x65,1,{0x5F}},
    {0x5e,1,{0x00}},
    {0x73,1,{0x01}},
    {0x30,1,{0x0a}},
    {0x33,1,{0x00}}, 
    {0x3f,1,{0x50}},
    {0x40,1,{0x15}},
    {0x42,1,{0x31}},
    {0x43,1,{0x00}},
    {0x49,1,{0x03}},
    {0x30,1,{0x0b}},
    {0x33,2,{0x00,0x5E}},
    {0x3c,2,{0x00,0x5E}},
    {0x43,1,{0xab}},
    {0x44,1,{0x2b}}, 
    {0x3e,5,{0x00,0x03,0x09,0x0E,0x14}}, 
    {0x3f,14,{0x1D,0x37,0x3B,0x44,0x40,0x5F,0x67,0x71,0x84,0x88,0x92,0x9F,0xB5,0xBF}},
    {0x40,9,{0x4F,0x53,0x5C,0x65,0x00,0x03,0x09,0x0E,0x14}}, 
    {0x41,14,{0x1D,0x37,0x3B,0x44,0x40,0x5F,0x67,0x71,0x84,0x88,0x92,0x9F,0xB5,0xBF}}, 
    {0x42,4,{0x4F,0x53,0x5C,0x65}},
    {0x45,1,{0x50}},
    {0x46,1,{0x58}},
    {0x48,1,{0x2C}},
    {0x49,1,{0x26}},
    {0x4A,1,{0x1B}}, 
    {0x4E,1,{0x0A}}, 
    {0x4F,1,{0x96}},  
    {0x50,1,{0x50}},
    {0x5A,1,{0x04}},
    {0x5B,1,{0x43}},  
    {0x60,1,{0x07}}, 
    {0x61,1,{0x28}}, 
    {0x67,1,{0x13}}, 
    {0x30,1,{0x0C}},
    {0x4A,1,{0xAB}},
    {0x4B,1,{0xD3}},
    {0x32,1,{0x62}},
    {0x42,1,{0x34}},
    {0x5e,1,{0x00}}, 
    {0x6a,1,{0x7f}}, 
    {0x62,1,{0x41}}, 
    {0x30,1,{0x0d}},
    {0x4c,1,{0x74}},
    {0x30,1,{0x08}},
    {0x52,1,{0xC3}},  
    {0x54,1,{0x20}},  
    {0x30,1,{0x08}},
    {0x30,1,{0x00}},
    //{0x36,1,{0x03}},//iscan
};

static __attribute__((unused)) struct LCM_setting_table te_setting[] = {
    {0x35,1,{0x00}},
    {0x30,1,{0x0C}},
    {0x4A,1,{0xAB}},
    {0x4B,1,{0xD3}},
    {0x30,1,{0x00}},
};
static void init_lcm_registers(void)
{
	unsigned int data_array[16];
	push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);
	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
    push_table(te_setting, sizeof(te_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_init(void)
{
	lcd_power_en(1);
	MDELAY(5);
	display_bias_set(1);
	MDELAY(10);

	lcd_reset(1);
	MDELAY(20);
	lcd_reset(0);
	MDELAY(50);
	lcd_reset(1);

	MDELAY(20);//Must > 5ms
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

	display_bias_set(0);
	MDELAY(10);
	lcd_power_en(0);
	MDELAY(20);
}

static void lcm_resume(void)
{
	lcm_init();
}

struct LCM_DRIVER jf1028_sq_jd9366tc_inx_wxga_ips_101_lcm_drv =
{
	.name			= "jf1028_sq_jd9366tc_inx_wxga_ips_101",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params		= lcm_get_params,
	.init			= lcm_init,
	.suspend		= lcm_suspend,
	.resume			= lcm_resume,
};
