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

#define FRAME_WIDTH  										(1200)
#define FRAME_HEIGHT 										(2000)


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
    params->dsi.LANE_NUM           = LCM_FOUR_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3;

    params->dsi.vertical_sync_active                            = 6; //2; //4;
    params->dsi.vertical_backporch                              = 34; //10; //16;
    params->dsi.vertical_frontporch                             = 255;//5;
    params->dsi.vertical_active_line                            = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active                          = 8; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 20; //60; 
    params->dsi.horizontal_frontporch                           = 20; //60;
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 520;
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
	
//============CMD WR enable=============	
		{0x00, 1, {0x00}},
		{0xFF, 3, {0x82,0x01,0x01}},
		{0x00, 1, {0x80}},
		{0xFF, 2, {0x82,0x01}},
//======voltage set(AVDD/AVEE=+6/-6,VGHO/VGLO=18V/-10V,GVDD/NGVDD=5.4V/-5.4V)==========
		{0x00, 1, {0x93}}, //VGH_N 19V  step=0.1V
		{0xC5, 1, {0x7A}},
		{0x00, 1, {0x97}}, //VGH_I 19V  step=0.1V
		{0xC5, 1, {0x7A}},
		{0x00, 1, {0x9E}},
		{0xC5, 1, {0x0A}},  //2AVDD-2AVEE
		{0x00, 1, {0x9A}},  //VGL_N -11V   step=0.1V
		{0xC5, 1, {0x39}},  //AVEE-AVDD
		{0x00, 1, {0x9C}},  //VGL_I -11V   step=0.1V
		{0xC5, 1, {0x39}},  //AVEE-AVDD
		{0x00, 1, {0xB6}},
		{0xC5, 2, {0x6B,0x6B}},   //VGHO1_N_I 18V   step=0.1V
		{0x00, 1, {0xB8}},
		{0xC5, 2, {0x2F,0x2F}},  //VGLO1_N_I -10V   step=0.1V
		{0x00, 1, {0x00}},
		{0xD8, 2, {0xBE,0xBE}},  //GVDDP/N 5.4V/-5.4V   step=0.01V
//		{0x00, 1, {0x00}},
//		{0xD9, 3, {0x00,0x82,0x82}},  //VCOM(-1V) step=0.01V
		{0x00, 1, {0x82}},
		{0xC5, 1, {0x95}},  //LVD
		{0x00, 1, {0x83}},
		{0xC5, 1, {0x07}},  //LVD Enable
		{0x00, 1, {0x94}},
		{0xC5, 1, {0x46}},  //N_VGH_Pump_CLK_Freq[2:0]:4Line
		{0x00, 1, {0x9B}},
		{0xC5, 1, {0x65}},  //N_VGL_Pump_CLK_Freq[6:4]:4Line
		{0x00, 1, {0x9D}},
		{0xC5, 1, {0x65}},  //I_VGL_Pump_CLK_Freq[6:4]:4Line
//==========gamma==============
//Analog Gamma
		{0x00, 1, {0x00}},
		{0xE1, 16, {0x05,0x12,0x2A,0x3F,0x49,0x55,0x67,0x73,0x74,0x7F,0x7F,0x90,0x76,0x65,0x66,0x5B}},
		{0x00, 1, {0x10}},
		{0xE1, 8, {0x53,0x48,0x37,0x2D,0x26,0x10,0x07,0x00}},
		{0x00, 1, {0x00}},
		{0xE2, 16, {0x05,0x12,0x2A,0x3F,0x49,0x55,0x67,0x73,0x74,0x7F,0x7F,0x90,0x76,0x65,0x66,0x5B}},
		{0x00, 1, {0x10}},
		{0xE2, 8, {0x53,0x48,0x37,0x2D,0x26,0x10,0x07,0x00}},
//reg_sd_sap
		{0x00, 1, {0x80}},
		{0xA4, 1, {0x8C}},
//OSC Auto calibration
		{0x00, 1, {0xA0}},
		{0xF3, 1, {0x10}},
