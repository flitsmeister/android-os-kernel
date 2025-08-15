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

#define FRAME_WIDTH  										(800)
#define FRAME_HEIGHT 										(1280)


extern unsigned int GPIO_LCM_PWR_EN;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_BL_EN;
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
#ifdef BUILD_LK
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
    mt_set_gpio_mode(GPIO, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO, (output>0)? GPIO_OUT_ONE: GPIO_OUT_ZERO);
}
#else
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	if (GPIO == 0xFFFFFFFF) {
		return;
	}
	gpio_set_value(GPIO, (output > 0) ? GPIO_OUT_ONE : GPIO_OUT_ZERO);
}
#endif

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

static void backlight_enable(unsigned char enabled)
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
	params->type                   = LCM_TYPE_DSI;
	params->width                  = FRAME_WIDTH;
	params->height                 = FRAME_HEIGHT;
	params->dsi.mode               = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM           = LCM_FOUR_LANE;   //   LCM_THREE_LANE
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

	params->dsi.vertical_sync_active 				= 16; 
    params->dsi.vertical_backporch					= 18;
    params->dsi.vertical_frontporch 				= 20;	
    params->dsi.vertical_active_line                = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active				= 20;
    params->dsi.horizontal_backporch				= 20;
    params->dsi.horizontal_frontporch				= 80; 
    params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 220;
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
                // MDELAY(table[i].count}},
                // break;
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
                break;
		}
    }
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
	data_array[1]=0x000050EE;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00033902;
	data_array[1]=0x005585EA;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00000356;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00033902;
	data_array[1]=0x00402190;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x0000A024;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00001099;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00000997;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00007195;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x0000207A;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x0000007D;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x000060EE;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00006227;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00008829;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x0000242A;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00000130;   // 0x00000030  three lane
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x0000D932;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x0000c333;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00003F34;
	dsi_set_cmdq(data_array,2,1);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000243A;/////////////////
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x0000003B;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x0000003C;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00033902;
	data_array[1]=0x0082023D;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00003842;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00003843;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00000044;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00000A46;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x0000247F;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00002480;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x0000908B;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x0000458D;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00002291;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00001192;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x00009F93;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x0000F37A;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x0000009A;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00063902;
	data_array[1]=0x271B0047;
	data_array[2]=0x00003637;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00063902;
	data_array[1]=0x271B005A;
	data_array[2]=0x00003637;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00063902;
	data_array[1]=0x4837424C;
	data_array[2]=0x00002928;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00063902;
	data_array[1]=0x4837425F;
	data_array[2]=0x00002928;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00063902;
	data_array[1]=0x2A132B51;
	data_array[2]=0x00003627;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00063902;
	data_array[1]=0x2A132B64;
	data_array[2]=0x00003627;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00053902;
	data_array[1]=0x5B493C56;
	data_array[2]=0x0000007F;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00053902;
	data_array[1]=0x5B493C69;
	data_array[2]=0x0000007F;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00023902;
	data_array[1]=0x000070EE;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00053902;
	data_array[1]=0x00050300;
	data_array[2]=0x00000001;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00053902;
	data_array[1]=0x00030104;
	data_array[2]=0x00000001;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00033902;
	data_array[1]=0x0005050C;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00063902;
	data_array[1]=0x00080710;
	data_array[2]=0x00000000;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00063902;
	data_array[1]=0x0D0A0015;
	data_array[2]=0x00000004;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00033902;
	data_array[1]=0x00151529;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00063902;
	data_array[1]=0x3C3C3C60;
	data_array[2]=0x00000100;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00063902;
	data_array[1]=0x3F3F3C65;
	data_array[2]=0x00003C04;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00063902;
	data_array[1]=0x3C3C3C6A;
	data_array[2]=0x00003C3C;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00063902;
	data_array[1]=0x123C3C6F;
	data_array[2]=0x00001013;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00033902;
	data_array[1]=0x003C1174;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00063902;
	data_array[1]=0x3C3C3C80;
	data_array[2]=0x00000100;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00063902;
	data_array[1]=0x3F3F3C85;
	data_array[2]=0x00003C04;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00063902;
	data_array[1]=0x3C3C3C8A;
	data_array[2]=0x00003C3C;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00063902;
	data_array[1]=0x123C3C8F;
	data_array[2]=0x00001013;
	dsi_set_cmdq(data_array,3,1);

	data_array[0]=0x00033902;
	data_array[1]=0x003C1194;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00033902;
	data_array[1]=0x000000EA;
	dsi_set_cmdq(data_array,2,1);

	data_array[0]=0x00023902;
	data_array[1]=0x000000EE;
	dsi_set_cmdq(data_array,2,1);
	
	data_array[0]=0x00110500;
	dsi_set_cmdq(data_array,1,1);
	MDELAY(100);
	data_array[0]=0x00290500;
	dsi_set_cmdq(data_array,1,1);	
	MDELAY(100);
/*
	data_array[0]=0x00023902;
	data_array[1]=0x00000011;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(100);
	data_array[0]=0x00023902;
	data_array[1]=0x00000029;
	dsi_set_cmdq(data_array,2,1);
	MDELAY(100);
	*/
}


static void lcm_init(void)
{
	lcd_power_en(1);
	MDELAY(20);
	lcd_reset(0);
	MDELAY(20);
	lcd_reset(1);
	MDELAY(80);//Must > 5ms
	init_lcm_registers();
	MDELAY(20);
    backlight_enable(1);
}

static void lcm_suspend(void)
{
    backlight_enable(0);
    MDELAY(150);
    lcd_reset(0);
    MDELAY(100);
	lcd_power_en(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
	lcm_init();
}

struct LCM_DRIVER us717_fx_ili9881c_boe_wxga_ips_101_lcm_drv =
{
    .name			= "us717_fx_ili9881c_boe_wxga_ips_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
