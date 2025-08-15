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

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(800)
#define FRAME_HEIGHT 										(1280)


#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static struct LCM_UTIL_FUNCS lcm_util = {0};

//#define GPIO_LCM_PWREN                                      (GPIO122 |0x80000000)
//#define GPIO_LCM_EN                                         (GPIO26 | 0x80000000)
//#define GPIO_LCM_RST                                        (GPIO83 | 0x80000000)

extern unsigned int GPIO_LCM_PWR;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_VDD;


#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

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



//extern void DSI_clk_HS_mode(unsigned char enter);
static void init_lcm_registers(void)
{
    	unsigned int data_array[16];
data_array[0] = 0x00E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x93E11500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x65E21500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0xF8E31500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x03801500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x01E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x00001500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x66011500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x010E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x00171500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0xAF181500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x01191500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x001A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0xAF1B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x011C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x3E1F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
     
data_array[0] = 0x28201500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
     
data_array[0] = 0x28211500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
     
data_array[0] = 0x7E221500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
     
data_array[0] = 0x26351500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x09371500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x04381500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x00391500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x013A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x7C3C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0xFF3D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0xFF3E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x7F3F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x06401500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0xA0411500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
	
data_array[0] = 0x81421500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
	
data_array[0] = 0x08431500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
	
data_array[0] = 0x0B441500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
	
data_array[0] = 0x28451500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
  
data_array[0] = 0x01551500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x01561500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x69571500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x0A581500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x0A591500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x295A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x155B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x7C5D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x655E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
      
data_array[0] = 0x555F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x47601500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x43611500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x32621500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x34631500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x1C641500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x33651500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x31661500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x30671500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x4E681500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x3C691500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x446A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x356B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x316C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x236D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x116E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x006F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x7C701500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x65711500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x55721500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x47731500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x43741500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x32751500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x34761500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x1C771500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x33781500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x31791500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x307A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x4E7B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x3C7C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x447D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x357E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x317F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x23801500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x11811500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x00821500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
    
data_array[0] = 0x02E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                                
data_array[0] = 0x1E001500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x1E011500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x41021500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x41031500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x43041500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x43051500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x1F061500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x1F071500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x35081500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x1F091500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x150A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x150B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x1F0C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x470D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x470E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x450F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x45101500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x4B111500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x4B121500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x49131500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x49141500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x1F151500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                       
data_array[0] = 0x1E161500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
               
data_array[0] = 0x1E171500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x40181500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x40191500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x421A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x421B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x1F1C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x1F1D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x351E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x1F1F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x15201500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x15211500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x1f221500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x46231500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x46241500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x44251500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x44261500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x4A271500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x4A281500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x48291500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x482A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
              
data_array[0] = 0x1f2B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
                               
data_array[0] = 0x40581500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x305B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x035C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x305D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x015E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x025F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x14631500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x6A641500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x73671500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x05681500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x14691500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x6A6A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x086B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x006C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x006D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x006E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x886F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0xDD771500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x0E791500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x037A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x147D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x6A7E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);
 
data_array[0] = 0x04E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x2B2B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x442E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x11091500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x032D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x480E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x00E01500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x02E61500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x02E71500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);

data_array[0] = 0x00110500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(120);

data_array[0] = 0x00290500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(20); 
}

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	if (GPIO == 0xFFFFFFFF) {		
		return;
	}
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

    params->dsi.mode    = BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 


    params->dsi.vertical_sync_active                            = 4; //2; //4;
    params->dsi.vertical_backporch                              = 8; //10; //16;
    params->dsi.vertical_frontporch                             = 24;//5; 
    params->dsi.vertical_active_line                            = FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active                          = 18; // 10; //5;//6;
    params->dsi.horizontal_backporch                            = 18; //60; //60; //80;
    params->dsi.horizontal_frontporch                           = 18; //60; 
    params->dsi.horizontal_active_pixel                         = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 180;


}


static void lcd_power_en(unsigned char enabled)
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

static void avdd_enable(unsigned char enabled)
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

static void lcm_init(void)
{
    lcd_power_en(1);
	MDELAY(10);
	avdd_enable(1);
	MDELAY(10);
	
	lcd_reset(1);
	MDELAY(5);
	lcd_reset(0);	
	MDELAY(10);
	lcd_reset(1);
	MDELAY(120);//Must > 5ms
	
	init_lcm_registers();
	
	MDELAY(20);
}


static void lcm_suspend(void)
{
	lcd_power_en(0);
    MDELAY(150);
    lcd_reset(0);
    MDELAY(100);
    avdd_enable(0);
   // DSI_clk_HS_mode(0);
    MDELAY(20);

}


static void lcm_resume(void)
{
  lcm_init();
}


struct LCM_DRIVER gs716e_xdf_jd9365_boe_wxga_ips_101_lcm_drv = 
{
    .name			= "gs716e_xdf_jd9365_boe_wxga_ips_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

