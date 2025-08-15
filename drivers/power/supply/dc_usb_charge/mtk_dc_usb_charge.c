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
#include <mach/mtk_diso.h>
#include <mt-plat/diso.h>

#if !defined(MTK_AUXADC_IRQ_SUPPORT)
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#endif

#define TAG "charge_dc_adc"

#define STATUS_OK    0
#define STATUS_FAIL	 1
#define SW_POLLING_PERIOD 100 /*100 ms*/
#define MSEC_TO_NSEC(x)		(x * 1000000UL)

/***************************************************************************************************************************************/
/*power_supply_type                   power_supply_usb_type                                                                            */
/*POWER_SUPPLY_TYPE_UNKNOWN-0:                                       CHARGER_UNKNOWN        "none"      none            			   */
/*POWER_SUPPLY_TYPE_USB-4:                                           Standard USB Host      "usb"       usb_charger_current            */
/*POWER_SUPPLY_TYPE_USB_CDP-6:      POWER_SUPPLY_USB_TYPE_CDP-3      Charging USB Host      "usb-h"     charging_host_charger_current  */
/*POWER_SUPPLY_TYPE_USB_DCP-5:      POWER_SUPPLY_USB_TYPE_DCP-2      Standard Charger       "std"       ac_charger_current             */
/*POWER_SUPPLY_TYPE_USB_FLOAT-16:                                    Non-standard Charger   "nonstd"    usb_charger_current            */

/*power_supply_usb_type                                          */
/*POWER_SUPPLY_USB_TYPE_SDP-1                                    */
/*POWER_SUPPLY_USB_TYPE_DCP-2                                    */
/*POWER_SUPPLY_USB_TYPE_CDP-3                                    */
/**************************************************************************************************************************************/
static DEFINE_MUTEX(diso_polling_mutex);
static DECLARE_WAIT_QUEUE_HEAD(diso_polling_thread_wq);
static struct hrtimer diso_kthread_timer;
static kal_bool diso_thread_timeout = KAL_FALSE;
static struct delayed_work diso_polling_work;
static void diso_polling_handler(struct work_struct *work);
#if !defined(CONFIG_TCPC_CLASS)
extern void dc_usb_charger_plug_in(void);
#endif
static DISO_Polling_Data DISO_Polling;
DISO_ChargerStruct DISO_data;
int g_diso_state;
kal_bool dc_charge = KAL_FALSE;
kal_bool usb_charge = KAL_FALSE;
static char *DISO_state_s[8] = {
		  "IDLE",
		  "OTG_ONLY",
		  "USB_ONLY",
		  "USB_WITH_OTG",
		  "DC_ONLY",
		  "DC_WITH_OTG",
		  "DC_WITH_USB",
		  "DC_USB_OTG",
};

static unsigned int dc_adc_num;
static unsigned int usb_adc_num;
static int iio_init;
struct iio_channel *dc_channel;
struct iio_channel *usb_channel;
int r_dc_pull_up;
int r_dc_pull_down;
int r_usb_pull_up;
int r_usb_pull_down;


void set_vusb_auxadc_irq(bool enable, bool flag)
{
	hrtimer_cancel(&diso_kthread_timer);

	DISO_Polling.reset_polling = KAL_TRUE;
	DISO_Polling.vusb_polling_measure.notify_irq_en = enable;
	DISO_Polling.vusb_polling_measure.notify_irq = flag;

	hrtimer_start(&diso_kthread_timer, ktime_set(0, MSEC_TO_NSEC(SW_POLLING_PERIOD)), HRTIMER_MODE_REL);
	dc_usb_printk("enable: %d, flag: %d!\n",enable, flag);
}

void set_vdc_auxadc_irq(bool enable, bool flag)
{
	hrtimer_cancel(&diso_kthread_timer);

	DISO_Polling.reset_polling = KAL_TRUE;
	DISO_Polling.vdc_polling_measure.notify_irq_en = enable;
	DISO_Polling.vdc_polling_measure.notify_irq = flag;

	hrtimer_start(&diso_kthread_timer, ktime_set(0, MSEC_TO_NSEC(SW_POLLING_PERIOD)), HRTIMER_MODE_REL);
	dc_usb_printk("enable: %d, flag: %d!\n",enable, flag);
}


