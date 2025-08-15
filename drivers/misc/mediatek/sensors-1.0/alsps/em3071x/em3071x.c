/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/* drivers/hwmon/mt6516/amit/em3071x.c - em3071x ALS/PS driver
 *
 * Author:
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/sysfs.h>
#include <linux/device.h>

// #include <linux/wakelock.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <cust_alsps.h>
#include <alsps.h>
#include "em3071x.h"

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
//#define POWER_NONE_MACRO MT65XX_POWER_NONE


/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/

#define EM3071X_DEV_NAME     "em3071x"
#define EM3071X_DEBUG 0
/*----------------------------------------------------------------------------*/
#ifdef EM3071X_DEBUG
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               printk(APS_TAG"%s %d \n", __FUNCTION__ , __LINE__)
#define APS_ERR(fmt, args...)    printk(APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    printk(APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(APS_TAG fmt, ##args)
#else
#define APS_ERR(fmt, args...)
#define APS_LOG(fmt, args...)
#define APS_DBG(fmt, args...)
#endif
/******************************************************************************
 * extern functions
*******************************************************************************/
#define PS_ENABLE 0xB8  //0XA0 25ma ; A8 50ma ; B0 100ma ; B8 200ma //wangzhongping changed a8->a0
// struct wake_lock ps_suspend_lock;     //pj add for debug
static DEFINE_MUTEX(EM3071x_i2c_mutex);

#define EM3071X_PS_CALI

#ifdef EM3071X_PS_CALI
static int data_cali_buff;
struct PS_CALI_DATA_STRUCT
{
    int close;
    int far_away;
    int valid;
} ;
static struct PS_CALI_DATA_STRUCT ps_cali={0,0,0};
static struct PS_CALI_DATA_STRUCT ps_store_cali={0xe0,0xd0,0};
static int em3071x_read_data_for_cali(struct i2c_client *client, struct PS_CALI_DATA_STRUCT *ps_data_cali);
static void em3071x_WriteCalibration(struct i2c_client *client, struct PS_CALI_DATA_STRUCT *ps_data_cali);
#endif
/*----------------------------------------------------------------------------*/
static struct i2c_client *em3071x_i2c_client = NULL;
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id em3071x_i2c_id[] = {{EM3071X_DEV_NAME,0},{}};
/*----------------------------------------------------------------------------*/

static int em3071x_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int em3071x_i2c_remove(struct i2c_client *client);
static int em3071x_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
static int em3071x_i2c_suspend(struct device *dev);
static int em3071x_i2c_resume(struct device *dev);
int em3071x_setup_eint(struct i2c_client *client);
int em3071x_read_ps(struct i2c_client *client, u16 *data);
int em3071x_read_als(struct i2c_client *client, u16 *data);

struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;
//struct platform_device *alspsPltFmDev;

 int for_far_distance_flg=1;       //1<==>for far distance 0<====>for call modem
static int    em3071x_init_flag = -1;    // 0<==>OK -1 <==> fail
static int intr_flag_value = 0;
static int  em3071x_local_init(void);
static int  em3071x_local_uninit(void);

//static int em3071_read(struct i2c_client *client,u8 reg);
//static int em3071_write(struct i2c_client *client,u8 reg,u8 data);

//static int dev_attr_config(u8 reg,int config,int dev_s,int data);
static DEFINE_MUTEX(sensor_lock);
static struct alsps_init_info em3071x_init_info = {
        .name = "em3071x",
        .init = em3071x_local_init,
        .uninit = em3071x_local_uninit,
};
/*----------------------------------------------------------------------------*/
typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;
/*----------------------------------------------------------------------------*/
struct em3071x_i2c_addr {    /*define a series of i2c slave address*/
    u8  write_addr;
    u8  ps_thd;     /*PS INT threshold*/
};
/*----------------------------------------------------------------------------*/
struct em3071x_priv {
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct work_struct  eint_work;

    /*i2c address group*/
    struct em3071x_i2c_addr  addr;

    /*misc*/
    u16         als_modulus;
    atomic_t    i2c_retry;
    atomic_t    als_suspend;
    atomic_t    als_debounce;   /*debounce time after enabling als*/
    atomic_t    als_deb_on;     /*indicates if the debounce is on*/
    atomic_t    als_deb_end;    /*the jiffies representing the end of debounce*/
    atomic_t    ps_mask;        /*mask ps: always return far away*/
    atomic_t    ps_debounce;    /*debounce time after enabling ps*/
    atomic_t    ps_deb_on;      /*indicates if the debounce is on*/
    atomic_t    ps_deb_end;     /*the jiffies representing the end of debounce*/
    atomic_t    ps_suspend;
    atomic_t    trace;
    atomic_t    init_done;
    struct device_node *irq_node;
    int        irq;


    /*data*/
    u16         als;
    u16         ps;
    u8          _align;
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];
    int        ps_cali;

    atomic_t    als_cmd_val;    /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_cmd_val;     /*the cmd value can't be read, stored in ram*/
    atomic_t     ps_thd_val_high;    /*the cmd value can't be read, stored in ram */
    atomic_t     ps_thd_val_low;    /*the cmd value can't be read, stored in ram */
    atomic_t     als_thd_val_high;    /*the cmd value can't be read, stored in ram */
    atomic_t     als_thd_val_low;    /*the cmd value can't be read, stored in ram */
    atomic_t    ps_thd_val;     /*the cmd value can't be read, stored in ram*/
    ulong       enable;         /*enable mask*/
    ulong       pending_intr;   /*pending interrupt*/
    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif
};

#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
    {.compatible = "mediatek,alsps"},
    {},
};
#endif

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops em3071x_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(em3071x_i2c_suspend, em3071x_i2c_resume)
};
#endif
static struct i2c_driver em3071x_i2c_driver = {
    .probe      = em3071x_i2c_probe,
    .remove     = em3071x_i2c_remove,
    .detect     = em3071x_i2c_detect,
    .id_table   = em3071x_i2c_id,
    //.address_data = &em3071x_addr_data,
    .driver = {
        .owner          = THIS_MODULE,
        .name           = EM3071X_DEV_NAME,
#ifdef CONFIG_PM_SLEEP
        .pm   = &em3071x_pm_ops,
#endif
#ifdef CONFIG_OF
        .of_match_table = alsps_of_match,
#endif
    },
};

static int ps_enabled=0;

static struct em3071x_priv *em3071x_obj = NULL;
static struct platform_driver em3071x_alsps_driver;

//static struct alsps_hw *get_cust_alsps(void)
//{
    //return &alsps_cust;
//}

int em3071x_read_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
    int err;
    u8 beg = addr;
    struct i2c_msg msgs[2] =
    {
        {
            .addr = client->addr,
            .flags = 0,
            .len = 1,
            .buf= &beg
        },
        {
            .addr = client->addr,
            .flags = I2C_M_RD,
            .len = len,
            .buf = data,
        }
    };

    mutex_lock(&EM3071x_i2c_mutex);
    if (!client)
    {
        mutex_unlock(&EM3071x_i2c_mutex);
        return -EINVAL;
    }
    else if (len > 10)
    {
        mutex_unlock(&EM3071x_i2c_mutex);
        APS_LOG(" length %d exceeds %d\n", len, 10);
        return -EINVAL;
    }

    err = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
    mutex_unlock(&EM3071x_i2c_mutex);
    if (err != 2)
    {
        APS_LOG("i2c_transfer error: (%d %p %d) %d\n", addr, data, len, err);
        err = -EIO;
    }
    else
    {
        err = 0;/*no error*/
    }
    return err;
}

/*----------------------------------------------------------------------------*/
int em3071x_master_recv(struct i2c_client *client, u16 addr, u8 *buf ,int count)
{
//    struct em3071x_priv *obj = i2c_get_clientdata(client);
    int ret = 0, retry = 0;
//    int trc = atomic_read(&obj->trace);
//    int max_try = atomic_read(&obj->i2c_retry);

    while(retry++ < 3)
    {
        ret = em3071x_read_block(client, addr, buf, count);
        if(ret == 0)
            break;
        udelay(100);
    }

/*     if(unlikely(trc))
    {
        if((retry != 1) && (trc & STK_TRC_DEBUG))
        {
            APS_LOG("(recv) %d/%d\n", retry-1, max_try);

        }
    } */

    /* If everything went ok (i.e. 1 msg transmitted), return #bytes
    transmitted, else error code. */
    return (ret == 0) ? count : ret;
}
/*----------------------------------------------------------------------------*/
int em3071x_master_send(struct i2c_client *client, u16 addr, u8 *buf ,int count)
{
    int ret = 0, retry = 0;
//    struct em3071x_priv *obj = i2c_get_clientdata(client);
//    int trc = atomic_read(&obj->trace);
//    int max_try = atomic_read(&obj->i2c_retry);


    while(retry++ < 3)
    {
        ret = hwmsen_write_block(client, addr, buf, count);
        if (ret == 0)
            break;
        udelay(100);
    }

/*     if(unlikely(trc))
    {
        if((retry != 1) && (trc & STK_TRC_DEBUG))
        {
            APS_LOG("(send) %d/%d\n", retry-1, max_try);
        }
    } */
    /* If everything went ok (i.e. 1 msg transmitted), return #bytes
    transmitted, else error code. */
    return (ret == 0) ? count : ret;
}

