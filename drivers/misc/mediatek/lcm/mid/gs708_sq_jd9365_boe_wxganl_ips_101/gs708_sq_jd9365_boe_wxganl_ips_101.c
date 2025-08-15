/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

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

#define GPIO_LCD_RST_EN        GPIO_LCM_RST
#define GPIO_LCD_PWR_EN         GPIO_LCM_PWR
#define GPIO_LCM_BIAS_EN            GPIO_LCM_VDD

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	gpio_direction_output(GPIO, output);
	gpio_set_value(GPIO, output);
}

/* ------------------------------------------------------------------- */
/* Local Constants */
/* ------------------------------------------------------------------- */

#define FRAME_WIDTH  (800)
#define FRAME_HEIGHT (1280)

/* ------------------------------------------------------------------- */
/* Local Variables */
/* ------------------------------------------------------------------- */

static struct LCM_UTIL_FUNCS lcm_util = { 0 };

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

/* ------------------------------------------------------------------- */
/* Local Functions */
/* ------------------------------------------------------------------- */
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
	lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define   LCM_DSI_CMD_MODE	0

static void init_lcm_registers(void)
{
    	unsigned int data_array[16];

//Page0
    data_array[0] = 0x00E01500;
    dsi_set_cmdq(data_array, 1, 1);
//--- PASSWORD  ----//
    data_array[0] = 0x93E11500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x65E21500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xF8E31500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x03801500;
    dsi_set_cmdq(data_array, 1, 1);


//--- Page1  ----//
    data_array[0] = 0x01E01500;
    dsi_set_cmdq(data_array, 1, 1);
//Set VCOM
    data_array[0] = 0x00001500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x6F011500;
    dsi_set_cmdq(data_array, 1, 1);
//Set VCOM_Reverse
    data_array[0] = 0x00031500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x6A041500;
    dsi_set_cmdq(data_array, 1, 1);
//Set Gamma Power, VGMP,VGMN,VGSP,VGSN
    data_array[0] = 0x00171500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xAF181500;//4.3V
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01191500;//0.3V
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001A1500;//
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xAF1B1500;//4.3V
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x011C1500;//0.3V
    dsi_set_cmdq(data_array, 1, 1);
               
//Set Gate Power
    data_array[0] = 0x3E1F1500;     //VGH_R  = 15V                       
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x28201500;     //VGL_R  = -12V                      
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x28211500;     //VGL_R2 = -12V                      
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x7E221500;     //PA[6]=1, PA[5]=1, PA[4]=1, PA[0]=0 
    dsi_set_cmdq(data_array, 1, 1);
//SETPANEL
    data_array[0] = 0x26351500;	//ASP=0110
    dsi_set_cmdq(data_array, 1, 1);

//SETPANEL
    data_array[0] = 0x09371500;	//SS=1,BGR=1
    dsi_set_cmdq(data_array, 1, 1);
//SET RGBCYC
    data_array[0] = 0x04381500;	//JDT=100 column inversion
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00391500;	//RGB_N_EQ1, 0x12
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x013A1500;	//RGB_N_EQ2, 0x18
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x7C3C1500;	//SET EQ3 for TE_H
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xFF3D1500;	//SET CHGEN_ON, 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xFF3E1500;	//SET CHGEN_OFF, 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x7F3F1500;	//SET CHGEN_OFF2,
    dsi_set_cmdq(data_array, 1, 1);

//Set TCON
    data_array[0] = 0x06401500;	//RSO=800 RGB
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xA0411500;	//LN=640->1280 line
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x81421500;	//SLT
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x08431500;	//VFP=8
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0B441500;	//VBP=12
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x28451500;  //HBP=40
    dsi_set_cmdq(data_array, 1, 1);
//--- power voltage  ----//
    data_array[0] = 0x01551500;	//DCDCM=0001, JD PWR_IC
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x69571500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0A591500;	//VCL = -2.9V
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x285A1500;	//VGH = 15V
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x145B1500;	//VGL = -11V
    dsi_set_cmdq(data_array, 1, 1);

//--- Gamma  ----//
    data_array[0] = 0x7C5D1500;              
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x655E1500;      
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x555F1500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x47601500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x43611500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x32621500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x34631500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1C641500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x33651500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x31661500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x30671500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x4E681500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x3C691500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x446A1500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x356B1500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x316C1500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x236D1500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x116E1500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006F1500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x7C701500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x65711500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x55721500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x47731500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x43741500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x32751500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x34761500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1C771500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x33781500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x31791500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x307A1500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x4E7B1500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x3C7C1500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x447D1500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x357E1500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x317F1500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x23801500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x11811500;    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00821500;    
    dsi_set_cmdq(data_array, 1, 1);

                                 
//Page2, for GIP                                      
    data_array[0] = 0x02E01500;                                
    dsi_set_cmdq(data_array, 1, 1);
//GIP_L Pin mapping                                   
    data_array[0] = 0x1E001500;//1  VDS                        
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E011500;//2  VDS                        
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x41021500;//3  STV2                       
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x41031500;//4  STV2                       
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F041500;//5                         
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F051500;//6                       
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F061500;//7  VSD                        
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F071500;//8  VSD                        
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F081500;//9  GCL                        
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F091500;//10                            
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E0A1500;//11 GCH                        
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E0B1500;//12 GCH                        
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F0C1500;//13                            
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x470D1500;//14 CLK8                       
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x470E1500;//15 CLK8                       
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x450F1500;//16 CLK6                       
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x45101500;//17 CLK6                       
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x4B111500;//18 CLK4                       
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x4B121500;//19 CLK4                       
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x49131500;//20 CLK2                       
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x49141500;//21 CLK2                       
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F151500;//22 VGL                        
    dsi_set_cmdq(data_array, 1, 1);
                                                      
                                                      
//GIP_R Pin mapping                                   
    data_array[0] = 0x1E161500;//1  VDS                 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E171500;//2  VDS                
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x40181500;//3  STV1               
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x40191500;//4  STV1               
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F1A1500;//5                
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F1B1500;//6             
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F1C1500;//7  VSD                
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F1D1500;//8  VSD                
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F1E1500;//9  GCL                
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F1F1500;//10                    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E201500;//11 GCH                
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E211500;//12 GCH                
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1f221500;//13                    
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x46231500;//14 CLK7               
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x46241500;//15 CLK7               
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x44251500;//16 CLK5               
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x44261500;//17 CLK5               
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x4A271500;//18 CLK3               
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x4A281500;//19 CLK3               
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x48291500;//20 CLK1               
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x482A1500;//21 CLK1               
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F2B1500;//22 VGL                                 
    dsi_set_cmdq(data_array, 1, 1);

//GIP_L_GS Pin mapping
    data_array[0] = 0x1F2C1500;//1  VDS 		0x1E
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F2D1500;//2  VDS          0x1E
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x402E1500;//3  STV2         0x41
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x402F1500;//4  STV2         0x41
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F301500;//5 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F311500;//6  
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E321500;//7  VSD          0x1F
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E331500;//8  VSD          0x1F
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F341500;//9  GCL          0x1F
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F351500;//10              0x1F
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E361500;//11 GCH          0x1E
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E371500;//12 GCH          0x1E
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F381500;//13              0x1F
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x48391500;//14 CLK8         0x47
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x483A1500;//15 CLK8         0x47
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x4A3B1500;//16 CLK6         0x45
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x4A3C1500;//17 CLK6         0x45
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x443D1500;//18 CLK4         0x4B
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x443E1500;//19 CLK4         0x4B
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x463F1500;//20 CLK2         0x49
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x46401500;//21 CLK2         0x49
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F411500;//22 VGL          0x1F
    dsi_set_cmdq(data_array, 1, 1);

//GIP_R_GS Pin mapping
    data_array[0] = 0x1F421500;//1  VDS 		0x1E
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F431500;//2  VDS          0x1E
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x41441500;//3  STV1         0x40
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x41451500;//4  STV1         0x40
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F461500;//5  
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F471500;//6  
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E481500;//7  VSD          0x1F
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E491500;//8  VSD          0x1F
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E4A1500;//9  GCL          0x1F
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F4B1500;//10              0x1f
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E4C1500;//11 GCH          0x1E
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1E4D1500;//12 GCH          0x1E
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F4E1500;//13              0x1f
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x494F1500;//14 CLK7         0x46
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x49501500;//15 CLK7         0x46
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x4B511500;//16 CLK5         0x44
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x4B521500;//17 CLK5         0x44
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x45531500;//18 CLK3         0x4A
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x45541500;//19 CLK3         0x4A
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x47551500;//20 CLK1         0x48
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x47561500;//21 CLK1         0x48
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1F571500;//22 VGL          0x1f
    dsi_set_cmdq(data_array, 1, 1);

//GIP Timing  
    data_array[0] = 0x40581500; 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x305B1500; //STV_NUM,STV_S0
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x035C1500; //STV_S0
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x305D1500; //STV_W / S1
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x015E1500; //STV_S2
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x025F1500; //STV_S3
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x14631500; //SETV_ON  
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x6A641500; //SETV_OFF 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x73671500; 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05681500; 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x14691500; 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x6A6A1500; 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x086B1500; //Dummy clk
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x006C1500; 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006D1500; 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006E1500; 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x886F1500; 
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0xDD771500; 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0E791500;//0x0C 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x037A1500;//0x04
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x147D1500; 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x6A7E1500; 
    dsi_set_cmdq(data_array, 1, 1);


//Page4
    data_array[0] = 0x04E01500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x11091500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x480E1500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x2B2B1500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x032D1500;//defult 0x01
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x442E1500;
    dsi_set_cmdq(data_array, 1, 1);

//Page0
    data_array[0] = 0x00E01500;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x02E61500;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0CE71500;
    dsi_set_cmdq(data_array, 1, 1);


//SLP OUT

    data_array[0] = 0x00110500;  	// SLPOUT
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);


