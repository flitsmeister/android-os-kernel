#include "sitronix_ts.h"


static int busErrorCount;
static int fwStatusErrorCount;
static int sensingCounterErrorCount = 0;
static int distCheckErrorCount = 0;
static atomic_t iMonitorThreadPostpone = ATOMIC_INIT(0);
static uint8_t PreCheckData[4];
static int mtTpRstCount = 0;
//#define SITRONIX_MT_CHECK_DIS
#ifdef SITRONIX_MT_CHECK_DIS
//#define SITRONIX_MT_CHECK_DIS_IS_0
#endif //SITRONIX_MT_CHECK_DIS

//#define MT_RESET_ENTER_DSTB
#define MT_TP_RESET_MAX		3

int sitroinx_ts_check_display_off(void)
{
	uint8_t cmd[8] = {0};
	int ret = 0;
	int off = 0;
	
	cmd[1] = 0x53;
	cmd[2] = 0x71;
	cmd[3] = 0x23;
	cmd[4] = 0xA5;
	cmd[5] = 0x3C;
	ret = sitronix_ts_addrmode_write(gts, cmd, 5);
	
	cmd[1] = 0x53;
	cmd[2] = 0x71;
	cmd[3] = 0x23;
	cmd[4] = 0x14;
	cmd[5] = 0x55;
	ret = sitronix_ts_addrmode_write(gts, cmd, 5);
	
	
	cmd[1] = 0x03;
	cmd[2] = 0x06;
	cmd[3] = 0xFC;
	cmd[4] = 0x5A;
	cmd[5] = 0x9E;
	cmd[6] = 0x0A;
	cmd[7] = 0x00;
		
	ret = sitronix_ts_addrmode_write(gts, cmd, 7);
	if (ret < 0) {
		sterr("read display CMD1 fail - 1\n");
		return -1;
	}
	
	wbuf[1] = 0x83;
	wbuf[2] = 0x07;
	wbuf[3] = 0x00;
	
	off = sitronix_ts_addrmode_split_read(gts, wbuf, 3, rbuf, 2);
	
	if (off < 0) {
		sterr("read display CMD1 fail - 2\n");
		ret = -1;
	}
	
	cmd[1] = 0x03;
	cmd[2] = 0x06;
	cmd[3] = 0xFE;
	cmd[4] = 0x00;
	cmd[5] = 0xA5;
	
	sitronix_ts_addrmode_write(gts, cmd,5);
	
	if (ret < 0) {
		sterr("read display CMD1 fail - 3\n");
		return -1;
	}
	
	stmsg("Display status = 0x%X\n",rbuf[off]);
	
	cmd[1] = 0x53;
	cmd[2] = 0x71;
	cmd[3] = 0x23;
	cmd[4] = 0x41;
	cmd[5] = 0x4C;
	ret = sitronix_ts_addrmode_write(gts, cmd, 5);
	
	cmd[1] = 0x53;
	cmd[2] = 0x71;
	cmd[3] = 0x23;
	cmd[4] = 0x5A;
	cmd[5] = 0xC3;
	ret = sitronix_ts_addrmode_write(gts, cmd, 5);
	
	if( rbuf[off] & 0x04 )
		return 0;
	else 
		return 1;
}

void sitronix_ts_mt_reset_process(void)
{

#ifdef SITRONIX_HDL_IN_MT
	mutex_lock(&gts->mutex);
	sitronix_do_upgrade();
	mutex_unlock(&gts->mutex);
#else
	sitronix_ts_reset_device(gts);
#endif /* SITRONIX_HDL_IN_MT */

	/* restore status*/
	if( gts->in_suspend ) {
		mutex_lock(&gts->mutex);
		if (gts->swu_flag) {
			sitronix_ts_set_smart_wake_up(gts, true);			
		} else {
			sitronix_ts_set_smart_wake_up(gts, false);			
		}

		msleep(20);
		sitronix_ts_powerdown(gts, true);
		mutex_unlock(&gts->mutex);
	}
	
	mutex_lock(&gts->mutex);
	sitronix_mode_restore();
	mutex_unlock(&gts->mutex);

#ifdef MT_RESET_ENTER_DSTB
	mtTpRstCount++;
	if(mtTpRstCount > MT_TP_RESET_MAX){
		//do Sleep IN and Deep Standby
		stmsg("TP reset times have  exceeded the limit. Do Display ESD reset.\n");
		sitronix_write_driver_cmd(0x10, wbuf, 0);	//sleep in
		sitronix_swite_driver_deep_standby();		//deep standby
		mtTpRstCount = 0;	//reset tp reset count	
	}
#endif	//MT_RESET_ENTER_DSTB
}