static int em3071x_read(struct i2c_client *client,u8 reg)
{
    int res = 0;
    u8 buffer[2];


    res  = em3071x_master_recv(client, reg, buffer, 0x01);
    if(res < 0)
    {
        APS_DBG("error: %d\n", res );
        return -EFAULT;
    }

    return buffer[0];

    buffer[0] = reg;
    res = i2c_master_send(client, buffer, 0x1);
    if(res <= 0)
    {
        return 0;
    }
    res = i2c_master_recv(client, buffer, 0x1);
    if(res <= 0)
    {
        return 0;
    }
    return buffer[0];
}

static int em3071x_write(struct i2c_client *client,u8 reg,u8 data)
{
    int res = 0;
    u8 buffer[2];
    buffer[0] = reg;
    buffer[1] = data;
    res = i2c_master_send(client, buffer, 0x2);
    if(res <= 0)
    {
        return 0;
    }
    return 1;
}
/******************************************************************************
*********************DEVICE ATTER**********************************************
******************************************************************************/

//static struct kobject *em_kobj;


/*----------------------------------------------------------------------------*/
static ssize_t ps_show(struct device_driver *ddri, char *buf)
{
    int res;
    struct em3071x_priv *obj = em3071x_obj;
    if(!obj)
    {
        APS_ERR("em3071x_obj is null!!\n");
        return 0;
    }

    if((res = em3071x_read_ps(obj->client, &obj->ps)))
    {
        return scnprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
    }
    else
    {
        #ifndef VENDOR_EDIT
            return scnprintf(buf, PAGE_SIZE, "0x%04X\n", obj->ps);
        #else/* VENDOR_EDIT */
            return scnprintf(buf, PAGE_SIZE, "%d\n", obj->ps);
        #endif/* VENDOR_EDIT */
    }
}

static int ps_cal=190;
static int ps_cali_symbol=0;
static ssize_t ps_store(struct device_driver *ddri, const char *buf, size_t count)
{
    struct em3071x_priv *obj = em3071x_obj;
    int cal_temp = 0;
    //char buf_cal[64];

    //printk("jipeng buf_cal%d\n",ps_cal);
    if(!obj)
    {
      APS_ERR("em3071x_obj is null!!\n");
      return 0;
    }
 
    sscanf(buf, "%d", &cal_temp);
    if(cal_temp>0)
    {
        ps_cal=cal_temp;
        ps_cali_symbol=1;
    }
    //printk("jipeng buf_cal%d\n",ps_cal);
#if 0
    fd_file = filp_open("/mnt/jipeng_sd.txt", O_RDONLY , 0777);
            if (IS_ERR(fd_file))
           {
               //printk("jipeng create file error 1111111\n");
               //count_temp--;
               // return -1;
           }

  // #else

    strcpy(value_buf,buf_cal);
    //printk("jipeng value_buf%s\n",value_buf);

    set_env(name_buf,value_buf);

    ps_data_cal=1;
     //val_temp1=get_env(name_buf);
     //printk("jipeng name_buf %s\n",value_buf);

#endif
 //   fs = get_fs();
//    set_fs(KERNEL_DS);
//    file_size = vfs_llseek(fd_file, 0, SEEK_END);
 //   vfs_write(fd_file,buf,file_size,&pos);
  //  filp_close(fd_file, NULL);
//    set_fs(fs);
    return count;
}

/*----------------------------------------------------------------------------*/

static ssize_t als_show(struct device_driver *ddri, char *buf)
{
    struct em3071x_priv *obj = em3071x_obj;
    int res;

    if(!obj)
    {
        APS_ERR("em3071x_obj is null!!\n");
        return 0;
    }

    if((res = em3071x_read_als(obj->client, &obj->als)))
    {
        return scnprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
    }
    else
    {
        return scnprintf(buf, PAGE_SIZE, "%d\n", obj->als);
    }

}
static ssize_t id_show(struct device_driver *ddri, char *buf)
{
    struct em3071x_priv *obj = em3071x_obj;
    int res;

    if(!obj)
    {
        APS_ERR("em3071x_obj is null!!\n");
        return 0;
    }
    res=em3071x_read(obj->client,EM3071X_CMM_ID);

    if(res)
    {
        return snprintf(buf, PAGE_SIZE, "%x\n", res); 
    }
    else
    {
        return scnprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
    }
}

static ssize_t config_show(struct device_driver *ddri, char *buf)
{
    struct em3071x_priv *obj = em3071x_obj;
    int res;

    if(!obj)
    {
        APS_ERR("em3071x_obj is null!!\n");
        return 0;
    }
    res = em3071x_read(obj->client,EM3071X_CMM_ENABLE);
    if(res)
    {
        return snprintf(buf, PAGE_SIZE, "%x\n", res);
    }
    else
    {
        return scnprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
    }

}
static ssize_t config_store(struct device_driver *ddri, const char *buf, size_t count)
{
    struct em3071x_priv *obj = em3071x_obj;
    int config;

    if(!obj)
    {
        APS_ERR("em3071x_obj is null!!\n");
        return 0;
    }

    if(1 != sscanf(buf, "%x", &config))
    {
        APS_ERR("invalid content: \n");

    }
    else
    {
        em3071x_write(obj->client,EM3071X_CMM_ENABLE,config);
    }
    return count; 
}

static DRIVER_ATTR_RW(ps);
static DRIVER_ATTR_RO(als);
static DRIVER_ATTR_RO(id);
static DRIVER_ATTR_RW(config);

static struct driver_attribute *em3071x_attr_list[]={
    &driver_attr_ps,
    &driver_attr_als,
    &driver_attr_id,
    &driver_attr_config,

};

static int em3071x_create_attr(struct device_driver *driver) 
{
    //int ret;
    int idx, err = 0;
    int num = (int)(sizeof(em3071x_attr_list)/sizeof(em3071x_attr_list[0]));
    if (driver == NULL)
    {
        return -EINVAL;
    }
    for(idx = 0; idx < num; idx++)
    {
        if((err = driver_create_file(driver, em3071x_attr_list[idx])))
        {
            APS_ERR("driver_create_file (%s) = %d\n", em3071x_attr_list[idx]->attr.name, err);
            break;
        }
    }
    return err;
}

