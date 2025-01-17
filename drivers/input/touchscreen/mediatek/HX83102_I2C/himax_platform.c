/* SPDX-License-Identifier: GPL-2.0 */
/*  Himax Android Driver Sample Code for MTK kernel 4.4 platform
 *
 *  Copyright (C) 2019 Himax Corporation.
 *
 *  This software is licensed under the terms of the GNU General Public
 *  License version 2,  as published by the Free Software Foundation,  and
 *  may be copied,  distributed,  and modified under those terms.
 *
 *  This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include "himax_platform.h"
#include "himax_common.h"

int i2c_error_count;
bool ic_boot_done;

const struct of_device_id himax_match_table[] = {
//	{.compatible = "mediatek,cap_touch" },
//	{.compatible = "mediatek,touch" },
	{.compatible = "himax,hxcommon" },
	{},
};
MODULE_DEVICE_TABLE(of, himax_match_table);

static int himax_tpd_int_gpio = 5;
unsigned int himax_touch_irq;
unsigned int himax_tpd_rst_gpio_number = -1;
unsigned int himax_tpd_int_gpio_number = -1;
struct device *g_device;

static unsigned short force[] = {0, 0x90, I2C_CLIENT_END, I2C_CLIENT_END};
static const unsigned short *const forces[] = { force, NULL };

#if defined(MTK_I2C_DMA)
u8 *gpDMABuf_va;
u8 *gpDMABuf_pa;
#else
u8 *gp_rw_buf;
#endif

/* Custom set some config  [1]=X resolution, [3]=Y resolution */
//static int hx_panel_coords[4] = {0, 800, 0, 1340};
//static int hx_display_coords[4] = {0, 800, 0, 1340};
static int report_type = PROTOCOL_TYPE_B;

struct i2c_client *i2c_client_point;

void (*kp_tpd_gpio_as_int)(int gpio);
int (*kp_tpd_driver_add)(struct tpd_driver_t *drv);
void (*kp_tpd_get_dts_info)(void);
void (*kp_tpd_gpio_output)(int pinnum, int value);
int (*kp_tpd_driver_remove)(struct tpd_driver_t *drv);
const struct of_device_id *kp_touch_of_match;
struct tpd_device **kp_tpd;