int sitronix_ts_monitor_thread_v3(void *data)
{
	int ret = 0;
	uint8_t buf[12] = {0};	
//	uint8_t drvBuf[4] = {0};
	bool disStaus;
#ifdef SITRONIX_MT_CHECK_DIS	
	int i;
	signed short disv;
	bool distOK; 
	uint8_t disbuf[40*2+4] = {0};
# ifdef SITRONIX_MT_CHECK_DIS_IS_0
	bool dist_is_0 = false;
# endif //SITRONIX_MT_CHECK_DIS_IS_0
#endif //SITRONIX_MT_CHECK_DIS
	int mt_period = DELAY_MONITOR_THREAD_PEROID_NORMAL;

	disStaus = true; 
	stmsg("%s start and delay %d ms\n", __func__, gts->sitronix_ts_delay_monitor_thread_start);
	mtTpRstCount = 0;

	msleep(gts->sitronix_ts_delay_monitor_thread_start);
	while (!kthread_should_stop()) {
		if (gts->is_reset_chip) {
			sitronix_ts_mt_reset_process();
			gts->is_reset_chip = false;
		} else if( gts->is_suspend_mt ) {
			stdbg("MT suspended\n");
		} else if (gts->is_pause_mt || gts->upgrade_doing) {
			stdbg("MT paused\n");
		} else if (atomic_read(&iMonitorThreadPostpone)) {
			atomic_set(&iMonitorThreadPostpone, 0);
		} else {
			mutex_lock(&gts->mutex);
			ret = sitronix_ts_reg_read(gts, FIRMWARE_VERSION, buf, 12);			
#ifdef SITRONIX_MT_CHECK_DIS			
			ret = sitronix_ts_reg_read(gts, DATA_OUTPUT_BUFFER, disbuf, 4+(gts->ts_dev_info.y_chs*2));
#endif //SITRONIX_MT_CHECK_DIS
			mutex_unlock(&gts->mutex);

#if 0 //get dirver display status 0x09
			if(!gts->in_suspend){
				//check driver display status		
				ret = sitronix_read_driver_cmd(0x09, drvBuf, sizeof(drvBuf));
				if(ret >= 0){
					stmsg("display status = 0x%02X 0x%02X 0x%02X  0x%02X \n", drvBuf[0], drvBuf[1], drvBuf[2], drvBuf[3]);
					if(drvBuf[0] !=  0x80 || drvBuf[1] != 0x03 ||
						 drvBuf[2] != 0x06 || drvBuf[3] != 0x00){
						//reset driver
						if(disStaus){
							sterr("display status error, reset driver!\n");
							sitronix_write_driver_cmd(0x28, wbuf, 0);
							ret = -1;
							disStaus = false;
							msleep(DELAY_MONITOR_THREAD_PEROID_NORMAL);	//wait for driver esd check
							goto exit_i2c_invalid;
						}
					}
					else{
						disStaus = true;
					}
				}
				else{
					sterr("Failed to read display status(0x09)!\n");
				}
			}//end of if(!gts->in_suspend)
#endif 

			stmsg("monitor sensing counter: %02x %02x\n", buf[0xA], buf[0xB]);

			//Bus read failed.
			if (ret < 0) {
				sterr("Bus read failed. (ret=%d)\n", ret);
				busErrorCount++;
				if (busErrorCount >= 2) {
					stmsg("Bus abnormal, reset it!\n");
					sitronix_ts_mt_reset_process();
					mt_period = DELAY_MONITOR_THREAD_PEROID_NORMAL;
					busErrorCount = 0;
				}
				continue;
			} else {
				busErrorCount = 0;
				mtTpRstCount = 0;
			}

			//Check status.
			if ((buf[1] & 0x0F) == 0x6) {
				sterr("Read FW status: bootcode.\n");
				fwStatusErrorCount++;
				if (fwStatusErrorCount >= 2) {
					stmsg("FW status is bootcode, reset it!\n");
					sitronix_ts_mt_reset_process();
					mt_period = DELAY_MONITOR_THREAD_PEROID_NORMAL;
					fwStatusErrorCount = 0;
				}
				continue;
			} else {
				fwStatusErrorCount = 0;
				mtTpRstCount = 0;
			}

			//Check sensing counter.
			if (PreCheckData[0] == buf[0xA] && PreCheckData[1] == buf[0xB]) {
				mutex_lock(&gts->mutex);
				if ( sitroinx_ts_check_display_off() == 1) {
					mt_period = DELAY_MONITOR_THREAD_PEROID_NORMAL;
				} else {
					mt_period = DELAY_MONITOR_THREAD_PEROID_ERROR;
				}
				mutex_unlock(&gts->mutex);
				sensingCounterErrorCount++;
				if (sensingCounterErrorCount >= 3) {
					sterr("IC Status doesn't update!\n");
					sterr("ESD detected chip abnormal, reset device!\n");
					sitronix_ts_mt_reset_process();
					mt_period = DELAY_MONITOR_THREAD_PEROID_NORMAL;
					sensingCounterErrorCount = 0;
				}
			} else {
				PreCheckData[0] = buf[0xA];
				PreCheckData[1] = buf[0xB];
				sensingCounterErrorCount = 0;
				mtTpRstCount = 0;
			}

#ifdef SITRONIX_MT_CHECK_DIS
			//Check FW Dist data.
			if (disbuf[0] == 0x93) {
				distOK = true;
				//Check FW Dist out of range.
				for (i = 0; i < gts->ts_dev_info.y_chs; i++) {
					disv = (signed short)((disbuf[4+2*i])*0x100 + disbuf[5+2*i]);
					if (disv > SITRONIX_MT_DIS_LIMIT || disv < -SITRONIX_MT_DIS_LIMIT) {
						sterr("MT get error Distance for (%d,%d) , distance value = %d\n", disbuf[2], i, disv);
						distOK = false;
						break;
					}
				}
# ifdef SITRONIX_MT_CHECK_DIS_IS_0
				//Check FW Dists of one line are all 0.
				dist_is_0 = true;
				for (i = 0; i < gts->ts_dev_info.y_chs; i++) {
					disv = (signed short)((disbuf[4+2*i])*0x100 + disbuf[5+2*i]);
					if (disv != 0) {
						dist_is_0 = false;
						break;
					}
				}

				if (dist_is_0) {
					sterr("MT get error Distance, all distance value = 0\n");
					distOK = false;
				}
# endif //SITRONIX_MT_CHECK_DIS_IS_0

				if (!distOK) {
					distCheckErrorCount++;
					mt_period = DELAY_MONITOR_THREAD_PEROID_ERROR;
					if (distCheckErrorCount >= 3) {
						sterr("Distance error for 3 times!\n");
						sitronix_ts_mt_reset_process();
						mt_period = DELAY_MONITOR_THREAD_PEROID_NORMAL;
					}
				} else {
					distCheckErrorCount = 0;
					mtTpRstCount = 0;
				}
			}
#endif	//SITRONIX_MT_CHECK_DIS
		}

		msleep(mt_period);
		mt_period = DELAY_MONITOR_THREAD_PEROID_NORMAL;
	}
	stdbg("%s exit\n", __func__);
	return 0;
}