static int em3071x_delete_attr(struct device_driver *driver)
{
    int idx ,err = 0;
    int num = (int)(sizeof(em3071x_attr_list)/sizeof(em3071x_attr_list[0]));

    if(driver == NULL)
    {
        return -EINVAL;
    }
    

    for(idx = 0; idx < num; idx++)
    {
        driver_remove_file(driver, em3071x_attr_list[idx]);
    }
    

    return err;
}
/*----------------------------------------------------------------------------*/
#if 0
int em3071x_get_addr(struct alsps_hw *hw, struct em3071x_i2c_addr *addr)
{
    if(!hw || !addr)
    {
        return -EFAULT;
    }
    addr->write_addr= 0x48>>1;//hw->i2c_addr[0];
    return 0;
}
#endif
/*----------------------------------------------------------------------------*/
#if 0
static void em3071x_power(struct alsps_hw *hw, unsigned int on) 
{
    static unsigned int power_on = 0;

    APS_LOG("power %s\n", on ? "on" : "off");
/*
    if(hw->power_id != POWER_NONE_MACRO)
    {
        if(power_on == on)
        {
            APS_LOG("ignore power control: %d\n", on);
        }
        else if(on)
        {
            if(!hwPowerOn(hw->power_id, hw->power_vol, "EM3071X"))
            {
                APS_ERR("power on fails!!\n");
            }
        }
        else
        {
            if(!hwPowerDown(hw->power_id, "EM3071X"))
            {
                APS_ERR("power off fail!!\n");
            }
        }
    }

#ifdef GPIO_ALS_PWREN_PIN
    if(power_on == on)
    {
            APS_LOG("ignore power control: %d\n", on);
    }
    else if( on)
    {
        mt_set_gpio_mode(GPIO_ALS_PWREN_PIN, GPIO_ALS_PWREN_PIN_M_GPIO);
        mt_set_gpio_dir(GPIO_ALS_PWREN_PIN, GPIO_DIR_OUT);
        mt_set_gpio_out(GPIO_ALS_PWREN_PIN, 1);
        msleep(100);
    }
    else
    {
        mt_set_gpio_mode(GPIO_ALS_PWREN_PIN, GPIO_ALS_PWREN_PIN_M_GPIO);
        mt_set_gpio_dir(GPIO_ALS_PWREN_PIN, GPIO_DIR_OUT);
        mt_set_gpio_out(GPIO_ALS_PWREN_PIN, 0);
    }
#endif
    */

    power_on = on;
}
#endif
/*----------------------------------------------------------------------------*/
static int em3071x_enable_als(struct i2c_client *client, int enable)
{
        struct em3071x_priv *obj = i2c_get_clientdata(client);
        u8 databuf[2];
        int res = 0;

        if(client == NULL)
        {
            APS_DBG("CLIENT CANN'T EQUL NULL\n");
            return -1;
        }

          databuf[0] = EM3071X_CMM_ENABLE;
            res = i2c_master_send(client, databuf, 0x1);
            if(res <= 0)
            {
                goto EXIT_ERR;
            }
            res = i2c_master_recv(client, databuf, 0x1);
            if(res <= 0)
            {
                goto EXIT_ERR;
            }

        if(enable)
        {
            databuf[1] = ((databuf[0] & 0xF8) | 0x06);
            databuf[0] = EM3071X_CMM_ENABLE;
            res = i2c_master_send(client, databuf, 0x2);
            if(res <= 0)
            {
                goto EXIT_ERR;
            }

            atomic_set(&obj->als_deb_on, 1);
            atomic_set(&obj->als_deb_end, jiffies+atomic_read(&obj->als_debounce)/(1000/HZ));
            APS_DBG("em3071x als power on\n");
        }
        else
        {
            databuf[1] = (databuf[0] & 0xF8);
            databuf[0] = EM3071X_CMM_ENABLE;
            res = i2c_master_send(client, databuf, 0x2);
            if(res <= 0)
            {
                goto EXIT_ERR;
            }
            atomic_set(&obj->als_deb_on, 0);
            APS_DBG("EM3071X als power off\n");
        }

        return 0;

    EXIT_ERR:
        APS_ERR("EM3071X_enable_als fail\n");
        return res;
}
int em3071x_read_ps(struct i2c_client *client, u16 *data)
{
    //struct em3071x_priv *obj = i2c_get_clientdata(client);
    u8 ps_value[1];
    u8 buffer[1];
    int res = 0;

    if(client == NULL)
    {
        APS_DBG("CLIENT CANN'T EQUL NULL\n");
        return -1;
    }

    buffer[0]=EM3071X_CMM_PDATA;
    res = i2c_master_send(client, buffer, 0x1);
    if(res <= 0)
    {
        goto EXIT_ERR;
    }
    res = i2c_master_recv(client, ps_value, 0x1);
    if(res <= 0)
    {
        goto EXIT_ERR;
    }
    *data = ps_value[0];
    //printk("bingo *******************ps_data=%d\n", *data);
    return 0;

EXIT_ERR:
    APS_ERR("em3071x_read_ps fail\n");
    return res;
}
/*----------------------------------------------------------------------------*/
static int em3071x_enable_ps(struct i2c_client *client, int enable)
{
    struct em3071x_priv *obj = i2c_get_clientdata(client);
    u8 databuf[2];
    int res = 0;
    struct PS_CALI_DATA_STRUCT ps_cali_temp;

    if(client == NULL)
    {
        APS_DBG("CLIENT CANN'T EQUL NULL\n");
        return -1;
    }

    APS_DBG("em3071x_enable_ps, enable = %d\n", enable);

        databuf[0] = EM3071X_CMM_ENABLE;
        res = i2c_master_send(client, databuf, 0x1);
        if(res <= 0)
        {
            goto EXIT_ERR;
        }
        res = i2c_master_recv(client, databuf, 0x1);
        if(res <= 0)
        {
            goto EXIT_ERR;
        }

    if(enable)
    {
        databuf[1] = ((databuf[0] & 0x06)|PS_ENABLE);
        databuf[0] = EM3071X_CMM_ENABLE;

        res = i2c_master_send(client, databuf, 0x2);
        if(res <= 0)
        {
            goto EXIT_ERR;
        }

        APS_DBG("em3071x ps power on\n");
        #ifdef EM3071X_PS_CALI
            em3071x_read_data_for_cali(client,&ps_cali_temp);
            em3071x_WriteCalibration(client,&ps_cali_temp);
        #endif
        /*for interrup work mode support */
        if(0 == obj->hw->polling_mode_ps)
        {

            databuf[0] = EM3071X_CMM_INT_PS_LB;    
            databuf[1] = (u8)(ps_cali.far_away);
            res = i2c_master_send(client, databuf, 0x2);
            if(res <= 0)
            {
                APS_FUN();
                goto EXIT_ERR;
            }
            databuf[0] = EM3071X_CMM_INT_PS_HB;
            databuf[1] = (u8)(ps_cali.close );
            res = i2c_master_send(client, databuf, 0x2);
            if(res <= 0)
            {
                APS_FUN();
                goto EXIT_ERR;
            }

            enable_irq(em3071x_obj->irq);
        }
    }
    else
    {
        databuf[1] = (databuf[0] & 0x06);
        databuf[0] = EM3071X_CMM_ENABLE;

        res = i2c_master_send(client, databuf, 0x2);
        if(res <= 0)
        {
            goto EXIT_ERR;
        }
        databuf[1] = 0x00;
        databuf[0] = EM3071X_CMM_OFFSET;

        res = i2c_master_send(client, databuf, 0x2);
        if(res <= 0)
        {
            goto EXIT_ERR;
        }
        atomic_set(&obj->ps_deb_on, 0);
        APS_DBG("em3071x ps power off\n");

        /*for interrup work mode support */
        if(0 == obj->hw->polling_mode_ps)
        {
            cancel_work_sync(&obj->eint_work);
            //mt_eint_mask(CUST_EINT_ALS_NUM);
            disable_irq(em3071x_obj->irq);
        }
    }
    ps_enabled=enable;
    return 0;

EXIT_ERR:
    APS_ERR("em3071x_enable_ps fail\n");
    return res;
}

/*----------------------------------------------------------------------------*/
/*for interrup work mode support --*/
static int em3071x_check_and_clear_intr(struct i2c_client *client) 
{
    //struct em3071x_priv *obj = i2c_get_clientdata(client);
    int res;
    u8 buffer[2];

    buffer[0] = EM3071X_CMM_STATUS;
    res = i2c_master_send(client, buffer, 0x1);
    if(res <= 0)
    {
        goto EXIT_ERR;
    }
    res = i2c_master_recv(client, buffer, 0x1);
    if(res <= 0)
    {
        goto EXIT_ERR;
    }
    //APS_ERR("em3071x_check_and_clear_intr status=0x%x\n", buffer[0]);
    if(0 != (buffer[0] & 0x80))
    {
        buffer[0] = EM3071X_CMM_STATUS;
        buffer[1] = 0x00;
        res = i2c_master_send(client, buffer, 0x2);
        if(res <= 0)
        {
            goto EXIT_ERR;
        }
        res = 0;
    }

    return res;

EXIT_ERR:
    APS_ERR("em3071x_check_and_clear_intr fail\n");
    return 1;
}
/*----------------------------------------------------------------------------*/

void em3071x_eint_func(void)
{
    struct em3071x_priv *obj = em3071x_obj;
    if(!obj)
    {
        return;
    }

    schedule_work(&obj->eint_work);
}

static irqreturn_t em3071x_eint_handler(int irq, void *desc)
{
    APS_ERR("em3071x_eint_handler enter");
    em3071x_eint_func();
    disable_irq_nosync(em3071x_obj->irq);

    return IRQ_HANDLED;
}

/*----------------------------------------------------------------------------*/
/*for interrup work mode support --*/
int em3071x_setup_eint(struct i2c_client *client)
{
    u32 ints[2] = {0};
//    int err = 0;
    int ret;
    struct pinctrl *pinctrl;
    struct pinctrl_state *pins_default;
    struct pinctrl_state *pins_cfg;

    if (!em3071x_obj) {
        APS_ERR("hals3201_obj is null!!\n");
        return 0;
    }
    //alspsPltFmDev = get_alsps_platformdev();

    pinctrl = devm_pinctrl_get(&client->dev);
    if (IS_ERR(pinctrl)) {
        ret = PTR_ERR(pinctrl);
        APS_ERR("Cannot find alsps pinctrl!\n");
    //    return ret;
    }
    pins_default = pinctrl_lookup_state(pinctrl, "pin_default");
    if (IS_ERR(pins_default)) {
        ret = PTR_ERR(pins_default);
        APS_ERR("Cannot find alsps pinctrl default!\n");

    }

    pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
    if (IS_ERR(pins_cfg)) {
        ret = PTR_ERR(pins_cfg);
        APS_ERR("Cannot find alsps pinctrl pin_cfg!\n");
    //    return ret;
    }
    pinctrl_select_state(pinctrl, pins_cfg); //wangzhongping changed

    if (em3071x_obj->irq_node) {
#ifndef CONFIG_MTK_EIC
        /*upstream code*/
        ints[0] = of_get_named_gpio(em3071x_obj->irq_node, "deb-gpios", 0);
        if (ints[0] < 0) {
            pr_err("debounce gpio not found\n");
        } else{
            ret = of_property_read_u32(em3071x_obj->irq_node, "debounce", &ints[1]);
            if (ret < 0)
                pr_err("debounce time not found\n");
            else
                gpio_set_debounce(ints[0], ints[1]);
            APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);
        }
#else
        of_property_read_u32_array(em3071x_obj->irq_node, "debounce", ints, ARRAY_SIZE(ints));
        gpio_request(ints[0], "p-sensor");
        gpio_set_debounce(ints[0], ints[1]);
        APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);