#if !defined(HX_USE_KSYM)
#define setup_symbol(sym) ({kp_##sym = &(sym); kp_##sym; })
#define setup_symbol_func(sym) ({kp_##sym = (sym); kp_##sym; })
#else
#define setup_symbol(sym) ({kp_##sym = \
	(void *)kallsyms_lookup_name(#sym); kp_##sym; })
#define setup_symbol_func(sym) setup_symbol(sym)
#endif
#define assert_on_symbol(sym) \
		do {\
			if (!setup_symbol(sym)) {\
				E("%s: setup %s failed!\n", __func__, #sym);\
				ret = -1;\
			} \
		} while (0)
#define assert_on_symbol_func(sym) \
		do {\
			if (!setup_symbol_func(sym)) {\
				E("%s: setup %s failed!\n", __func__, #sym);\
				ret = -1;\
			} \
		} while (0)

int32_t setup_tpd_vars(void)
{
	int32_t ret = 0;

	assert_on_symbol_func(tpd_gpio_as_int);
	assert_on_symbol_func(tpd_driver_add);
	assert_on_symbol_func(tpd_get_dts_info);
	assert_on_symbol_func(tpd_gpio_output);
	assert_on_symbol_func(tpd_driver_remove);
	kp_touch_of_match = (const struct of_device_id *)(&(touch_of_match[0]));
	assert_on_symbol(tpd);

	return ret;
}

int himax_dev_set(struct himax_ts_data *ts)
{
	int ret = 0;

	ts->input_dev = input_allocate_device();

	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		E("%s: Failed to allocate input device-input_dev\n", __func__);
		return ret;
	}

	ts->input_dev->name = "himax-touchscreen";

	if (!ic_data->HX_STYLUS_FUNC)
		goto skip_stylus_operation;

	ts->stylus_dev = input_allocate_device();

	if (ts->stylus_dev == NULL) {
		ret = -ENOMEM;
		E("%s: Failed to allocate input device-stylus_dev\n", __func__);
		input_free_device(ts->input_dev);
		return ret;
	}

	ts->stylus_dev->name = "himax-stylus";
skip_stylus_operation:

	return ret;
}

int himax_input_register_device(struct input_dev *input_dev)
{
	return input_register_device(input_dev);
}

int himax_parse_dt(struct himax_ts_data *ts, struct himax_platform_data *pdata)
{
	struct device_node *dt = ts->client->dev.of_node;
	struct i2c_client *client = ts->client;
	
	int rc ,ret,coords_size = 0;
	uint32_t coords[4] = {0};
	struct property *prop;

	if (dt) {
		const struct of_device_id *match;

		match = of_match_device(of_match_ptr(himax_match_table),
			&client->dev);

		if (!match) {
			I("[Himax]Error: No device match found\n");
			return -ENODEV;
		}
	}

	/* pdata->gpio_reset = of_get_named_gpio(dev->of_node, "rst-gpio", 0);
	 * pdata->gpio_irq = of_get_named_gpio(dev->of_node, "int-gpio", 0);
	 * I("pdata->gpio_reset: %d\n", pdata->gpio_reset );
	 * I("pdata->gpio_irq: %d\n", pdata->gpio_irq );
	 */
	/* himax_tpd_rst_gpio_number = of_get_named_gpio(dt, "rst-gpio", 0); */
	/* himax_tpd_int_gpio_number = of_get_named_gpio(dt, "int-gpio", 0); */
	/* It will be a non-zero and non-one value for MTK PINCTRL API */
	himax_tpd_rst_gpio_number = GTP_RST_PORT;
	himax_tpd_int_gpio_number = GTP_INT_PORT;
	pdata->gpio_reset	= himax_tpd_rst_gpio_number;
	pdata->gpio_irq		= himax_tpd_int_gpio_number;
	I("%s: int : %2.2x\n", __func__, pdata->gpio_irq);
	I("%s: rst : %2.2x\n", __func__, pdata->gpio_reset);
#if defined(HX_FIRMWARE_HEADER)
	mapping_panel_id_from_dt(dt);
#endif
	/* Set device tree data */
	
	#if 1
		prop = of_find_property(dt, "himax,panel-coords", NULL);
	if (prop) {
		coords_size = prop->length / sizeof(u32);
		if (coords_size != 4)
			D(" %s:Invalid panel coords size %d\n",
				__func__, coords_size);
	}

	ret = of_property_read_u32_array(dt, "himax,panel-coords",
			coords, coords_size);
	if (ret == 0) {
		pdata->abs_x_min = coords[0];
		pdata->abs_x_max = (coords[1] - 1);
		pdata->abs_y_min = coords[2];
		pdata->abs_y_max = (coords[3] - 1);
		I("kzhkzh  DT-%s:panel-coords = %d, %d, %d, %d\n", __func__,
				pdata->abs_x_min,
				pdata->abs_x_max,
				pdata->abs_y_min,
				pdata->abs_y_max);
	}

	prop = of_find_property(dt, "himax,display-coords", NULL);
	if (prop) {
		coords_size = prop->length / sizeof(u32);
		if (coords_size != 4)
			D(" %s:Invalid display coords size %d\n",
				__func__, coords_size);
	}

	rc = of_property_read_u32_array(dt, "himax,display-coords",
			coords, coords_size);
	if (rc && (rc != -EINVAL)) {
		D(" %s:Fail to read display-coords %d\n", __func__, rc);
		return rc;
	}
	pdata->screenWidth  = coords[1];
	pdata->screenHeight = coords[3];
	I(" kzhkzh DT-%s:display-coords = (%d, %d)\n", __func__,
			pdata->screenWidth,
			pdata->screenHeight);
	#endif
	
	
	/* Set panel coordinates */
/* 	pdata->abs_x_min = hx_panel_coords[0];
	pdata->abs_x_max = (hx_panel_coords[1] - 1);
	pdata->abs_y_min = hx_panel_coords[2];
	pdata->abs_y_max = (hx_panel_coords[3] - 1);
	I(" %s:panel-coords = %d, %d, %d, %d\n", __func__,
			pdata->abs_x_min,
			pdata->abs_x_max,
			pdata->abs_y_min,
			pdata->abs_y_max);
	pdata->screenWidth  = hx_display_coords[1];
	pdata->screenHeight = hx_display_coords[3];
	I(" %s:display-coords = (%d, %d)\n", __func__,
			pdata->screenWidth,
			pdata->screenHeight); */
	/* report type */
	pdata->protocol_type = report_type;
	return 0;
}
EXPORT_SYMBOL(himax_parse_dt);

#if defined(MTK_I2C_DMA)
int himax_bus_read(uint8_t cmd, uint8_t *buf, uint32_t len)
{
	int ret = -1;
	s32 retry = 0;
	u8 buffer[1];
	struct i2c_client *client = private_ts->client;
	struct i2c_msg msg[] = {
		{
			.addr = (client->addr & I2C_MASK_FLAG),
			.flags = 0,
			.buf = buffer,
			.len = 1,
			.timing = 400
		},
		{
			.addr = (client->addr & I2C_MASK_FLAG),
			.ext_flag = (client->ext_flag
				| I2C_ENEXT_FLAG
				| I2C_DMA_FLAG),
			.flags = I2C_M_RD,
			.buf = gpDMABuf_pa,
			.len = len,
			.timing = 400
		},
	};

	if (len > BUS_R_DLEN) {
		E("%s: len[%d] is over %d\n", __func__, len, BUS_R_DLEN);
		return -EFAULT;
	}

	if (buf == NULL) {
		E("%s: buf is NULL\n", __func__);
		return -EFAULT;
	}

	mutex_lock(&private_ts->rw_lock);
	buffer[0] = cmd;

	for (retry = 0; retry < HIMAX_BUS_RETRY_TIMES; ++retry) {
		ret = i2c_transfer(client->adapter, &msg[0], 2);
		if (ret < 0)
			continue;

		memcpy(buf, gpDMABuf_va, len);
		mutex_unlock(&private_ts->rw_lock);
		return 0;
	}

	E("Dma I2C Read Error: %d byte(s), err-code: %d\n", len, ret);
	i2c_error_count = HIMAX_BUS_RETRY_TIMES;
	mutex_unlock(&private_ts->rw_lock);
	return ret;
}
EXPORT_SYMBOL(himax_bus_read);

int himax_bus_write(uint8_t cmd, uint8_t *addr, uint8_t *data, uint32_t len)
{
	int rc = 0, retry = 0;
	uint8_t offset = 0;
	uint32_t tmp_len = len;
	struct i2c_client *client = private_ts->client;
	struct i2c_msg msg[] = {
		{
			.addr = (client->addr & I2C_MASK_FLAG),
			.ext_flag = (client->ext_flag
				| I2C_ENEXT_FLAG
				| I2C_DMA_FLAG),
			.flags = 0,
			.buf = gpDMABuf_pa,
			.len = len+BUS_W_HLEN,
			.timing = 400
		},
	};

	if (len > BUS_W_DLEN) {
		E("%s: len[%d] is over %d\n", __func__, len, BUS_W_DLEN);
		return -EFAULT;
	}

	mutex_lock(&private_ts->rw_lock);

	gpDMABuf_va[0] = cmd;
	offset = BUS_W_HLEN;

	if (addr != NULL) {
		memcpy(gpDMABuf_va+offset, addr, 4);
		offset += 4;
		tmp_len -= 4;
	}

	if (data != NULL)
		memcpy(gpDMABuf_va+offset, data, tmp_len);

	for (retry = 0; retry < HIMAX_BUS_RETRY_TIMES; ++retry) {
		rc = i2c_transfer(client->adapter, &msg[0], 1);

		if (rc < 0)
			continue;

		mutex_unlock(&private_ts->rw_lock);
		return 0;
	}

	E("Dma I2C master write Error: %d byte(s), err-code: %d\n", len, rc);
	i2c_error_count = HIMAX_BUS_RETRY_TIMES;
	mutex_unlock(&private_ts->rw_lock);
	return rc;
}
EXPORT_SYMBOL(himax_bus_write);


#else
int himax_bus_read(uint8_t cmd, uint8_t *buf, uint32_t len)
{
	int retry;
	struct i2c_client *client = private_ts->client;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &cmd,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = gp_rw_buf,
		}
	};

	if (len > BUS_R_DLEN) {
		E("%s: len[%d] is over %d\n", __func__, len, BUS_R_DLEN);
		return -EFAULT;
	}

	mutex_lock(&private_ts->rw_lock);

	for (retry = 0; retry < HIMAX_BUS_RETRY_TIMES; retry++) {
		if (i2c_transfer(client->adapter, msg, 2) == 2) {
			memcpy(buf, gp_rw_buf, len);
			break;
		}
		/*msleep(10);*/
	}

	if (retry == HIMAX_BUS_RETRY_TIMES) {
		E("%s: i2c_read_block retry over %d\n",
		  __func__, HIMAX_BUS_RETRY_TIMES);
		i2c_error_count = HIMAX_BUS_RETRY_TIMES;
		mutex_unlock(&private_ts->rw_lock);
		return -EIO;
	}

	mutex_unlock(&private_ts->rw_lock);
	return 0;
}
EXPORT_SYMBOL(himax_bus_read);

