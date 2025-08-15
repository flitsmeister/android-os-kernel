#include "jadard_platform.h"
#include "jadard_common.h"

DEFINE_MUTEX(jd_wr_mux);

MODULE_DEVICE_TABLE(of, jadard_match_table);
struct of_device_id jadard_match_table[] = {
	{.compatible = "jadard,jd9366tc" },
	{},
};

/* Custom set some config */
unsigned int jadard_touch_irq = 0;
unsigned int jadard_tpd_rst_gpio_number = -1;
unsigned int jadard_tpd_int_gpio_number = -1;

u8 *gpDMABuf_va = NULL;
u8 *gpDMABuf_pa = NULL;
struct i2c_client *i2c_client_point = NULL;

extern struct jadard_ts_data *pjadard_ts_data;
extern struct jadard_ic_data *pjadard_ic_data;

int jadard_dev_set(struct jadard_ts_data *ts)
{
	ts->input_dev = tpd->dev;
	return JD_NO_ERR;
}

int jadard_input_register_device(struct input_dev *input_dev)
{
	return JD_NO_ERR;
}

int jadard_parse_dt(struct jadard_ts_data *ts, struct jadard_i2c_platform_data *pdata)
{
	pjadard_ic_data->JD_MAX_PT = 10;
	pjadard_ic_data->JD_INT_EDGE = true;

	JD_I("DT:MAX_PT = %d, INT_IS_EDGE = %d\n", pjadard_ic_data->JD_MAX_PT,
		pjadard_ic_data->JD_INT_EDGE);

	pjadard_ic_data->JD_X_NUM = 22;
	pjadard_ic_data->JD_Y_NUM = 36;
	JD_I("DT:panel-sense-num = %d, %d\n",
		pjadard_ic_data->JD_X_NUM, pjadard_ic_data->JD_Y_NUM);

	pdata->abs_x_min = 0;
	pdata->abs_x_max = 800;
	pdata->abs_y_min = 0;
	pdata->abs_y_max = 1280;
	pjadard_ic_data->JD_X_RES = pdata->abs_x_max;
	pjadard_ic_data->JD_Y_RES = pdata->abs_y_max;
	JD_I("DT:panel-coords = %d, %d, %d, %d\n", pdata->abs_x_min,
		pdata->abs_x_max, pdata->abs_y_min, pdata->abs_y_max);

	jadard_tpd_rst_gpio_number = 0;//GTP_RST_PORT;
	jadard_tpd_int_gpio_number = 1;//GTP_INT_PORT;
	pdata->gpio_reset = jadard_tpd_rst_gpio_number;
	pdata->gpio_irq = jadard_tpd_int_gpio_number;
	JD_I("DT:gpio_irq = %d, gpio_rst = %d\n", pdata->gpio_irq, pdata->gpio_reset);

	return 0;
}

#ifdef MTK_I2C_DMA
int jadard_bus_read(uint8_t *cmd, uint8_t cmd_len, uint8_t *data, uint32_t data_len, uint8_t toRetry)
{
	int ret = 0;
	s32 retry = 0;
	uint8_t cmd_buf[cmd_len];
	struct i2c_client *client = pjadard_ts_data->client;
	struct i2c_msg msg[] = {
		{
			.addr = (client->addr & I2C_MASK_FLAG),
			.flags = 0,
			.buf = cmd_buf,
			.len = cmd_len,
			.timing = 400
		},
		{
			.addr = (client->addr & I2C_MASK_FLAG),
			.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
			.flags = I2C_M_RD,
			.buf = gpDMABuf_pa,
			.len = data_len,
			.timing = 400
		},
	};

	mutex_lock(&jd_wr_mux);

	memcpy(cmd_buf, cmd, cmd_len);

	if (data == NULL) {
		mutex_unlock(&jd_wr_mux);
		return -EFAULT;
	}

	for (retry = 0; retry < toRetry; ++retry) {
		ret = i2c_transfer(client->adapter, &msg[0], 2);

		if (ret < 0) {
			continue;
		}

		memcpy(data, gpDMABuf_va, data_len);
		mutex_unlock(&jd_wr_mux);
		return 0;
	}

	JD_E("Dma I2C Read Error: %d byte(s), err-code: %d\n", data_len, ret);

	mutex_unlock(&jd_wr_mux);

	return ret;
}

