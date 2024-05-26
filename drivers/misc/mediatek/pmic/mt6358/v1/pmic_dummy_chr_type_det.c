/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2016 MediaTek Inc.
 */


#include <generated/autoconf.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
//#include <linux/pm_wakeup.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <linux/seq_file.h>
#include <linux/power_supply.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/reboot.h>

#include <mt-plat/v1/mtk_battery.h>
#include <mt-plat/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <mt-plat/mtk_boot.h>
#include <mt-plat/v1/charger_type.h>
#include <mt-plat/v1/charger_class.h>

#define __SW_CHRDET_IN_PROBE_PHASE__

static enum charger_type g_chr_type;
#ifdef __SW_CHRDET_IN_PROBE_PHASE__
static struct work_struct chr_work;
#endif

//IPF460_UX30 add begin
#if defined(CONFIG_MTK_DC_USB_INPUT_CHARGER_SUPPORT)
#include <mt-plat/diso.h>
#include <mach/mtk_diso.h>
#include <linux/gpio.h>
#endif
//IPF460_UX30 add end

#if defined(CONFIG_CHARGER_SGM41516D) || defined(CONFIG_CHARGER_ETA696X)
static struct charger_device *primary_charger;
static int first_connect;
#endif

static DEFINE_MUTEX(chrdet_lock);

static struct power_supply *chrdet_psy;

//IPF460_UX30 add begin
#if !defined(CONFIG_MTK_DC_USB_INPUT_CHARGER_SUPPORT)
static int chrdet_inform_psy_changed(enum charger_type chg_type,
				bool chg_online)
#else
int chrdet_inform_psy_changed(enum charger_type chg_type,
				bool chg_online)
#endif
//IPF460_UX30 add end
{
	int ret = 0;
	union power_supply_propval propval;

	pr_debug("charger type: %s: online = %d, type = %d\n",
		__func__, chg_online, chg_type);

	/* Inform chg det power supply */
	if (chg_online) {
		propval.intval = chg_online;
		ret = power_supply_set_property(chrdet_psy,
			POWER_SUPPLY_PROP_ONLINE, &propval);
		if (ret < 0)
			pr_debug("%s: psy online failed, ret = %d\n",
				__func__, ret);

		propval.intval = chg_type;
		ret = power_supply_set_property(chrdet_psy,
			POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);
		if (ret < 0)
			pr_debug("%s: psy type failed, ret = %d\n",
				__func__, ret);

		return ret;
	}

	propval.intval = chg_type;
	ret = power_supply_set_property(chrdet_psy,
		POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);
	if (ret < 0)
		pr_debug("%s: psy type failed, ret(%d)\n",
			__func__, ret);

	propval.intval = chg_online;
	ret = power_supply_set_property(chrdet_psy,
		POWER_SUPPLY_PROP_ONLINE, &propval);
	if (ret < 0)
		pr_debug("%s: psy online failed, ret(%d)\n",
			__func__, ret);
	return ret;
}


int hw_charging_get_charger_type(void)
{
#if !defined(CONFIG_CHARGER_SGM41516D) && !defined(CONFIG_CHARGER_ETA696X)  //IPF460_UX30
	return STANDARD_HOST;
#else
	enum charger_type chr_type;
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6357)  //IPF460_UX30
	int timeout = 200;
	int boot_mode = get_boot_mode();

	pr_info("hw_bc11_init boot_mode = %d\n", boot_mode);

	msleep(200);
	if (boot_mode != RECOVERY_BOOT) {
		if (first_connect == true) {
			if (is_usb_rdy() == false) {
				while (is_usb_rdy() == false && timeout > 0) {
					msleep(100);
					timeout--;
				}
				if (timeout == 0)
					pr_info("CDP, timeout\n");
				else
					pr_info("CDP, free\n");
			} else
				pr_info("CDP, pass\n");
			first_connect = false;
		}
	}
	#endif
	chr_type = charger_dev_get_ext_chgtyp(primary_charger);
	return chr_type;
#endif
}

//IPF460_UX30 add begin
#if defined(CONFIG_MTK_DC_USB_INPUT_CHARGER_SUPPORT)
static unsigned int diso_get_current_voltage(int Channel)
{
	int ret = 0, data[4], i, ret_value = 0, ret_temp = 0, times = 5, vol = 0;

	if (IMM_IsAdcInitReady() == 0) {
		printk( "[DISO] AUXADC is not ready");
		return 0;
	}

	i = times;
	while (i--) {
		ret_value = IMM_GetOneChannelValue(Channel, data, &ret_temp);
		if (ret_value == 0) {
			ret += ret_temp;
		} else {
			times = times > 1 ? times - 1 : 1;
			printk( "[diso_get_current_voltage] ret_value=%d, times=%d\n",
			ret_value, times);
		}
	}

	ret = ret*1500/4096;
	ret = ret/times;

	if(Channel == AP_AUXADC_DISO_VDC_CHANNEL){
		vol = (R_DISO_DC_PULL_UP + R_DISO_DC_PULL_DOWN)*100*ret/(R_DISO_DC_PULL_DOWN)/100;
	}else if(Channel == AP_AUXADC_DISO_VUSB_CHANNEL){
		vol = (R_DISO_VBUS_PULL_UP + R_DISO_VBUS_PULL_DOWN)*100*ret/(R_DISO_VBUS_PULL_DOWN)/100;
	}

	return  vol;
}