int himax_bus_write(uint8_t cmd, uint8_t *addr, uint8_t *data, uint32_t len)
{
	int retry/*, loop_i*/;
	uint8_t offset = 0;
	uint32_t tmp_len = len;
	struct i2c_client *client = private_ts->client;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = len+BUS_W_HLEN,
			.buf = gp_rw_buf,
		}
	};

	if (len > BUS_W_DLEN) {
		E("%s: len[%d] is over %d\n", __func__, len, BUS_W_DLEN);
		return -EFAULT;
	}

	mutex_lock(&private_ts->rw_lock);

	gp_rw_buf[0] = cmd;
	offset = BUS_W_HLEN;

	if (addr != NULL) {
		memcpy(gp_rw_buf+offset, addr, 4);
		offset += 4;
		tmp_len -= 4;
	}

	if (data != NULL)
		memcpy(gp_rw_buf+offset, data, tmp_len);

	for (retry = 0; retry < HIMAX_BUS_RETRY_TIMES; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;

		/*msleep(10);*/
	}

	if (retry == HIMAX_BUS_RETRY_TIMES) {
		E("%s: i2c_write_block retry over %d\n",
		  __func__, HIMAX_BUS_RETRY_TIMES);
		i2c_error_count = HIMAX_BUS_RETRY_TIMES;
		mutex_unlock(&private_ts->rw_lock);
		return -EIO;
	}

	mutex_unlock(&private_ts->rw_lock);
	return 0;
}
EXPORT_SYMBOL(himax_bus_write);