int jadard_bus_write(uint8_t *cmd, uint8_t cmd_len, uint8_t *data, uint32_t data_len, uint8_t toRetry)
{
	int rc = 0, retry = 0;
	u8 *pWriteData = gpDMABuf_va;
	struct i2c_client *client = pjadard_ts_data->client;
	struct i2c_msg msg[] = {
		{
			.addr = (client->addr & I2C_MASK_FLAG),
			.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
			.flags = 0,
			.buf = gpDMABuf_pa,
			.len = cmd_len + data_len,
			.timing = 400
		},
	};

	mutex_lock(&jd_wr_mux);

	if (!pWriteData) {
		JD_E("dma_alloc_coherent failed!\n");
		mutex_unlock(&jd_wr_mux);
		return -EFAULT;
	}

	memcpy(pWriteData, cmd, cmd_len);
	memcpy(pWriteData + cmd_len, data, data_len);

	for (retry = 0; retry < toRetry; ++retry) {
		rc = i2c_transfer(client->adapter, &msg[0], 1);

		if (rc < 0) {
			continue;
		}

		mutex_unlock(&jd_wr_mux);
		return 0;
	}

	JD_E("Dma I2C master write Error: %d byte(s), err-code: %d\n", cmd_len + data_len, rc);

	mutex_unlock(&jd_wr_mux);

	return rc;
}

#else
int jadard_bus_read(uint8_t *cmd, uint8_t cmd_len, uint8_t *data, uint32_t data_len, uint8_t toRetry)
{
	int retry;
	uint8_t cmd_buf[cmd_len];
	struct i2c_client *client = pjadard_ts_data->client;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = cmd_len,
			.buf = cmd_buf,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = data_len,
			.buf = data,
		}
	};

	memcpy(cmd_buf, cmd, cmd_len);

	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 2) == 2)
			break;

		msleep(10);
	}

	if (retry == toRetry) {
		JD_E("%s: i2c_read_block retry over %d\n",
		  __func__, toRetry);
		return -EIO;
	}

	return 0;
}

int jadard_bus_write(uint8_t *cmd, uint8_t cmd_len, uint8_t *data, uint32_t data_len, uint8_t toRetry)
{
	int retry;
	uint8_t buf[cmd_len + data_len];
	struct i2c_client *client = pjadard_ts_data->client;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = cmd_len + data_len,
			.buf = buf,
		}
	};

	memcpy(buf, cmd, cmd_len);
	memcpy(buf + cmd_len, data, data_len);

	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;

		msleep(10);
	}

	if (retry == toRetry) {
		JD_E("%s: i2c_write_block retry over %d\n",
		  __func__, toRetry);
		return -EIO;
	}

	return 0;
}
#endif

void jadard_int_enable(bool enable)
{
	int irqnum = pjadard_ts_data->client->irq;

	if (enable && (pjadard_ts_data->irq_enabled == 0)) {
		enable_irq(irqnum);
		pjadard_ts_data->irq_enabled = 1;
	} else if ((!enable) && (pjadard_ts_data->irq_enabled == 1)) {
		disable_irq_nosync(irqnum);
		pjadard_ts_data->irq_enabled = 0;
	}

	JD_I("irq_enable = %d\n", pjadard_ts_data->irq_enabled);
}

#ifdef JD_RST_PIN_FUNC
void jadard_gpio_set_value(int pin_num, uint8_t value)
{
#if 1//add by LQ
	tpd_gpio_output(jadard_tpd_rst_gpio_number, value);
#else
	tpd_gpio_output(GTP_RST_PORT, value);
#endif
}
#endif

int jadard_gpio_power_config(struct jadard_i2c_platform_data *pdata)
{
#if 0
	int error = 0;

	error = regulator_enable(tpd->reg);
	if (error != 0)
		JD_E("Failed to enable reg-vgp6: %d\n", error);

	msleep(100);
#endif
     msleep(100);
#ifdef JD_RST_PIN_FUNC
	#if 1//add by LQ
	tpd_gpio_output(jadard_tpd_rst_gpio_number, 1);
	msleep(20);
	tpd_gpio_output(jadard_tpd_rst_gpio_number, 0);
	msleep(20);
	tpd_gpio_output(jadard_tpd_rst_gpio_number, 1);
	msleep(50);
	#else
	tpd_gpio_output(GTP_RST_PORT, 1);
	msleep(20);
	tpd_gpio_output(GTP_RST_PORT, 0);
	msleep(20);
	tpd_gpio_output(GTP_RST_PORT, 1);
	msleep(50);
	#endif
#endif
	JD_I("mtk_tpd: jadard reset over \n");
	/* set INT mode */
	tpd_gpio_as_int(jadard_tpd_int_gpio_number);
	return 0;
}

void jadard_gpio_power_deconfig(struct jadard_i2c_platform_data *pdata)
{
#if 0
	int error = 0;

	error = regulator_disable(tpd->reg);

	if (error != 0)
		JD_I("Failed to disable reg-vgp6: %d\n", error);

	regulator_put(tpd->reg);
#endif
	JD_I("%s: regulator put, completed.\n", __func__);
}

irqreturn_t jadard_ts_isr_func(int irq, void *ptr)
{
#ifdef JD_ZERO_FLASH
	if (pjadard_ts_data->fw_ready == true) {
		jadard_ts_work((struct jadard_ts_data *)ptr);
	}
#else
	jadard_ts_work((struct jadard_ts_data *)ptr);
#endif

	return IRQ_HANDLED;
}

