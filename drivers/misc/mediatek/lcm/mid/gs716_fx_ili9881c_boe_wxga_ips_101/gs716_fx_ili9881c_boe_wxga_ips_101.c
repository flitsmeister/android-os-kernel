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
data_array[0] = 0x00043902;
data_array[1] = 0x038198FF;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);
//GIP_1
data_array[0] = 0x00011500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00021500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x73031500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);     
data_array[0] = 0x00041500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x00051500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x08061500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x00071500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00081500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x00091500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x010A1500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x010B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x000C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x010D1500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x010E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x000F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00101500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00111500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00121500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00131500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00141500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00151500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00161500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00171500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00181500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00191500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x001A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x001B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x001C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x001D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x401E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0xC01F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x06201500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x01211500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x06221500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x01231500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x88241500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x88251500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00261500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00271500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x3B281500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x03291500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x002A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x002B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x002C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x002D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x002E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x002F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00301500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00311500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00321500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00331500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00341500;  // GPWR1/2 non overlap time 2.62us     
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);       
data_array[0] = 0x00351500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00361500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00371500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00381500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00391500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x003A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x003B1500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x003C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x003D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x003E1500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x003F1500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x00401500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x00411500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00421500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x00431500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x00441500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1); 
           
//G  IP_        
data_array[0] = 0x01501500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x23511500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x45521500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x67531500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x89541500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0xAB551500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x01561500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x23571500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x45581500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x67591500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x895A1500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0xAB5B1500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0xCD5C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0xEF5D1500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);     
            
//G  IP_     
data_array[0] = 0x005E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x015F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x01601500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x06611500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x06621500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x07631500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x07641500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00651500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x00661500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x02671500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x02681500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x05691500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x056A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x026B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0D6C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0D6D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0C6E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0C6F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0F701500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0F711500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0E721500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0E731500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x02741500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x01751500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x01761500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x06771500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x06781500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x07791500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x077A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x007B1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x007C1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x027D1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x027E1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x057F1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x05801500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x02811500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0D821500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0D831500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0C841500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0C851500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0F861500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0F871500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0E881500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x0E891500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x028A1500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1); 

//Page4 command;    
data_array[0] = 0x00043902;
data_array[1] = 0x048198FF;          
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);

data_array[0] = 0xC03B1500;     // ILI4003D sel
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1); 
data_array[0] = 0x156C1500;        //Set VCORE voltage =1.5V
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x106E1500;        //di_pwr_reg=0 for power mode 2A //VGH clamp 18V  
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x336F1500;    //45 //pumping ratio VGH=5x VGL=-3x  
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);                    
data_array[0] = 0x1B8D1500;        //VGL clamp -10V  
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0xBA871500;        //ESD 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);  
data_array[0] = 0x243A1500;        //POWER SAVING
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);                      
data_array[0] = 0x76261500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0xD1B21500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            

 // Pae 1 command             
data_array[0] = 0x00043902;
data_array[1] = 0x018198FF;   
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);      
data_array[0] = 0x0A221500;        //BGR, SS   
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);         
data_array[0] = 0x00311500;        //Zigzag type3 inversion 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x53401500;        // ILI4003D sel 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x66431500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);                  
data_array[0] = 0x33531500;///////////////////////////
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);                    
data_array[0] = 0x87501500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);                    
data_array[0] = 0x82511500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);                     
data_array[0] = 0x15601500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);  
data_array[0] = 0x01611500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1); 
data_array[0] = 0x0C621500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);  
data_array[0] = 0x00631500; 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
   
//   Gama P    
data_array[0] = 0x00A01500;    
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1); 
data_array[0] = 0x13A11500;        //VP251   
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);       
data_array[0] = 0x23A21500;        //VP247
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);  
data_array[0] = 0x14A31500;        //VP243   
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);       
data_array[0] = 0x16A41500;        //VP239
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x29A51500;        //VP231
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x1EA61500;        //VP219
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x1DA71500;        //VP203
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x86A81500;        //VP175
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x1EA91500;        //VP144
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x29AA1500;        //VP111
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x74AB1500;        //VP80 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x19AC1500;        //VP52
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x17AD1500;        //VP36
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x4BAE1500;        //VP24
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x20AF1500;        //VP16
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x26B01500;        //VP12 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x4CB11500;        //VP8
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x5DB21500;        //VP4
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x3FB31500;        //VP0
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
             
//   Gama N   
data_array[0] = 0x00C01500;        //VN255 GAMMA N 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x13C11500;        //VN251
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x23C21500;        //VN247
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x14C31500;        //VN243
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x16C41500;        //VN239
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x29C51500;        //VN231
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x1EC61500;        //VN219 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);         
data_array[0] = 0x1DC71500;        //VN203
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x86C81500;        //VN175
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x1EC91500;        //VN144
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x29CA1500;        //VN111 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);         
data_array[0] = 0x74CB1500;        //VN80 
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);          
data_array[0] = 0x19CC1500;        //VN52
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x17CD1500;        //VN36
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x4BCE1500;        //VN24
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x20CF1500;        //VN16
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x26D01500;        //VN12
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);           
data_array[0] = 0x4CD11500;        //VN8
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x5DD21500;        //VN4
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1);            
data_array[0] = 0x3FD31500;        //VN0
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1); 
             
// Pag 0 command           
data_array[0] = 0x00043902;
data_array[1] = 0x008198FF;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);

data_array[0] = 0x00351500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(1); 

	
	data_array[0] = 0x00110500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(120);

data_array[0] = 0x00290500;
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


	params->dsi.vertical_sync_active 			    = 6;
    params->dsi.vertical_backporch				    = 15;
    params->dsi.vertical_frontporch 				= 16;
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active			    = 8;
    params->dsi.horizontal_backporch				= 48;
    params->dsi.horizontal_frontporch				= 52;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 220;
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
	printk("[Kernel/LCM-kzh]: gs716 enter lcm_suspend   !\n");
}
  
static void lcm_resume(void)
{
	lcm_init_lcm();
	//printk("resume_kzh:%s:%d GPIO_LCD_PWR_EN3.3V:%d GPIO_LCM_BIAS_EN:%d GPIO_LCD_RST_EN:%d\n",__func__,__LINE__,gpio_get_value(GPIO_LCD_PWR_EN),gpio_get_value(GPIO_LCM_BIAS_EN),gpio_get_value(GPIO_LCD_RST_EN));
	printk("[Kernel/LCM-kzh]: gs716 enter lcm_resume   !\n");
}

struct LCM_DRIVER gs716_fx_ili9881c_boe_wxga_ips_101_lcm_drv = {
	.name = "gs716_fx_ili9881c_boe_wxga_ips_101",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