#endif

uint8_t himax_int_gpio_read(int pinnum)
{
	return  gpio_get_value(himax_tpd_int_gpio);
}

void himax_int_enable(int enable)
{
	struct himax_ts_data *ts = private_ts;
	unsigned long irqflags = 0;
	int irqnum = ts->client->irq;

	spin_lock_irqsave(&ts->irq_lock, irqflags);
	I("%s: Entering!\n", __func__);
	if (enable == 1 && atomic_read(&ts->irq_state) == 0) {
		atomic_set(&ts->irq_state, 1);
		enable_irq(irqnum);
		private_ts->irq_enabled = 1;
	} else if (enable == 0 && atomic_read(&ts->irq_state) == 1) {
		atomic_set(&ts->irq_state, 0);
		disable_irq_nosync(irqnum);
		private_ts->irq_enabled = 0;
	}
	I("enable = %d\n", enable);
	spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}
EXPORT_SYMBOL(himax_int_enable);

#if defined(HX_RST_PIN_FUNC)
void himax_rst_gpio_set(int pinnum, uint8_t value)
{
	if (value)
		kp_tpd_gpio_output(himax_tpd_rst_gpio_number, 1);
	else
		kp_tpd_gpio_output(himax_tpd_rst_gpio_number, 0);
}
EXPORT_SYMBOL(himax_rst_gpio_set);
#endif