int jadard_int_register_trigger(void)
{
	int ret = JD_NO_ERR;
	struct jadard_ts_data *ts = pjadard_ts_data;
	struct i2c_client *client = pjadard_ts_data->client;

	if (pjadard_ic_data->JD_INT_EDGE) {
		ret = request_threaded_irq(client->irq, NULL, jadard_ts_isr_func,
									IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->name, ts);
	} else {
		ret = request_threaded_irq(client->irq, NULL, jadard_ts_isr_func,
									IRQF_TRIGGER_LOW | IRQF_ONESHOT, client->name, ts);
	}

	return ret;
}

void jadard_int_en_set(bool enable)
{
	struct jadard_ts_data *ts = pjadard_ts_data;

	if (enable) {
		if (jadard_int_register_trigger() == 0) {
			ts->irq_enabled = 1;
		}
	} else {
		jadard_int_enable(false);
		free_irq(ts->client->irq, ts);
	}
}

int jadard_ts_register_interrupt(void)
{
	struct jadard_ts_data *ts = pjadard_ts_data;
	struct i2c_client *client = pjadard_ts_data->client;
	struct device_node *node = NULL;
	u32 ints[2] = {0, 0};
	int ret = 0;
	node = of_find_matching_node(node, touch_of_match);

	if (node) {
		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_set_debounce(ints[0], ints[1]);
		jadard_touch_irq = irq_of_parse_and_map(node, 0);
		JD_I("jadard_touch_irq=%d \n", jadard_touch_irq);
		client->irq = jadard_touch_irq;
		ts->jd_irq = (int)client->irq;
	} else {
		JD_I("[%s] tpd request_irq can not find touch eint device node!\n", __func__);
	}

	ts->irq_enabled = 0;

	/* Work functon */
	if (client->irq) {
		ret = jadard_int_register_trigger();

		if (ret == 0) {
			ts->irq_enabled = 1;
			JD_I("%s: irq enabled at qpio: %d\n", __func__, client->irq);
		} else {
			JD_E("%s: request_irq failed\n", __func__);
		}
	} else {
		JD_I("%s: client->irq is empty.\n", __func__);
	}

	tpd_load_status = 1;

	return ret;
}

int jadard_common_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct jadard_ts_data *ts;
	int ret = 0;
	
	printk("jadard_common_probe 1\n");
		
#if defined(MTK_I2C_DMA)
	client->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	gpDMABuf_va = (u8 *)dma_alloc_coherent(&client->dev, 4096, (dma_addr_t *)&gpDMABuf_pa, GFP_KERNEL);

	if (!gpDMABuf_va) {
		JD_E("Allocate DMA I2C Buffer failed\n");
		ret = -ENODEV;
		goto err_alloc_MTK_DMA_failed;
	}

	memset(gpDMABuf_va, 0, 4096);
#endif
	printk("jadard_common_probe 2\n");
	/* Check I2C functionality */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		JD_E("%s: i2c check functionality error\n", __func__);
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}
	printk("jadard_common_probe 3\n");
	ts = kzalloc(sizeof(struct jadard_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		JD_E("%s: allocate jadard_ts_data failed\n", __func__);
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
	printk("jadard_common_probe 4\n");	
	printk("LQ >>> %s addr=0x%x\n",__func__,client->addr);
	if(0x68!=client->addr)
		client->addr=0x68;

	i2c_set_clientdata(client, ts);
	i2c_client_point = client;
	ts->client = client;
	ts->dev = &client->dev;
	ts->spi = NULL;
	pjadard_ts_data = ts;

	ret = jadard_chip_common_init();
	printk("jadard_common_probe 5\n");	
err_alloc_data_failed:
err_check_functionality_failed:
#if defined(MTK_I2C_DMA)

	if (ret) {
		if (gpDMABuf_va) {
			dma_free_coherent(&client->dev, 4096, gpDMABuf_va, (dma_addr_t)gpDMABuf_pa);
			gpDMABuf_va = NULL;
			gpDMABuf_pa = NULL;
		}
	}

err_alloc_MTK_DMA_failed:
#endif
	if (ret  == JD_NO_ERR)
		tpd_type_cap = 1;
	return ret;
}


int jadard_common_remove(struct i2c_client *client)
{
	int ret = 0;
	jadard_chip_common_deinit();

	if (gpDMABuf_va) {
		dma_free_coherent(&client->dev, 4096, gpDMABuf_va, (dma_addr_t)gpDMABuf_pa);
		gpDMABuf_va = NULL;
		gpDMABuf_pa = NULL;
	}

	return ret;
}

static void jadard_common_suspend(struct device *dev)
{
	struct jadard_ts_data *ts = dev_get_drvdata(&i2c_client_point->dev);
	JD_I("%s: enter \n", __func__);
	jadard_chip_common_suspend(ts);
	JD_I("%s: end \n", __func__);
	return ;
}
static void jadard_common_resume(struct device *dev)
{
	struct jadard_ts_data *ts = dev_get_drvdata(&i2c_client_point->dev);
	JD_I("%s: enter \n", __func__);
	jadard_chip_common_resume(ts);
	JD_I("%s: end \n", __func__);
	return ;
}

#if defined(JD_CONFIG_FB)
int jadard_fb_notifier_callback(struct notifier_block *self,
							unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	struct jadard_ts_data *ts =
		container_of(self, struct jadard_ts_data, fb_notif);
	JD_I(" %s\n", __func__);

	if (evdata && evdata->data && event == FB_EVENT_BLANK && ts &&
		ts->client) {
		blank = evdata->data;

		switch (*blank) {
		case FB_BLANK_UNBLANK:
			jadard_common_resume(&ts->client->dev);
			break;

		case FB_BLANK_POWERDOWN:
		case FB_BLANK_HSYNC_SUSPEND:
		case FB_BLANK_VSYNC_SUSPEND:
		case FB_BLANK_NORMAL:
			jadard_common_suspend(&ts->client->dev);
			break;
		}
	}

	return 0;
}
#endif

static int jadard_common_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strlcpy(info->type, TPD_DEVICE, sizeof(info->type));
	return 0;
}