int hw_charging_get_charger_type_c(void)
{
	enum charger_type CHR_Type_num = CHARGER_UNKNOWN;
	int dc_vol = 0, usb_vol = 0;
	int typec_id;

	dc_vol = diso_get_current_voltage(AP_AUXADC_DISO_VDC_CHANNEL);
	mdelay(100);
	usb_vol = diso_get_current_voltage(AP_AUXADC_DISO_VUSB_CHANNEL);
	printk("dc_vol ==%d    usb_vol ==%d\n",dc_vol,usb_vol);

	typec_id = gpio_get_value(41 + 64);	//GPIO41 -- IDDIG

	if(dc_vol > 4000 && usb_vol > 4000){
		if(typec_id > 0)
			CHR_Type_num = STANDARD_HOST;
		else
			CHR_Type_num = NONSTANDARD_CHARGER;
		return CHR_Type_num;
	}

	if(dc_vol > 4000){
		CHR_Type_num = NONSTANDARD_CHARGER;
		return CHR_Type_num;
	}

	if((usb_vol > 4000) && (typec_id > 0)){
		CHR_Type_num = hw_charging_get_charger_type();
		return CHR_Type_num;
	}

	return CHR_Type_num;
}
#endif
//IPF460_UX30 add end

/*****************************************************************************
 * Charger Detection
 ******************************************************************************/
void __attribute__((weak)) mtk_pmic_enable_chr_type_det(bool en)
{
}

void do_charger_detect(void)
{
//IPF460_UX30 add being
#if !defined(CONFIG_MTK_DC_USB_INPUT_CHARGER_SUPPORT)
	if (!mt_usb_is_device()) {
		g_chr_type = CHARGER_UNKNOWN;
		pr_debug("charger type: UNKNOWN, Now is usb host mode. Skip detection!!!\n");
		return;
	}
#endif
//IPF460_UX30 add being

	mutex_lock(&chrdet_lock);

	if (pmic_get_register_value(PMIC_RGS_CHRDET)) {
		pr_info("charger type: charger IN\n");
//IPF460_UX30 add being
#if !defined(CONFIG_MTK_DC_USB_INPUT_CHARGER_SUPPORT)
		g_chr_type = hw_charging_get_charger_type();
#else
		g_chr_type = hw_charging_get_charger_type_c();
#endif
//IPF460_UX30 add end
		if(g_chr_type != CHARGER_UNKNOWN)
			chrdet_inform_psy_changed(g_chr_type, 1);
		else
			chrdet_inform_psy_changed(g_chr_type, 0);
		printk("wang %s %d: %d\n",__func__,__LINE__,g_chr_type);
	} else {
		pr_info("charger type: charger OUT\n");
		g_chr_type = CHARGER_UNKNOWN;
		chrdet_inform_psy_changed(g_chr_type, 0);
	}

	mutex_unlock(&chrdet_lock);
}



/*****************************************************************************
 * PMIC Int Handler
 ******************************************************************************/
void chrdet_int_handler(void)
{
	/*
	 * pr_info("[chrdet_int_handler]CHRDET status = %d....\n",
	 *	pmic_get_register_value(PMIC_RGS_CHRDET));
	 */
#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	if (!pmic_get_register_value(PMIC_RGS_CHRDET)) {
		int boot_mode = 0;

		//boot_mode = get_boot_mode(); //IPF460_UX30

		if (boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT ||
		    boot_mode == LOW_POWER_OFF_CHARGING_BOOT) {
			pr_info("[%s] Unplug Charger/USB\n", __func__);
#ifndef CONFIG_TCPC_CLASS
			orderly_poweroff(true);
#else
			return;
#endif
		}
	}
#endif
	do_charger_detect();
}


/************************************************
 * Charger Probe Related
 ************************************************/
#ifdef __SW_CHRDET_IN_PROBE_PHASE__
static void do_charger_detection_work(struct work_struct *data)
{
	if (pmic_get_register_value(PMIC_RGS_CHRDET))
		do_charger_detect();
}
#endif

static int __init pmic_chrdet_init(void)
{
	mutex_init(&chrdet_lock);
	chrdet_psy = power_supply_get_by_name("charger");
	if (!chrdet_psy) {
		pr_debug("%s: get power supply failed\n", __func__);
		return -EINVAL;
	}

#if defined(CONFIG_CHARGER_SGM41516D) || defined(CONFIG_CHARGER_ETA696X)  //IPF460_UX30
	primary_charger = get_charger_by_name("primary_chg");
	if (!primary_charger) {
		pr_debug("%s: get primary charger device failed\n", __func__);
		return -EINVAL;
	}
	first_connect = true;
#endif

#ifdef __SW_CHRDET_IN_PROBE_PHASE__
	/* do charger detect here to prevent HW miss interrupt*/
	INIT_WORK(&chr_work, do_charger_detection_work);
	schedule_work(&chr_work);
#endif

	pmic_register_interrupt_callback(INT_CHRDET_EDGE, chrdet_int_handler);
	pmic_enable_interrupt(INT_CHRDET_EDGE, 1, "PMIC");

	return 0;
}

late_initcall(pmic_chrdet_init);