int himax_gpio_power_config(struct himax_platform_data *pdata)
{
/* 	int error = 0;

	error = regulator_enable((*kp_tpd)->reg);

	if (error != 0)
		I("Failed to enable reg-vgp6: %d\n", error); */

//	msleep(100);
#if defined(HX_RST_PIN_FUNC)
	kp_tpd_gpio_output(himax_tpd_rst_gpio_number, 1);
	msleep(20);
	kp_tpd_gpio_output(himax_tpd_rst_gpio_number, 0);
	msleep(20);
	kp_tpd_gpio_output(himax_tpd_rst_gpio_number, 1);
#endif
	I("mtk_tpd: himax reset over\n");
	/* set INT mode */
	kp_tpd_gpio_as_int(himax_tpd_int_gpio_number);
	return 0;
}

void himax_gpio_power_deconfig(struct himax_platform_data *pdata)
{
/* 	int error = 0;

	error = regulator_disable(tpd->reg);

	if (error != 0)
		I("Failed to disable reg-vgp6: %d\n", error);

	regulator_put(tpd->reg);
	I("%s: regulator put, completed.\n", __func__); */
}

static void himax_ts_isr_func(struct himax_ts_data *ts)
{
	himax_ts_work(ts);
}

irqreturn_t himax_ts_thread(int irq, void *ptr)
{
	himax_ts_isr_func((struct himax_ts_data *)ptr);

	return IRQ_HANDLED;
}

static void himax_ts_work_func(struct work_struct *work)
{
	struct himax_ts_data *ts = container_of(work,
		struct himax_ts_data, work);

	himax_ts_work(ts);
}

int himax_int_register_trigger(void)
{
	int ret = NO_ERR;
	struct himax_ts_data *ts = private_ts;
	struct i2c_client *client = private_ts->client;

	if (ic_data->HX_INT_IS_EDGE) {
		ret = request_threaded_irq(client->irq, NULL, himax_ts_thread,
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->name, ts);
	} else {
		ret = request_threaded_irq(client->irq, NULL, himax_ts_thread,
			IRQF_TRIGGER_LOW | IRQF_ONESHOT, client->name, ts);
	}

	return ret;
}

int himax_int_en_set(void)
{
	int ret = NO_ERR;

	ret = himax_int_register_trigger();
	return ret;
}
EXPORT_SYMBOL(himax_int_en_set);

int himax_ts_register_interrupt(void)
{
	struct himax_ts_data *ts = private_ts;
	struct i2c_client *client = private_ts->client;
	struct device_node *node = NULL;
	u32 ints[2] = {0, 0};
	int ret = 0;

	node = of_find_matching_node(node, (kp_touch_of_match));

	if (node) {
		if (of_property_read_u32_array(node, "debounce",
			ints, ARRAY_SIZE(ints)) == 0) {
			I("%s: Now it has set debounce\n", __func__);
			gpio_set_debounce(ints[0], ints[1]);
			I("%s: Now debounce are ints[0] = %d, ints[1] = %d\n",
				__func__, ints[0], ints[1]);
		} else {
			I("%s: DOES NOT set debounce!\n", __func__);
		}
		himax_touch_irq = irq_of_parse_and_map(node, 0);
		I("himax_touch_irq=%ud\n", himax_touch_irq);
		client->irq = himax_touch_irq;
		ts->client->irq = himax_touch_irq;
	} else {
		I("[%s] tpd request_irq can not find touch eint device node!\n",
				__func__);
	}

	ts->irq_enabled = 0;
	ts->use_irq = 0;

	/* Work functon */
	if (client->irq) {/*INT mode*/
		ts->use_irq = 1;
		ret = himax_int_register_trigger();

		if (ret == 0) {
			ts->irq_enabled = 1;
			atomic_set(&ts->irq_state, 1);
			I("%s: irq enabled at qpio: %d\n",
				__func__, client->irq);
#if defined(HX_SMART_WAKEUP)
			irq_set_irq_wake(client->irq, 1);
#endif
		} else {
			ts->use_irq = 0;
			E("%s: request_irq failed\n", __func__);
		}
	} else {
		I("%s: client->irq is empty, use polling mode.\n", __func__);
	}

	/*if use polling mode need to disable HX_ESD_RECOVERY function*/
	if (!ts->use_irq) {
		ts->himax_wq = create_singlethread_workqueue("himax_touch");
		INIT_WORK(&ts->work, himax_ts_work_func);
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = himax_ts_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
		I("%s: polling mode enabled\n", __func__);
	}
	tpd_load_status = 1;
	return ret;
}