#endif

        em3071x_obj->irq = irq_of_parse_and_map(em3071x_obj->irq_node, 0);
        APS_LOG("em3071x_obj->irq = %d\n", em3071x_obj->irq);
        if (!em3071x_obj->irq) {
            APS_ERR("irq_of_parse_and_map fail!!\n");
            return -EINVAL;
        }
        if (request_irq
            (em3071x_obj->irq, em3071x_eint_handler, IRQF_TRIGGER_NONE, "ALS-eint", NULL)) {// Linc 20160715 IRQF_TRIGGER_FALLING
            APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
            return -EINVAL;
        }
        enable_irq_wake(em3071x_obj->irq);
        enable_irq(em3071x_obj->irq);
    } else {
        APS_ERR("null irq node!!\n");
        return -EINVAL;
    }

    return 0;

}

/******************************************************************************
**
*****************************************************************************/
int em3071x_read_als(struct i2c_client *client, u16 *data)
{
    //struct em3071x_priv *obj = i2c_get_clientdata(client);
    //u16 als_value;
    u8 als_value_low[1], als_value_high[1];
    u8 buffer[1];
    int res = 0;

    if(client == NULL)
    {
        APS_DBG("CLIENT CANN'T EQUL NULL\n");
        return -1;
    }

    buffer[0]=EM3071X_CMM_C0DATA_L;
    res = i2c_master_send(client, buffer, 0x1);
    if(res <= 0)
    {
        goto EXIT_ERR;
    }
    res = i2c_master_recv(client, als_value_low, 0x1);
    if(res <= 0)
    {
        goto EXIT_ERR;
    }

    buffer[0]=EM3071X_CMM_C0DATA_H;
    res = i2c_master_send(client, buffer, 0x1);
    if(res <= 0)
    {
        goto EXIT_ERR;
    }
    res = i2c_master_recv(client, als_value_high, 0x01);
    if(res <= 0)
    {
        goto EXIT_ERR;
    }

    *data = als_value_low[0] | ((als_value_high[0])<<8);

    //printk("em3071x als value = %d\t\n",*data);
    return 0;

EXIT_ERR:
    APS_ERR("em3071x_read_ps fail\n");
    return res;
}
/*----------------------------------------------------------------------------*/