static void diso_polling_handler(struct work_struct *work)
{
	int trigger_channel = -1;
	int trigger_flag = -1;

	if (DISO_Polling.vdc_polling_measure.notify_irq_en)
		trigger_channel = AP_AUXADC_DISO_VDC_CHANNEL;
	else if (DISO_Polling.vusb_polling_measure.notify_irq_en)
		trigger_channel = AP_AUXADC_DISO_VUSB_CHANNEL;

	dc_usb_printk("auxadc handler triggered\n");
	switch (trigger_channel) {
	case AP_AUXADC_DISO_VDC_CHANNEL:
		trigger_flag = DISO_Polling.vdc_polling_measure.notify_irq;
		dc_usb_printk("VDC IRQ triggered, channel ==%d, flag ==%d\n", trigger_channel, trigger_flag);
#ifdef MTK_DISCRETE_SWITCH /*for DSC DC plugin handle */
		set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
		set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
		set_vusb_auxadc_irq(DISO_IRQ_ENABLE, DISO_IRQ_FALLING);
		if (trigger_flag == DISO_IRQ_RISING) {
			DISO_data.diso_state.pre_vusb_state  = DISO_ONLINE;
			DISO_data.diso_state.pre_vdc_state  = DISO_OFFLINE;
			DISO_data.diso_state.pre_otg_state  = DISO_OFFLINE;
			DISO_data.diso_state.cur_vusb_state  = DISO_ONLINE;
			DISO_data.diso_state.cur_vdc_state  = DISO_ONLINE;
			DISO_data.diso_state.cur_otg_state  = DISO_OFFLINE;
			dc_usb_printk("cur diso_state is %s!\n", DISO_state_s[2]);
		}
#else /*for load switch OTG leakage handle*/
		set_vdc_auxadc_irq(DISO_IRQ_ENABLE, (~trigger_flag) & 0x1);
		if (trigger_flag == DISO_IRQ_RISING) {
			DISO_data.diso_state.pre_vusb_state  = DISO_OFFLINE;
			DISO_data.diso_state.pre_vdc_state  = DISO_OFFLINE;
			DISO_data.diso_state.pre_otg_state  = DISO_ONLINE;
			DISO_data.diso_state.cur_vusb_state  = DISO_OFFLINE;
			DISO_data.diso_state.cur_vdc_state  = DISO_ONLINE;
			DISO_data.diso_state.cur_otg_state  = DISO_ONLINE;
			dc_usb_printk("cur diso_state is %s!\n", DISO_state_s[5]);
		} else if (trigger_flag == DISO_IRQ_FALLING) {
			DISO_data.diso_state.pre_vusb_state  = DISO_OFFLINE;
			DISO_data.diso_state.pre_vdc_state  = DISO_ONLINE;
			DISO_data.diso_state.pre_otg_state  = DISO_ONLINE;
			DISO_data.diso_state.cur_vusb_state  = DISO_OFFLINE;
			DISO_data.diso_state.cur_vdc_state  = DISO_OFFLINE;
			DISO_data.diso_state.cur_otg_state  = DISO_ONLINE;
			dc_usb_printk( "cur diso_state is %s!\n", DISO_state_s[1]);
		} else{
			dc_usb_printk(" wrong trigger flag!\n");
		}
#endif
		break;
	case AP_AUXADC_DISO_VUSB_CHANNEL:
		trigger_flag = DISO_Polling.vusb_polling_measure.notify_irq;
		dc_usb_printk( "[DISO]VUSB IRQ triggered, channel ==%d, flag ==%d\n",
			trigger_channel, trigger_flag);
		set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
		set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
		if (trigger_flag == DISO_IRQ_FALLING) {
			DISO_data.diso_state.pre_vusb_state  = DISO_ONLINE;
			DISO_data.diso_state.pre_vdc_state  = DISO_ONLINE;
			DISO_data.diso_state.pre_otg_state  = DISO_OFFLINE;
			DISO_data.diso_state.cur_vusb_state  = DISO_OFFLINE;
			DISO_data.diso_state.cur_vdc_state  = DISO_ONLINE;
			DISO_data.diso_state.cur_otg_state  = DISO_OFFLINE;
			dc_usb_printk("cur diso_state is %s!\n", DISO_state_s[4]);
		} else if (trigger_flag == DISO_IRQ_RISING) {
			DISO_data.diso_state.pre_vusb_state  = DISO_OFFLINE;
			DISO_data.diso_state.pre_vdc_state  = DISO_ONLINE;
			DISO_data.diso_state.pre_otg_state  = DISO_OFFLINE;
			DISO_data.diso_state.cur_vusb_state  = DISO_ONLINE;
			DISO_data.diso_state.cur_vdc_state  = DISO_ONLINE;
			DISO_data.diso_state.cur_otg_state  = DISO_OFFLINE;
			dc_usb_printk("cur diso_state is %s!\n", DISO_state_s[6]);
		} else{
			dc_usb_printk("wrong trigger flag!\n");
			set_vusb_auxadc_irq(DISO_IRQ_ENABLE, (~trigger_flag)&0x1);
		}
		break;
	default:
		set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
		set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
		dc_usb_printk("VUSB auxadc IRQ triggered ERROR OR TEST\n");
		return; /* in error or unexecpt state just return */
	}

	g_diso_state = *(int *)&DISO_data.diso_state;
	dc_usb_printk("g_diso_state: 0x%x\n", g_diso_state);
	return;
	//DISO_data.irq_callback_func(0, NULL);
}