int himax_ts_unregister_interrupt(void)
{
	struct himax_ts_data *ts = private_ts;
	int ret = 0;

	I("%s: entered.\n", __func__);

	/* Work functon */
	if (private_ts->hx_irq && ts->use_irq) {/*INT mode*/
#if defined(HX_SMART_WAKEUP)
		irq_set_irq_wake(ts->hx_irq, 0);
#endif
		free_irq(ts->hx_irq, ts);
		I("%s: irq disabled at gpio: %d\n", __func__,
			private_ts->hx_irq);
	}

	/*if use polling mode need to disable HX_ESD_RECOVERY function*/
	if (!ts->use_irq) {
		hrtimer_cancel(&ts->timer);
		cancel_work_sync(&ts->work);
		if (ts->himax_wq != NULL)
			destroy_workqueue(ts->himax_wq);
		I("%s: polling mode destroyed", __func__);
	}

	return ret;
}

int himax_common_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct himax_ts_data *ts;
	int ret = 0;

#if defined(MTK_I2C_DMA)
	client->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	gpDMABuf_va = (u8 *)dma_alloc_coherent(&client->dev, BUS_RW_MAX_LEN,
		(dma_addr_t *)&gpDMABuf_pa, GFP_KERNEL);

	if (!gpDMABuf_va) {
		E("Allocate DMA I2C Buffer failed\n");
		ret = -ENODEV;
		goto err_alloc_rw_buf_failed;
	}

	memset(gpDMABuf_va, 0, BUS_RW_MAX_LEN);
#else
	gp_rw_buf = kcalloc(BUS_RW_MAX_LEN, sizeof(uint8_t), GFP_KERNEL);
	if (!gp_rw_buf) {
		E("Allocate I2C RW Buffer failed\n");
		ret = -ENODEV;
		goto err_alloc_rw_buf_failed;
	}
#endif
	/* Check I2C functionality */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		E("%s: i2c check functionality error\n", __func__);
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(struct himax_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		E("%s: allocate himax_ts_data failed\n", __func__);
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	client->addr = 0x48;

	i2c_set_clientdata(client, ts);
	i2c_client_point = client;
	ts->client = client;
	ts->dev = &client->dev;
	g_device = &client->dev;
	private_ts = ts;
	mutex_init(&ts->rw_lock);
	mutex_init(&ts->reg_lock);

	ret = himax_chip_common_init();
	if (ret < 0)
		goto err_common_init_failed;

	return ret;

err_common_init_failed:
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:

	if (ret) {
#if defined(MTK_I2C_DMA)
		if (gpDMABuf_va) {
			dma_free_coherent(&client->dev, BUS_RW_MAX_LEN,
				gpDMABuf_va, (dma_addr_t)gpDMABuf_pa);
			gpDMABuf_va = NULL;
			gpDMABuf_pa = NULL;
		}
#else
		kfree(gp_rw_buf);
#endif
	}

err_alloc_rw_buf_failed:

	return ret;
}


int himax_common_remove(struct i2c_client *client)
{
	int ret = 0;

	if (g_hx_chip_inited)
		himax_chip_common_deinit();

#if defined(MTK_I2C_DMA)
	if (gpDMABuf_va) {
		dma_free_coherent(&client->dev, BUS_RW_MAX_LEN, gpDMABuf_va,
			(dma_addr_t)gpDMABuf_pa);
		gpDMABuf_va = NULL;
		gpDMABuf_pa = NULL;
	}
#else
		kfree(gp_rw_buf);
#endif

	return ret;
}

static void himax_common_suspend(struct device *dev)
{
	struct himax_ts_data *ts = dev_get_drvdata(&i2c_client_point->dev);

	I("%s: enter\n", __func__);
	himax_chip_common_suspend(ts);
	I("%s: END\n", __func__);
}

static void himax_common_resume(struct device *dev)
{
	struct himax_ts_data *ts = dev_get_drvdata(&i2c_client_point->dev);

	I("%s: enter\n", __func__);
	himax_chip_common_resume(ts);
	I("%s: END\n", __func__);
}


