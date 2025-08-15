#ifndef __STK83XX_MTK_I2C_H__
#define __STK83XX_MTK_I2C_H__
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/vmalloc.h>
#include <accel.h>
#include "cust_acc.h"
#include "sensors_io.h"

#include <stk83xx.h>
#include <common_define.h>

typedef struct stk83xx_wrapper
{
    struct i2c_manager      i2c_mgr;
    struct stk_data         stk;
    struct acc_hw               hw;
    struct hwmsen_convert       cvt;
} stk83xx_wrapper;

extern int stk_init_flag;
extern struct acc_init_info stk_acc_init_info;

int stk_i2c_probe(struct i2c_client* client,
                  struct common_function* common_fn);
int stk_i2c_remove(struct i2c_client* client);
int stk83xx_suspend(struct device* dev);
int stk83xx_resume(struct device* dev);

#endif // __STK83XX_MTK_I2C_H__