static int em3071x_get_als_value(struct em3071x_priv *obj, u16 als)
{
    int idx;
    int invalid = 0;
    for(idx = 0; idx < obj->als_level_num; idx++)
    {
        if(als < obj->hw->als_level[idx])
        {
            break;
        }
    }

    if(idx >= obj->als_value_num)
    {
        APS_ERR("exceed range\n");
        idx = obj->als_value_num - 1;
    }

    if(1 == atomic_read(&obj->als_deb_on))
    {
        unsigned long endt = atomic_read(&obj->als_deb_end);
        if(time_after(jiffies, endt))
        {
            atomic_set(&obj->als_deb_on, 0);
        }

        if(1 == atomic_read(&obj->als_deb_on))
        {
            invalid = 1;
        }
    }

    if(!invalid)
    {
        APS_DBG("zjb ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);
        return obj->hw->als_value[idx];
    }
    else
    {
        APS_ERR(" zjb ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);
        return -1;
    }
}
/*----------------------------------------------------------------------------*/
#ifdef EM3071X_PS_CALI
static int em3071x_read_data_for_cali(struct i2c_client *client, struct PS_CALI_DATA_STRUCT *ps_data_cali)
{
    int i=0 ,err = 0,sum=0,data_cali=0;
    u16 data[5] = {0};

    for(i = 0;i<5;i++)
    {
        mdelay(150);
        if(!em3071x_read_ps(client,&data[i]))
        {
            sum += data[i];
            //APS_LOG("[cbh++]data[%d] = %d\n", i, data);
        }
        else
        {
        break;
        }
    }
    data_cali = sum/5;
    if(data_cali< 200)
    {
        //APS_LOG("[cbh++]em3071x_read_data_for_cali data_cali = %d\n",data_cali);
        ps_data_cali->far_away = data_cali + 15;
        ps_data_cali->close = data_cali + 20;
        ps_data_cali->valid = 1;
        err= 0;
    }
    else
    {
        data_cali_buff=data_cali;
                ps_data_cali->valid = 0;
                //APS_LOG("em3071x_read_data_for_cali get data error!\n");
                err=  -1;
    }
        if(err==0)
    {
        data_cali_buff=data_cali;
        ps_store_cali.far_away=ps_data_cali->far_away;
        ps_store_cali.close=ps_data_cali->close;
        ps_store_cali.valid=ps_data_cali->valid;
    }
    return err;
}
static void em3071x_WriteCalibration(struct i2c_client *client, struct PS_CALI_DATA_STRUCT *ps_data_cali)
{
//    struct em3071x_priv *obj = i2c_get_clientdata(client);    //pj debug
    
    //APS_FUN();
    //APS_LOG("em3071x_WriteCalibration() ...start!\n");

    if(ps_data_cali->valid == 1)
    {
        ps_cali.close = ps_data_cali->close;
        ps_cali.far_away = ps_data_cali->far_away;
        ps_cali.valid = 1;
    }
    else if(ps_data_cali->valid == 0){
        ps_cali.close = ps_store_cali.close;
        ps_cali.far_away = ps_store_cali.far_away;
        ps_cali.valid = 0;
    }
    //atomic_set(&obj->ps_thd_val_high,ps_cali.close);
    //atomic_set(&obj->ps_thd_val_low, ps_cali.far_away);
}
#endif

/*----------------------------------------------------------------------------*/
static int em3071x_get_ps_value(struct em3071x_priv *obj, u16 ps)
{
    int val=1;
//    int invalid = 0;
    static int val_temp=1;
    //int mask = atomic_read(&obj->ps_mask);
    //printk("jipeng  em3071x  get ps %d\n",ps);
    //printk("jipeng em3071x  0921ps_cali_symbol %d\n",ps_cali_symbol);
    //printk("jipeng em3071x  ps_cal %d\n",ps_cal);
    //if(ps > ps_cali.close )
    //dump_stack();

    if(ps_cali_symbol==1)
    {
        if (ps > ps_cal+2)
        {
            //printk("jipeng em3071x  get ps hight %d\n",ps);
            val = 0;  /*close*/
            val_temp = 0;
            intr_flag_value = 1;
        }
        else if(ps < ps_cal+5)  //(ps < ps_cali.far_away)
        {
            //printk("jipeng em3071x  get ps low %d\n",ps);
            val = 1;  /*far away*/
            val_temp = 1;
            intr_flag_value = 0;
        }
        else
               val = val_temp;
    }
    
 #if 1
    else
    {
        if (ps > ps_cali.close) 
        {
            //printk("jipeng em3071x  get ps hight %d\n",ps);
            val = 0;  /*close*/
            val_temp = 0;
            intr_flag_value = 1;
        }
        else if(ps < ps_cali.far_away)  //(ps < ps_cali.far_away)
        {
            //printk("jipeng em3071x  get ps low %d\n",ps);
            val = 1;  /*far away*/
            val_temp = 1;
            intr_flag_value = 0;
        }
        else
               val = val_temp;
     }
#endif
/*
    if(atomic_read(&obj->ps_suspend))
    {
        invalid = 1;
    }
    else if(1 == atomic_read(&obj->ps_deb_on))
    {
        unsigned long endt = atomic_read(&obj->ps_deb_end);
        if(time_after(jiffies, endt))
        {
            atomic_set(&obj->ps_deb_on, 0);
        }
        
        if (1 == atomic_read(&obj->ps_deb_on))
        {
            invalid = 1;
        }
    }

    if(!invalid)
    {
        return val;
    }
    else
    {
        return -1;
    }
    */
    return val;
}

/*for interrup work mode support --*/
static int em3071x_init_client(struct i2c_client *client)
{
    struct em3071x_priv *obj = i2c_get_clientdata(client);
    u8 databuf[2]/*,ps_data*/;
    int res = 0;
    //i=0,sum=0;
    //struct PS_CALI_DATA_STRUCT ps_cali_temp;
      //u8 avg_ps,offset_val;


    u8 data;
    data = 0xbe;
    res = em3071x_master_send(client, 0x01, &data, 2);
    if (res < 0)
    {
        APS_ERR("write 0x87 = %d\n", res);
        return -EFAULT;
    }
    //printk("write 0x87 = %d\n", res);

    return 0;

       databuf[0] = EM3071X_CMM_ENABLE;
       databuf[1] = PS_ENABLE;
       res = i2c_master_send(client, databuf, 0x2);
       if(res <= 0)
       {
            APS_FUN();
            goto EXIT_ERR;
       }
       databuf[0] = EM3071X_CMM_STATUS;
       databuf[1] = 0X00;
       res = i2c_master_send(client, databuf, 0x2);
       if(res <= 0)
       {
            APS_FUN();
            goto EXIT_ERR;
       }

      databuf[0] = 0X0E;
      databuf[1] = 0x00;
       res = i2c_master_send(client, databuf, 0x2);
       if(res <= 0)
       {
           APS_FUN();
           goto EXIT_ERR;
       }
       databuf[0] = EM3071X_CMM_OFFSET;
       databuf[1] = 0x00;
       res = i2c_master_send(client, databuf, 0x2);
       if(res <= 0)
       {
           APS_FUN();
           goto EXIT_ERR;
        }
        //printk("wangzhongping alsps obj->hw->polling_mode_ps = %d\n", obj->hw->polling_mode_ps);
        /*for interrup work mode support*/
        if(0 == obj->hw->polling_mode_ps)
        {
            databuf[0] = EM3071X_CMM_INT_PS_LB;    
            databuf[1] = (u8)(ps_cali.far_away);
            res = i2c_master_send(client, databuf, 0x2);
            if(res <= 0)
            {
                APS_FUN();
                goto EXIT_ERR;
            }
            databuf[0] = EM3071X_CMM_INT_PS_HB;    
            databuf[1] = (u8)(ps_cali.close );
            res = i2c_master_send(client, databuf, 0x2);
            if(res <= 0)
            {
                APS_FUN();
                goto EXIT_ERR;
            }
            /*for interrup work mode support */
            res = em3071x_setup_eint(client);
            if(res )
            {
                APS_ERR("setup eint: %d\n", res);
                return res;
            }
            res = em3071x_check_and_clear_intr(client);
            if(res)
            {
                APS_ERR("check/clear intr: %d\n", res);
                //    return res;
            }

        //mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
        }

    return EM3071X_SUCCESS;

EXIT_ERR:
    APS_ERR("init dev: %d\n", res);
    return res;
}

static void em3071x_eint_work(struct work_struct *work)
{
    struct em3071x_priv *obj = (struct em3071x_priv *)container_of(work, struct em3071x_priv, eint_work);
    int err;
    //struct hwm_sensor_data sensor_data;
    int values = -1;

    if((err = em3071x_check_and_clear_intr(obj->client)))
    {
        APS_ERR("em3071x_eint_work check intrs: %d\n", err);
    }
    else
    {
        //get raw data
        em3071x_read_ps(obj->client, &obj->ps);

        APS_DBG("em3071x_eint_work rawdata ps=%d \n",obj->ps);
        values = em3071x_get_ps_value(obj, obj->ps);

        ps_report_interrupt_data(values);
    }

    if((err = em3071x_check_and_clear_intr(obj->client)))
    {
        APS_ERR("em3071x_eint_work check intrs: %d\n", err);
    }
    em3071x_read_ps(obj->client, &obj->ps);

    if((err = em3071x_check_and_clear_intr(obj->client)))
    {
        APS_ERR("em3071x_eint_work check intrs: %d\n", err);
    }
        enable_irq(em3071x_obj->irq);   
        return;
//EXIT_ERR:
         //mt_eint_unmask(CUST_EINT_ALS_NUM);  
        //return;

}

/******************************************************************************
 * Function Configuration
******************************************************************************/
#ifdef EM3071X_MISC_DEVICE
static int em3071x_open(struct inode *inode, struct file *file)
{
    file->private_data = em3071x_i2c_client;

    if (!file->private_data)
    {
        APS_ERR("null pointer!!\n");
        return -EINVAL;
    }

    return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int em3071x_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}
/*----------------------------------------------------------------------------*/

static long em3071x_ioctl( struct file *file, unsigned int cmd,unsigned long arg)
{
    struct i2c_client *client = (struct i2c_client*)file->private_data;
    struct em3071x_priv *obj = i2c_get_clientdata(client);  
    int err = 0;
    void __user *ptr = (void __user*) arg;
    int dat;
    uint32_t enable;
    //wt add for ps calibration start
    //struct PS_CALI_DATA_STRUCT ps_cali_temp;
    //wt add for ps calibration end

    switch (cmd)
    {
        case ALSPS_SET_PS_MODE:
            if(copy_from_user(&enable, ptr, sizeof(enable)))
            {
                err = -EFAULT;
                goto err_out;
            }
            if(enable)
            {
                err = em3071x_enable_ps(obj->client, 1);
                if(err )
                {
                    APS_ERR("enable ps fail: %d\n", err);
                    goto err_out;
                }

                set_bit(CMC_BIT_PS, &obj->enable);
            }
            else
            {
                err = em3071x_enable_ps(obj->client, 0);
                if(err)
                {
                    APS_ERR("disable ps fail: %d\n", err);
                    goto err_out;
                }

                clear_bit(CMC_BIT_PS, &obj->enable);
            }
            break;

        case ALSPS_GET_PS_MODE:
            enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);
            if(copy_to_user(ptr, &enable, sizeof(enable)))
            {
                err = -EFAULT;
                goto err_out;
            }
            break;

        case ALSPS_GET_PS_DATA:
            err = em3071x_read_ps(obj->client, &obj->ps);
            if(err)
            {
                goto err_out;
            }

            dat = em3071x_get_ps_value(obj, obj->ps);
            if(dat == -1)
            {
                err = -EFAULT;
                goto err_out;
            }

            if(copy_to_user(ptr, &dat, sizeof(dat)))
            {
                err = -EFAULT;
                goto err_out;
            }
            break;
               //wt add for ps calibration start
#if 0
        case ALSPS_SET_PS_CALI:
            if(ptr == NULL)
            {
                //APS_LOG("ptr == NULL\n");
                err = -EINVAL;
                break;
            }
            if(copy_from_user(&ps_cali_temp, ptr, sizeof(ps_cali_temp)))
            {
                //APS_LOG("copy_from_user is fail\n");
                err = -EFAULT;
                break;
            }
            em3071x_WriteCalibration(obj->client,&ps_cali_temp);
           // APS_LOG(" ALSPS_SET_PS_CALI %d,%d,%d\t",ps_cali_temp.close,ps_cali_temp.far_away,ps_cali_temp.valid);
            break;
        case ALSPS_GET_PS_RAW_DATA_FOR_CALI:
            em3071x_enable_ps(obj->client, 1);
            em3071x_read_data_for_cali(obj->client,&ps_cali_temp);
            if(copy_to_user(ptr, &ps_cali_temp, sizeof(ps_cali_temp)))
            {
                err = -EFAULT;
                goto err_out;
            }
            break;
        //wt add for ps calibration end
#endif
        case ALSPS_GET_PS_RAW_DATA:
            err = em3071x_read_ps(obj->client, &obj->ps);
            if(err)
            {
                goto err_out;
            }

            dat = obj->ps;
            if(copy_to_user(ptr, &dat, sizeof(dat)))
            {
                err = -EFAULT;
                goto err_out;
            }
            break;

        case ALSPS_SET_ALS_MODE:
            if(copy_from_user(&enable, ptr, sizeof(enable)))
            {
                err = -EFAULT;
                goto err_out;
            }
            if(enable)
            {
                err = em3071x_enable_als(obj->client, 1);
                if(err)
                {
                    APS_ERR("enable als fail: %d\n", err);
                    goto err_out;
                }
                set_bit(CMC_BIT_ALS, &obj->enable);
            }
            else
            {
                err = em3071x_enable_als(obj->client, 0);
                if(err)
                {
                    APS_ERR("disable als fail: %d\n", err);
                    goto err_out;
                }
                clear_bit(CMC_BIT_ALS, &obj->enable);
            }
            break;

        case ALSPS_GET_ALS_MODE:
            enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
            if(copy_to_user(ptr, &enable, sizeof(enable)))
            {
                err = -EFAULT;
                goto err_out;
            }
            break;

        case ALSPS_GET_ALS_DATA: 
            err = em3071x_read_als(obj->client, &obj->als);
            if(err)
            {
                goto err_out;
            }

            dat = em3071x_get_als_value(obj, obj->als);
            if(copy_to_user(ptr, &dat, sizeof(dat)))
            {
                err = -EFAULT;
                goto err_out;
            }
            break;

        case ALSPS_GET_ALS_RAW_DATA:    
            err = em3071x_read_als(obj->client, &obj->als);
            if(err)
            {
                goto err_out;
            }

            dat = obj->als;
            if(copy_to_user(ptr, &dat, sizeof(dat)))
            {
                err = -EFAULT;
                goto err_out;
            }
            break;
        default:
            APS_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
            err = -ENOIOCTLCMD;
            break;
    }

    err_out:
    return err;
}
/*----------------------------------------------------------------------------*/
static struct file_operations em3071x_fops = {
    .owner = THIS_MODULE,
    .open = em3071x_open,
    .release = em3071x_release,
    .unlocked_ioctl = em3071x_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice em3071x_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "als_ps",
    .fops = &em3071x_fops,
};
#endif
/*----------------------------------------------------------------------------*/
static int em3071x_i2c_suspend(struct device *dev) 
{
    //struct em3071x_priv *obj = i2c_get_clientdata(client);
    //int err;

    APS_FUN();
#if 0
    if(msg.event == PM_EVENT_SUSPEND)
    {
        if(!obj)
        {
            APS_ERR("null pointer!!\n");
            return -EINVAL;
        }

        atomic_set(&obj->als_suspend, 1);
        err = em3071x_enable_als(obj->client, 0);
        if(err < 0)
        {
            APS_ERR("disable als: %d\n", err);
            return err;
        }

        atomic_set(&obj->ps_suspend, 1);
        err = em3071x_enable_ps(obj->client, 0);
        if(err < 0)
        {
            APS_ERR("disable ps:  %d\n", err);
            return err;
        }

        em3071x_power(obj->hw, 0);
    }
#endif
    return 0;
}
/*----------------------------------------------------------------------------*/
static int em3071x_i2c_resume(struct device *dev)
{
//    struct em3071x_priv *obj = i2c_get_clientdata(client);
//    int err;
    APS_FUN();
#if 0
    if(!obj)
    {
        APS_ERR("null pointer!!\n");
        return -EINVAL;
    }

    em3071x_power(obj->hw, 1);
    atomic_set(&obj->als_suspend, 0);
    if(test_bit(CMC_BIT_ALS, &obj->enable))
    {
        err = em3071x_enable_als(obj->client, 1);
        if (err < 0)
        {
            APS_ERR("enable als fail: %d\n", err);
        }
    }
    atomic_set(&obj->ps_suspend, 0);
    if(test_bit(CMC_BIT_PS,  &obj->enable))
    {
        err = em3071x_enable_ps(obj->client, 1);
        if (err < 0)
        {
            APS_ERR("enable ps fail: %d\n", err);
        }
    }
#endif
    return 0;
}
/*----------------------------------------------------------------------------*/
#ifdef CONFIG_HAS_EARLYSUSPEND
static void em3071x_early_suspend(struct early_suspend *h)
{   /*early_suspend is only applied for ALS*/
    struct em3071x_priv *obj = container_of(h, struct em3071x_priv, early_drv);
    int err;
    APS_FUN();

    if(!obj)
    {
        APS_ERR("null pointer!!\n");
        return;
    }

    atomic_set(&obj->als_suspend, 1);

    //mutex_lock(&sensor_lock);
    err = em3071x_enable_als(obj->client, 0);
    if(err)
    {
        APS_ERR("disable als fail: %d\n", err);
    }
    //mutex_unlock(&sensor_lock);

}
/*----------------------------------------------------------------------------*/
static void em3071x_late_resume(struct early_suspend *h)
{   /*late_resume is only applied for ALS*/
    struct em3071x_priv *obj = container_of(h, struct em3071x_priv, early_drv);
    int err;
    APS_FUN();

    if(!obj)
    {
        APS_ERR("null pointer!!\n");
        return;
    }

    atomic_set(&obj->als_suspend, 0);
    if(test_bit(CMC_BIT_ALS, &obj->enable))
    {
        mutex_lock(&sensor_lock);
        err = em3071x_enable_als(obj->client, 1);
        if(err)
        {
            APS_ERR("enable als fail: %d\n", err);
        }

        mutex_unlock(&sensor_lock);
    }

}
#endif

int em3071x_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value;
    struct hwm_sensor_data* sensor_data;
    struct em3071x_priv *obj = (struct em3071x_priv *)self;

    switch (command)
    {
        case SENSOR_DELAY:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                APS_ERR("Set delay parameter error!\n");
                err = -EINVAL;
            }
            break;

        case SENSOR_ENABLE:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                APS_ERR("Enable sensor parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                if(value)
                {
                  // wake_lock(&ps_suspend_lock);     //pj add for debug
                    mutex_lock(&sensor_lock);
                    err = em3071x_enable_ps(obj->client, 1);
                    if(err)
                    {
                        APS_ERR("enable ps fail: %d\n", err);

                        mutex_unlock(&sensor_lock);
                        return -1;
                    }
                    mutex_unlock(&sensor_lock);

                    set_bit(CMC_BIT_PS, &obj->enable);

                }
                else
                {
                  // wake_unlock(&ps_suspend_lock);     //pj add for debug
                    mutex_lock(&sensor_lock);
                    err = em3071x_enable_ps(obj->client, 0);
                    if(err)
                    {
                        APS_ERR("disable ps fail: %d\n", err); 

                        mutex_unlock(&sensor_lock);
                        return -1;
                    }
                    mutex_unlock(&sensor_lock);
                    clear_bit(CMC_BIT_PS, &obj->enable);

                }
            }
            break;

        case SENSOR_GET_DATA:
            if((buff_out == NULL) || (size_out< sizeof(struct hwm_sensor_data)))
            {
                APS_ERR("get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                sensor_data = (struct hwm_sensor_data *)buff_out;
                mutex_lock(&sensor_lock);
                em3071x_read_ps(obj->client, &obj->ps);
                mutex_unlock(&sensor_lock);
                APS_ERR("e3071_ps_operate ps data=%d!\n",obj->ps);
                sensor_data->values[0] = em3071x_get_ps_value(obj, obj->ps);
                sensor_data->value_divide = 1;
                sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
            }
            break;
        default:
            APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }

    return err;
}

