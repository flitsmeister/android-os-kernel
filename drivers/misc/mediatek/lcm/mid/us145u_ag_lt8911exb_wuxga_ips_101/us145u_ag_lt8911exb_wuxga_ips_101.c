

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
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

#define FRAME_WIDTH  (1920)
#define FRAME_HEIGHT (1080)

#define REGFLAG_DELAY 0xFD
//#define GPIO_LCM_PWREN                                      (GPIO173 |0x80000000)//GPIO_LCM_PWR2_EN//GPIO_LCM_PWR2_EN
//#define GPIO_LCM_BL_EN                                      (GPIO174 | 0x80000000)//GPIO_LCM_PWR_EN//GPIO_LCM_PWR_EN
//#define GPIO_LCM_RST                                        (GPIO45 | 0x80000000)
#define REGFLAG_END_OF_TABLE 0xFE

#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util;

//extern unsigned int GPIO_LCM_3v3_EN;
extern unsigned int GPIO_LCM_RST;
//extern unsigned int GPIO_LCM_BL_EN;
//extern unsigned int GPIO_LCM_1v8_EN;

extern unsigned int EDP_LCM_LED_EN1;
extern unsigned int GPIO_LCM_VLCM33_EN;
extern unsigned int GPIO_LCM_LT8911B_VDD18_EN;
extern unsigned int GPIO_LCM_GPIO5_IRQO2;


#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define MDELAY(n) 											(lcm_util.mdelay(n))
#define UDELAY(n) 											(lcm_util.udelay(n))
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




















#define _eDP_2G7_

//extern void LT8911EXB_IIC_Write_byte( u8 RegAddr, u8 data );    // IIC write operation, IIC rate do not exceed 100KHz
//extern u8 LT8911EXB_IIC_Read_byte( u8 RegAddr );                // IIC read operation, IIC rate do not exceed 100KHz

//********************************************************//

enum {
	hfp = 0,
	hs,
	hbp,
	hact,
	htotal,
	vfp,
	vs,
	vbp,
	vact,
	vtotal,
	pclk_10khz
};

u8		Read_DPCD010A = 0x00;
bool	ScrambleMode = 0;
bool	flag_mipi_on = 0;

#ifdef _read_edid_ // read eDP panel EDID
u8		EDID_DATA[128] = { 0 };
u16		EDID_Timing[11] = { 0 };
bool	EDID_Reply = 0;
#endif

//////////////////////LT8911EXB Config////////////////////////////////
//#define _1920x1200_eDP_Panel_
#define _1080P_eDP_Panel_
//#define _1366x768_eDP_Panel_
//#define _1280x800_eDP_Panel_
//#define _1600x900_eDP_Panel_
//#define _1920x1200_eDP_Panel_

#define _MIPI_Lane_ 4 // 3 /2 / 1

//#define _eDP_scramble_ // eDP scramble mode

#ifdef _1920x1200_eDP_Panel_

#define eDP_lane		2
#define PCR_PLL_PREDIV	0x40

#define _6bit_
//#define _8bit_

//const struct video_timing video[] =
static int MIPI_Timing[] =
// hfp, hs,	hbp,	hact,	htotal,	vfp,	vs,	vbp,	vact,	vtotal,	pixel_CLK/10000
//-----|---|------|-------|--------|-----|-----|-----|--------|--------|---------------
 { 48, 	32, 80, 	1920, 	2080, 	5, 		5, 	20, 	1200, 	1230, 	15350 };                     //


/*******************************************************************
   全志平台的屏参 lcd_hbp 是 上面参数 hbp + hs 的和；
   The lcd_hbp of Allwinner platform is the sum of the above parameters hbp + hs;

   同样的，全志平台的屏参 lcd_vbp 是 上面参数 vbp + vs的 和，这点要注意。
   In the same, lcd_vbp are the sum of the above parameters vbp + vs, which should be noted.
   //-------------------------------------------------------------------

   EOTP(End Of Transmite Packet，hs 传完了，会发这样一个包) 要打开，(之前的LT8911B 需要关闭EOTP，这里LT8911EXB 需要打开)
   Eotp (end of transmit packet, HS will send such a packet) must be turn-on (lt8911b needs to turn-off eotp, here lt8911exb needs to turn-on)

	1、MTK平台 的 dis_eotp_en 的值，改成 false;   LK和kernel都需要修改,改成false。
	1. The value of dis_eotp_en of MTK platform should be changed to 'false'; LK and kernel need to be changed to 'false'.

	2、展讯平台的 tx_eotp 的值置 1 。
	2. The value of tx_eotp of Spreadtrum platform is set to 1.

	3、RK平台 EN_EOTP_TX 置1.
	3. The value of EN_EOTP_TX of RK platform is set to 1.

	4、高通平台 找到 dsi_ctrl_hw_cmn.c -->void dsi_ctrl_hw_cmn_host_setup(struct dsi_ctrl_hw *ctrl,struct dsi_host_common_cfg *cfg)-->增加cfg->append_tx_eot = true;
	4、Qualcomm: dsi_ctrl_hw_cmn.c -->void dsi_ctrl_hw_cmn_host_setup(struct dsi_ctrl_hw *ctrl,struct dsi_host_common_cfg *cfg)-->add cfg->append_tx_eot = true;
*******************************************************************/

#endif

#ifdef _1080P_eDP_Panel_

#define eDP_lane		2
#define PCR_PLL_PREDIV	0x40

// 根据前端MIPI信号的Timing，修改以下参数：
//According to the timing of the Mipi signal, modify the following parameters:
static int MIPI_Timing[] =
// hfp,	hs,	hbp,	hact,	htotal,	vfp,	vs,	vbp,	vact,	vtotal,	pixel_CLK/10000
//-----|---|------|-------|--------|-----|-----|-----|--------|--------|---------------
//{ 88, 44, 148,    1920,   2200,   4,      5,  36,     1080,   1125,   14850 };// VESA
  { 48, 32, 80, 	1920, 	2080, 	3, 		5, 	23, 	1080, 	1111, 	13850 };    // SL156PP36

/*******************************************************************
   全志平台的屏参 lcd_hbp 是 上面参数 hbp + hs 的和；
   The lcd_hbp of Allwiner platform is the sum of the above parameters hbp + hs;

   同样的，全志平台的屏参 lcd_vbp 是 上面参数 vbp + vs的 和，这点要注意。
   In the same, lcd_vbp are the sum of the above parameters vbp + vs, which should be noted.
   //-------------------------------------------------------------------

   EOTP(End Of Transmite Packet，hs 传完了，会发这样一个包) 要打开，(之前的LT8911B 需要关闭EOTP，这里LT8911EXB 需要打开)
   Eotp (end of transmit packet, HS will send such a packet) must be turn-on (lt8911b needs to turn-off eotp, here lt8911exb needs to turn-on)

	1、MTK平台 的 dis_eotp_en 的值，改成 false;   LK和kernel都需要修改,改成false。
	1. The value of dis_eotp_en of MTK platform should be changed to 'false'; LK and kernel need to be changed to 'false'.

	2、展讯平台的 tx_eotp 的值置 1 。
	2. The value of tx_eotp of Spreadtrum platform is set to 1.

	3、RK平台 EN_EOTP_TX 置1.
	3. The value of EN_EOTP_TX of RK platform is set to 1.

	4、高通平台 找到 dsi_ctrl_hw_cmn.c -->void dsi_ctrl_hw_cmn_host_setup(struct dsi_ctrl_hw *ctrl,struct dsi_host_common_cfg *cfg)-->增加cfg->append_tx_eot = true;
	4、Qualcomm: dsi_ctrl_hw_cmn.c -->void dsi_ctrl_hw_cmn_host_setup(struct dsi_ctrl_hw *ctrl,struct dsi_host_common_cfg *cfg)-->add cfg->append_tx_eot = true;

*******************************************************************/

//#define _6bit_ // eDP panel Color Depth，262K color
#define _8bit_                                              // eDP panel Color Depth，16.7M color