static void _get_diso_interrupt_state(void)
{
	int vol = 0;
	int diso_state = 0;

	mdelay(AUXADC_CHANNEL_DELAY_PERIOD);

	vol = get_dc_val();
	dc_usb_printk("Current DC voltage mV = %d\n", vol);

	/* force delay for switching as no flag for check switching done */
	mdelay(SWITCH_RISING_TIMING + LOAD_SWITCH_TIMING_MARGIN);
	if (vol > VDC_MIN_VOLTAGE/1000 && vol < VDC_MAX_VOLTAGE/1000)
		diso_state |= 0x4; /*SET DC bit as 1*/
	else
		diso_state &= ~0x4; /*SET DC bit as 0*/

	vol = get_usb_val();	
	dc_usb_printk("Current VBUS voltage  mV = %d\n", vol);

	if (vol > VBUS_MIN_VOLTAGE/1000 && vol < VBUS_MAX_VOLTAGE/1000) {
		if (!mt_usb_is_device()) {
			diso_state |= 0x1; /*SET OTG bit as 1*/
			diso_state &= ~0x2; /*SET VBUS bit as 0*/
		} else {
			diso_state &= ~0x1; /*SET OTG bit as 0*/
			diso_state |= 0x2; /*SET VBUS bit as 1;*/
		}
	} else {
		diso_state &= 0x4; /*SET OTG and VBUS bit as 0*/
	}
	dc_usb_printk("DISO_STATE==0x%x\n", diso_state);
	g_diso_state = diso_state;
}

int _get_irq_direction(int pre_vol, int cur_vol)
{
	int ret = -1;

	/*threshold 1000mv*/
	if ((cur_vol - pre_vol) > 1000)
		ret = DISO_IRQ_RISING;
	else if ((pre_vol - cur_vol) > 1000)
		ret = DISO_IRQ_FALLING;

	return ret;
}

static void _get_polling_state(void)
{
	unsigned int vdc_vol = 0, vusb_vol = 0;
	int vdc_vol_dir = -1;
	int vusb_vol_dir = -1;

	DISO_polling_channel *VDC_Polling = &DISO_Polling.vdc_polling_measure;
	DISO_polling_channel *VUSB_Polling = &DISO_Polling.vusb_polling_measure;

	vdc_vol = get_dc_val();
	dc_usb_printk("Current vdc_vol 6357 voltage mV = %d\n", vdc_vol);

	if(vdc_vol <= 2000 && dc_charge){
    	dc_charge = KAL_FALSE;
    }else if(vdc_vol >= 4000 && !dc_charge){
    	dc_charge = KAL_TRUE;
		//dc_usb_charger_plug_in();//set current
    }

	vusb_vol = get_usb_val();
	dc_usb_printk("Current vusb_vol 6357 voltage mV = %d\n", vusb_vol);

	if(vusb_vol <= 500 && usb_charge){
    	usb_charge = KAL_FALSE;
		if(dc_charge){
			mt_usb_disconnect();
			//dc_usb_charger_plug_in();//set current
		}
    }else if(vusb_vol >= 4000 && !usb_charge){
    	usb_charge = KAL_TRUE;
		if(dc_charge){
			mt_usb_connect();
			//dc_usb_charger_plug_in();//set current
		}
    }

	VDC_Polling->preVoltage = VDC_Polling->curVoltage;
	VUSB_Polling->preVoltage = VUSB_Polling->curVoltage;
	VDC_Polling->curVoltage = vdc_vol;
	VUSB_Polling->curVoltage = vusb_vol;

	if (DISO_Polling.reset_polling) {
		DISO_Polling.reset_polling = KAL_FALSE;
		VDC_Polling->preVoltage = vdc_vol;
		VUSB_Polling->preVoltage = vusb_vol;

		if (vdc_vol > 1000)
			vdc_vol_dir = DISO_IRQ_RISING;
		else
			vdc_vol_dir = DISO_IRQ_FALLING;

		if (vusb_vol > 1000)
			vusb_vol_dir = DISO_IRQ_RISING;
		else
			vusb_vol_dir = DISO_IRQ_FALLING;
	} else {
		/*get voltage direction*/
		vdc_vol_dir = _get_irq_direction(VDC_Polling->preVoltage, VDC_Polling->curVoltage);
		vusb_vol_dir = _get_irq_direction(VUSB_Polling->preVoltage, VUSB_Polling->curVoltage);
	}

	if (VDC_Polling->notify_irq_en &&
	(vdc_vol_dir == VDC_Polling->notify_irq)) {
		schedule_delayed_work(&diso_polling_work, 10*HZ/1000); /*10ms*/
		dc_usb_printk( "ready to trig VDC irq, irq: %d\n", VDC_Polling->notify_irq);
	} else if (VUSB_Polling->notify_irq_en && (vusb_vol_dir == VUSB_Polling->notify_irq)) {
		schedule_delayed_work(&diso_polling_work, 10*HZ/1000);
		dc_usb_printk( "ready to trig VUSB irq, irq: %d\n",VUSB_Polling->notify_irq);
	} else if ((vdc_vol == 0) && (vusb_vol == 0)) {
		VDC_Polling->notify_irq_en = 0;
		VUSB_Polling->notify_irq_en = 0;
	}

}