int em3071x_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value;
    struct hwm_sensor_data* sensor_data;
    struct em3071x_priv *obj = (struct em3071x_priv *)self;

    switch (command)
    {
        case SENSOR_DELAY:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                APS_ERR("Set delay parameter error!\n");
                err = -EINVAL;
            }
            break;

        case SENSOR_ENABLE:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                APS_ERR("Enable sensor parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                if(value)
                {
                    mutex_lock(&sensor_lock);
                    err = em3071x_enable_als(obj->client, 1);
                    if(err)
                    {
                        APS_ERR("enable als fail: %d\n", err); 
                        mutex_unlock(&sensor_lock);
                        return -1;
                    }
                    mutex_unlock(&sensor_lock);
                    set_bit(CMC_BIT_ALS, &obj->enable);
                }
                else
                {
                    mutex_lock(&sensor_lock);
                    err = em3071x_enable_als(obj->client, 0);
                    if(err)
                    {
                        mutex_unlock(&sensor_lock);
                        APS_ERR("disable als fail: %d\n", err);
                        return -1;
                    }
                    mutex_unlock(&sensor_lock);
                    clear_bit(CMC_BIT_ALS, &obj->enable);
                }

            }
            break;

        case SENSOR_GET_DATA:
            if((buff_out == NULL) || (size_out< sizeof(struct hwm_sensor_data)))
            {
                APS_ERR("get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                sensor_data = (struct hwm_sensor_data *)buff_out;

                mutex_lock(&sensor_lock);
                em3071x_read_als(obj->client, &obj->als);
                mutex_unlock(&sensor_lock);

                sensor_data->values[0] = em3071x_get_als_value(obj, obj->als);
                sensor_data->value_divide = 1;
                sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
            }
            break;
        default:
            APS_ERR("light sensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }

    return err;
}


/*----------------------------------------------------------------------------*/
static int em3071x_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    strcpy(info->type, EM3071X_DEV_NAME);
    return 0;
}

/*----------------------------------------------------------------------------*/

//Gionee BSP1 yaoyc 20151215 add for em3071x before
static int als_open_report_data(int open)
{
    /* should queuq work to report event if  is_report_input_direct=true */
    return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */

static int als_enable_nodata(int en)
{
    //int res = 0;
    //0112 add
    int err = 0;
    //u8 databuf[2];
    APS_LOG("em3071x als enable value = %d\n", en);
    if(em3071x_obj == NULL){
        APS_ERR("em3071x_obj is null\n");
        return -1;
    }

        if(en)
        {
            mutex_lock(&sensor_lock);
            err = em3071x_enable_als(em3071x_obj->client, 1);
            if(err)
            {
                APS_ERR("enable ps fail: %d\n", err); 

                mutex_unlock(&sensor_lock);
                return -1;
            }
            mutex_unlock(&sensor_lock);

            set_bit(CMC_BIT_ALS, &em3071x_obj->enable);
        }
        else
        {
            mutex_lock(&sensor_lock);
            err = em3071x_enable_als(em3071x_obj->client, 0);
            if(err)
            {
                APS_ERR("disable ps fail: %d\n", err); 

                mutex_unlock(&sensor_lock);
                return -1;
            }
            mutex_unlock(&sensor_lock);
            clear_bit(CMC_BIT_ALS, &em3071x_obj->enable);
        }

    return 0;
}

static int als_set_delay(u64 ns)
{
    return 0;
}
#define ALS_BATCH_ENABLE
#ifdef ALS_BATCH_ENABLE
static int als_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
    return als_set_delay(samplingPeriodNs);
}

static int als_flush(void)
{
    return als_flush_report();
}
#endif

static int als_get_data(int *value, int *status)
{
    int err = 0;

    APS_LOG("em3071x als get data \n");
    //0112 add
    if(em3071x_obj == NULL){
        APS_ERR("em3071x_obj is null\n");
        return -1;
    }
    mutex_lock(&sensor_lock);
    err = em3071x_read_als(em3071x_obj->client, &em3071x_obj->als);
    mutex_unlock(&sensor_lock);

    *value =  em3071x_obj->als;//em3071x_get_als_value(em3071x_obj, em3071x_obj->als);
    *status = SENSOR_STATUS_ACCURACY_MEDIUM;

    APS_LOG("em3071x als_get_data, value = %d\n",*value);
    //0112 end
    return err;
}