#endif

//-----------------------------------------

#ifdef _1366x768_eDP_Panel_

#define eDP_lane		1
#define PCR_PLL_PREDIV	0x40

// 根据前端MIPI信号的Timing，修改以下参数：
static int MIPI_Timing[] =
// hfp,	hs,	hbp,	hact,	htotal,	vfp,	vs,	vbp,	vact,	vtotal,	pixel_CLK/10000
//-----|---|------|-------|--------|-----|-----|-----|--------|--------|---------------
 { 48, 	32, 146, 	1368, 	1592, 	03, 		05, 	22, 	768, 	798, 	7622 };


/*******************************************************************
   全志平台的屏参 lcd_hbp 是 上面参数 hbp + hs 的和；
   The lcd_hbp of Allwiner platform is the sum of the above parameters hbp + hs;

   同样的，全志平台的屏参 lcd_vbp 是 上面参数 vbp + vs的 和，这点要注意。
   In the same, lcd_vbp are the sum of the above parameters vbp + vs, which should be noted.
   //-------------------------------------------------------------------

   EOTP(End Of Transmite Packet，hs 传完了，会发这样一个包) 要打开，(之前的LT8911B 需要关闭EOTP，这里LT8911EXB 需要打开)
   Eotp (end of transmit packet, HS will send such a packet) must be turn-on (lt8911b needs to turn-off eotp, here lt8911exb needs to turn-on)

   1、MTK平台 的 dis_eotp_en 的值，改成 false;	 LK和kernel都需要修改,改成false。
   1. The value of dis_eotp_en of MTK platform should be changed to 'false'; LK and kernel need to be changed to 'false'.
   
   2、展讯平台的 tx_eotp 的值置 1 。
   2. The value of tx_eotp of Spreadtrum platform is set to 1.
   
   3、RK平台 EN_EOTP_TX 置1.
   3. The value of EN_EOTP_TX of RK platform is set to 1.
   
   4、高通平台 找到 dsi_ctrl_hw_cmn.c -->void dsi_ctrl_hw_cmn_host_setup(struct dsi_ctrl_hw *ctrl,struct dsi_host_common_cfg *cfg)-->增加cfg->append_tx_eot = true;
   4、Qualcomm: dsi_ctrl_hw_cmn.c -->void dsi_ctrl_hw_cmn_host_setup(struct dsi_ctrl_hw *ctrl,struct dsi_host_common_cfg *cfg)-->add cfg->append_tx_eot = true;
   
*******************************************************************/

#define _6bit_                                                                      // eDP panel Color Depth，262K color

#endif


enum
{
	_Level1_ = 0,   // 15mA     0x82/0x00
	_Level2_,       // 12.8mA   0x81/0x00
	_Level3_,       // 11.2mA   0x80/0xe0
	_Level4_,       // 9.6mA    0x80/0xc0
	_Level5_,       // 8mA      0x80/0xa0
	_Level6_        // 6mA      0x80/0x80
};

u8	Swing_Setting1[] = { 0x82, 0x81, 0x80, 0x80, 0x80, 0x80 };
u8	Swing_Setting2[] = { 0x00, 0x00, 0xe0, 0xc0, 0xa0, 0x80 };

u8	Level = _Level1_;


#define LT8911EXB_SLAVE_ADDR 		0x52
#define LT8911EXB_DRVNAME "LT8911EXB"
#define _uart_debug_

/*****************************************************************************
 *** STATIC FUNCTION
 *****************************************************************************/
static int LT8911EXB_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int LT8911EXB_i2c_remove(struct i2c_client *client);

static struct i2c_client *g_LT8911EXB_i2cClient;

//extern unsigned int GPIO_LCD_BIAS_ENP_PIN; //Backlight
//extern void lcm_set_gpio_output(unsigned int GPIO, unsigned int output);

static int LT8911EXB_i2c_write(u8 cmd, u8 data)
{
    //int ret = 0;
	unsigned char write_buf[2] = {cmd, data};
	//printk("liwy [kernel/LCM] LT8911EXB_i2c_write enter\n");
	printk("dsy log : [Kernel/LCM] %s %d \n",__FUNCTION__,__LINE__);

    if ( i2c_master_send(g_LT8911EXB_i2cClient, write_buf, 2) < 0 ) {
        printk("[Kernel/LCM] LT8911EXB_i2c_write send failed!!\n");
         	printk("dsy log : [Kernel/LCM] %s %d error \n",__FUNCTION__,__LINE__);
        return -1;
    }
	printk("dsy log : [Kernel/LCM] %s %d end \n",__FUNCTION__,__LINE__);
    return 0;
}

static int LT8911EXB_i2c_read(u8 cmd, u8 *data)
{
    //int ret = 0;
    unsigned char pBuff;
	unsigned char puSendCmd[1];
	puSendCmd[0] = cmd;
	
	//printk("liwy [kernel/LCM] LT8911EXB_i2c_read enter\n");
 	printk("dsy log : [Kernel/LCM] %s %d \n",__FUNCTION__,__LINE__);
	if ( i2c_master_send(g_LT8911EXB_i2cClient, puSendCmd, 1) < 0 ) {
		printk("[Kernel/LCM] LT8911EXB_i2c_read  - send failed!!\n");
		return -1;
	}

	if ( i2c_master_recv(g_LT8911EXB_i2cClient, &pBuff, 1) < 0 ) {
		printk("[Kernel/LCM] LT8911EXB_i2c_read  - recv failed!!\n");
		return -1;
	}
	*data = pBuff;
 	printk("dsy log : [Kernel/LCM] %s %d end\n",__FUNCTION__,__LINE__);
    return 0;
}


void LT8911EXB_IIC_Write_byte(u8 cmd, u8 data)
{
	LT8911EXB_i2c_write(cmd, data);
}

u8 LT8911EXB_IIC_Read_byte(u8 cmd)
{
	u8 data = 0;
	
	LT8911EXB_i2c_read(cmd, &data);
	
	return data;
}


// //extern unsigned int GPIO_LCD_RST;
// static void LT8911EXB_set_gpio_output(unsigned int GPIO, unsigned int output)
// {
// 	gpio_direction_output(GPIO, output);
// 	gpio_set_value(GPIO, output);
// }



static int LT8911EXB_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	printk("dsy [kernel/LCM] [LT8911EXB_i2c_probe] enter\n");
	client->addr = (LT8911EXB_SLAVE_ADDR>>1);  
	 printk("[LT8911EXB_i2c_probe] [kernel/LCM] client->addr=0x%x", client->addr);
	g_LT8911EXB_i2cClient = client;
	 printk("[LT8911EXB_i2c_probe] [kernel/LCM] client->addr=0x%x", client->addr);
	if (g_LT8911EXB_i2cClient->addr)
		printk("[LT8911EXB_i2c_probe] [kernel/LCM] g_LT8911EXB_i2cClient->addr=0x%x", g_LT8911EXB_i2cClient->addr);
	printk("dsy [kernel/LCM] [LT8911EXB_i2c_probe] end\n");	
	return 0;
}

static int LT8911EXB_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id LT8911EXB_i2c_id[] = {{LT8911EXB_DRVNAME, 0}, {} };

static const struct of_device_id LT8911EXB_of_match[] = {
    {.compatible = "mediatek,lt8911exb"}, {},
};

static struct i2c_driver LT8911EXB_i2c_driver = {
    .probe = LT8911EXB_i2c_probe,
    .remove = LT8911EXB_i2c_remove,
	.driver = {
		   .name = LT8911EXB_DRVNAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = LT8911EXB_of_match,
#endif
		   },
    .id_table = LT8911EXB_i2c_id,
};

static int __init LT8911EXB_driver_init(void)
{
        int ret;
        printk("[kernel/LCM] LT8911EXB_driver_init enter\n");
        ret=i2c_add_driver(&LT8911EXB_i2c_driver);
        return ret; 		
}

