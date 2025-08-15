#include "jadard_platform.h"
#include "jadard_module.h"

static struct spi_device *spi;
extern struct jadard_module_fp g_module_fp;
extern struct jadard_ts_data *pjadard_ts_data;
extern struct jadard_ic_data *pjadard_ic_data;
static int jd_g_mmi_refcnt;

const struct of_device_id jadard_match_table[] = {
	{.compatible = "jadard,jdcommon" },
	{},
};
MODULE_DEVICE_TABLE(of, jadard_match_table);

#ifdef CONFIG_MTK_SPI
const struct mt_chip_conf spi_ctrdata = {
	.setuptime = 25,
	.holdtime = 25,
	.high_time = 5,	/* 10MHz (SPI_SPEED=100M / (high_time+low_time(10ns)))*/
	.low_time = 5,
	.cs_idletime = 2,
	.ulthgh_thrsh = 0,
	.cpol = 0,
	.cpha = 0,
	.rx_mlsb = 0,
	.tx_mlsb = 0,
	.tx_endian = 0,
	.rx_endian = 0,
	.com_mod = DMA_TRANSFER,
	.pause = 0,
	.finish_intr = 1,
	.deassert = 0,
	.ulthigh = 0,
	.tckdly = 0,
};
#endif

#ifdef CONFIG_SPI_MT65XX
const struct mtk_chip_config spi_ctrdata = {
    .rx_mlsb = 1,
    .tx_mlsb = 1,

    .cs_setuptime = 50,
};
#endif

static void (*kp_tpd_gpio_as_int)(int gpio);
static int (*kp_tpd_driver_add)(struct tpd_driver_t *drv);
static void (*kp_tpd_get_dts_info)(void);
static void (*kp_tpd_gpio_output)(int pinnum, int value);
static int (*kp_tpd_driver_remove)(struct tpd_driver_t *drv);
static const struct of_device_id *kp_touch_of_match;
static struct tpd_device **kp_tpd;