//DISP ON

    data_array[0] = 0x00290500;  	// DSPON
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(5);

}
// ---------------------------------------------------------------------------
// LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    params->dsi.mode    = SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;//LCM_THREE_LANE;

    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.word_count=800*3; 


	params->dsi.vertical_sync_active 			    = 4;
    params->dsi.vertical_backporch				    = 8;
    params->dsi.vertical_frontporch 				= 8;
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active			    = 20;
    params->dsi.horizontal_backporch				= 20;
    params->dsi.horizontal_frontporch				= 40;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 200;
   // params->dsi.cont_clock=0;
}

static void lcd_reset(unsigned char enabled)
{
	lcm_set_gpio_output(GPIO_LCD_RST_EN, enabled);
}
	
static void lcd_power_en(unsigned char enabled)
{
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, enabled);
}
static void lcd_bias_en(unsigned char enabled)
{
	lcm_set_gpio_output(GPIO_LCM_BIAS_EN, enabled);
}



static void lcm_init_lcm(void)
{
    lcd_power_en(1);//LCD 3.3V
	MDELAY(100);
	
	lcd_reset(1);
	MDELAY(20);
	lcd_reset(0);//LCD rst
	MDELAY(20);
	lcd_reset(1);
	MDELAY(150);//Must > 5ms

	lcd_bias_en(1);//bias en 偏压使能
	init_lcm_registers();
	MDELAY(50);
//	lcm_set_gpio_output(GPIO_LCD_BL_EN,1);

}