/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int ps_open_report_data(int open)
{
    /* should queuq work to report event if  is_report_input_direct=true */
    return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */

static int ps_enable_nodata(int en)
{
    //int res = 0;
    int err = 0;
    APS_LOG("em3071x ps enable value = %d\n", en);
    if(em3071x_obj == NULL){
        APS_ERR("em3071x_obj is null\n");
        return -1;
    }
    if(en)
    {
        // wake_lock(&ps_suspend_lock);     //pj add for debug
        mutex_lock(&sensor_lock);
        err = em3071x_enable_ps(em3071x_obj->client, 1);
        if(err)
        {
            APS_ERR("enable ps fail: %d\n", err); 

            mutex_unlock(&sensor_lock);
            return -1;
        }
        mutex_unlock(&sensor_lock);

        set_bit(CMC_BIT_PS, &em3071x_obj->enable);

    }
    else
    {
        // wake_unlock(&ps_suspend_lock);     //pj add for debug
        mutex_lock(&sensor_lock);
        err = em3071x_enable_ps(em3071x_obj->client, 0);
        if(err)
        {
            APS_ERR("disable ps fail: %d\n", err); 

            mutex_unlock(&sensor_lock);
            return -1;
        }
        mutex_unlock(&sensor_lock);
        clear_bit(CMC_BIT_PS, &em3071x_obj->enable);

    }
    return 0;

}

static int ps_set_delay(u64 ns)
{
    return 0;
}
#ifdef ALS_BATCH_ENABLE
static int ps_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
    return 0;
}

static int ps_flush(void)
{
    return ps_flush_report();
}
#endif

static int ps_get_data(int *value, int *status)
{
    int err = 0;
    APS_LOG("em3071x ps get data \n");

    if(em3071x_obj == NULL){
        APS_ERR("em3071x_obj is null\n");
        return -1;
    }
    mutex_lock(&sensor_lock);
    err = em3071x_read_ps(em3071x_obj->client, &em3071x_obj->ps);
    mutex_unlock(&sensor_lock);
    if( err == 0){
        *value = em3071x_get_ps_value(em3071x_obj, em3071x_obj->ps);
        if(*value < 0 )
            err = -1;
        *status = SENSOR_STATUS_ACCURACY_MEDIUM;
    }
    //printk("moon em3071x ps_get_data, value = %d\n",*value);

    return err;
}
//Gionee BSP1 yaoyc 20151215 add for em3071x end
#if 1
static int set_psensor_threshold(struct i2c_client *client)
{
#if 0
    struct em3071x_priv *obj = i2c_get_clientdata(client);
    int res = 0;
    u8 data = 0;

    APS_LOG("set_psensor_threshold function high: 0x%x, low:0x%x\n", atomic_read(&obj->ps_thd_val_high),
        atomic_read(&obj->ps_thd_val_low));

    data = (u8) (atomic_read(&obj->ps_thd_val_low) & 0xFF);
    res = em3071x_write(client, EM3071X_CMM_INT_PS_LB, data);
    if (res <= 0) {
        APS_ERR("i2c_master_send function err\n");
        return -1;
    }

    data = (u8) (atomic_read(&obj->ps_thd_val_high) & 0xFF);
    res = em3071x_write(client, EM3071X_CMM_INT_PS_HB, data);
    if (res <= 0) {
        APS_ERR("i2c_master_send function err\n");
        return -1;
    }
#endif
    return 0;
}

static int em3071x_als_factory_enable_sensor(bool enable_disable, int64_t sample_periods_ms)
{
    int err = 0;

    err = als_enable_nodata(enable_disable ? 1 : 0);
    if (err) {
        APS_ERR("%s:%s failed\n", __func__, enable_disable ? "enable" : "disable");
        return -1;
    }
    err = als_batch(0, sample_periods_ms * 1000000, 0);
    if (err) {
        APS_ERR("%s set_batch failed\n", __func__);
        return -1;
    }
    return 0;
}

static int em3071x_als_factory_get_data(int32_t *data)
{
    int status;

    return als_get_data(data, &status);
}

static int em3071x_als_factory_get_raw_data(int32_t *data)
{
    int err = 0;
    struct em3071x_priv *obj = em3071x_obj;

    if (!obj) {
        APS_ERR("obj is null!!\n");
        return -1;
    }

    err = em3071x_read_als(obj->client, &obj->als);
    if (err) {
        APS_ERR("%s failed\n", __func__);
        return -1;
    }
    *data = em3071x_obj->als;

    return 0;
}

static int em3071x_als_factory_enable_calibration(void)
{
    return 0;
}

static int em3071x_als_factory_clear_cali(void)
{
    return 0;
}

static int em3071x_als_factory_set_cali(int32_t offset)
{
    return 0;
}

static int em3071x_als_factory_get_cali(int32_t *offset)
{
    return 0;
}

static int em3071x_ps_factory_enable_sensor(bool enable_disable, int64_t sample_periods_ms)
{
    int err = 0;

    err = ps_enable_nodata(enable_disable ? 1 : 0);
    if (err) {
        APS_ERR("%s:%s failed\n", __func__, enable_disable ? "enable" : "disable");
        return -1;
    }
    err = ps_batch(0, sample_periods_ms * 1000000, 0);
    if (err) {
        APS_ERR("%s set_batch failed\n", __func__);
        return -1;
    }
    return err;
}

static int em3071x_ps_factory_get_data(int32_t *data)
{
    int err = 0, status = 0;

    err = ps_get_data(data, &status);
    if (err < 0)
        return -1;
    return 0;
}

static int em3071x_ps_factory_get_raw_data(int32_t *data)
{
    int err = 0;
    struct em3071x_priv *obj = em3071x_obj;

    err = em3071x_read_ps(obj->client, &obj->ps);
    if (err) {
        APS_ERR("%s failed\n", __func__);
        return -1;
    }
    *data = em3071x_obj->ps;
    return 0;
}

static int em3071x_ps_factory_enable_calibration(void)
{
    return 0;
}

static int em3071x_ps_factory_clear_cali(void)
{
    struct em3071x_priv *obj = em3071x_obj;

    obj->ps_cali = 0;
    return 0;
}

static int em3071x_ps_factory_set_cali(int32_t offset)
{
    struct em3071x_priv *obj = em3071x_obj;

    obj->ps_cali = offset;
    return 0;
}

static int em3071x_ps_factory_get_cali(int32_t *offset)
{
    struct em3071x_priv *obj = em3071x_obj;

    *offset = obj->ps_cali;
    return 0;
}

static int em3071x_ps_factory_set_threshold(int32_t threshold[2])
{
    int err = 0;
    struct em3071x_priv *obj = em3071x_obj;

    APS_LOG("%s set threshold high: 0x%x, low: 0x%x\n", __func__, threshold[0], threshold[1]);
    atomic_set(&obj->ps_thd_val_high, (threshold[0] + obj->ps_cali));
    atomic_set(&obj->ps_thd_val_low, (threshold[1] + obj->ps_cali));
    err = set_psensor_threshold(obj->client);

    if (err < 0) {
        APS_ERR("set_psensor_threshold fail\n");
        return -1;
    }
    return 0;
}

static int em3071x_ps_factory_get_threshold(int32_t threshold[2])
{
    struct em3071x_priv *obj = em3071x_obj;

    threshold[0] = atomic_read(&obj->ps_thd_val_high) - obj->ps_cali;
    threshold[1] = atomic_read(&obj->ps_thd_val_low) - obj->ps_cali;
    return 0;
}

static struct alsps_factory_fops em3071x_factory_fops = {
    .als_enable_sensor = em3071x_als_factory_enable_sensor,
    .als_get_data = em3071x_als_factory_get_data,
    .als_get_raw_data = em3071x_als_factory_get_raw_data,
    .als_enable_calibration = em3071x_als_factory_enable_calibration,
    .als_clear_cali = em3071x_als_factory_clear_cali,
    .als_set_cali = em3071x_als_factory_set_cali,
    .als_get_cali = em3071x_als_factory_get_cali,

    .ps_enable_sensor = em3071x_ps_factory_enable_sensor,
    .ps_get_data = em3071x_ps_factory_get_data,
    .ps_get_raw_data = em3071x_ps_factory_get_raw_data,
    .ps_enable_calibration = em3071x_ps_factory_enable_calibration,
    .ps_clear_cali = em3071x_ps_factory_clear_cali,
    .ps_set_cali = em3071x_ps_factory_set_cali,
    .ps_get_cali = em3071x_ps_factory_get_cali,
    .ps_set_threshold = em3071x_ps_factory_set_threshold,
    .ps_get_threshold = em3071x_ps_factory_get_threshold,
};