#if defined(HX_CONFIG_FB)
int fb_notifier_callback(struct notifier_block *self,
		unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	struct himax_ts_data *ts =
	    container_of(self, struct himax_ts_data, fb_notif);
	I(" %s\n", __func__);

	if (ic_boot_done != 1) {
		E("%s: IC is booting\n", __func__);
		return -ECANCELED;
	}

	if (evdata && evdata->data && event == FB_EVENT_BLANK && ts &&
	    ts->client) {
		blank = evdata->data;

		switch (*blank) {
		case FB_BLANK_UNBLANK:
			himax_common_resume(&ts->client->dev);
			break;

		case FB_BLANK_POWERDOWN:
		case FB_BLANK_HSYNC_SUSPEND:
		case FB_BLANK_VSYNC_SUSPEND:
		case FB_BLANK_NORMAL:
			himax_common_suspend(&ts->client->dev);
			break;
		}
	}

	return 0;
}
#endif

static int himax_common_detect(struct i2c_client *client,
		struct i2c_board_info *info)
{
	strlcpy(info->type, TPD_DEVICE, sizeof(info->type));
	return 0;
}

static const struct i2c_device_id himax_common_ts_id[] = {
	{HIMAX_common_NAME, 0 },
	{}
};

static struct i2c_driver tpd_i2c_driver = {
	.probe = himax_common_probe,
	.remove = himax_common_remove,
	.detect = himax_common_detect,
	.driver	= {
		.name = HIMAX_common_NAME,
		.of_match_table = of_match_ptr(himax_match_table),
	},
	.id_table = himax_common_ts_id,
	.address_list = (const unsigned short *) forces,
};

static int himax_common_local_init(void)
{
	//int retval;

	I("[Himax] Himax_ts I2C Touchscreen Driver local init\n");
	/*(*kp_tpd)->reg = regulator_get((*kp_tpd)->tpd_dev, "vtouch");
	retval = regulator_set_voltage((*kp_tpd)->reg, 2800000, 2800000);

	if (retval != 0)
		E("Failed to set voltage 2V8: %d\n", retval); */

	if (i2c_add_driver(&tpd_i2c_driver) != 0) {
		I("unable to add i2c driver.\n");
		return -EFAULT;
	}

	/* input_set_abs_params((*kp_tpd)->input_dev,*/
	/*		ABS_MT_TRACKING_ID, 0, (HIMAX_MAX_TOUCH-1), 0, 0); */
	/*  set vendor string */
	/* client->input_devid.vendor = 0x00;
	 * client->input_dev->id.product = tpd_info.pid;
	 * client-->input_dev->id.version = tpd_info.vid;
	 */
	I("end %s.\n", __func__);
	tpd_type_cap = 1;
	return 0;
}

static struct tpd_driver_t tpd_device_driver = {
	.tpd_device_name = HIMAX_common_NAME,
	.tpd_local_init = himax_common_local_init,
	.suspend = himax_common_suspend,
	.resume = himax_common_resume,
#if defined(TPD_HAVE_BUTTON)
	.tpd_have_button = 1,
#else
	.tpd_have_button = 0,
#endif
};

static int __init himax_common_init(void)
{
	I("Himax_common touch panel driver init\n");
	D("Himax check double loading\n");
	if (g_mmi_refcnt++ > 0) {
		I("Himax driver has been loaded! ignoring....\n");
		return 0;
	}
	if (setup_tpd_vars() != 0) {
		E("Failed to get tpd variables!\n");
		goto ERR;
	}
	kp_tpd_get_dts_info();

	if (kp_tpd_driver_add(&tpd_device_driver) < 0) {
		I("Failed to add Driver!\n");
		goto ERR;
	}

	return 0;
ERR:
	return HX_INIT_FAIL;
}

static void __exit himax_common_exit(void)
{
	kp_tpd_driver_remove(&tpd_device_driver);
}

//#if defined(__HIMAX_MOD__)
module_init(himax_common_init);
//#else
//late_initcall(himax_common_init);
//#endif
module_exit(himax_common_exit);

MODULE_DESCRIPTION("Himax_common driver");
MODULE_LICENSE("GPL");