static void lcm_suspend(void)
{
	lcd_bias_en(0);//bias en 偏压使能
	//unsigned int data_array[16];
	lcd_reset(0);
	MDELAY(100);
   	lcd_power_en(0);
	MDELAY(150);


/* 	data_array[0] = 0x00280500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(40);
	data_array[0] = 0x00100500;
	dsi_set_cmdq(data_array, 1, 1); */
	     /* enable AVDD & AVEE */

	//printk("suspend_kzh:%s:%d GPIO_LCD_PWR_EN3.3V:%d GPIO_LCM_BIAS_EN:%d GPIO_LCD_RST_EN:%d\n",__func__,__LINE__,gpio_get_value(GPIO_LCD_PWR_EN),gpio_get_value(GPIO_LCM_BIAS_EN),gpio_get_value(GPIO_LCD_RST_EN));
	//printk("[Kernel/LCM-kzh]: gs716 enter lcm_suspend   !\n");
}
  
static void lcm_resume(void)
{
	lcm_init_lcm();
	//printk("resume_kzh:%s:%d GPIO_LCD_PWR_EN3.3V:%d GPIO_LCM_BIAS_EN:%d GPIO_LCD_RST_EN:%d\n",__func__,__LINE__,gpio_get_value(GPIO_LCD_PWR_EN),gpio_get_value(GPIO_LCM_BIAS_EN),gpio_get_value(GPIO_LCD_RST_EN));
	//printk("[Kernel/LCM-kzh]: gs716 enter lcm_resume   !\n");
}

struct LCM_DRIVER gs708_sq_jd9365_boe_wxganl_ips_101_lcm_drv = {
	.name = "gs708_sq_jd9365_boe_wxganl_ips_101",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
