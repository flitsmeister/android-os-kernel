#include "sitronix_ts.h"

#ifdef SITRONIX_MTK_TDP_PSENSOR_EN
#include <hwmsensor.h>
#include <alsps.h>

#define PS_FAR_AWAY		10
#define PS_NEAR			0

static int ps_state = PS_FAR_AWAY;

extern int ps_report_interrupt_data(int value);

static int st_proximity_get_ps_data(int proximity_status)
{
	int value;

	switch (proximity_status) {
	case 1:
		value = PS_FAR_AWAY;
		break;
	case 2:
		value = PS_FAR_AWAY;
		break;
	case 3:
		value = PS_NEAR;
		break;
	case 4:
		value = PS_NEAR;
		break;
	case 5:
		value = PS_FAR_AWAY;
		break;
	default:
		value = PS_FAR_AWAY;
		break;
	}

	return value;
}

/*
	For Display Proximity
	1. st_is_proximity_enabled();
	2. st_get_proximity_value();
*/
int st_is_proximity_enabled(void)
{  
	int enabled = false;

	if (gts) {
		enabled = gts->proximity_enabled;
	} else {
		sterr("%s: gts = 0x%X.", __func__, (unsigned int)gts);
	}

	return enabled;
}
EXPORT_SYMBOL(st_is_proximity_enabled);

int st_get_proximity_value(void)
{
	return ps_state;	//send to OS to controll backlight on/off
}
EXPORT_SYMBOL(st_get_proximity_value);


int st_proximity_report_ps_data(int proximity_status)
{
	ps_state = st_proximity_get_ps_data(proximity_status);
	ps_report_interrupt_data(ps_state);

	return 0;
}



static int ps_open_report_data(int open)
{
    /* should queue work to report event if  is_report_input_direct=true */
    return 0;
}

/* if use  this type of enable , Psensor only enabled but not report inputEvent to HAL */
static int ps_enable_nodata(int en)
{
	int err;

	stmsg("[PROXIMITY] SENSOR_ENABLE = %d.", en);

	ps_report_interrupt_data(PS_FAR_AWAY);		//Report PS_FAR_AWAY when enabled.

	/* Enable proximity */
	if (gts) {
		mutex_lock(&gts->mutex);
		err = sitronix_ts_proximity_enable(gts, en);
		mutex_unlock(&gts->mutex);
	} else {
		sterr("%s: gts = 0x%X.", __func__, (unsigned int)gts);
	}

	return 0;
}

static int ps_set_delay(u64 ns)
{
	return 0;
}

static int ps_get_data(int *value, int *status)
{
	uint8_t proximity_status = 0xFF;

	*value = ps_state;

	if (gts) {
		proximity_status = gts->proximity_status;
	} else {
		sterr("%s: gts = 0x%X.", __func__, (unsigned int)gts);
	}

	stmsg("%s: proximity_status = %d, value = %d\n", __func__, proximity_status, *value);

	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return 0;
}

static int ps_batch(int flag, int64_t sampling_period_ns, int64_t max_batch_report_ns)
{
	return 0;
}

static int ps_flush(void)
{
	return 0;
}

int sitronix_ps_local_init(void)
{
	int err = 0;
	struct ps_control_path ps_ctl = { 0 };
	struct ps_data_path ps_data = { 0 };

	ps_ctl.is_use_common_factory = false;
	ps_ctl.open_report_data = ps_open_report_data;
	ps_ctl.enable_nodata = ps_enable_nodata;
	ps_ctl.set_delay = ps_set_delay;
	ps_ctl.batch = ps_batch;
	ps_ctl.flush = ps_flush;
	ps_ctl.is_report_input_direct = false;
	ps_ctl.is_support_batch = false;

	sterr("Calling ps_register_control_path()...\n");
	err = ps_register_control_path(&ps_ctl);
	if (err) {
		sterr("ps register control path fail = %d\n", err);
	}

	ps_data.get_data = ps_get_data;
	ps_data.vender_div = 100;

	sterr("Calling ps_register_data_path()...\n");
	err = ps_register_data_path(&ps_data);
	if (err) {
		sterr("ps register data path fail = %d\n", err);
	}

	ps_state = PS_FAR_AWAY;

	return err;
}

int sitronix_ps_local_uninit(void)
{
	return 0;
}

struct alsps_init_info sitronix_ps_init_info = {
	.name = "sitronix_ps",
	.init = sitronix_ps_local_init,
	.uninit = sitronix_ps_local_uninit,
};

int st_proximity_init(void)
{
	stmsg("%s.\n", __func__);

	stmsg("Calling alsps_driver_add()...\n");
	alsps_driver_add(&sitronix_ps_init_info);

	return 0;
}

int st_proximity_exit(void)
{
	return 0;
}
#endif //SITRONIX_MTK_TDP_PSENSOR_EN

