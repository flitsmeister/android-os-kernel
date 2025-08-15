/*
 * Copyright (C) 2021 mid Inc.
 */
#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/power_supply.h>
#include <linux/pm_wakeup.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/scatterlist.h>
#include <linux/suspend.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/reboot.h>
#include <linux/iio/consumer.h>
#include <linux/iio/iio.h>

#if !defined(MTK_AUXADC_IRQ_SUPPORT)
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#endif
#include <linux/input.h>
#include "linux/sched.h"

#define STATUS_OK    0
#define STATUS_FAIL  1

#define MID_DC_DET_DEBUG  1

#ifdef MID_DC_DET_DEBUG
    #define MID_DC_TAG "[mid_dc_det]"
    #define DC_DET_DEBUG(fmt, ...) printk(KERN_ERR MID_DC_TAG fmt, ##__VA_ARGS__)
#else
    #define DC_DET_DEBUG(fmt, ...)
#endif
struct adc_data {
    struct iio_channel *typec_channel;
    struct iio_channel *dc_channel;

	struct device *dev;
	struct gpio_desc *dc_gpiod;
	struct gpio_desc *typec_gpiod;

	struct delayed_work chg_det_work;
	DEFINE_MUTEX(dc_det_mutex);
	DECLARE_WAIT_QUEUE_HEAD(chg_det_thread_wq);
	struct hrtimer chg_det_kthread_timer;
	bool chg_det_thread_timeout = false;

	int plug_flag;
};

static struct adc_data *g_adc_data = NULL;

// 全局接口：读取AC通道的值
int read_typec_channel_value(int *value)
{
    if (!g_adc_data || !g_adc_data->typec_channel)
        return -ENODEV;
	DC_DET_DEBUG("success to get Typec channel: %s:%d\n", __func__,__LINE__);
    return iio_read_channel_processed(g_adc_data->typec_channel, value);
}
EXPORT_SYMBOL(read_typec_channel_value);

// 全局接口：读取DC通道的值
int read_dc_channel_value(int *value)
{
    if (!g_adc_data || !g_adc_data->dc_channel)
        return -ENODEV;
	DC_DET_DEBUG("success to get Dc channel: %s:%d\n", __func__,__LINE__);
    return iio_read_channel_processed(g_adc_data->dc_channel, value);
}
EXPORT_SYMBOL(read_dc_channel_value);

static void chg_det_handler(struct work_struct *work)
{
	struct adc_data *data = container_of(to_delayed_work(work),
						    struct adc_data,
						    dc_detcable);
	int dc_pin, typec_pin;

	dc_pin = data->dc_gpiod ?
		gpiod_get_value_cansleep(data->dc_gpiod) : 1;

	if (dc_pin) {
		data->plug_flag &= ~0x2;
		DC_DET_DEBUG("dc_pin:%d is disconnect, plug_flag = %d \n", dc_pin, data->plug_flag);
	} else {
		data->plug_flag |= 0x2;
		DC_DET_DEBUG("dc_pin:%d is connect ~~~~plug_flag = %d \n", dc_pin, data->plug_flag);
	}

	typec_pin = data->typec_gpiod ?
		gpiod_get_value_cansleep(data->typec_gpiod) : 1;

	if (typec_pin) {
		data->plug_flag &= ~0x1;
		DC_DET_DEBUG("typec_pin:%d is disconnect, plug_flag = %d \n", typec_pin, data->plug_flag);
	} else {
		data->plug_flag |= 0x1;
		DC_DET_DEBUG("typec_pin:%d is connect ~~~~plug_flag = %d \n", typec_pin, data->plug_flag);
	}
}


enum hrtimer_restart chg_det_kthread_hrtimer_func(struct hrtimer *timer)
{
	struct adc_data *data = container_of(timer,
							struct adc_data,
							chg_det_kthread_timer);

	data->chg_det_thread_timeout = true;
	wake_up(&data->chg_det_thread_wq);

	return HRTIMER_NORESTART;
}

int dc_det_thread_kthread(void *x)
{
	while (1) {
		wait_event(g_adc_data->chg_det_thread_wq, (g_adc_data->chg_det_thread_timeout = true));
		g_adc_data->chg_det_thread_timeout = false;
		mutex_lock(&g_adc_data->dc_det_mutex);
	}
}

static int chg_det_pin_init(struct adc_data *data)
{
	int ret = 0;
	int dc_pin;

	data->dc_gpiod = devm_gpiod_get(data->dev, "dc_det", GPIOD_IN);
	if (!data->dc_gpiod || IS_ERR(data->dc_gpiod)) {
		DC_DET_DEBUG("Failed to get dc_det pin: %d\n", ret);
		data->dc_gpiod = NULL;
		return -EINVAL;
	}

	data->typec_gpiod = devm_gpiod_get(data->dev, "typec_det", GPIOD_IN);
	if (!data->typec_gpiod || IS_ERR(data->typec_gpiod)) {
		DC_DET_DEBUG("Failed to get typec_det pin: %d\n", ret);
		data->typec_gpiod = NULL;
		return -EINVAL;
	}

	hrtimer_init(&data->chg_det_kthread_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->chg_det_kthread_timer.function = chg_det_kthread_hrtimer_func;
	INIT_DELAYED_WORK(&data->chg_det_work, chg_det_handler);

	kthread_run(dc_det_thread_kthread, NULL, "dc_det_thread_kthread");

	return ret;
}



static int adc_probe(struct platform_device *pdev)
{
    struct adc_data *data;
    int ret;

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
	}
	data->dev = dev;

    // 获取AC通道
    data->typec_channel = devm_iio_channel_get(&pdev->dev, "typec_channel");
    if (IS_ERR(data->typec_channel)) {
        ret = PTR_ERR(data->typec_channel);
        dev_err(&pdev->dev, "Failed to get AC channel: %d\n", ret);
        return ret;
    }

    // 获取DC通道
    data->dc_channel = devm_iio_channel_get(&pdev->dev, "dc_channel");
    if (IS_ERR(data->dc_channel)) {
        ret = PTR_ERR(data->dc_channel);
        dev_err(&pdev->dev, "Failed to get DC channel: %d\n", ret);
        return ret;
    }

	ret = chg_det_pin_init(data);
	if (ret < 0) 
		DC_DET_DEBUG("failed to init dc det pin: %d\n", ret);

    // 将数据保存到全局变量
    g_adc_data = data;

    dev_info(&pdev->dev, "ADC driver probed successfully\n");

    return 0;
}

static int adc_remove(struct platform_device *pdev)
{
    g_adc_data = NULL;
    dev_info(&pdev->dev, "ADC driver removed\n");
    return 0;
}

static const struct of_device_id adc_of_match[] = {
    { .compatible = "mediatek,dc_usb_auxadc", },
    { }
};
MODULE_DEVICE_TABLE(of, adc_of_match);

static struct platform_driver adc_driver = {
    .probe = adc_probe,
    .remove = adc_remove,
    .driver = {
        .name = "adc_example",
        .of_match_table = adc_of_match,
    },
};

module_platform_driver(adc_driver);

/* module_init(mid_dc_auxadc_init);
module_exit(mid_dc_auxadc_exit); */

MODULE_AUTHOR("kuangzenghui");
MODULE_DESCRIPTION("mid dc auxadc driver");
MODULE_LICENSE("GPL");
