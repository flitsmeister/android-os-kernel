#ifndef BUILD_LK
#include <linux/string.h>
#else
#include <string.h>
#endif 

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#elif (defined BUILD_UBOOT)
#include <asm/arch/mt6577_gpio.h>
#else
	#include "upmu_common.h"
    //#include <mt-plat/mt_gpio.h>
	//#include <mach/gpio_const.h>
	//#include <mt-plat/mtk_thermal_typedefs.h>
#endif
// tong add for  DSI_clk_HS_mode
#include "ddp_hal.h"  
#include "lcm_drv.h"

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
#include "lcm_drv.h"
//#include <mt-plat/mt_gpio.h>


extern unsigned int LCM_POWER_EN;
extern unsigned int EDP_RESET;
extern unsigned int EDP_STANDBY;
extern unsigned int EDP_POWER_EN;
extern unsigned int VLED_EN;

#define GPIO_OUT_ZERO 0
#define GPIO_OUT_ONE 1

extern void DSI_clk_HS_mode(enum DISP_MODULE_ENUM module, void *cmdq, bool enter);

#ifdef BUILD_LK
	#define LT8911_LOG printf
	#define LT8911_REG_WRITE(add, data) lt8911_reg_i2c_write(add, data)
	#define LT8911_REG_READ(add) lt8911_reg_i2c_read(add)
#elif (defined BUILD_UBOOT)
#else
	//extern int lt8911_i2c_write(u8 addr, u8 data);
	extern int  lt8911_reg_i2c_read(u16 addr, u8 len);
	extern int lt8911_reg_i2c_write(u16 addr, u32 val, u8 len);
	#define LT8911_LOG printk
	#define LT8911_REG_WRITE(add, data, len) lt8911_reg_i2c_write(add, data, len)
	#define LT8911_REG_READ(add,len) lt8911_reg_i2c_read(add,len)
	
	extern int lcm_i2c_suspend(void);
	extern int lcm_i2c_resume(void);
#endif

// #define _8_bit_	// 6 bit color depth
#define _6_bit_
//#define _Test_Pattern_ 

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (1280)
#define FRAME_HEIGHT (800)
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))
#ifdef BUILD_LK
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output){
	if(GPIO == 0xFFFFFFFF)  
		{  
		#ifdef BUILD_LK    
		printf("[LK/LCM] invalid gpio\n");	  
		#else	  	
		printk("[LK/LCM] invalid gpio\n");	  
		#endif 
		}       
	mt_set_gpio_mode(GPIO, GPIO_MODE_00);    
	mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);    
	mt_set_gpio_out(GPIO, (output>0)? GPIO_OUT_ONE: GPIO_OUT_ZERO);
}
#else
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
        gpio_direction_output(GPIO, output);
        gpio_set_value(GPIO, output);
}
#endif
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

    params->dsi.mode    = SYNC_EVENT_VDO_MODE;//SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;//LCM_DSI_FORMAT_RGB666;//LCM_DSI_FORMAT_RGB888;

    params->dsi.PS= LCM_PACKED_PS_24BIT_RGB888;//LCM_PACKED_PS_18BIT_RGB666;//LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 

//add
    //params->dsi.packet_size = 256;
    //params->lcm_if = LCM_INTERFACE_DSI0;
    //params->lcm_cmd_if = LCM_INTERFACE_DSI0;
   // params->dsi.word_count = FRAME_WIDTH * 3;
	//params->dsi.pll_select = 0;	/* 0: MIPI_PLL; 1: LVDS_PLL */
	params->dsi.cont_clock=1; 
//

    params->dsi.vertical_sync_active    = 4;
    params->dsi.vertical_backporch      = 4;
    params->dsi.vertical_frontporch     = 4;
    params->dsi.vertical_active_line = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active  = 15;
    params->dsi.horizontal_backporch    = 15;
    params->dsi.horizontal_frontporch   = 20;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;

    params->dsi.PLL_CLOCK=200;//408; 
}



///////////////////////////////////////////////////////////////////////////////////////
#ifdef BUILD_LK
#define I2C_CH                I2C4
#define LT8911_I2C_ADDR      0x1f

u32 _lt8911_i2c_read (U8 chip, U8 *cmdBuffer, int cmdBufferLen, U8 *dataBuffer, int dataBufferLen)
{
    U32 ret_code = I2C_OK;
	U8 len; 
	struct mt_i2c_t i2c;
	
	i2c.id = I2C_CH;
	i2c.addr = (LT8911_I2C_ADDR>>1);
	i2c.mode = ST_MODE;
	i2c.speed = 100;	
	len  = 1; 

	int write_len = cmdBufferLen;
	int read_len = dataBufferLen;
	
	//*dataBuffer = *cmdBuffer;
	ret_code = i2c_write_read(&i2c, dataBuffer, write_len, read_len);
    return ret_code;
}

u32 _lt8911_i2c_write(U8 chip, U8 *cmdBuffer, int cmdBufferLen, U8 *dataBuffer, int dataBufferLen)
{
    U32 ret_code = I2C_OK;
    U8 write_data[I2C_FIFO_SIZE];
    int transfer_len = cmdBufferLen + dataBufferLen;
    int i=0, cmdIndex=0, dataIndex=0;


	U8 len; 
	struct mt_i2c_t i2c;
	
	i2c.id = I2C_CH;
	i2c.addr = (LT8911_I2C_ADDR>>1);
	i2c.mode = ST_MODE;
	i2c.speed = 100;	
	write_data[0] = *cmdBuffer;
	write_data[1] = *dataBuffer;
	len  = 2; 
	
	ret_code = i2c_write(&i2c, write_data, len);
    return ret_code;
}