//############################################################################################################
//===FT8201AB===
// B3A1~B3A4 Panel resolution
// B3A5[7:5] Source Resolution
//B3A6=0x13  [4]:reg_panel_sd_resol, [1]:reg_panel_size, [0]:reg_panel_zigzag
		{0x00, 1, {0xA1}},
		{0xB3, 2, {0x04,0xB0}},	//X=1200
		{0x00, 1, {0xA3}},
		{0xB3, 2, {0x07,0xD0}},	//Y=2000
		{0x00, 1, {0xA5}},
		{0xB3, 2, {0x80,0x13}},	//600RGB
// DMA Setting C1D0=0x30 (Single Chip) / 0xb0 (Multiple Drop)
		{0x00, 1, {0xD0}},
		{0xC1, 1, {0xB0}},  //Multiple Drop
// ## GOA Timing ##
//SKIP LVD Power-OFF0
		{0x00, 1, {0x80}},
		{0xCB, 7, {0x33,0x33,0x30,0x33,0x30,0x33,0x30}},
		{0x00, 1, {0x87}},
		{0xCB, 1, {0x33}},
		{0x00, 1, {0x88}},
		{0xCB, 8, {0x33,0x33,0x33,0x33,0x33,0x30,0x33,0x33}},
		{0x00, 1, {0x90}},
		{0xCB, 7, {0x30,0x33,0x33,0x33,0x30,0x30,0x33}},
		{0x00, 1, {0x97}},
		{0xCB, 1, {0x33}},
//Power-OFF NORM
		{0x00, 1, {0x98}},
		{0xCB, 8, {0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04}},
		{0x00, 1, {0xA0}},
		{0xCB, 8, {0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04}},
		{0x00, 1, {0xA8}},
		{0xCB, 8, {0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04}},