enum hrtimer_restart diso_kthread_hrtimer_func(struct hrtimer *timer)
{
	diso_thread_timeout = KAL_TRUE;
	wake_up(&diso_polling_thread_wq);

	return HRTIMER_NORESTART;
}

int diso_thread_kthread(void *x)
{
	/* Run on a process content */
	while (1) {
		wait_event(diso_polling_thread_wq, (diso_thread_timeout == KAL_TRUE));
		diso_thread_timeout = KAL_FALSE;
		mutex_lock(&diso_polling_mutex);

		_get_polling_state();

		if (DISO_Polling.vdc_polling_measure.notify_irq_en || DISO_Polling.vusb_polling_measure.notify_irq_en)
			hrtimer_start(&diso_kthread_timer, ktime_set(0, MSEC_TO_NSEC(SW_POLLING_PERIOD)),
			HRTIMER_MODE_REL);
		else
			hrtimer_cancel(&diso_kthread_timer);

		mutex_unlock(&diso_polling_mutex);
	}

	return 0;
}

static unsigned int __maybe_unused charging_diso_init(void *data)
{
	unsigned int status = STATUS_OK;
	
	DISO_ChargerStruct *pDISO_data = (DISO_ChargerStruct *)data;
	
	/* Initialization DISO Struct */
	pDISO_data->diso_state.cur_otg_state	 = DISO_OFFLINE;
	pDISO_data->diso_state.cur_vusb_state = DISO_OFFLINE;
	pDISO_data->diso_state.cur_vdc_state	 = DISO_OFFLINE;
	
	pDISO_data->diso_state.pre_otg_state	 = DISO_OFFLINE;
	pDISO_data->diso_state.pre_vusb_state = DISO_OFFLINE;
	pDISO_data->diso_state.pre_vdc_state	 = DISO_OFFLINE;
	
	pDISO_data->chr_get_diso_state = KAL_FALSE;
	pDISO_data->hv_voltage = VBUS_MAX_VOLTAGE;
	
	hrtimer_init(&diso_kthread_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	diso_kthread_timer.function = diso_kthread_hrtimer_func;
	INIT_DELAYED_WORK(&diso_polling_work, diso_polling_handler);
	
	kthread_run(diso_thread_kthread, NULL, "diso_thread_kthread");
	dc_usb_printk("done");

	return status;
}

unsigned int __maybe_unused charging_get_diso_state(void *data)
{
	unsigned int status = STATUS_OK;

	int diso_state = 0x0;
	DISO_ChargerStruct *pDISO_data = (DISO_ChargerStruct *)data;

	_get_diso_interrupt_state();
	diso_state = g_diso_state;
	dc_usb_printk("current diso state is %s!\n", DISO_state_s[diso_state]);
	if (((diso_state >> 1) & 0x3) != 0x0) {
		switch (diso_state) {
		case USB_ONLY:
			set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
#ifdef MTK_DISCRETE_SWITCH
			set_vdc_auxadc_irq(DISO_IRQ_ENABLE, 1);
#endif
			pDISO_data->diso_state.cur_vusb_state = DISO_ONLINE;
			pDISO_data->diso_state.cur_vdc_state = DISO_OFFLINE;
			pDISO_data->diso_state.cur_otg_state = DISO_OFFLINE;
			break;
		case DC_ONLY:
			set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_ENABLE, DISO_IRQ_RISING);
			pDISO_data->diso_state.cur_vusb_state = DISO_OFFLINE;
			pDISO_data->diso_state.cur_vdc_state = DISO_ONLINE;
			pDISO_data->diso_state.cur_otg_state = DISO_OFFLINE;
			break;
		case DC_WITH_USB:
			set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_ENABLE, DISO_IRQ_FALLING);
			pDISO_data->diso_state.cur_vusb_state = DISO_ONLINE;
			pDISO_data->diso_state.cur_vdc_state = DISO_ONLINE;
			pDISO_data->diso_state.cur_otg_state = DISO_OFFLINE;
			break;
		case DC_WITH_OTG:
			set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
			pDISO_data->diso_state.cur_vusb_state = DISO_OFFLINE;
			pDISO_data->diso_state.cur_vdc_state = DISO_ONLINE;
			pDISO_data->diso_state.cur_otg_state = DISO_ONLINE;
			break;
		default: /*OTG only also can trigger vcdt IRQ*/
			pDISO_data->diso_state.cur_vusb_state = DISO_OFFLINE;
			pDISO_data->diso_state.cur_vdc_state = DISO_OFFLINE;
			pDISO_data->diso_state.cur_otg_state = DISO_ONLINE;
			dc_usb_printk( " switch load vcdt irq triggerd by OTG Boost!\n");
			break; /*OTG plugin no need battery sync action*/
		}
	}

	if (pDISO_data->diso_state.cur_vdc_state == DISO_ONLINE)
		pDISO_data->hv_voltage = VDC_MAX_VOLTAGE;
	else
		pDISO_data->hv_voltage = VBUS_MAX_VOLTAGE;

	return status;
}
/****************************************************adc_control************************************************************/
unsigned int get_dc_adc_num(void)
{
	return dc_adc_num;
}
EXPORT_SYMBOL(get_dc_adc_num);