#if !defined(JD_USE_KSYM)
#define jd_setup_symbol(sym) ({kp_##sym = &(sym); kp_##sym; })
#define jd_setup_symbol_func(sym) ({kp_##sym = (sym); kp_##sym; })
#else
#define jd_setup_symbol(sym) ({kp_##sym = \
	(void *)kallsyms_lookup_name(#sym); kp_##sym; })
#define jd_setup_symbol_func(sym) jd_setup_symbol(sym)
#endif
#define jd_assert_on_symbol(sym) \
		do { \
			if (!jd_setup_symbol(sym)) { \
				JD_E("%s: setup %s failed!\n", __func__, #sym); \
				ret = -1; \
			} \
		} while (0)
#define jd_assert_on_symbol_func(sym) \
		do { \
			if (!jd_setup_symbol_func(sym)) { \
				JD_E("%s: setup %s failed!\n", __func__, #sym); \
				ret = -1; \
			} \
		} while (0)

static int32_t setup_tpd_vars(void)
{
	int32_t ret = 0;

	jd_assert_on_symbol_func(tpd_gpio_as_int);
	jd_assert_on_symbol_func(tpd_driver_add);
	jd_assert_on_symbol_func(tpd_get_dts_info);
	jd_assert_on_symbol_func(tpd_gpio_output);
	jd_assert_on_symbol_func(tpd_driver_remove);
	kp_touch_of_match = (const struct of_device_id *)(&(touch_of_match[0]));
	jd_assert_on_symbol(tpd);

	return ret;
}

int jadard_dev_set(struct jadard_ts_data *ts)
{
	int ret = 0;
	ts->input_dev = input_allocate_device();

	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		JD_E("%s: Failed to allocate input device\n", __func__);
		return ret;
	}

	ts->input_dev->name = "jadard-touchscreen";
	return ret;
}

int jadard_input_register_device(struct input_dev *input_dev)
{
	return input_register_device(input_dev);
}

int jadard_parse_dt(struct jadard_ts_data *ts,
					struct jadard_i2c_platform_data *pdata)
{
	int coords_size;
	uint32_t coords[4] = {0};
	uint32_t ret, data;
	struct property *prop = NULL;
	struct device_node *dt = ts->dev->of_node;

	ret = of_property_read_u32(dt, "jadard,panel-max-points", &data);
	pjadard_ic_data->JD_MAX_PT = (!ret ? data : 10);
	ret = of_property_read_u32(dt, "jadard,int-is-edge", &data);
	pjadard_ic_data->JD_INT_EDGE = (!ret ? (data > 0 ? true : false) : true);

	JD_I("DT:MAX_PT = %d, INT_IS_EDGE = %d\n", pjadard_ic_data->JD_MAX_PT,
		pjadard_ic_data->JD_INT_EDGE);

	prop = of_find_property(dt, "jadard,panel-sense-nums", NULL);
	if (prop) {
		coords_size = prop->length / sizeof(uint32_t);

		if (coords_size != 2) {
			JD_E("%s:Invalid panel sense number size %d\n", __func__, coords_size);
			return -EINVAL;
		}
	}

	if (of_property_read_u32_array(dt, "jadard,panel-sense-nums", coords, coords_size) == 0) {
		pjadard_ic_data->JD_X_NUM = 22;
		pjadard_ic_data->JD_Y_NUM = 36;
		JD_I("DT:panel-sense-num = %d, %d\n",
			pjadard_ic_data->JD_X_NUM, pjadard_ic_data->JD_Y_NUM);
	}

	prop = of_find_property(dt, "jadard,panel-coords", NULL);
	if (prop) {
		coords_size = prop->length / sizeof(uint32_t);

		if (coords_size != 4) {
			JD_E("%s:Invalid panel coords size %d\n", __func__, coords_size);
			return -EINVAL;
		}
	}

	if (of_property_read_u32_array(dt, "jadard,panel-coords", coords, coords_size) == 0) {
		pdata->abs_x_min = 0;
		pdata->abs_x_max = 800;
		pdata->abs_y_min = 0;
		pdata->abs_y_max = 1340;
		pjadard_ic_data->JD_X_RES = pdata->abs_x_max;
		pjadard_ic_data->JD_Y_RES = pdata->abs_y_max;

		JD_I("DT:panel-coords = %d, %d, %d, %d\n", pdata->abs_x_min,
			pdata->abs_x_max, pdata->abs_y_min, pdata->abs_y_max);
	} else {
		/* Config resolution by tpd, if devicetree was`t set up */
		pdata->abs_x_min = 0;
		pdata->abs_x_max = TPD_RES_X;
		pdata->abs_y_min = 0;
		pdata->abs_y_max = TPD_RES_Y;
		pjadard_ic_data->JD_X_RES = pdata->abs_x_max;
		pjadard_ic_data->JD_Y_RES = pdata->abs_y_max;

		JD_I("TPD:panel-coords = %d, %d, %d, %d\n", pdata->abs_x_min,
			pdata->abs_x_max, pdata->abs_y_min, pdata->abs_y_max);
	}

	pdata->gpio_irq = of_get_named_gpio(dt, "jadard,irq-gpio", 0);
	if (!gpio_is_valid(pdata->gpio_irq)) {
		/* Config irq gpio by tpd, if devicetree wasn`t set up */
		pdata->gpio_irq = GTP_INT_PORT;
		JD_I("TPD:gpio_irq value is GTP_INT_PORT\n");
	}

	pdata->gpio_reset = of_get_named_gpio(dt, "jadard,rst-gpio", 0);
	if (!gpio_is_valid(pdata->gpio_reset)) {
		/* Config tp reset gpio by tpd, if devicetree wasn`t set up */
		pdata->gpio_reset = GTP_RST_PORT;
		JD_I("TPD:gpio_rst value is GTP_RST_PORT\n");
	}

	JD_I("DT:gpio_irq = %d, gpio_rst = %d\n", pdata->gpio_irq, pdata->gpio_reset);

	return 0;
}

int jadard_bus_read(uint8_t *cmd, uint8_t cmd_len, uint8_t *data, uint32_t data_len, uint8_t toRetry)
{
	struct spi_message m;
	struct spi_transfer t[2];
	int retry;
	int error;

	mutex_lock(&(pjadard_ts_data->spi_lock));
	memset(&t, 0, sizeof(t));

	spi_message_init(&m);
	t[0].tx_buf = cmd;
	t[0].len = cmd_len;
	spi_message_add_tail(&t[0], &m);

	t[1].rx_buf = data;
	t[1].len = data_len;
	spi_message_add_tail(&t[1], &m);

	for (retry = 0; retry < toRetry; retry++) {
		error = spi_sync(pjadard_ts_data->spi, &m);
		if (unlikely(error)) {
			JD_E("SPI read error: %d\n", error);
			mdelay(2);
		} else{
			break;
		}
	}

	if (retry == toRetry) {
		JD_E("%s: SPI read error retry over %d\n",
			__func__, toRetry);
		mutex_unlock(&(pjadard_ts_data->spi_lock));
		return -EIO;
	}

	mutex_unlock(&(pjadard_ts_data->spi_lock));

	return 0;
}

static int jadard_spi_write(struct spi_device *spi, uint8_t *buf, uint32_t length)
{
	int status;
	struct spi_message m;
	struct spi_transfer t = {
			.tx_buf		= buf,
			.len		= length,
	};

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	status = spi_sync(spi, &m);

	if (status == 0) {
		status = m.status;
		if (status == 0)
			status = m.actual_length;
	}

	return status;
}

int jadard_bus_write(uint8_t *cmd, uint8_t cmd_len, uint8_t *data, uint32_t data_len, uint8_t toRetry)
{
	uint8_t *buf = NULL;
	int status;
	int retry;

	mutex_lock(&(pjadard_ts_data->spi_lock));

	buf = kzalloc((cmd_len + data_len) * sizeof(uint8_t), GFP_KERNEL);

	if (buf == NULL) {
		JD_E("%s: Memory alloc fail\n", __func__);
		mutex_unlock(&(pjadard_ts_data->spi_lock));
		return -ENOMEM;
	}

	memcpy(buf, cmd, cmd_len);
	memcpy(buf + cmd_len, data, data_len);

	for (retry = 0; retry < toRetry; retry++) {
		status = jadard_spi_write(pjadard_ts_data->spi, buf, cmd_len + data_len);

		if (status < 0) {
			JD_E("SPI write error: %d\n", status);
			mdelay(2);
		} else{
			break;
		}
	}
	kfree(buf);

	if (retry == toRetry) {
		JD_E("%s: SPI write error retry over %d\n",
			__func__, toRetry);
		mutex_unlock(&(pjadard_ts_data->spi_lock));
		return -EIO;
	}

	mutex_unlock(&(pjadard_ts_data->spi_lock));

	return 0;
}

void jadard_int_enable(bool enable)
{
	int irqnum = pjadard_ts_data->jd_irq;
    unsigned long irqflags = 0;
    spin_lock_irqsave(&pjadard_ts_data->irq_active, irqflags);

	if (enable && (pjadard_ts_data->irq_enabled == 0)) {
		enable_irq(irqnum);
		pjadard_ts_data->irq_enabled = 1;
	} else if ((!enable) && (pjadard_ts_data->irq_enabled == 1)) {
		disable_irq_nosync(irqnum);
		pjadard_ts_data->irq_enabled = 0;
	}

	JD_I("irq_enable = %d\n", pjadard_ts_data->irq_enabled);
    spin_unlock_irqrestore(&pjadard_ts_data->irq_active, irqflags);
}

/*#ifdef JD_RST_PIN_FUNC*/
void jadard_gpio_set_value(int pin_num, uint8_t value)
{
	kp_tpd_gpio_output(pin_num, value);
}
/*#endif*/

int jadard_gpio_power_config(struct jadard_i2c_platform_data *pdata)
{
	int error = 0;

	error = regulator_enable((*kp_tpd)->reg);

	if (error != 0)
		JD_I("Failed to enable reg-vgp6: %d\n", error);

	msleep(100);
#if defined(JD_RST_PIN_FUNC)
	kp_tpd_gpio_output(pdata->gpio_reset, 1);
	msleep(20);
	kp_tpd_gpio_output(pdata->gpio_reset, 0);
	msleep(20);
	kp_tpd_gpio_output(pdata->gpio_reset, 1);
	msleep(100);
#endif
	JD_I("mtk_tpd: jadard reset over\n");
	/* set INT mode */
	kp_tpd_gpio_as_int(pdata->gpio_irq);

	return 0;
}

void jadard_gpio_power_deconfig(struct jadard_i2c_platform_data *pdata)
{
	int error = 0;

	error = regulator_disable(tpd->reg);

	if (error != 0)
		JD_I("Failed to disable reg-vgp6: %d\n", error);

	regulator_put(tpd->reg);
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

	if (pjadard_ic_data->JD_INT_EDGE) {
		JD_I("%s edge triiger falling\n", __func__);
		ret = request_threaded_irq(ts->jd_irq, NULL, jadard_ts_isr_func,
									IRQF_TRIGGER_FALLING | IRQF_ONESHOT, JADARD_common_NAME, ts);
	} else {
		JD_I("%s level trigger low\n", __func__);
		ret = request_threaded_irq(ts->jd_irq, NULL, jadard_ts_isr_func,
									IRQF_TRIGGER_LOW | IRQF_ONESHOT, JADARD_common_NAME, ts);
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
		free_irq(ts->jd_irq, ts);
	}
}

int jadard_ts_register_interrupt(void)
{
	struct jadard_ts_data *ts = pjadard_ts_data;
	struct device_node *node = NULL;
	u32 ints[2] = {0, 0};
	int ret = 0;

	node = of_find_matching_node(node, (kp_touch_of_match));

	if (node) {
		if (of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints)) == 0) {
			JD_I("%s: Now it has set debounce\n", __func__);
			gpio_set_debounce(ints[0], ints[1]);
			JD_I("%s: Now debounce are ints[0] = %d, ints[1] = %d\n", __func__, ints[0], ints[1]);
		} else {
			JD_I("%s: DOES NOT set debounce!\n", __func__);
		}
		ts->jd_irq = irq_of_parse_and_map(node, 0);
		JD_I("Jadard_touch_irq=%d\n", ts->jd_irq);
	} else {
		JD_I("[%s] tpd request_irq can not find touch eint device node!\n",
				__func__);
		ts->jd_irq = 0;
	}

	ts->irq_enabled = 0;

	/* Work functon */
	if (ts->jd_irq) {
		ret = jadard_int_register_trigger();

		if (ret == 0) {
			ts->irq_enabled = 1;
			JD_I("%s: irq enabled at qpio: %d\n", __func__, ts->jd_irq);
		} else {
			JD_E("%s: request_irq failed\n", __func__);
		}
	} else {
		JD_I("%s: ts->jd_irq is empty.\n", __func__);
	}

	tpd_load_status = 1;

	return ret;
}