//Power-On
		{0x00, 1, {0xB0}},
		{0xCB, 7, {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
		{0x00, 1, {0xB7}},
		{0xCB, 1, {0x00}},
		{0x00, 1, {0xB8}},
		{0xCB, 8, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
		{0x00, 1, {0xC0}},
		{0xCB, 7, {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
		{0x00, 1, {0xC7}},
		{0xCB, 1, {0x00}},
//U2D						
		{0x00, 1, {0x80}},
		{0xCC, 8, {0x00,0x00,0x00,0x00,0x00,0x29,0x31,0x34}},
		{0x00, 1, {0x88}},
		{0xCC, 8, {0x35,0x13,0x14,0x15,0x16,0x07,0x1F,0x00}},
		{0x00, 1, {0x90}},
		{0xCC, 6, {0x00,0x00,0x00,0x00,0x00,0x00}},				
		{0x00, 1, {0x80}},
		{0xCD, 8, {0x00,0x00,0x00,0x00,0x00,0x29,0x31,0x34}},
		{0x00, 1, {0x88}},
		{0xCD, 8, {0x35,0x17,0x18,0x19,0x1A,0x08,0x20,0x00}},
		{0x00, 1, {0x90}},
		{0xCD, 6, {0x00,0x00,0x00,0x00,0x00,0x00}},
//D2U						
		{0x00, 1, {0xA0}},
		{0xCC, 8, {0x00,0x00,0x00,0x00,0x00,0x31,0x2A,0x34}},
		{0x00, 1, {0xA8}},
		{0xCC, 8, {0x35,0x1A,0x19,0x18,0x17,0x20,0x08,0x00}},
		{0x00, 1, {0xB0}},
		{0xCC, 6, {0x00,0x00,0x00,0x00,0x00,0x00}},				
		{0x00, 1, {0xA0}},
		{0xCD, 8, {0x00,0x00,0x00,0x00,0x00,0x31,0x2A,0x34}},
		{0x00, 1, {0xA8}},
		{0xCD, 8, {0x35,0x16,0x15,0x14,0x13,0x1F,0x07,0x00}},
		{0x00, 1, {0xB0}},
		{0xCD, 6, {0x00,0x00,0x00,0x00,0x00,0x00}},
//goa_vstx_shift_cnt_sel 
		{0x00, 1, {0x81}},
		{0xC2, 1, {0x40}},	//C281[6]=1
//VST5							
		{0x00, 1, {0x90}},//STV1_R
		{0xC2, 4, {0x84,0x02,0x68,0x9D}},				
//VST6	
		{0x00, 1, {0x94}},//STV1_L
		{0xC2, 4, {0x83,0x02,0x68,0x9D}}, 
//VEND5				
		{0x00, 1, {0xB0}},//STV2_R
		{0xC2, 4, {0x00,0x02,0x68,0x9D}},				
//VEND6	
		{0x00, 1, {0xB4}},//STV2_L
		{0xC2, 4, {0x01,0x02,0x68,0x9D}},
//CLKC						
		{0x00, 1, {0x80}},//GATE1:CLK1_R
		{0xC3, 8, {0x82,0x87,0x02,0x68,0x9D,0x00,0x02,0x07}},
		{0x00, 1, {0x88}},
		{0xC3, 8, {0x00,0x85,0x02,0x68,0x9D,0x00,0x02,0x07}},
		{0x00, 1, {0x90}},
		{0xC3, 8, {0x02,0x83,0x02,0x68,0x9D,0x00,0x02,0x07}},
		{0x00, 1, {0x98}},
		{0xC3, 8, {0x04,0x81,0x02,0x68,0x9D,0x00,0x02,0x07}},	
//CLKD
		{0x00, 1, {0xC0}},//GATE2:CLK1_L
		{0xCD, 8, {0x81,0x86,0x02,0x68,0x9D,0x00,0x02,0x07}},
		{0x00, 1, {0xC8}},
		{0xCD, 8, {0x01,0x84,0x02,0x68,0x9D,0x00,0x02,0x07}},
		{0x00, 1, {0xD0}},
		{0xCD, 8, {0x03,0x82,0x02,0x68,0x9D,0x00,0x02,0x07}},
		{0x00, 1, {0xD8}},
		{0xCD, 8, {0x05,0x80,0x02,0x68,0x9D,0x00,0x02,0x07}}, 
//GCH & GCL					
		{0x00, 1, {0xE2}},//GCL1:BW
		{0xCC, 4, {0x08,0xD4,0x00,0x08}},
//ECLK 
		{0x00, 1, {0xF0}},//GPWR1&GPWR2
		{0xCC, 5, {0x3D,0x88,0x88,0xD0,0x5E}},
//reg_goa_select_dg_rtn
		{0x00, 1, {0xFD}},
		{0xCB, 1, {0x82}},	
//=============== ## TCON Timing START ##	===============//
		{0x00, 1, { 0x80}},
		{0xC0, 6, { 0x00 ,0x91 ,0x01 ,0x17 ,0x00 ,0x10}},
		{0x00, 1, { 0x90}},
		{0xC0, 6, { 0x00 ,0x91 ,0x01 ,0x17 ,0x00 ,0x10}},
		{0x00, 1, { 0xA0}},
		{0xC0, 6, { 0x01 ,0x30 ,0x01 ,0x17 ,0x00 ,0x10}},
		{0x00, 1, { 0xB0}},
		{0xC0, 5, { 0x00 ,0x91 ,0x01 ,0x17 ,0x10}},
		{0x00, 1, { 0xA3}},
		{0xC1, 3, { 0x28, 0x28, 0x04}},
		{0x00, 1, { 0x80}},
		{0xCE, 1, { 0x00 }},
		{0x00, 1, { 0xD0}},
		{0xCE, 8, { 0x01 ,0x00 ,0x0A ,0x01 ,0x01 ,0x00 ,0xF7 ,0x00}},
		{0x00, 1, { 0xE0}},
		{0xCE, 1, { 0x00}},
		{0x00, 1, { 0xF0}},
		{0xCE, 1, { 0x00}},
		{0x00, 1, { 0xB0}},
		{0xCF, 4, { 0x06 ,0x06 ,0xBA ,0xBE}},
		{0x00, 1, { 0xB5}},
		{0xCF, 4, { 0x04 ,0x04 ,0x40 ,0x44}},
		{0x00, 1, { 0xC0}},
		{0xCF, 4, { 0x07 ,0x07 ,0xB4 ,0xB8}},
		{0x00, 1, { 0xC5}},
		{0xCF, 4, { 0x00 ,0x07 ,0x08 ,0xD0}},
//=============== ## TCON Timing END ##	===============//
//CLK Source delay	
		{0x00, 1, {0x90}},
		{0xC4, 1, {0x88}},	//CLK Source delay
//CLK allon
		{0x00, 1, {0x92}},
		{0xC4, 1, {0xC0}},
//CLK EQ
		{0x00, 1, {0xC8}},
		{0xC3, 3, {0xEE,0xE0,0x00}},
//tp_term_sync_vb
		{0x00, 1, {0xC9}},
		{0xCE, 1, {0x00}},
		{0x1C, 1, {0x00}},	//FIFO mode
//M&S PLL from the same 4M (CCLEE 20200828-1)
		{0x00, 1, { 0xD6}},
		{0xC1, 1, { 0x00}},
//=== CCLEE (20200828-1)
		{0x00, 1, {0xD5}},
		{0xC0, 1, {0xF0}},
//Vsync sync with mipi
		{0x00, 1, {0xA0}},
		{0xC1, 1, {0xE0}},
//pump VGH afd off
		{0x00, 1, {0x9F}},
		{0xC5, 1, {0x00}},
//PUMP  VGH CLK all on disable
		{0x00, 1, {0x91}},
		{0xC5, 1, {0x4C}},
//tcon_tp_term2_vb_ln
		{0x00, 1, {0xD7}},
		{0xCE, 1, {0x01}},
//tp term VGH pump off CLK gating H
		{0x00, 1, {0x98}},
		{0xC5, 1, {0x67}},//I_VGH_Pump_CLK_Freq[6:4]: 4Line
//Sync gamma enable
		{0x00, 1, {0x82}},
		{0xA5, 1, {0x01}},
//VGH pump AFD sequence
		{0x00, 1, {0x8C}},
		{0xCF, 2, {0x40,0x40}},
//en_Vcom can't be gating by LVD
		{0x00, 1, {0x9A}},
		{0xF5, 1, {0x35}},
//Source pull low setting
		{0x00, 1, {0xA2}},
		{0xF5, 1, {0x1F}},
//### For dual chip only ###	//(20200812)

//for dual chip master
		{0x00, 1, {0xA4}},
		{0xF3, 1, {0x0B}},
		{0x00, 1, {0x00}},//for master only mode
		{0xFA, 1, {0x02}},	
//VGHO1 sink Off
		{0x00, 1, {0xA8}},
		{0xC5, 1, {0x09}},
//VGLO1 sink ON
		{0x00, 1, {0xCB}},
		{0xC5, 1, {0x01}},
//VGHO1 & VGLO1 
		{0x00, 1, {0xB6}}, 
		{0xC5, 2, {0x6B,0x6B}},   //M_VGHO1_N_I 18V
		{0x00, 1, {0xB8}}, 
		{0xC5, 2, {0x2F,0x2F}},  //M_VGLO1_N_I -10V
//master vcom on
		{0x00, 1, {0x91}}, 
		{0xA5, 1, {0x00}},
		{0x00, 1, {0xA4}},
		{0xF3, 1, {0x0B}},
		{0x00, 1, {0x00}},//Return to normal
		{0xFA, 1, {0x5A}},	
//for dual chip slave
		{0x00, 1, {0xA4}},
		{0xF3, 1, {0x0B}},
		{0x00, 1, {0x00}},//for slave only mode
		{0xFA, 1, {0x01}},	
//VGHO1 sink OFF
		{0x00, 1, {0xA8}},
		{0xC5, 1, {0x09}},
//VGLO1 sink OFF
		{0x00, 1, {0xCB}},
		{0xC5, 1, {0x09}},
//VGHO1 & VGLO1 -2step
		{0x00, 1, {0xB6}}, 
		{0xC5, 2, {0x69,0x69}},   //S_VGHO1_N_I 17.8V
		{0x00, 1, {0xB8}}, 
		{0xC5, 2, {0x2D,0x2D}},  //S_VGLO1_N_I -9.8V
//slave vcom off
		{0x00, 1, {0x91}}, 
		{0xA5, 1, {0x40}},
		{0x00, 1, {0xA4}},
		{0xF3, 1, {0x0B}},
		{0x00, 1, {0x00}},//Return to normal
		{0xFA, 1, {0x5A}}, 
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
	lcd_power_en(1);
    backlight_enable(1);
	display_bias_enable();
	MDELAY(100);
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
    MDELAY(100);
	display_bias_disable();
	lcd_power_en(0);
    backlight_enable(0);
    MDELAY(20);
}

static void lcm_resume(void)
{
	lcm_init();
}

struct LCM_DRIVER us828_hjr_ft8201ba_hsd_wvxga_ips_103_lcm_drv =
{
    .name			= "us828_hjr_ft8201ba_hsd_wvxga_ips_103",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};