unsigned int get_usb_adc_num(void)
{
	return usb_adc_num;
}
EXPORT_SYMBOL(get_usb_adc_num);

int get_adc_val(struct iio_channel *adc_channel){
	int ret, vol = 0, adc_val = 0, i, times = 5;

	i = times;

	while (i--) {
		ret = iio_read_channel_raw(adc_channel, &vol);
		if (ret < 0) {
			times = times > 1 ? times - 1 : 1;
			dc_usb_printk("iio_read_channel_raw fail");
			//goto Fail;
		}else{
			adc_val += vol;
		}
    }

	adc_val = adc_val*1500/4096;
	adc_val = adc_val/times;

	return adc_val;
}

unsigned int get_dc_val(void)
{	
	int adc_val = 0;
	unsigned int dc_val = 0;

	if(!iio_init){
		return 0;
	}

	adc_val = get_adc_val(dc_channel);
	dc_usb_printk("dc_adc_vol=%d",adc_val);

	dc_val = (r_dc_pull_up + r_dc_pull_down)*100*adc_val/(r_dc_pull_down)/100;

	return dc_val;
}
EXPORT_SYMBOL(get_dc_val);

unsigned int get_usb_val(void)
{	
	int adc_val = 0;
	unsigned int usb_val = 0;

	if(!iio_init){
		return 0;
	}

	adc_val = get_adc_val(usb_channel);
	dc_usb_printk("usb_adc_vol=%d",adc_val);

	usb_val = (r_usb_pull_up + r_usb_pull_down)*100*adc_val/(r_usb_pull_down)/100;

	return usb_val;
}
EXPORT_SYMBOL(get_usb_val);