static void __exit LT8911EXB_driver_exit(void)
{
        printk("[kernel/LCM] lt8911exb_exit enter\n");
        i2c_del_driver(&LT8911EXB_i2c_driver);
        return ;
}

/*----------------------------------------------------------------------------*/
module_init(LT8911EXB_driver_init);
module_exit(LT8911EXB_driver_exit);
/*----------------------------------------------------------------------------*/
MODULE_DESCRIPTION("LT8911EXB Driver");
MODULE_AUTHOR("Along");
MODULE_LICENSE("GPL");


void LT8911EXB_MIPI_Video_Timing( void )                                    // ( struct video_timing *video_format )
{
	//printk("liwy [kernel/LCM] %s\n", __func__);
	printk("dsy log : [kernel/LCM] %s %d start \n",__FUNCTION__,__LINE__);
	LT8911EXB_IIC_Write_byte( 0xff, 0xd0 );
	LT8911EXB_IIC_Write_byte( 0x0d, (u8)( MIPI_Timing[vtotal] / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x0e, (u8)( MIPI_Timing[vtotal] % 256 ) );    //vtotal
	LT8911EXB_IIC_Write_byte( 0x0f, (u8)( MIPI_Timing[vact] / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x10, (u8)( MIPI_Timing[vact] % 256 ) );      //vactive

	LT8911EXB_IIC_Write_byte( 0x11, (u8)( MIPI_Timing[htotal] / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x12, (u8)( MIPI_Timing[htotal] % 256 ) );    //htotal
	LT8911EXB_IIC_Write_byte( 0x13, (u8)( MIPI_Timing[hact] / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x14, (u8)( MIPI_Timing[hact] % 256 ) );      //hactive

	LT8911EXB_IIC_Write_byte( 0x15, (u8)( MIPI_Timing[vs] % 256 ) );        //vsa
	LT8911EXB_IIC_Write_byte( 0x16, (u8)( MIPI_Timing[hs] % 256 ) );        //hsa
	LT8911EXB_IIC_Write_byte( 0x17, (u8)( MIPI_Timing[vfp] / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x18, (u8)( MIPI_Timing[vfp] % 256 ) );       //vfp

	LT8911EXB_IIC_Write_byte( 0x19, (u8)( MIPI_Timing[hfp] / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x1a, (u8)( MIPI_Timing[hfp] % 256 ) );       //hfp
}

void DpcdWrite( u32 Address, u8 Data )
{
	/***************************
	   注意大小端的问题!
	   这里默认是大端模式

	   Pay attention to the Big-Endian and Little-Endian!
	   The default mode is Big-Endian here.

	 ****************************/
	u8	AddressH   = 0x0f & ( Address >> 16 );
	u8	AddressM   = 0xff & ( Address >> 8 );
	u8	AddressL   = 0xff & Address;

	u8	reg;
	printk("dsy log : [kernel/LCM] %s %d start \n",__FUNCTION__,__LINE__);
	LT8911EXB_IIC_Write_byte( 0xff, 0xa6 );
	LT8911EXB_IIC_Write_byte( 0x2b, ( 0x80 | AddressH ) );  //CMD
	LT8911EXB_IIC_Write_byte( 0x2b, AddressM );             //addr[15:8]
	LT8911EXB_IIC_Write_byte( 0x2b, AddressL );             //addr[7:0]
	LT8911EXB_IIC_Write_byte( 0x2b, 0x00 );                 //data lenth
	LT8911EXB_IIC_Write_byte( 0x2b, Data );                 //data
	LT8911EXB_IIC_Write_byte( 0x2c, 0x00 );                 //start Aux

	MDELAY( 20 );                                         //more than 10ms
	reg = LT8911EXB_IIC_Read_byte( 0x25 );

	if( ( reg & 0x0f ) == 0x0c )
	{
		return;
	}
}

void LT8911EXB_read_edid( void )
{
#ifdef _read_edid_
	u8 reg, i, j;
//	bool	aux_reply, aux_ack, aux_nack, aux_defer;
	LT8911EXB_IIC_Write_byte( 0xff, 0xac );
	LT8911EXB_IIC_Write_byte( 0x00, 0x20 ); //Soft Link train
	LT8911EXB_IIC_Write_byte( 0xff, 0xa6 );
	LT8911EXB_IIC_Write_byte( 0x2a, 0x01 );

	/*set edid offset addr*/
	LT8911EXB_IIC_Write_byte( 0x2b, 0x40 ); //CMD
	LT8911EXB_IIC_Write_byte( 0x2b, 0x00 ); //addr[15:8]
	LT8911EXB_IIC_Write_byte( 0x2b, 0x50 ); //addr[7:0]
	LT8911EXB_IIC_Write_byte( 0x2b, 0x00 ); //data lenth
	LT8911EXB_IIC_Write_byte( 0x2b, 0x00 ); //data lenth
	LT8911EXB_IIC_Write_byte( 0x2c, 0x00 ); //start Aux read edid

#ifdef _uart_debug_
	printf( "\r\n[LK/LCM]" );
	printf( "\r\n[LK/LCM]Read eDP EDID......" );
#endif
	printk("dsy log : [kernel/LCM] %s %d start \n",__FUNCTION__,__LINE__);
	MDELAY( 20 );                         //more than 10ms
	reg = LT8911EXB_IIC_Read_byte( 0x25 );
	if( ( reg & 0x0f ) == 0x0c )
	{
		for( j = 0; j < 8; j++ )
		{
			if( j == 7 )
			{
				LT8911EXB_IIC_Write_byte( 0x2b, 0x10 ); //MOT
			}else
			{
				LT8911EXB_IIC_Write_byte( 0x2b, 0x50 );
			}

			LT8911EXB_IIC_Write_byte( 0x2b, 0x00 );
			LT8911EXB_IIC_Write_byte( 0x2b, 0x50 );
			LT8911EXB_IIC_Write_byte( 0x2b, 0x0f );
			LT8911EXB_IIC_Write_byte( 0x2c, 0x00 ); //start Aux read edid
			MDELAY( 50 );                         //more than 50ms

			if( LT8911EXB_IIC_Read_byte( 0x39 ) == 0x31 )
			{
				LT8911EXB_IIC_Read_byte( 0x2b );
				for( i = 0; i < 16; i++ )
				{
					EDID_DATA[j * 16 + i] = LT8911EXB_IIC_Read_byte( 0x2b );
				}

				EDID_Reply = 1;
			}else
			{
				EDID_Reply = 0;
#ifdef _uart_debug_
				printf( "\r\nno_reply" );
				printf( "\r\n" );
#endif
				//		print("\r\n*************End***************");
				return;
			}
		}

#ifdef _uart_debug_

		for( i = 0; i < 128; i++ ) //print edid data
		{
			if( ( i % 16 ) == 0 )
			{
				printf( "\r\n" );
			}
			printf( ", ", EDID_DATA[i] );
		}

		printf( "\r\n" );
		printf( "\r\n[LK/LCM]eDP Timing = { H_FP / H_pluse / H_BP / H_act / H_tol / V_FP / V_pluse / V_BP / V_act / V_tol / D_CLK };" );
		printf( "\r\n[LK/LCM]eDP Timing = { " );
		EDID_Timing[hfp] = ( ( EDID_DATA[0x41] & 0xC0 ) * 4 + EDID_DATA[0x3e] );
		//Debug_DispNum( (u32)EDID_Timing[hfp] );         // HFB
		printf( ", " );

		EDID_Timing[hs] = ( ( EDID_DATA[0x41] & 0x30 ) * 16 + EDID_DATA[0x3f] );
		//Debug_DispNum( (u32)EDID_Timing[hs] );          // Hsync Wid
		printf( ", " );

		EDID_Timing[hbp] = ( ( ( EDID_DATA[0x3a] & 0x0f ) * 0x100 + EDID_DATA[0x39] ) - ( ( EDID_DATA[0x41] & 0x30 ) * 16 + EDID_DATA[0x3f] ) - ( ( EDID_DATA[0x41] & 0xC0 ) * 4 + EDID_DATA[0x3e] ) );
		//Debug_DispNum( (u32)EDID_Timing[hbp] );         // HBP
		printf( ", " );

		EDID_Timing[hact] = ( ( EDID_DATA[0x3a] & 0xf0 ) * 16 + EDID_DATA[0x38] );
		//Debug_DispNum( (u32)EDID_Timing[hact] );        // H active
		printf( ", " );

		EDID_Timing[htotal] = ( ( EDID_DATA[0x3a] & 0xf0 ) * 16 + EDID_DATA[0x38] + ( ( EDID_DATA[0x3a] & 0x0f ) * 0x100 + EDID_DATA[0x39] ) );
		//Debug_DispNum( (u32)EDID_Timing[htotal] );      // H total
		printf( ", " );

		EDID_Timing[vfp] = ( ( EDID_DATA[0x41] & 0x0c ) * 4 + ( EDID_DATA[0x40] & 0xf0 ) / 16 );
		//Debug_DispNum( (u32)EDID_Timing[vfp] );         // VFB
		printf( ", " );

		EDID_Timing[vs] = ( ( EDID_DATA[0x41] & 0x03 ) * 16 + EDID_DATA[0x40] & 0x0f );
		//Debug_DispNum( (u32)EDID_Timing[vs] );          // Vsync Wid
		printf( ", " );

		EDID_Timing[vbp] = ( ( ( EDID_DATA[0x3d] & 0x03 ) * 0x100 + EDID_DATA[0x3c] ) - ( ( EDID_DATA[0x41] & 0x03 ) * 16 + EDID_DATA[0x40] & 0x0f ) - ( ( EDID_DATA[0x41] & 0x0c ) * 4 + ( EDID_DATA[0x40] & 0xf0 ) / 16 ) );
		//Debug_DispNum( (u32)EDID_Timing[vbp] );         // VBP
		printf( ", " );

		EDID_Timing[vact] = ( ( EDID_DATA[0x3d] & 0xf0 ) * 16 + EDID_DATA[0x3b] );
		//Debug_DispNum( (u32)EDID_Timing[vact] );        // V active
		printf( ", " );

		EDID_Timing[vtotal] = ( ( EDID_DATA[0x3d] & 0xf0 ) * 16 + EDID_DATA[0x3b] + ( ( EDID_DATA[0x3d] & 0x03 ) * 0x100 + EDID_DATA[0x3c] ) );
		//Debug_DispNum( (u32)EDID_Timing[vtotal] );      // V total
		printf( ", " );

		EDID_Timing[pclk_10khz] = ( EDID_DATA[0x37] * 0x100 + EDID_DATA[0x36] );
		//Debug_DispNum( (u32)EDID_Timing[pclk_10khz] );  // CLK
		printf( " };" );
		printf( "\r\n" );
#endif
	}

	return;

#endif
}

void LT8911EXB_init( void )
{
	u8	i;
	u8	pcr_pll_postdiv;
	u8	pcr_m;
	u16 Temp16;

	/* init */
	LT8911EXB_IIC_Write_byte( 0xff, 0x81 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte( 0x08, 0x7f ); // i2c over aux issue
	LT8911EXB_IIC_Write_byte( 0x49, 0xff ); // enable 0x87xx

	LT8911EXB_IIC_Write_byte( 0xff, 0x82 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte( 0x5a, 0x0e ); // GPIO test output

	//for power consumption//
	LT8911EXB_IIC_Write_byte( 0xff, 0x81 );
	LT8911EXB_IIC_Write_byte( 0x05, 0x06 );
	LT8911EXB_IIC_Write_byte( 0x43, 0x00 );
	LT8911EXB_IIC_Write_byte( 0x44, 0x1f );
	LT8911EXB_IIC_Write_byte( 0x45, 0xf7 );
	LT8911EXB_IIC_Write_byte( 0x46, 0xf6 );
	LT8911EXB_IIC_Write_byte( 0x49, 0x7f );

	LT8911EXB_IIC_Write_byte( 0xff, 0x82 );
	LT8911EXB_IIC_Write_byte( 0x12, 0x33 );

	/* mipi Rx analog */
	LT8911EXB_IIC_Write_byte( 0xff, 0x82 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte( 0x32, 0x51 );
	LT8911EXB_IIC_Write_byte( 0x35, 0x22 ); //EQ current
	LT8911EXB_IIC_Write_byte( 0x3a, 0x77 ); //EQ 12.5db
	LT8911EXB_IIC_Write_byte( 0x3b, 0x77 ); //EQ 12.5db

	LT8911EXB_IIC_Write_byte( 0x4c, 0x0c );
	LT8911EXB_IIC_Write_byte( 0x4d, 0x00 );

	/* dessc_pcr  pll analog */
	LT8911EXB_IIC_Write_byte( 0xff, 0x82 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte( 0x6a, 0x40 );
	LT8911EXB_IIC_Write_byte( 0x6b, PCR_PLL_PREDIV );

	Temp16 = MIPI_Timing[pclk_10khz];

	if( MIPI_Timing[pclk_10khz] < 8800 )
	{
		LT8911EXB_IIC_Write_byte( 0x6e, 0x82 ); //0x44:pre-div = 2 ,pixel_clk=44~ 88MHz
		pcr_pll_postdiv = 0x08;
	}else
	{
		LT8911EXB_IIC_Write_byte( 0x6e, 0x81 ); //0x40:pre-div = 1, pixel_clk =88~176MHz
		pcr_pll_postdiv = 0x04;
	}

	
	pcr_m = (u8)( Temp16 * pcr_pll_postdiv / 25 / 100 );

	/* dessc pll digital */
	LT8911EXB_IIC_Write_byte( 0xff, 0x85 );     // Change Reg bank
	LT8911EXB_IIC_Write_byte( 0xa9, 0x31 );
	LT8911EXB_IIC_Write_byte( 0xaa, 0x17 );
	LT8911EXB_IIC_Write_byte( 0xab, 0xba );
	LT8911EXB_IIC_Write_byte( 0xac, 0xe1 );
	LT8911EXB_IIC_Write_byte( 0xad, 0x47 );
	LT8911EXB_IIC_Write_byte( 0xae, 0x01 );
	LT8911EXB_IIC_Write_byte( 0xae, 0x11 );

	/* Digital Top */
	LT8911EXB_IIC_Write_byte( 0xff, 0x85 );             // Change Reg bank
	LT8911EXB_IIC_Write_byte( 0xc0, 0x01 );             //select mipi Rx
#ifdef _6bit_
	LT8911EXB_IIC_Write_byte( 0xb0, 0xd0 );             //enable dither
#else
	LT8911EXB_IIC_Write_byte( 0xb0, 0x00 );             // disable dither
#endif

	/* mipi Rx Digital */
	LT8911EXB_IIC_Write_byte( 0xff, 0xd0 );             // Change Reg bank
	LT8911EXB_IIC_Write_byte( 0x00, _MIPI_Lane_ % 4 );  // 0: 4 Lane / 1: 1 Lane / 2 : 2 Lane / 3: 3 Lane
	LT8911EXB_IIC_Write_byte( 0x02, 0x08 );             //settle
	LT8911EXB_IIC_Write_byte( 0x08, 0x00 );
//	LT8911EXB_IIC_Write_byte( 0x0a, 0x12 );               //pcr mode

	LT8911EXB_IIC_Write_byte( 0x0c, 0x80 );             //fifo position
	LT8911EXB_IIC_Write_byte( 0x1c, 0x80 );             //fifo position

	//	hs mode:MIPI行采样；vs mode:MIPI帧采样
	LT8911EXB_IIC_Write_byte( 0x24, 0x70 );             // 0x30  [3:0]  line limit	  //pcr mode( de hs vs)

	LT8911EXB_IIC_Write_byte( 0x31, 0x0a );

	/*stage1 hs mode*/
	LT8911EXB_IIC_Write_byte( 0x25, 0x90 );             // 0x80		   // line limit
	LT8911EXB_IIC_Write_byte( 0x2a, 0x3a );             // 0x04		   // step in limit
	LT8911EXB_IIC_Write_byte( 0x21, 0x4f );             // hs_step
	LT8911EXB_IIC_Write_byte( 0x22, 0xff );

	/*stage2 de mode*/
	LT8911EXB_IIC_Write_byte( 0x0a, 0x02 );             //de adjust pre line
	LT8911EXB_IIC_Write_byte( 0x38, 0x02 );             //de_threshold 1
	LT8911EXB_IIC_Write_byte( 0x39, 0x04 );             //de_threshold 2
	LT8911EXB_IIC_Write_byte( 0x3a, 0x08 );             //de_threshold 3
	LT8911EXB_IIC_Write_byte( 0x3b, 0x10 );             //de_threshold 4

	LT8911EXB_IIC_Write_byte( 0x3f, 0x04 );             //de_step 1
	LT8911EXB_IIC_Write_byte( 0x40, 0x08 );             //de_step 2
	LT8911EXB_IIC_Write_byte( 0x41, 0x10 );             //de_step 3
	LT8911EXB_IIC_Write_byte( 0x42, 0x60 );             //de_step 4

	/*stage2 hs mode*/
	LT8911EXB_IIC_Write_byte( 0x1e, 0x01 );             // 0x11
	LT8911EXB_IIC_Write_byte( 0x23, 0xf0 );             // 0x80			   //

	LT8911EXB_IIC_Write_byte( 0x2b, 0x80 );             // 0xa0

#ifdef _Test_Pattern_
	LT8911EXB_IIC_Write_byte( 0x26, ( pcr_m | 0x80 ) );
#else

//	LT8911EXB_IIC_Write_byte( 0x26, pcr_m  );
	LT8911EXB_IIC_Write_byte( 0x26, 0x15 );

	LT8911EXB_IIC_Write_byte( 0x27, 0xb7 );
	LT8911EXB_IIC_Write_byte( 0x28, 0xb0 );
#endif

	LT8911EXB_MIPI_Video_Timing( );         //defualt setting is 1080P

	LT8911EXB_IIC_Write_byte( 0xff, 0x81 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte( 0x03, 0x7b ); //PCR reset
	LT8911EXB_IIC_Write_byte( 0x03, 0xff );

#ifdef _eDP_2G7_
	LT8911EXB_IIC_Write_byte( 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte( 0x19, 0x31 );
	LT8911EXB_IIC_Write_byte( 0x1a, 0x36 ); // sync m
	LT8911EXB_IIC_Write_byte( 0x1b, 0x00 ); // sync_k [7:0]
	LT8911EXB_IIC_Write_byte( 0x1c, 0x00 ); // sync_k [13:8]

	// txpll Analog
	LT8911EXB_IIC_Write_byte( 0xff, 0x82 );
//	LT8911EXB_IIC_Write_byte( 0x01, 0x18 );// default : 0x18
	LT8911EXB_IIC_Write_byte( 0x02, 0x42 );
	LT8911EXB_IIC_Write_byte( 0x03, 0x00 ); // txpll en = 0
	LT8911EXB_IIC_Write_byte( 0x03, 0x01 ); // txpll en = 1
//	LT8911EXB_IIC_Write_byte( 0x04, 0x3a );// default : 0x3A

	LT8911EXB_IIC_Write_byte( 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte( 0x0c, 0x10 ); // cal en = 0

	LT8911EXB_IIC_Write_byte( 0xff, 0x81 );
	LT8911EXB_IIC_Write_byte( 0x09, 0xfc );
	LT8911EXB_IIC_Write_byte( 0x09, 0xfd );

	LT8911EXB_IIC_Write_byte( 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte( 0x0c, 0x11 ); // cal en = 1

	// ssc
	LT8911EXB_IIC_Write_byte( 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte( 0x13, 0x83 );
	LT8911EXB_IIC_Write_byte( 0x14, 0x41 );
	LT8911EXB_IIC_Write_byte( 0x16, 0x0a );
	LT8911EXB_IIC_Write_byte( 0x18, 0x0a );
	LT8911EXB_IIC_Write_byte( 0x19, 0x33 );
#endif

#ifdef _eDP_1G62_
	LT8911EXB_IIC_Write_byte( 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte( 0x19, 0x31 );
	LT8911EXB_IIC_Write_byte( 0x1a, 0x20 ); // sync m
	LT8911EXB_IIC_Write_byte( 0x1b, 0x19 ); // sync_k [7:0]
	LT8911EXB_IIC_Write_byte( 0x1c, 0x99 ); // sync_k [13:8]

	// txpll Analog
	LT8911EXB_IIC_Write_byte( 0xff, 0x82 );
	//	LT8911EXB_IIC_Write_byte( 0x01, 0x18 );// default : 0x18
	LT8911EXB_IIC_Write_byte( 0x02, 0x42 );
	LT8911EXB_IIC_Write_byte( 0x03, 0x00 ); // txpll en = 0
	LT8911EXB_IIC_Write_byte( 0x03, 0x01 ); // txpll en = 1
	//	LT8911EXB_IIC_Write_byte( 0x04, 0x3a );// default : 0x3A

	LT8911EXB_IIC_Write_byte( 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte( 0x0c, 0x10 ); // cal en = 0

	LT8911EXB_IIC_Write_byte( 0xff, 0x81 );
	LT8911EXB_IIC_Write_byte( 0x09, 0xfc );
	LT8911EXB_IIC_Write_byte( 0x09, 0xfd );

	LT8911EXB_IIC_Write_byte( 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte( 0x0c, 0x11 ); // cal en = 1

	//ssc
	LT8911EXB_IIC_Write_byte( 0xff, 0x87 );
	LT8911EXB_IIC_Write_byte( 0x13, 0x83 );
	LT8911EXB_IIC_Write_byte( 0x14, 0x41 );
	LT8911EXB_IIC_Write_byte( 0x16, 0x0a );
	LT8911EXB_IIC_Write_byte( 0x18, 0x0a );
	LT8911EXB_IIC_Write_byte( 0x19, 0x33 );
#endif
	printk("dsy log : [kernel/LCM] %s %d start \n",__FUNCTION__,__LINE__);
	for( i = 0; i < 5; i++ )      //Check Tx PLL
	{
		MDELAY( 5 );
		if( LT8911EXB_IIC_Read_byte( 0x37 ) & 0x02 )
		{
			printk( "\r\nLT8911 tx pll locked" );
			break;
		}else
		{
			printk( "\r\nLT8911 tx pll unlocked" );
			LT8911EXB_IIC_Write_byte( 0xff, 0x81 );
			LT8911EXB_IIC_Write_byte( 0x09, 0xfc );
			LT8911EXB_IIC_Write_byte( 0x09, 0xfd );

			LT8911EXB_IIC_Write_byte( 0xff, 0x87 );
			LT8911EXB_IIC_Write_byte( 0x0c, 0x10 );
			LT8911EXB_IIC_Write_byte( 0x0c, 0x11 );
		}
	}
	/* tx phy */
	LT8911EXB_IIC_Write_byte( 0xff, 0x82 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte( 0x11, 0x00 );
	LT8911EXB_IIC_Write_byte( 0x13, 0x10 );
	LT8911EXB_IIC_Write_byte( 0x14, 0x0c );
	LT8911EXB_IIC_Write_byte( 0x14, 0x08 );
	LT8911EXB_IIC_Write_byte( 0x13, 0x20 );

	LT8911EXB_IIC_Write_byte( 0xff, 0x82 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte( 0x0e, 0x35 );
//	LT8911EXB_IIC_Write_byte( 0x12, 0xff );
//	LT8911EXB_IIC_Write_byte( 0xff, 0x80 );
//	LT8911EXB_IIC_Write_byte( 0x40, 0x22 );

	/*eDP Tx Digital */
	LT8911EXB_IIC_Write_byte( 0xff, 0xa8 ); // Change Reg bank
#ifdef _Test_Pattern_
	LT8911EXB_IIC_Write_byte( 0x24, 0x50 ); // bit2 ~ bit 0 : test panttern image mode
	LT8911EXB_IIC_Write_byte( 0x25, 0x70 ); // bit6 ~ bit 4 : test Pattern color
	LT8911EXB_IIC_Write_byte( 0x27, 0x50 ); //0x50:Pattern; 0x10:mipi video
#else
	LT8911EXB_IIC_Write_byte( 0x27, 0x10 ); //0x50:Pattern; 0x10:mipi video
#endif

#ifdef _6bit_
	LT8911EXB_IIC_Write_byte( 0x17, 0x00 );
	LT8911EXB_IIC_Write_byte( 0x18, 0x00 );
#else
	// _8bit_
	LT8911EXB_IIC_Write_byte( 0x17, 0x10 );
	LT8911EXB_IIC_Write_byte( 0x18, 0x20 );
#endif

	LT8911EXB_IIC_Write_byte( 0xff, 0xa0 ); // Change Reg bank
	LT8911EXB_IIC_Write_byte( 0x00, 0x00 );// 0x08
	LT8911EXB_IIC_Write_byte( 0x01, 0x80 );// 0x00
}


static void LT8911EXB_eDP_Video_cfg( void )                                        // ( struct video_timing *video_format )
{
	LT8911EXB_IIC_Write_byte( 0xff, 0xa8 );
	LT8911EXB_IIC_Write_byte( 0x2d, 0x88 );                                 // MSA from register
	printk("dsy log : [kernel/LCM] %s %d start \n",__FUNCTION__,__LINE__);
#ifdef _Msa_Active_Only_
	LT8911EXB_IIC_Write_byte( 0x05, 0x00 );
	LT8911EXB_IIC_Write_byte( 0x06, 0x00 );                                 //htotal
	LT8911EXB_IIC_Write_byte( 0x07, 0x00 );
	LT8911EXB_IIC_Write_byte( 0x08, 0x00 );                                 //h_start
	LT8911EXB_IIC_Write_byte( 0x09, 0x00 );
	LT8911EXB_IIC_Write_byte( 0x0a, 0x00 );                                 //hsa
	LT8911EXB_IIC_Write_byte( 0x0b, (u8)( MIPI_Timing[hact] / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x0c, (u8)( MIPI_Timing[hact] % 256 ) );      //hactive
	LT8911EXB_IIC_Write_byte( 0x0d, 0x00 );
	LT8911EXB_IIC_Write_byte( 0x0e, 0x00 );                                 //vtotal
	LT8911EXB_IIC_Write_byte( 0x11, 0x00 );
	LT8911EXB_IIC_Write_byte( 0x12, 0x00 );
	LT8911EXB_IIC_Write_byte( 0x14, 0x00 );
	LT8911EXB_IIC_Write_byte( 0x15, (u8)( MIPI_Timing[vact] / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x16, (u8)( MIPI_Timing[vact] % 256 ) );      //vactive
	printk("dsy log : [kernel/LCM] %s %d 1111 \n",__FUNCTION__,__LINE__);
#else

	LT8911EXB_IIC_Write_byte( 0x05, (u8)( MIPI_Timing[htotal] / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x06, (u8)( MIPI_Timing[htotal] % 256 ) );
	LT8911EXB_IIC_Write_byte( 0x07, (u8)( ( MIPI_Timing[hs] + MIPI_Timing[hbp] ) / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x08, (u8)( ( MIPI_Timing[hs] + MIPI_Timing[hbp] ) % 256 ) );
	LT8911EXB_IIC_Write_byte( 0x09, (u8)( MIPI_Timing[hs] / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x0a, (u8)( MIPI_Timing[hs] % 256 ) );
	LT8911EXB_IIC_Write_byte( 0x0b, (u8)( MIPI_Timing[hact] / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x0c, (u8)( MIPI_Timing[hact] % 256 ) );
	LT8911EXB_IIC_Write_byte( 0x0d, (u8)( MIPI_Timing[vtotal] / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x0e, (u8)( MIPI_Timing[vtotal] % 256 ) );
	LT8911EXB_IIC_Write_byte( 0x11, (u8)( ( MIPI_Timing[vs] + MIPI_Timing[vbp] ) / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x12, (u8)( ( MIPI_Timing[vs] + MIPI_Timing[vbp] ) % 256 ) );
	LT8911EXB_IIC_Write_byte( 0x14, (u8)( MIPI_Timing[vs] % 256 ) );
	LT8911EXB_IIC_Write_byte( 0x15, (u8)( MIPI_Timing[vact] / 256 ) );
	LT8911EXB_IIC_Write_byte( 0x16, (u8)( MIPI_Timing[vact] % 256 ) );
		printk("dsy log : [kernel/LCM] %s %d 2222 \n",__FUNCTION__,__LINE__);
#endif
}


static void LT8911EX_ChipID( void )                                                        // read Chip ID
{
	//printk("liwy [kernel/LCM] %s\n", __func__);
//	//printk( "\r\n###################start#####################" );
	LT8911EXB_IIC_Write_byte( 0xff, 0x81 );                                         //register bank
	LT8911EXB_IIC_Write_byte( 0x08, 0x7f );
	printk("dsy log : [kernel/LCM] %s %d start \n",__FUNCTION__,__LINE__);
#ifdef _uart_debug_
	printk( "dsy [kernel/LCM] LT8911EXB chip ID: 0x%x", LT8911EXB_IIC_Read_byte( 0x00 ) );  // 0x17
	printk( ", [kernel/LCM] ID: 0x%x", LT8911EXB_IIC_Read_byte( 0x01 ) );                      // 0x05
	printk( ", [kernel/LCM] ID: 0x%x\n", LT8911EXB_IIC_Read_byte( 0x02 ) );                      // 0xE0
#endif
}

// void Reset_LT8911EXB( void )
// {
// 	//printk("liwy [kernel/LCM] %s\n", __func__);
// 	//_LT8911_RSTN_High;  // GPIO High
// 	//LT8911EXB_set_gpio_output(GPIO_LCD_RST, 1);
// 	//mdelay(100);
// 	//_LT8911_RSTN_LOW;   // GPIO Low
// 	LT8911EXB_set_gpio_output(GPIO_LCD_RST, 0);
// 	mdelay(100);
// 	//_LT8911_RSTN_High;  // GPIO High
// 	LT8911EXB_set_gpio_output(GPIO_LCD_RST, 1);
// 	mdelay(100);
// }

u8 DpcdRead( u32 Address )
{
	/***************************
	   注意大小端的问题!
	   这里默认是大端模式

	   Pay attention to the Big-Endian and Little-Endian!
	   The default mode is Big-Endian here.

	 ****************************/

	u8	DpcdValue  = 0x00;
	u8	AddressH   = 0x0f & ( Address >> 16 );
	u8	AddressM   = 0xff & ( Address >> 8 );
	u8	AddressL   = 0xff & Address;
	u8	reg;

	LT8911EXB_IIC_Write_byte( 0xff, 0xac );
	LT8911EXB_IIC_Write_byte( 0x00, 0x20 );                 //Soft Link train
	LT8911EXB_IIC_Write_byte( 0xff, 0xa6 );
	LT8911EXB_IIC_Write_byte( 0x2a, 0x01 );
	printk("dsy log : [kernel/LCM] %s %d start \n",__FUNCTION__,__LINE__);
	LT8911EXB_IIC_Write_byte( 0xff, 0xa6 );
	LT8911EXB_IIC_Write_byte( 0x2b, ( 0x90 | AddressH ) );  //CMD
	LT8911EXB_IIC_Write_byte( 0x2b, AddressM );             //addr[15:8]
	LT8911EXB_IIC_Write_byte( 0x2b, AddressL );             //addr[7:0]
	LT8911EXB_IIC_Write_byte( 0x2b, 0x00 );                 //data lenth
	LT8911EXB_IIC_Write_byte( 0x2c, 0x00 );                 //start Aux read edid

	MDELAY( 50 );                                         //more than 10ms
	reg = LT8911EXB_IIC_Read_byte( 0x25 );
	if( ( reg & 0x0f ) == 0x0c )
	{
		if( LT8911EXB_IIC_Read_byte( 0x39 ) == 0x22 )
		{
			LT8911EXB_IIC_Read_byte( 0x2b );
			DpcdValue = LT8911EXB_IIC_Read_byte( 0x2b );
		}


		/*
		   else
		   {
		   //	goto no_reply;
		   //	DpcdValue = 0xff;
		   return DpcdValue;
		   }//*/
	}

	return DpcdValue;
}

void LT8911EX_link_train( void )
{
	LT8911EXB_IIC_Write_byte( 0xff, 0x85 );
	printk("dsy log : [kernel/LCM] %s %d start \n",__FUNCTION__,__LINE__);
//#ifdef _eDP_scramble_
	if( ScrambleMode )
	{
		LT8911EXB_IIC_Write_byte( 0xa1, 0x82 ); // eDP scramble mode;

		/* Aux operater init */
		LT8911EXB_IIC_Write_byte( 0xff, 0xac );
		LT8911EXB_IIC_Write_byte( 0x00, 0x20 ); //Soft Link train
		LT8911EXB_IIC_Write_byte( 0xff, 0xa6 );
		LT8911EXB_IIC_Write_byte( 0x2a, 0x01 );

		DpcdWrite( 0x010a, 0x01 );
		MDELAY( 10 );
		DpcdWrite( 0x0102, 0x00 );
		MDELAY( 10 );
		DpcdWrite( 0x010a, 0x01 );

		MDELAY( 200 );
	}
//#else
	else
	{
		LT8911EXB_IIC_Write_byte( 0xa1, 0x02 ); // DP scramble mode;
	}
//#endif

	/* Aux setup */
	LT8911EXB_IIC_Write_byte( 0xff, 0xac );
	LT8911EXB_IIC_Write_byte( 0x00, 0x60 );     //Soft Link train
	LT8911EXB_IIC_Write_byte( 0xff, 0xa6 );
	LT8911EXB_IIC_Write_byte( 0x2a, 0x00 );

	LT8911EXB_IIC_Write_byte( 0xff, 0x81 );
	LT8911EXB_IIC_Write_byte( 0x07, 0xfe );
	LT8911EXB_IIC_Write_byte( 0x07, 0xff );
	LT8911EXB_IIC_Write_byte( 0x0a, 0xfc );
	LT8911EXB_IIC_Write_byte( 0x0a, 0xfe );

	/* link train */

	LT8911EXB_IIC_Write_byte( 0xff, 0x85 );
	LT8911EXB_IIC_Write_byte( 0x1a, eDP_lane );

	LT8911EXB_IIC_Write_byte( 0xff, 0xac );
	LT8911EXB_IIC_Write_byte( 0x00, 0x64 );
	LT8911EXB_IIC_Write_byte( 0x01, 0x0a );
	LT8911EXB_IIC_Write_byte( 0x0c, 0x85 );
	LT8911EXB_IIC_Write_byte( 0x0c, 0xc5 );
//	MDELAY( 500 );
}

void LT8911EX_link_train_result( void )
{
	u8 i, reg;
	LT8911EXB_IIC_Write_byte( 0xff, 0xac );
	for( i = 0; i < 10; i++ )
	{
		reg = LT8911EXB_IIC_Read_byte( 0x82 );
		//  printf( "\r\n0x82 = ", reg );
		if( reg & 0x20 )
		{
			if( ( reg & 0x1f ) == 0x1e )
			{
				printk( "\r\n[kernel/LCM]Link train success, 0x82 = ", reg );
			} else
			{
				printk( "\r\n[kernel/LCM]Link train fail, 0x82 = ", reg );
			}

			printk( "\r\n[kernel/LCM] panel link rate: ", LT8911EXB_IIC_Read_byte( 0x83 ) );
			printk( "\r\n[kernel/LCM] panel link count: ", LT8911EXB_IIC_Read_byte( 0x84 ) );
			return;
		}else
		{
			printk( "\r\n [kernel/LCM] link trian on going..." );
		}
		MDELAY( 100 );
	}
}

void LT8911EX_TxSwingPreSet( void )
{
		printk("dsy log : [kernel/LCM] %s %d start \n",__FUNCTION__,__LINE__);
	LT8911EXB_IIC_Write_byte( 0xFF, 0x82 );
	LT8911EXB_IIC_Write_byte( 0x22, Swing_Setting1[Level] ); //lane 0 tap0
	LT8911EXB_IIC_Write_byte( 0x23, Swing_Setting2[Level] );
	LT8911EXB_IIC_Write_byte( 0x24, 0x80 );                  //lane 0 tap1
	LT8911EXB_IIC_Write_byte( 0x25, 0x00 );
	LT8911EXB_IIC_Write_byte( 0x26, Swing_Setting1[Level] ); //lane 1 tap0
	LT8911EXB_IIC_Write_byte( 0x27, Swing_Setting2[Level] );
	LT8911EXB_IIC_Write_byte( 0x28, 0x80 );                  //lane 1 tap1
	LT8911EXB_IIC_Write_byte( 0x29, 0x00 );
}

void LT8911EXB_video_check( void )
{
	//u8	temp;
	u32 reg = 0x00;
	/* mipi byte clk check*/
	LT8911EXB_IIC_Write_byte( 0xff, 0x85 );     // Change Reg bank
	LT8911EXB_IIC_Write_byte( 0x1d, 0x00 );     //FM select byte clk
	LT8911EXB_IIC_Write_byte( 0x40, 0xf7 );
	LT8911EXB_IIC_Write_byte( 0x41, 0x30 );
	printk("dsy log : [kernel/LCM] %s %d start \n",__FUNCTION__,__LINE__);
	//#ifdef _eDP_scramble_
	if( ScrambleMode )
	{
		LT8911EXB_IIC_Write_byte( 0xa1, 0x82 ); //eDP scramble mode;
	}
	//#else
	else
	{
		LT8911EXB_IIC_Write_byte( 0xa1, 0x02 ); // DP scramble mode;
	}
	//#endif

//	LT8911EXB_IIC_Write_byte( 0x17, 0xf0 ); // 0xf0:Close scramble; 0xD0 : Open scramble

	LT8911EXB_IIC_Write_byte( 0xff, 0x81 );
	LT8911EXB_IIC_Write_byte( 0x09, 0x7d );
	LT8911EXB_IIC_Write_byte( 0x09, 0xfd );

	LT8911EXB_IIC_Write_byte( 0xff, 0x85 );
	MDELAY( 200 );
	if( LT8911EXB_IIC_Read_byte( 0x50 ) == 0x03 )
	{
		reg	   = LT8911EXB_IIC_Read_byte( 0x4d );
		reg	   = reg * 256 + LT8911EXB_IIC_Read_byte( 0x4e );
		reg	   = reg * 256 + LT8911EXB_IIC_Read_byte( 0x4f );

		printk( "[kernel/LCM] video check: mipi byteclk =  %d \n", reg * 1000); // mipi byteclk = reg * 1000
		//Debug_DispNum( reg );
	}else
	{
		printk( "\r\n[kernel/LCM] video check: mipi clk unstable" );
	}

	/* mipi vtotal check*/
	reg	   = LT8911EXB_IIC_Read_byte( 0x76 );
	reg	   = reg * 256 + LT8911EXB_IIC_Read_byte( 0x77 );

	printk( "\r\n [kernel/LCM] video check: Vtotal =  0x%x" , reg);
	//Debug_DispNum( reg );

	/* mipi word count check*/
	LT8911EXB_IIC_Write_byte( 0xff, 0xd0 );
	reg	   = LT8911EXB_IIC_Read_byte( 0x82 );
	reg	   = reg * 256 + LT8911EXB_IIC_Read_byte( 0x83 );
	reg	   = reg / 3;

	printk( "\r\n [kernel/LCM] video check: Hact(word counter) = 0x%x " , reg );
	//Debug_DispNum( reg );

	/* mipi Vact check*/
	reg	   = LT8911EXB_IIC_Read_byte( 0x85 );
	reg	   = reg * 256 + LT8911EXB_IIC_Read_byte( 0x86 );

	printk( "\r\n [kernel/LCM] video check: Vact =  0x%x" , reg );
	//Debug_DispNum( reg );
}

void PCR_Status( void )                     // for debug
{
#ifdef _uart_debug_
	u8 reg;

	LT8911EXB_IIC_Write_byte( 0xff, 0xd0 );
	reg = LT8911EXB_IIC_Read_byte( 0x87 );

	printk( "[kernel/LCM] Reg0xD087 = 0x%x \n", reg);

	if( reg & 0x10 )
	{
		printk( "[kernel/LCM] PCR Clock stable\n" );
	}else
	{
		printk( "[kernel/LCM] PCR Clock unstable\n" );
	}
#endif
}


void LT8911EXB_config(void)
{
	printk("dsy [kernel/LCM] LT8911EXB_config enter\n");
	printk("dsy log : [kernel/LCM] %s %d start \n",__FUNCTION__,__LINE__);
	//Reset_LT8911EXB();     // 先Reset LT8911EXB ,用GPIO 先拉低LT8911EXB的复位脚 100ms左右，再拉高，保持100ms。

	LT8911EX_ChipID();     // read Chip ID

	LT8911EXB_eDP_Video_cfg();
	LT8911EXB_init();

	LT8911EXB_read_edid(); // for debug

	Read_DPCD010A = DpcdRead( 0x010A ) & 0x01;

#ifdef _uart_debug_
	//printk( "\r\n" );
	//printk( "\r\nDPCD010Ah:%x,", Read_DPCD010A );
#endif

	if( Read_DPCD010A ){
		ScrambleMode = 1;
	}else{
		ScrambleMode = 0;
	}

	LT8911EX_link_train();

	LT8911EX_link_train_result();  // for debug

	LT8911EX_TxSwingPreSet();

	LT8911EXB_video_check();       // just for Check MIPI Input

	PCR_Status();                  // just for Check PCR CLK
printk("dsy log : [kernel/LCM] %s %d end \n",__FUNCTION__,__LINE__);
//	lcm_set_gpio_output(GPIO_LCD_BIAS_ENP_PIN, 1); //Backlight
  //while( 1 )
  //{
  //// 循环检测MIPI信号，如果有断续，会出现reset PCR
  ////Loop detection of Mipi signal. If there is interruption, reset PCR will appear
  //LT8911_MainLoop( );
  //
  //mdelay( 1000 );
  //}
}



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

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
		memset(params, 0, sizeof(struct LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;
		
    #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
    #else
		params->dsi.mode   = SYNC_PULSE_VDO_MODE;//SYNC_EVENT_VDO_MODE;		//SYNC_EVENT_VDO_MODE;
    #endif
	printk("dsy log : [kernel/LCM] %s %d \n",__FUNCTION__,__LINE__);
		// DSI
		/* Command mode setting */
		// Three lane or Four lane
		params->dsi.LANE_NUM								= LCM_FOUR_LANE;
		
		//The following defined the fomat for data coming from LCD engine.
		//params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		//params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		//params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Highly depends on LCD driver capability.
		// Not support in MT6573
		//params->dsi.packet_size=256;

		// Video mode setting		
		//params->dsi.intermediat_buffer_num = 0;

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		//params->dsi.word_count=FRAME_WIDTH*3;
		
		
	params->dsi.vertical_sync_active = 5;
	params->dsi.vertical_backporch = 23;
	params->dsi.vertical_frontporch = 3;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 32;
	params->dsi.horizontal_backporch = 80;
	params->dsi.horizontal_frontporch = 48;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	/* video mode timing */
	//params->dsi.word_count = FRAME_WIDTH * 3;

	params->dsi.PLL_CLOCK = 420;
	params->dsi.ssc_disable = 1;
	params->dsi.cont_clock = 1;
	params->dsi.clk_lp_per_line_enable = 0;	
		
    //	params->dsi.ssc_disable							= 1;
	
	//	params->dsi.cont_clock= 1;
		
}
// static void lcd_power_en(unsigned int enabled)
// {
//     if (enabled)
//     {
//         lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ONE);
//     }
//     else
//     {
//         lcm_set_gpio_output(GPIO_LCM_BL_EN, GPIO_OUT_ZERO);
//     }
// }

// static void avdd_enable(unsigned int enabled)
// {
//     if (enabled)
//     {
//         lcm_set_gpio_output(GPIO_LCM_3v3_EN, GPIO_OUT_ONE);
//     }
//     else
//     {
//         lcm_set_gpio_output(GPIO_LCM_3v3_EN, GPIO_OUT_ZERO);
//     }
// }

// static void dvdd_enable(unsigned int enabled)
// {
//     if (enabled)
//     {
//         lcm_set_gpio_output(GPIO_LCM_1v8_EN, GPIO_OUT_ONE);
//     }
//     else
//     {
//         lcm_set_gpio_output(GPIO_LCM_1v8_EN, GPIO_OUT_ZERO);
//     }
// }

static void lcd_reset(unsigned int enabled)
{
	printk("dsy log : [kernel/LCM] %s %d \n",__FUNCTION__,__LINE__);
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




static void lcm_init(void)
{

	// avdd_enable(0);
	// dvdd_enable(0);
	// lcd_reset(0);
	// lcd_power_en(0);
	// MDELAY(10);
printk("dsy log : [kernel/LCM] %s %d start\n",__FUNCTION__,__LINE__);
		lcm_set_gpio_output(EDP_LCM_LED_EN1, 0);//bl
	MDELAY(10);


	lcm_set_gpio_output(GPIO_LCM_LT8911B_VDD18_EN, 1);;//en
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCM_VLCM33_EN, 1); //3.3
	 MDELAY(10);
	// avdd_enable(1);
	// MDELAY(10);
	lcm_set_gpio_output(GPIO_LCM_GPIO5_IRQO2, 1); 
	MDELAY(10);


	// lcd_reset(1);
	// MDELAY(20);
	lcd_reset(0);	
	MDELAY(100);
	lcd_reset(1);
	MDELAY(100);

	LT8911EXB_config();
	MDELAY(50);

	lcm_set_gpio_output(EDP_LCM_LED_EN1, 1);//bl
	MDELAY(10);
	printk("dsy log : [kernel/LCM] %s %d end\n",__FUNCTION__,__LINE__);
}

static void lcm_suspend(void)
{


	printk("dsy log : [kernel/LCM] %s %d start\n",__FUNCTION__,__LINE__);
	// lcd_power_en(0);
	// MDELAY(20);	
		lcm_set_gpio_output(EDP_LCM_LED_EN1, 0);
	MDELAY(10);

    lcd_reset(0);
    MDELAY(100);
      	lcm_set_gpio_output(GPIO_LCM_LT8911B_VDD18_EN, 0);;//en
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCM_VLCM33_EN, 0); 
	 MDELAY(10);
	// avdd_enable(1);
	// MDELAY(10);
	lcm_set_gpio_output(GPIO_LCM_GPIO5_IRQO2, 0); 
	MDELAY(10);
 //    avdd_enable(0);
	// MDELAY(20);
printk("dsy log : [kernel/LCM] %s %d end\n",__FUNCTION__,__LINE__);
	// dvdd_enable(0);
	// MDELAY(20);

}

static void lcm_resume(void)
{
	printk("dsy log : [kernel/LCM] %s %d start\n",__FUNCTION__,__LINE__);
	lcm_init();
}


struct LCM_DRIVER us145u_ag_lt8911exb_wuxga_ips_101_lcm_drv = 
{
  .name						= "us145u_ag_lt8911exb_wuxga_ips_101",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if (LCM_DSI_CMD_MODE)
  .update         = lcm_update,
#endif
};