void sitronix_mt_pause_one(void)
{
	if (gts->enable_monitor_thread == 1)
		atomic_set(&iMonitorThreadPostpone, 1);

}


void sitronix_mt_pause(void)
{
#ifdef SITRONIX_MONITOR_THREAD
	gts->is_pause_mt = 1;
#endif

}

void sitronix_mt_restore(void)
{

#ifdef SITRONIX_MONITOR_THREAD
	gts->is_pause_mt = 0;
#endif

}

void sitronix_mt_suspend(void)
{
	gts->is_suspend_mt = 1;
}

void sitronix_mt_resume(void)
{
	sitronix_mt_pause_one();
	gts->is_suspend_mt = 0;
}

void sitronix_mt_stop(void)
{
	stmsg("sitronix_mt_stop\n");
	if (gts && gts->enable_monitor_thread == 1) {
		if (gts->SitronixMonitorThread) {
			kthread_stop(gts->SitronixMonitorThread);
			gts->SitronixMonitorThread = NULL;
		}
	}
}


void sitronix_mt_start(int startDelayMS)
{
	stmsg("sitronix_mt_start\n");
	if (gts && gts->enable_monitor_thread == 1) {
		/* atomic_set(&ts_data->iMonitorThreadPostpone, 1); */
		sitronix_mt_pause_one();
		busErrorCount = 0;
		fwStatusErrorCount = 0;
		sensingCounterErrorCount = 0;
		distCheckErrorCount = 0;
		gts->sitronix_ts_delay_monitor_thread_start = startDelayMS;
		if (!gts->SitronixMonitorThread)
			gts->SitronixMonitorThread = kthread_run(gts->sitronix_mt_fp, gts, "Sitronix Monitor Thread");
		if (IS_ERR(gts->SitronixMonitorThread))
			gts->SitronixMonitorThread = NULL;
	}
}