static struct alsps_factory_public em3071x_factory_device = {
    .gain = 1,
    .sensitivity = 1,
    .fops = &em3071x_factory_fops,
};
#endif
static int em3071x_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = -1;
    struct em3071x_priv *obj;
    //struct hwmsen_object obj_ps, obj_als;
    int err = 0;
    //int i = 0;
    //Gionee BSP1 yaoyc 20151215 add for em3071x before
    struct als_control_path als_ctl = { 0 };
    struct als_data_path als_data = { 0 };
    struct ps_control_path ps_ctl = { 0 };
    struct ps_data_path ps_data = { 0 };
    //Gionee BSP1 yaoyc 20151215 add for em3071x end
    // wake_lock_init(&ps_suspend_lock, WAKE_LOCK_SUSPEND, "ps wakelock");  //pj add for debug
    if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
    {
        err = -ENOMEM;
        goto exit;
    }
    memset(obj, 0, sizeof(*obj));
    em3071x_obj = obj;
    //printk("Enter %s\n",__func__);
    ret = get_alsps_dts_func(client->dev.of_node, hw);
    if (ret < 0)
        APS_ERR("get dts info fail\n");

    //em3071x_power(hw, 1);
    //em3071x_get_addr(hw, &addr);

    APS_LOG("em3071x_init_client() in!\n");
    //obj->hw = get_cust_alsps();
    obj->hw=hw;
    obj->hw->polling_mode_ps=1;//by sugengxi
    //em3071x_get_addr(obj->hw, &obj->addr);

    client->addr = 0x24;//Linc 20160511 7_bit addr

    //printk("em3071x probe %s\n",__func__);
    /*for interrup work mode support --*/
    INIT_WORK(&obj->eint_work, em3071x_eint_work);
    obj->client = client;
    i2c_set_clientdata(client, obj);
    atomic_set(&obj->als_debounce, 300);
    atomic_set(&obj->als_deb_on, 0);
    atomic_set(&obj->als_deb_end, 0);
    atomic_set(&obj->ps_debounce, 300);
    atomic_set(&obj->ps_deb_on, 0);
    atomic_set(&obj->ps_deb_end, 0);
    atomic_set(&obj->ps_mask, 0);
    atomic_set(&obj->als_suspend, 0);
    atomic_set(&obj->als_cmd_val, 0xDF);
    atomic_set(&obj->ps_cmd_val,  0xC1);

    atomic_set(&obj->ps_thd_val,  0x0A);   //0x50
    atomic_set(&obj->ps_thd_val_high, obj->hw->ps_threshold_high);
    atomic_set(&obj->ps_thd_val_low, obj->hw->ps_threshold_low);
    obj->enable = 0;
    obj->pending_intr = 0;
    obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
    obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);

    obj->irq_node = client->dev.of_node;

    BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
    memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
    BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
    memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
    atomic_set(&obj->i2c_retry, 3);
    set_bit(CMC_BIT_ALS, &obj->enable);
    set_bit(CMC_BIT_PS, &obj->enable);

    //set_bit(CMC_BIT_PS, &obj->enable);
    // set i2c1 pull up

//
/*    PID = em3071x_read(client,0X00);

    for(i = 0;i<7;i++)
    {
        PID = em3071x_read(client,0X00);
        mdelay(50);
        //xy = em3071x_read(client,i);
        //printk("xiaoyi____read id PID :%x i:%d\n",PID);
    }

//printk("xiaoyi  em3071x read client->addr : %x   pid: %x\n",client->addr,PID);
    */
    err = em3071x_init_client(client);
    if(err)
    {
        goto exit_init_failed;
    }
    APS_LOG("em3071x_init_client() OK!\n");
#ifdef EM3071X_MISC_DEVICE
    err = misc_register(&em3071x_device);
    if(err)
    {
        APS_ERR("em3071x_device register failed\n");
        goto exit_misc_device_register_failed;
    }
#endif
    err = alsps_factory_device_register(&em3071x_factory_device);
    if (err) {
        APS_ERR("em3071x_device register failed\n");
        goto exit_misc_device_register_failed;
    }
    als_ctl.is_use_common_factory = false;
    ps_ctl.is_use_common_factory = false;
    APS_LOG("em3071x_device misc_register OK!\n");

    /*------------------------em3071x attribute file for debug--------------------------------------*/
    err = em3071x_create_attr(&(em3071x_init_info.platform_diver_addr->driver));
    if(err)
    {
        goto exit_create_attr_failed;
    }
    als_ctl.open_report_data = als_open_report_data;
    als_ctl.enable_nodata = als_enable_nodata;
    als_ctl.set_delay = als_set_delay;
    als_ctl.batch = als_batch;
    als_ctl.flush = als_flush;
    als_ctl.is_report_input_direct = false;
    als_ctl.is_support_batch = false;

    err = als_register_control_path(&als_ctl);
    if (err) {
        APS_ERR("register fail = %d\n", err);
        goto exit;
    }

    als_data.get_data = als_get_data;
    als_data.vender_div = 100;
    err = als_register_data_path(&als_data);
    if (err) {
        APS_ERR("tregister fail = %d\n", err);
        goto exit;
    }

    ps_ctl.open_report_data = ps_open_report_data;
    ps_ctl.enable_nodata = ps_enable_nodata;
    ps_ctl.set_delay = ps_set_delay;
    ps_ctl.batch = ps_batch;
    ps_ctl.flush = ps_flush;
    ps_ctl.is_report_input_direct = false;
    ps_ctl.is_support_batch = false;
    ps_ctl.is_polling_mode = obj->hw->polling_mode_ps;

    err = ps_register_control_path(&ps_ctl);
    if (err) {
        APS_ERR("register fail = %d\n", err);
        goto exit;
    }

    ps_data.get_data = ps_get_data;
    ps_data.vender_div = 100;
    err = ps_register_data_path(&ps_data);
    if (err) {
        APS_ERR("tregister fail = %d\n", err);
        goto exit;
    }
/*
    err = batch_register_support_info(ID_LIGHT, als_ctl.is_support_batch, 1, 0);
    if (err)
        APS_ERR("register light batch support err = %d\n", err);

    err = batch_register_support_info(ID_PROXIMITY, ps_ctl.is_support_batch, 1, 0);
    if (err)
        APS_ERR("register proximity batch support err = %d\n", err); */

//Gionee BSP1 yaoyc 20151215 add for em3071x end

#if defined(CONFIG_HAS_EARLYSUSPEND)
    obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
    obj->early_drv.suspend  = em3071x_early_suspend,
    obj->early_drv.resume   = em3071x_late_resume,
    register_early_suspend(&obj->early_drv);
#endif
    em3071x_init_flag = 0;
    //printk("em3071x probe %s OK\n",__func__);
    APS_LOG("%s: OK\n", __func__);
    return 0;

    exit_create_attr_failed:
    /*misc_deregister(&em3071x_device);*/
    exit_misc_device_register_failed:
    exit_init_failed:
    exit:
    em3071x_i2c_client = NULL;
    em3071x_init_flag = -1;
//    MT6516_EINTIRQMask(CUST_EINT_ALS_NUM);  /*mask interrupt if fail*/


    APS_ERR("%s: err = %d\n", __func__, err);
    return err;
}
/*----------------------------------------------------------------------------*/
static int em3071x_i2c_remove(struct i2c_client *client)
{
    int err;
    err = em3071x_delete_attr(&em3071x_alsps_driver.driver);
    if(err)
    {
        APS_ERR("em3071x_delete_attr fail: %d\n", err);
    }

    /*misc_deregister(&em3071x_device);*/

    em3071x_i2c_client = NULL;
    i2c_unregister_device(client);
    kfree(i2c_get_clientdata(client));

    return 0;
}
/*----------------------------------------------------------------------------*/
#ifdef EM3071X_MISC_DEVICE

static int em3071x_probe(struct platform_device *pdev) 
{
    struct alsps_hw *hw = get_cust_alsps();

    em3071x_power(hw, 1);
    if(i2c_add_driver(&em3071x_i2c_driver))
    {
        APS_ERR("add driver error\n");
        return -1;
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
static int em3071x_remove(void)
{
    struct alsps_hw *hw = get_cust_alsps();
    APS_FUN();
    em3071x_power(hw, 0);
    i2c_del_driver(&em3071x_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/

static struct platform_driver em3071x_alsps_driver = {
    .probe      = em3071x_probe,
    .remove     = em3071x_remove,
    .driver     = {
        .name  = "als_ps",
        .owner = THIS_MODULE,
    }
};
#endif
/*----------------------------------------------------------------------------*/

static int em3071x_local_init(void) 
{
    //struct alsps_hw *hw = get_cust_alsps();
    //struct em3071x_i2c_addr addr;

    if(i2c_add_driver(&em3071x_i2c_driver))
    {
        APS_ERR("add driver error\n");
        return -1;
    }
    if(-1 == em3071x_init_flag)
    {
        return -1;
    }

    return 0;
}

static int em3071x_local_uninit(void)
{
    //struct alsps_hw *hw = get_cust_alsps();
    APS_FUN();
    //em3071x_power(hw, 0);
    i2c_del_driver(&em3071x_i2c_driver);
    em3071x_i2c_client = NULL;
    return 0;
}

static int __init em3071x_init(void)
{
    //const char *name = "mediatek,em3071";
    //printk("%s",__func__);
    /*
    struct device_node *np = NULL;
    np = of_find_compatible_node(NULL, NULL, "mediatek,em3071");
    hw =   get_alsps_dts_func(np, hw);
    if (!hw)
        APS_ERR("get dts info fail\n");
    */
    //APS_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num);

    //i2c_register_board_info(hw->i2c_num, &i2c_em3071x, 1);
    alsps_driver_add(&em3071x_init_info);

    return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit em3071x_exit(void)
{
    APS_FUN();
    //platform_driver_unregister(&em3071x_alsps_driver);
}
/*----------------------------------------------------------------------------*/
module_init(em3071x_init);
module_exit(em3071x_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("binghua chen@epticore.com");
MODULE_DESCRIPTION("em3071x driver");
MODULE_LICENSE("GPL");