void jadard_ts_free_interrupt(void)
{
	struct jadard_ts_data *ts = pjadard_ts_data;

	free_irq(ts->jd_irq, ts);
}

static void jadard_common_suspend(struct device *dev)
{

	struct jadard_ts_data *ts = pjadard_ts_data;

	JD_I("%s: enter \n", __func__);
	
	if (ts != NULL) 
	{
	jadard_chip_common_suspend(ts);	
	}
	
	JD_I("%s: end \n", __func__);
	return;
}

static void jadard_common_resume(struct device *dev)
{

	struct jadard_ts_data *ts = pjadard_ts_data;

	JD_I("%s: enter \n", __func__);
	
	if (ts != NULL)
	{
	jadard_chip_common_resume(ts);	
	}
	
	JD_I("%s: end \n", __func__);
	return;
}

#if defined(JD_CONFIG_FB)
#ifdef JD_CONFIG_DRM
int jadard_drm_check_dt(struct jadard_ts_data *ts)
{
	struct device_node *dt = ts->dev->of_node;
	struct device_node *node = NULL;
	struct drm_panel *panel = NULL;
	int i = 0;
	int count = 0;
	
	count = of_count_phandle_with_args(dt, "panel", NULL);
	if (count <= 0) {
		JD_I("%s: find drm_panel count(%d) fail\n", __func__, count);
		return 0;
	}
	JD_I("%s: find drm_panel count(%d)\n", __func__, count);
	for (i = 0; i < count; i++) {
		node = of_parse_phandle(dt, "panel", i);
		JD_I("DRM:node = %p\n", node);
		panel = of_drm_find_panel(node);
		JD_I("DRM:panel = %p\n", panel);

		of_node_put(node);
		if (!IS_ERR(panel)) {
			JD_I("%s: find drm_panel successfully\n", __func__);
			ts->active_panel = panel;
			return 0;
		}
	}

	JD_E("%s: no find drm_panel\n", __func__);
	
	return 0;
}