static int dc_usb_get_adc_info(struct device *dev)
{
	int ret;

	/*dc info*/
	dc_channel = devm_kzalloc(dev, sizeof(*dc_channel), GFP_KERNEL);
	if (!dc_channel)
		return -ENOMEM;
	dc_channel = iio_channel_get(dev, "dc_adc_channel");

	ret = IS_ERR(dc_channel);
	if (ret) {
		if (PTR_ERR(dc_channel) == -EPROBE_DEFER) {
			dc_usb_printk("EPROBE_DEFER");
			return -EPROBE_DEFER;
		}
		ret = PTR_ERR(dc_channel);
		dc_usb_printk("fail to get iio channel (%d)", ret);
		goto Fail;
	}
	dc_adc_num = dc_channel->channel->channel;


	/*usb info*/
	usb_channel = devm_kzalloc(dev, sizeof(*usb_channel), GFP_KERNEL);
	if (!usb_channel)
		return -ENOMEM;
	usb_channel = iio_channel_get(dev, "usb_adc_channel");

	ret = IS_ERR(usb_channel);
	if (ret) {
		if (PTR_ERR(usb_channel) == -EPROBE_DEFER) {
			dc_usb_printk("EPROBE_DEFER");
			return -EPROBE_DEFER;
		}
		ret = PTR_ERR(usb_channel);
		dc_usb_printk("fail to get iio channel (%d)", ret);
		goto Fail;
	}
	usb_adc_num = usb_channel->channel->channel;


	dc_usb_printk("dc_usb_get_adc_info dc_adc_num = %d   usb_adc_num = %d\n",dc_adc_num, usb_adc_num);
	return ret;

Fail:
	return -1;
}

static void get_r_dts_info(struct device_node *node){
	int ret, val;

	ret = of_property_read_u32(node, "r_dc_pull_up", &val);
	if (!ret) {
		r_dc_pull_up = val;
		dc_usb_printk("---00 r_dc_pull_up=%d\n",r_dc_pull_up);
	}else{
		r_dc_pull_up = R_DISO_DC_PULL_UP;
		dc_usb_printk("---11 r_dc_pull_up=%d\n",r_dc_pull_up);
	}

	ret = of_property_read_u32(node, "r_dc_pull_down", &val);
	if (!ret) {
		r_dc_pull_down = val;
		dc_usb_printk("---00 r_dc_pull_down=%d\n",r_dc_pull_down);
	}else{
		r_dc_pull_down = R_DISO_DC_PULL_DOWN;
		dc_usb_printk("---11 r_dc_pull_down=%d\n",r_dc_pull_down);
	}

	ret = of_property_read_u32(node, "r_usb_pull_up", &val);
	if (!ret) {
		r_usb_pull_up = val;
		dc_usb_printk("---00 r_usb_pull_up=%d\n",r_usb_pull_up);
	}else{
		r_usb_pull_up = R_DISO_VBUS_PULL_UP;
		dc_usb_printk("---11 r_usb_pull_up=%d\n",r_usb_pull_up);
	}

	ret = of_property_read_u32(node, "r_usb_pull_down", &val);
	if (!ret) {
		r_usb_pull_down = val;
		dc_usb_printk("---00 r_usb_pull_down=%d\n",r_usb_pull_down);
	}else{
		r_usb_pull_down = R_DISO_VBUS_PULL_DOWN;
		dc_usb_printk("---11 r_usb_pull_down=%d\n",r_usb_pull_down);
	}
}

int get_dc_usb_auxadc_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *node;

	node = pdev->dev.of_node;
	if(node){
		ret = dc_usb_get_adc_info(&pdev->dev);
		if (ret < 0) {
			dc_usb_printk("dc_usb get adc info fail");
			return ret;
		}

		iio_init = 1;

		get_r_dts_info(node);

#ifndef CONFIG_TCPC_CLASS
		charging_diso_init(&DISO_data);
#endif
	}else{
		dc_usb_printk("find mediatek,dc_auxadc failed !!!\n");
		return -ENODEV;
	}

	return 0;
}


static const struct of_device_id dc_usb_auxadc_of_ids[] = {
	{.compatible = "mediatek,dc_usb_auxadc"},
	{}
};

static struct platform_driver dc_usb_auxadc_driver = {
	.driver = {
		.name = "dc_auxadc",
		.of_match_table = dc_usb_auxadc_of_ids,
	},
	.probe = get_dc_usb_auxadc_probe,
};

static int __init dc_usb_auxadc_init(void)
{
	int ret;

	ret = platform_driver_register(&dc_usb_auxadc_driver);
	if (ret) {
		dc_usb_printk("dc_usb auxadc driver init fail %d", ret);
		return ret;
	}
	return 0;
}

module_init(dc_usb_auxadc_init);

MODULE_AUTHOR("caozy");
MODULE_DESCRIPTION("dc_usb auxadc driver");
MODULE_LICENSE("GPL");