static const struct i2c_device_id jadard_common_ts_id[] = {
	{JADARD_common_NAME, 0 },
	{}
};

static struct i2c_driver tpd_i2c_driver = {
	.probe = jadard_common_probe,
	.remove = jadard_common_remove,
	.detect = jadard_common_detect,
	.driver = {
		.name = JADARD_common_NAME,
		.of_match_table = of_match_ptr(jadard_match_table),
	},
	.id_table = jadard_common_ts_id,
	.address_list = (const unsigned short *) forces,
};

static int jadard_common_local_init(void)
{
#if 0
	int retval;
	printk("I2C Touchscreen Driver local init\n");

	tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
	retval = regulator_set_voltage(tpd->reg, 2800000, 2800000);

	if (retval != 0) {
		JD_E("Failed to set voltage 2V8: %d\n", retval);
	}
#endif

	printk("jadard_common_local_init 1\n");
	
	if (i2c_add_driver(&tpd_i2c_driver) != 0) {
		JD_I("unable to add i2c driver.\n");
		return -EFAULT;
	}
	printk("jadard_common_local_init 2\n");
	JD_I("end %s, %d\n", __FUNCTION__, __LINE__);

	return 0;
}

#if defined(CONFIG_TP_GET_CHARGE_MODE_STATUS)
static void tpd_charge_mode(unsigned int data)
{
	module_jd9366t_charg_mode_set(data);
}
#endif

#if defined(CONFIG_TP_GET_HEADSET_MODE_STATUS)
static void tpd_headset_mode(unsigned int data)
{
	module_jd9366t_set_earphone_mode(data);
}
#endif

static struct tpd_driver_t tpd_device_driver = {
	.tpd_device_name = JADARD_common_NAME,
	.tpd_local_init = jadard_common_local_init,
	.suspend = jadard_common_suspend,
	.resume = jadard_common_resume,
#if defined(CONFIG_TP_GET_CHARGE_MODE_STATUS)
	.charge_mode = tpd_charge_mode,
#endif
#if defined(CONFIG_TP_GET_HEADSET_MODE_STATUS)
	.headset_mode = tpd_headset_mode,
#endif
#ifdef TPD_HAVE_BUTTON
	.tpd_have_button = 1,
#else
	.tpd_have_button = 0,
#endif
};

#if defined(__JADARD_KMODULE__)
int jadard_common_init(void)
{
	printk("Jadard_common touch panel driver init\n");
	tpd_get_dts_info();

	if (tpd_driver_add(&tpd_device_driver) < 0)
		JD_I("Failed to add Driver!\n");

	return 0;
}

void jadard_common_exit(void)
{
	tpd_driver_remove(&tpd_device_driver);
}

#else
static int __init jadard_common_init(void)
{
	printk("Jadard_common touch panel driver init 1\n");
	tpd_get_dts_info();
	printk("Jadard_common touch panel driver init 2\n");
	
	if (tpd_driver_add(&tpd_device_driver) < 0)
		JD_I("Failed to add Driver!\n");

	return 0;
}

static void __exit jadard_common_exit(void)
{
	tpd_driver_remove(&tpd_device_driver);
}

module_init(jadard_common_init);
module_exit(jadard_common_exit);

MODULE_DESCRIPTION("Jadard_common driver");
MODULE_LICENSE("GPL");

#endif