int jadard_fb_notifier_callback(struct notifier_block *self,
							unsigned long event, void *data)
{
	struct drm_panel_notifier *evdata = data;
	int *blank;
	struct jadard_ts_data *ts =
		container_of(self, struct jadard_ts_data, fb_notif);
	JD_I("DRM: %s\n", __func__);

	if (evdata && evdata->data && 
		((event == DRM_PANEL_EARLY_EVENT_BLANK) || (event == DRM_PANEL_EVENT_BLANK)) && 
		ts != NULL &&
		ts->dev != NULL) 
	{
		blank = evdata->data;
		JD_I("DRM event:%lu, blank:%d\n", event, *blank);

		switch (*blank) {
		case DRM_PANEL_BLANK_UNBLANK:
			if (DRM_PANEL_EARLY_EVENT_BLANK == event) {
				JD_I("resume: event = %lu, Skipped\n", event);
			} else if (DRM_PANEL_EVENT_BLANK == event) {
				JD_I("resume: event = %lu, TP_RESUME\n", event);
				jadard_common_resume(ts->dev);
			}
			break;

		case DRM_PANEL_BLANK_POWERDOWN:
			if (DRM_PANEL_EARLY_EVENT_BLANK == event) {
				JD_I("suspend: event = %lu, TP_SUSPEND\n", event);
				jadard_common_suspend(ts->dev);
			} else if (DRM_PANEL_EVENT_BLANK == event) {
				JD_I("suspend: event = %lu, Skipped\n", event);
			}
			break;
		}
	}

	return 0;
}
#else
#ifdef JD_CONFIG_DRM_MSM
int jadard_fb_notifier_callback(struct notifier_block *self,
							unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	struct jadard_ts_data *ts =
		container_of(self, struct jadard_ts_data, fb_notif);
	JD_I("MSM_DRM: %s\n", __func__);

	if (evdata && evdata->data && 
		((event == MSM_DRM_EARLY_EVENT_BLANK) || (event == MSM_DRM_EVENT_BLANK)) && 
		ts != NULL &&
		ts->dev != NULL) 
	{
		blank = evdata->data;
		JD_I("MSM_DRM event:%lu, blank:%d\n", event, *blank);

		switch (*blank) {
		case MSM_DRM_BLANK_UNBLANK:
			if (MSM_DRM_EARLY_EVENT_BLANK == event) {
				JD_I("resume: event = %lu, Skipped\n", event);
			} else if (MSM_DRM_EVENT_BLANK == event) {
				JD_I("resume: event = %lu, TP_RESUME\n", event);
				jadard_common_resume(ts->dev);
			}
			break;

		case MSM_DRM_BLANK_POWERDOWN:
			if (MSM_DRM_EARLY_EVENT_BLANK == event) {
				JD_I("suspend: event = %lu, TP_SUSPEND\n", event);
				jadard_common_suspend(ts->dev);
			} else if (MSM_DRM_EVENT_BLANK == event) {
				JD_I("suspend: event = %lu, Skipped\n", event);
			}
			break;
		}
	}

	return 0;
}
#else
int jadard_fb_notifier_callback(struct notifier_block *self,
							unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	struct jadard_ts_data *ts =
		container_of(self, struct jadard_ts_data, fb_notif);
	JD_I("FB: %s\n", __func__);

	if (evdata && evdata->data && 
		((event == FB_EARLY_EVENT_BLANK) || (event == FB_EVENT_BLANK)) && 
		ts != NULL &&
		ts->dev != NULL) 
	{
		blank = evdata->data;
		JD_I("FB event:%lu, blank:%d\n", event, *blank);

		switch (*blank) {
		case FB_BLANK_UNBLANK:
			if (FB_EARLY_EVENT_BLANK == event) {
				JD_I("resume: event = %lu, Skipped\n", event);
			} else if (FB_EVENT_BLANK == event) {
				JD_I("resume: event = %lu, TP_RESUME\n", event);
				jadard_common_resume(ts->dev);
			}
			break;

		case FB_BLANK_POWERDOWN:
		case FB_BLANK_HSYNC_SUSPEND:
		case FB_BLANK_VSYNC_SUSPEND:
		case FB_BLANK_NORMAL:
			if (FB_EARLY_EVENT_BLANK == event) {
				JD_I("suspend: event = %lu, TP_SUSPEND\n", event);
				jadard_common_suspend(ts->dev);
			} else if (FB_EVENT_BLANK == event) {
				JD_I("suspend: event = %lu, Skipped\n", event);
			}
			break;
		}
	}

	return 0;
}
#endif /* JD_CONFIG_DRM_MSM */
#endif /* JD_CONFIG_DRM */
#endif /* defined(JD_CONFIG_FB) */

