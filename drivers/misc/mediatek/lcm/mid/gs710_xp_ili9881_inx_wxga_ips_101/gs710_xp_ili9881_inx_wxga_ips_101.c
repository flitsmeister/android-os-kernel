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

extern unsigned int GPIO_LCM_PWR;
extern unsigned int GPIO_LCM_RST;
extern unsigned int GPIO_LCM_VDD;
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
//extern unsigned int GPIO_LCM_PWR_EN;
//extern unsigned int GPIO_LCM_RST;
//extern unsigned int GPIO_LCM_BL_EN;

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


void init_lcm_registers(void)
{
unsigned int data_array[16];
data_array[0]=0x00043902;
data_array[1]=0x038198FF;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000001;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000002;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00005303;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00005304;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001305;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000406;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000207;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000208;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000009;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000000A;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000000B;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000000C;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000000D;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000000E;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000000F;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000010;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000011;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000012;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000013;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000014;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000015;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000016;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000017;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000018;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000019;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000001A;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000001B;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000001C;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000001D;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000C01E;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000001F;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000220;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000921;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000022;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000023;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000024;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000025;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000026;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000027;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00005528;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000329;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000002A;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000002B;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000002C;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000002D;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000002E;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000002F;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000030;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000031;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000032;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000033;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000034;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000035;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000036;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000037;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00003C38;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000039;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000003A;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000003B;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000003C;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000003D;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000003E;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000003F;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000040;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000041;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000042;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000043;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000044;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000045;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000150;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00002351;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00004552;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00006753;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00008954;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000AB55;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000156;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00002357;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00004558;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00006759;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000895A;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000AB5B;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000CD5C;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000EF5D;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000015E;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000A5F;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000260;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000261;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000862;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001563;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001464;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000265;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001166;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001067;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000268;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000F69;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000E6A;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000026B;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000D6C;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000C6D;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000066E;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000026F;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000270;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000271;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000272;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000273;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000274;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000A75;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000276;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000277;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000678;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001579;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000147A;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000027B;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000107C;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000117D;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000027E;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000C7F;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000D80;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000281;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000E82;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000F83;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000884;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000285;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000286;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000287;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000288;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000289;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000028A;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00043902;
data_array[1]=0x048198FF;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000000;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000156C;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000306E;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000556F;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000243A;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001F8D;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000BA87;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00007626;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x0000D1B2;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000007B5;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001F35;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000B88;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00003021;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00043902;
data_array[1]=0x018198FF;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000A22;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000931;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00003340;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00003753;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00008855;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00009550;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00009551;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00003060;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000FA0;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000017A1;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000022A2;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000019A3;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000015A4;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000028A5;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001CA6;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001CA7;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000078A8;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001CA9;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000028AA;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000069AB;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001AAC;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000019AD;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00004BAE;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000022AF;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00002AB0;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00004BB1;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00006BB2;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00003FB3;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000001C0;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000017C1;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000022C2;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000019C3;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000015C4;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000028C5;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001CC6;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001DC7;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000078C8;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001CC9;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000028CA;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000069CB;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00001ACC;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000019CD;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00004BCE;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x000022CF;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00002AD0;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00004BD1;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00006BD2;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00003FD3;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00043902;
data_array[1]=0x008198FF;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000035;
dsi_set_cmdq(data_array,2,1);

data_array[0]=0x00023902;
data_array[1]=0x00000011;
dsi_set_cmdq(data_array,2,1);

MDELAY(130);
data_array[0]=0x00023902;
data_array[1]=0x00000029;
dsi_set_cmdq(data_array,2,1);

MDELAY(30);
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
    params->dsi.LANE_NUM                = LCM_THREE_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 

    params->dsi.vertical_sync_active = 6;
	params->dsi.vertical_backporch =  22;
	params->dsi.vertical_frontporch = 16;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 20;
	params->dsi.horizontal_backporch =	70; 
	params->dsi.horizontal_frontporch = 70;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.PLL_CLOCK=310; 

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
	avdd_enable(1);
	
	lcd_power_en(1);
	MDELAY(100);
	
	//lcd_reset(1);
	MDELAY(5);
	//lcd_reset(0);	
	MDELAY(10);
	//lcd_reset(1);
	MDELAY(120);//Must > 5ms
	
	
	init_lcm_registers();

	MDELAY(180);
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];
	
	data_array[0]=0x00280500; // Display Off
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(30); 

    data_array[0] = 0x00100500; // Sleep In
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(130); 
	
	lcd_power_en(0);
    MDELAY(150);
    lcd_reset(0);
    MDELAY(100);
    avdd_enable(0);
    MDELAY(20);

}


static void lcm_resume(void)
{
  lcm_init();
}


struct LCM_DRIVER gs710_xp_ili9881_inx_wxga_ips_101_lcm_drv = 
{
    .name			= "gs710_xp_ili9881_inx_wxga_ips_101",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.compare_id    = lcm_compare_id,
};