u32 lt8911_reg_i2c_read(u16 addr)
{
    u8 cmd[2];
	cmd[0] = (addr & 0xff00)>>8; ;
	cmd[1] = addr & 0xff;
    int cmd_len = 2;

    u8 data[4];
	data[0] = cmd[0];
	data[1] = cmd[1];
	data[2] = 0xFF;
	data[3] = 0xFF;
    int data_len = 4;
	u32 result_tmp;
    result_tmp = _lt8911_i2c_read(LT8911_I2C_ADDR, &cmd, cmd_len, &data, data_len);
	int read_data = data[0] | (data[1]<<8) | (data[1]<<16) | (data[1]<<24);		
    return read_data;
}

int lt8911_reg_i2c_write(u8 addr, u8 value)
{
    u8 cmd = addr;
    int cmd_len = 1;
    u8 data = value;
    int data_len = 1;	
    u32 result_tmp;

    result_tmp = _lt8911_i2c_write(LT8911_I2C_ADDR, &cmd, cmd_len, &data, data_len);
		return result_tmp;
}

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////

 void init_lt8911_registers(void)
{
	//LT8911_REG_WRITE(0xff,0x81);//register bank
	printk("KERNEL %s enter  \r\n", __func__);

    LT8911_REG_WRITE(0x013C,0x00030005,6);
    LT8911_REG_WRITE(0x0114,0x00000003,6);
    LT8911_REG_WRITE(0x0164,0x00000003,6);
    LT8911_REG_WRITE(0x0168,0x00000003,6);
    LT8911_REG_WRITE(0x016C,0x00000003,6);
    LT8911_REG_WRITE(0x0170,0x00000003,6);
    LT8911_REG_WRITE(0x0134,0x0000001F,6);
    LT8911_REG_WRITE(0x0210,0x0000001F,6);
    LT8911_REG_WRITE(0x0104,0x00000001,6);
    LT8911_REG_WRITE(0x0204,0x00000001,6);
    LT8911_REG_WRITE(0x0450,0x04700100,6);
    LT8911_REG_WRITE(0x0454,0x000F000F,6);
    LT8911_REG_WRITE(0x0458,0x00140500,6);
    LT8911_REG_WRITE(0x045C,0x00040004,6);
    LT8911_REG_WRITE(0x0460,0x00040320,6);
    LT8911_REG_WRITE(0x0464,0x00000001,6);
    LT8911_REG_WRITE(0x04A0,0x00448006,6);
    MDELAY(10);
    LT8911_REG_WRITE(0x04A0,0x00048006,6);
    LT8911_REG_WRITE(0x0504,0x00000004,6);

    LT8911_REG_WRITE(0x0480,0x03020100,6);
    LT8911_REG_WRITE(0x0484,0x08050704,6);
    LT8911_REG_WRITE(0x0488,0x0F0E0A09,6);
    LT8911_REG_WRITE(0x048C,0x100D0C0B,6);
    LT8911_REG_WRITE(0x0490,0x12111716,6);
    LT8911_REG_WRITE(0x0494,0x1B151413,6);
    LT8911_REG_WRITE(0x0498,0x061A1918,6);

    LT8911_REG_WRITE(0x049C,0x00000031,6);


	printk("KERNEL %s exit  \r\n", __func__);

}

static void lcm_init(void)
{   

	lcm_set_gpio_output(LCM_POWER_EN, GPIO_OUT_ONE);
	MDELAY(20);
	lcm_set_gpio_output(EDP_POWER_EN, GPIO_OUT_ONE);
	MDELAY(1);

	lcm_set_gpio_output(VLED_EN, GPIO_OUT_ONE);
	MDELAY(1);

	lcm_set_gpio_output(EDP_STANDBY, 1); 
	MDELAY(20);

	lcm_set_gpio_output(EDP_RESET, 1); 
	MDELAY(50);
	lcm_set_gpio_output(EDP_RESET, 0); 
	MDELAY(50);
	lcm_set_gpio_output(EDP_RESET, 1); 
	MDELAY(50);

	/* DSI clock enter High Speed mode */
	DSI_clk_HS_mode(DISP_MODULE_DSI0, NULL, 1);

	MDELAY(50);
	init_lt8911_registers();

	printk("KERNEL %s  enter", __func__);
}

static void lcm_suspend(void)
{	
#ifdef BUILD_LK
	printf("LK %s enter", __func__);
#else
	printk("KERNEL %s enter", __func__);
#endif

	lcm_set_gpio_output(VLED_EN, GPIO_OUT_ZERO);
	MDELAY(10);
	lcm_set_gpio_output(LCM_POWER_EN, GPIO_OUT_ZERO);
	MDELAY(10);
	lcm_set_gpio_output(EDP_STANDBY, 0);
	MDELAY(10);
	lcm_set_gpio_output(EDP_RESET, 0);
	MDELAY(10);
	lcm_set_gpio_output(EDP_POWER_EN, GPIO_OUT_ZERO);
	MDELAY(10);

#ifdef BUILD_LK
	printf("LK %s exit", __func__);
#else
	printk("KERNEL %s exit", __func__);
#endif

	
}


static void lcm_resume(void)
{
#ifdef BUILD_LK
	printf("LK %s enter", __func__);
#else
	printk("KERNEL %s enter\n", __func__);
#endif

	lcm_init();	

#ifdef BUILD_LK
	printf("LK %s exit", __func__);
#else
	printk("KERNEL %s exit\n", __func__);
#endif
 
}

struct LCM_DRIVER uf281_tc358775_ltl106hl01_lcm_drv = 
{
   .name			= "uf281_tc358775_ltl106hl01",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,

};