int jadard_chip_common_probe(struct spi_device *spi)
{
	struct jadard_ts_data *ts = NULL;

	JD_I("Enter %s \n", __func__);
	if (spi->master->flags & SPI_MASTER_HALF_DUPLEX) {
		dev_err(&spi->dev,
				"%s: Full duplex not supported by host\n", __func__);
		return -EIO;
	}

	ts = kzalloc(sizeof(struct jadard_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		JD_E("%s: allocate jadard_ts_data failed\n", __func__);
		return -ENOMEM;
	}
	
	//tpd_gpio_mode_set();
	
	pjadard_ts_data = ts;
	spi->bits_per_word = 8;
	spi->mode = SPI_MODE_0;

	ts->spi = spi;
	mutex_init(&(ts->spi_lock));
	ts->dev = &spi->dev;
	dev_set_drvdata(&spi->dev, ts);
	spi_set_drvdata(spi, ts);
#ifdef CONFIG_MTK_SPI
    /* old usage of MTK spi API */
    memcpy(&ts->spi_ctrl, &spi_ctrdata, sizeof(struct mt_chip_conf));
    ts->spi->controller_data = (void *)&ts->spi_ctrl;
#endif

#ifdef CONFIG_SPI_MT65XX
    /* new usage of MTK spi API */
    memcpy(&ts->spi_ctrl, &spi_ctrdata, sizeof(struct mtk_chip_config));
    ts->spi->controller_data = (void *)&ts->spi_ctrl;
#endif

    spin_lock_init(&ts->irq_active);

	return jadard_chip_common_init();
}

int jadard_chip_common_remove(struct spi_device *spi)
{
	struct jadard_ts_data *ts = spi_get_drvdata(spi);

	ts->spi = NULL;
	/* spin_unlock_irq(&ts->spi_lock); */
	spi_set_drvdata(spi, NULL);

	jadard_chip_common_deinit();

	return 0;
}

struct spi_device_id jd_spi_id_table  = {"jadard-spi", 1};
struct spi_driver jadard_common_driver = {
	.driver = {
		.name = JADARD_common_NAME,
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
#if defined(CONFIG_OF)
		.of_match_table = jadard_match_table,
#endif
	},
	.probe = jadard_chip_common_probe,
	.remove = jadard_chip_common_remove,
	.id_table = &jd_spi_id_table,
};

static int jadard_common_local_init(void)
{
	int retval;

	JD_I("SPI Touchscreen Driver local init\n");

	(*kp_tpd)->reg = regulator_get((*kp_tpd)->tpd_dev, "vtouch");
	retval = regulator_set_voltage((*kp_tpd)->reg, 2800000, 2800000);

	if (retval != 0)
		JD_E("Failed to set voltage 2V8: %d\n", retval);


	retval = spi_register_driver(&jadard_common_driver);
	if (retval < 0) {
		JD_E("unable to add SPI driver.\n");
		return -EFAULT;
	}
	JD_I("SPI Touchscreen Driver local init end\n");

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
	.resume	 = jadard_common_resume,
#if defined(CONFIG_TP_GET_CHARGE_MODE_STATUS)
	.charge_mode = tpd_charge_mode,
#endif
#if defined(CONFIG_TP_GET_HEADSET_MODE_STATUS)
	.headset_mode = tpd_headset_mode,
#endif
#if defined(TPD_HAVE_BUTTON)
	.tpd_have_button = 1,
#else
	.tpd_have_button = 0,
#endif
};

#if defined(__JADARD_KMODULE__)
int jadard_common_init(void)
{
	JD_I("SPI Jadard kmodule common touch panel driver init\n");

	if (jd_g_mmi_refcnt++ > 0) {
		JD_I("Jadard driver has been loaded! ignoring....\n");
		goto END;
	}

	if (setup_tpd_vars() != 0) {
		JD_E("Failed to get tpd variables!\n");
		goto ERR;
	}
	//tpd_get_dts_info();
	kp_tpd_get_dts_info();

	//if (tpd_driver_add(&tpd_device_driver) < 0) {
	if (kp_tpd_driver_add(&tpd_device_driver) < 0) {	
		JD_I("Failed to add Driver!\n");
		goto ERR;
	}

END:
	return 0;
ERR:
	return -1;
}

void jadard_common_exit(void)
{
	if (spi) {
		spi_unregister_device(spi);
		spi = NULL;
	}
	spi_unregister_driver(&jadard_common_driver);
	tpd_driver_remove(&tpd_device_driver);
}

#else
static int __init jadard_common_init(void)
{
	JD_I("SPI Jadard common touch panel driver init\n");

	if (jd_g_mmi_refcnt++ > 0) {
		JD_I("Jadard driver has been loaded! ignoring....\n");
		goto END;
	}

	if (setup_tpd_vars() != 0) {
		JD_E("Failed to get tpd variables!\n");
		goto ERR;
	}
	tpd_get_dts_info();

	if (tpd_driver_add(&tpd_device_driver) < 0) {
		JD_I("Failed to add Driver!\n");
		goto ERR;
	}

END:
	return 0;
ERR:
	return -1;
}

static void __exit jadard_common_exit(void)
{
	if (spi) {
		spi_unregister_device(spi);
		spi = NULL;
	}
	spi_unregister_driver(&jadard_common_driver);
	tpd_driver_remove(&tpd_device_driver);
}

module_init(jadard_common_init);
module_exit(jadard_common_exit);

MODULE_DESCRIPTION("Jadard_common driver");
MODULE_LICENSE("GPL");

#endif
