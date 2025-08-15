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

#define FRAME_WIDTH  										(1920)
#define FRAME_HEIGHT 										(1280)


extern unsigned int GPIO_LCM_PWR; // bias 使能AVDD AVEE 供电
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_VDD;
extern unsigned int GPIO_LCM_LT8911B_RST_EN;
#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0


#define LCM_DSI_CMD_MODE									0


// ---------------------------------------------------------------------------
//mid add for LT8911 edp bridge IC setting start
#ifdef _1366x768_eDP_Panel_
#undef _1366x768_eDP_Panel_
#endif
#ifdef _1280x800_eDP_Panel_
#undef _1280x800_eDP_Panel_
#endif
#ifdef _1280x800_eDP_Panel_
#undef _1280x800_eDP_Panel_
#endif
#ifdef _1600x900_eDP_Panel_
#undef _1600x900_eDP_Panel_
#endif
#ifdef _1920x1200_eDP_Panel_
#undef _1920x1200_eDP_Panel_
#endif
#ifdef _1080P_eDP_Panel_
#undef _1080P_eDP_Panel_
#endif

//real defined config
#ifndef _1920x1280_eDP_Panel_
#define  _1920x1280_eDP_Panel_
#endif

#ifdef _1920x1280_eDP_Panel_
#if defined(CONFIG_MID_LT8911EXB_SUPPORT)
extern int MIPI_Timing[];
extern void LT8911EXB_config(void);
#endif
#define eDP_lane		2
#define PCR_PLL_PREDIV	0x40

// 根据前端MIPI信号的Timing，修改以下参数：
//According to the timing of the Mipi signal, modify the following parameters:
//int MIPI_Timing[] =
//// hfp,	hs,	hbp,	hact,	htotal,	vfp,	vs,	vbp,	vact,	vtotal,	pixel_CLK/10000
////-----|---|------|-------|--------|-----|-----|-----|--------|--------|---------------
//  { 48, 32, 50, 	1920, 	2050, 	3, 		6, 	51, 	1280, 	1340, 	16482 };    // SL156PP36

//#define _6bit_ // eDP panel Color Depth，262K color
#define _8bit_                                              // eDP panel Color Depth，16.7M color

#endif
//mid add for LT8911 edp bridge IC setting end
// ---------------------------------------------------------------------------

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
#if 0
static void lcd_bias_en(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_PWR, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_PWR, GPIO_OUT_ZERO);
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

static void edp_reset(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_LT8911B_RST_EN, 1);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_LT8911B_RST_EN, 0);
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
	params->lcm_cmd_if = LCM_INTERFACE_DSI0;
	params->width                  = FRAME_WIDTH;
	params->height                 = FRAME_HEIGHT;
	params->dsi.mode               = SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM           = LCM_FOUR_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

    params->dsi.vertical_sync_active                            = 6; //2; //4;
    params->dsi.vertical_backporch                              = 51; //10; //16;
    params->dsi.vertical_frontporch                             = 3;//5;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 32; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 50; //60; //60; //80;
    params->dsi.horizontal_frontporch                           = 48; //60;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 494;
	params->dsi.ssc_disable = 1;
    params->dsi.edp_panel = 1;
	params->dsi.cont_clock = 1;

#ifdef _1920x1280_eDP_Panel_
#if defined(CONFIG_MID_LT8911EXB_SUPPORT)
	MIPI_Timing[vs] 		= 6;
	MIPI_Timing[vbp]		= 51;
	MIPI_Timing[vfp]		= 3;
	MIPI_Timing[vact]		= 1280;//1280    1340
	MIPI_Timing[vtotal]		= 1340;

	MIPI_Timing[hs]		    = 32 ;
	MIPI_Timing[hbp]		= 50;
	MIPI_Timing[hfp]		= 48;
	MIPI_Timing[hact]		= 1920;//1920  2050
	MIPI_Timing[htotal]		= 2050;
	
	//MIPI_Timing[pclk_10khz] = (((MIPI_Timing[vtotal] * MIPI_Timing[htotal] * 60)) / 10000);
	MIPI_Timing[pclk_10khz] = 16482;
#endif
#endif

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



static void lcm_init(void)
{
	
    vdd_enable(1);//3.3V /1.8V /1.2V
	MDELAY(20);
	edp_reset(1);
	MDELAY(10);

	lcd_reset(1);
	MDELAY(10);
	lcd_reset(0);
	MDELAY(10);
	lcd_reset(1);
	MDELAY(10);//Must > 5ms
#ifdef _1920x1280_eDP_Panel_
#if defined(CONFIG_MID_LT8911EXB_SUPPORT)
	LT8911EXB_config();
#endif
#endif
}

static void lcm_suspend(void)
{
	edp_reset(0);
	MDELAY(20);

    lcd_reset(0);
    MDELAY(10);
    vdd_enable(0);
    MDELAY(10); 
}

static void lcm_resume(void)
{
	lcm_init();
}

struct LCM_DRIVER pf829_nv_lt8911exb_boe_wuxgal_ips_120_lcm_drv =
{
    .name			= "pf829_nv_lt8911exb_boe_wuxgal_ips_120",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
