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

/* stk83xx.c - stk83xx accelerometer (driver)
 *
 * Author: STK
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
#include "stk83xx.h"

_stkOdrMap stk_odr_map[20];

/* SENSOR_HZ(_hz) = ((uint32_t)((_hz)*1024.0f)) */
static const _stkOdrMap stkODRTable[] =
{
    /* 0: ODR = 1Hz */
    {
        .sample_rate_us = 1000000,
        .regBwsel = STK83XX_BWSEL_BW_7_81,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 15
    },
    /* 1: ODR = 5Hz */
    {
        .sample_rate_us = 200000,
        .regBwsel = STK83XX_BWSEL_BW_7_81,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 0
    },
    /* 2: ODR = 15.62Hz */
    {
        .sample_rate_us = 62500,
        .regBwsel = STK83XX_BWSEL_BW_7_81,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 0
    },
    /* 3: ODR = 31.26Hz */
    {
        .sample_rate_us = 32000,
        .regBwsel = STK83XX_BWSEL_BW_15_63,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 0
    },
    /* 4: ODR = 50Hz */
    {
        .sample_rate_us = 20000,
        .regBwsel = STK83XX_BWSEL_BW_125,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 5
    },
    /* 5: ODR = 125Hz */
    {
        .sample_rate_us = 8000,
        .regBwsel = STK83XX_BWSEL_BW_62_5,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 0
    },
    /* 6: 200Hz */
    {
        .sample_rate_us = 5000,
        .regBwsel = STK83XX_BWSEL_BW_500,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 5
    },
    /* 7: 250Hz */
    {
        .sample_rate_us = 4000,
        .regBwsel = STK83XX_BWSEL_BW_125,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 0
    },
    /* 7: ODR = 400Hz */
    {
        .sample_rate_us = 2500,
        .regBwsel = STK83XX_BWSEL_BW_250,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 0
    },
};

/* ESM mode only for stk8327 & stk8329 */
static const _stkOdrMap stkESMTable[] =
{
    /* 0: ODR = 1Hz */
    {
        .sample_rate_us = 1000000,
        .regBwsel = STK83XX_BWSEL_BW_7_81,
        .regPwsel = STK83XX_PWMD_LOWPOWER | STK83XX_PWMD_SLEEP_TIMER | STK83XX_PWMD_1,
        .drop = 0
    },
    /* 1: ODR = 2Hz */
    {
        .sample_rate_us = 500000,
        .regBwsel = STK83XX_BWSEL_BW_7_81,
        .regPwsel = STK83XX_PWMD_LOWPOWER | STK83XX_PWMD_SLEEP_TIMER | STK83XX_PWMD_2,
        .drop = 0
    },
    /* 2: ODR = 5Hz (normal mode + FIFO) */
    {
        .sample_rate_us = 200000,
        .regBwsel = STK83XX_BWSEL_BW_7_81,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 0

    },
    /* 3: ODR = 10Hz */
    {
        .sample_rate_us = 100000,
        .regBwsel = STK83XX_BWSEL_BW_31_25,
        .regPwsel = STK83XX_PWMD_LOWPOWER | STK83XX_PWMD_SLEEP_TIMER | STK83XX_PWMD_10,
        .drop = 0
    },
    /* 4: ODR = 16Hz (normal mode)*/
    {
        .sample_rate_us = 62500,
        .regBwsel = STK83XX_BWSEL_BW_7_81,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 0
    },
    /* 5: ODR = 20Hz */
    {
        .sample_rate_us = 50000,
        .regBwsel = STK83XX_BWSEL_BW_62_5,
        .regPwsel = STK83XX_PWMD_LOWPOWER | STK83XX_PWMD_SLEEP_TIMER | STK83XX_PWMD_20,
        .drop = 0
    },
    /* 6: ODR = 25Hz */
    {
        .sample_rate_us = 40000,
        .regBwsel = STK83XX_BWSEL_BW_62_5,
        .regPwsel = STK83XX_PWMD_LOWPOWER | STK83XX_PWMD_SLEEP_TIMER | STK83XX_PWMD_25,
        .drop = 0
    },
    /* 7: ODR = 31Hz (normal mode)*/
    {
        .sample_rate_us = 32000,
        .regBwsel = STK83XX_BWSEL_BW_15_63,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 0
    },
    /* 8: ODR = 50Hz */
    {
        .sample_rate_us = 20000,
        .regBwsel = STK83XX_BWSEL_BW_62_5,
        .regPwsel = STK83XX_PWMD_LOWPOWER | STK83XX_PWMD_SLEEP_TIMER | STK83XX_PWMD_50,
        .drop = 0
    },
    /* 9: ODR = 62.5Hz (normal mode)*/
    {
        .sample_rate_us = 16000,
        .regBwsel = STK83XX_BWSEL_BW_31_25,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 0
    },
    /* 10: ODR = =100Hz */
    {
        .sample_rate_us = 10000,
        .regBwsel = STK83XX_BWSEL_BW_250,
        .regPwsel = STK83XX_PWMD_LOWPOWER | STK83XX_PWMD_SLEEP_TIMER | STK83XX_PWMD_100,
        .drop = 0
    },
    /* 11: ODR = 125Hz (normal mode)*/
    {
        .sample_rate_us = 8000,
        .regBwsel = STK83XX_BWSEL_BW_62_5,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 0
    },
    /* 12: ODR = =200Hz */
    {
        .sample_rate_us = 5000,
        .regBwsel = STK83XX_BWSEL_BW_500,
        .regPwsel = STK83XX_PWMD_LOWPOWER | STK83XX_PWMD_SLEEP_TIMER | STK83XX_PWMD_200,
        .drop = 0
    },
    /* 13: ODR = 250Hz (normal mode)*/
    {
        .sample_rate_us = 4000,
        .regBwsel = STK83XX_BWSEL_BW_125,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 0
    },
    /* 14: ODR = =300Hz */
    {
        .sample_rate_us = 3333,
        .regBwsel = STK83XX_BWSEL_BW_500,
        .regPwsel = STK83XX_PWMD_LOWPOWER | STK83XX_PWMD_SLEEP_TIMER | STK83XX_PWMD_300,
        .drop = 0
    },
    /* 15: ODR = 400Hz (normal mode + FIFO)*/
    {
        .sample_rate_us = 2500,
        .regBwsel = STK83XX_BWSEL_BW_250,
        .regPwsel = STK83XX_PWMD_NORMAL,
        .drop = 0
    },
};

/*	direction settings	*/
const int coordinate_trans[8][3][3] =
{
    /* x_after, y_after, z_after */
    {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
    {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
    {{0, 1, 0}, {-1, 0, 0}, {0, 0, 1}},
    {{-1, 0, 0}, {0, -1, 0}, {0, 0, 1}},
    {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}},
    {{-1, 0, 0}, {0, 1, 0}, {0, 0, -1}},
    {{0, -1, 0}, {-1, 0, 0}, {0, 0, -1}},
    {{1, 0, 0}, {0, -1, 0}, {0, 0, -1}},
};

void stk_register_queue(struct stk_data *stk)
{
#ifdef STK_INTERRUPT_MODE
    int err = 0;
    STK_ACC_LOG("register INT mode");
#if 0
    // need to request int_pin in sar and use common_gpio_mtk.c
    if (gpio_request(stk->gpio_info.int_pin, "stk_accel_int"))
    {
        STK_ACC_ERR("gpio_request failed");
        return;
    }
#endif

    STK_ACC_LOG("gpio_request int=%d", stk->gpio_info.int_pin);
    strcpy(stk->gpio_info.wq_name, "stk_accel_int");
    strcpy(stk->gpio_info.device_name, "stk_accel_irq");
    stk->gpio_info.gpio_cb = stk_work_queue;
    stk->gpio_info.trig_type = TRIGGER_FALLING;
    stk->gpio_info.is_active = false;
    stk->gpio_info.is_exist = false;
    stk->gpio_info.any = stk;
    err = STK_GPIO_IRQ_REGISTER(stk, &stk->gpio_info);
    err |= STK_GPIO_IRQ_START(stk, &stk->gpio_info);

    if (0 > err)
    {
        STK_ACC_ERR("STK_GPIO_IRQ_REGISTER failed");
        return;
    }

#elif defined STK_POLLING_MODE
    STK_ACC_LOG("register Polling mode");
    strcpy(stk->stk_timer_info.wq_name, "stk_wq");
    stk->stk_timer_info.timer_unit = U_SECOND;
    stk->stk_timer_info.interval_time = STK_POLLING_TIME;
    stk->stk_timer_info.timer_cb = stk_work_queue;
    stk->stk_timer_info.is_active = false;
    stk->stk_timer_info.is_exist = false;
    stk->stk_timer_info.any = stk;
    STK_TIMER_REGISTER(stk, &stk->stk_timer_info);
#endif /* STK_INTERRUPT_MODE, STK_POLLING_MODE */
}

/*
 * @brief: Initialize some data in stk_data.
 *
 * @param[in/out] stk: struct stk_data *
 */
void stk_data_initialize(struct stk_data *stk)
{
    atomic_set(&stk->enabled, 0);
    stk->temp_enable = false;
    stk->sr_no = stk->odr_table_count - 1;
    atomic_set(&stk->selftest, STK_SELFTEST_RESULT_NA);
#ifdef STK_CALI
    memset(stk->cali_sw, 0x0, sizeof(stk->cali_sw));
    atomic_set(&stk->cali_status, STK_K_NO_CALI);
#endif /* STK_CALI */
#ifdef STK_HW_STEP_COUNTER
    stk->steps = 0;
#endif /* STK_HW_STEP_COUNTER */
#if defined(STK_FIR) || defined(STK_AUTO_K)
    memset(&stk->data_log, 0, sizeof(struct data_log));
    stk->data_log_len = STK_DATA_LOG_LEN;
#endif /* STK_FIR or STK_AUTO_K */
    STK_ACC_LOG("done");
}

/*
 * @brief: SW reset for stk83xx
 *
 * @param[in/out] stk: struct stk_data *
 *
 * @return: Success or fail.
 *          0: Success
 *          others: Fail
 */
static int stk_sw_reset(struct stk_data *stk)
{
    int err = 0;
    err = STK_REG_WRITE(stk, STK83XX_REG_SWRST, STK83XX_SWRST_VAL);

    if (err < 0)
        return err;

    STK_TIMER_BUSY_WAIT(stk, 1000, 2000, US_RANGE_DELAY);
    stk->power_mode = STK83XX_PWMD_NORMAL;
    atomic_set(&stk->enabled, 1);
    return 0;
}

/*
 * @brief: Read PID and write to stk_data.pid.
 *
 * @param[in/out] stk: struct stk_data *
 *
 * @return: Success or fail.
 *          0: Success
 *          others: Fail
 */
int stk_get_pid(struct stk_data *stk)
{
    int err = 0;
    err = STK_REG_READ(stk, STK83XX_REG_CHIPID, &stk->pid);

    if (err < 0)
    {
        return -1;
    }

    switch (stk->pid)
    {
    case STK8BA50_R_ID:
        stk->data_shift = 6;// 10bit
        stk->fifo = false;
        stk->is_esm_mode = false;
        memcpy(&stk_odr_map, &stkODRTable, sizeof(stkODRTable));
        stk->odr_table_count = sizeof(stkODRTable) / sizeof(_stkOdrMap);
        break;

    case STK8BA53_ID:
        stk->data_shift = 4;// 12bit
        stk->fifo = false;
        stk->is_esm_mode = false;
        memcpy(&stk_odr_map, &stkODRTable, sizeof(stkODRTable));
        stk->odr_table_count = sizeof(stkODRTable) / sizeof(_stkOdrMap);
        break;

    case STK8323_ID:
        stk->data_shift = 4;// 12bit
        stk->fifo = true;
        stk->is_esm_mode = false;
        memcpy(&stk_odr_map, &stkODRTable, sizeof(stkODRTable));
        stk->odr_table_count = sizeof(stkODRTable) / sizeof(_stkOdrMap);
        break;

    case STK8327_ID:
    case STK8329_ID:
        stk->data_shift = 0;// 16bit
        stk->fifo = true;
        stk->is_esm_mode = true;
        memcpy(&stk_odr_map, &stkESMTable, sizeof(stkESMTable));
        stk->odr_table_count = sizeof(stkESMTable) / sizeof(_stkOdrMap);
        break;

    default:
        STK_ACC_ERR("Cannot find correct PID: 0x%X", stk->pid);
        break;
    }

    return 0;
}

#ifdef STK_AMD
/*
 * @brief: Read any motion data, then report to userspace.
 *
 * @param[in/out] stk: struct stk_data *
 */
void stk_read_anymotion_data(struct stk_data *stk)
{
    int err = 0;
    u8 data = 0;
    err = STK_REG_READ(stk, STK83XX_REG_INTSTS1, &data);

    if (err < 0)
        return;

    if (STK83XX_INTSTS1_ANY_MOT_STS & data)
    {
        STK83XX_AMD_REPORT(stk, true);
    }
    else
    {
        STK83XX_AMD_REPORT(stk, false);
    }
}

/*
 * @brief: Trigger INT_RST for latched STS
 *
 * @param[in/out] stk: struct stk_data *
 */
static void stk_reset_latched_int(struct stk_data *stk)
{
    int ret = 0;
    u8 data = 0;
    ret = STK_REG_READ(stk, STK83XX_REG_INTCFG2, &data);

    if (ret > 0)
    {
        STK_REG_WRITE(stk, STK83XX_REG_INTCFG2, (data | STK83XX_INTCFG2_INT_RST));
    }
}
#endif /* STK_AMD */

#ifdef STK_6D_TILT
void stk_process_6d_tilt_status(struct stk_data* stk)
{
    int ret = 0;
    u8 data = 0;
    u8 int_value = ~(STK83XX_SIXDCFG_6D_CTRL_LOW_X |
                     STK83XX_SIXDCFG_6D_CTRL_HIGH_X |
                     STK83XX_SIXDCFG_6D_CTRL_LOW_Y |
                     STK83XX_SIXDCFG_6D_CTRL_HIGH_Y |
                     STK83XX_SIXDCFG_6D_CTRL_LOW_Z |
                     STK83XX_SIXDCFG_6D_CTRL_HIGH_Z |
                     STK83XX_SIXDCFG_6D_MODE_MASK);

    ret = STK_REG_READ(stk, STK83XX_REG_SIXDSTS, &data);

    STK_ACC_ERR("stk_process_6d_tilt_status:: 6D_status=0x%x", data);
    switch (data)
    {
    case STK83XX_SIXDSTS_X_L_MASK:
        STK_ACC_ERR("stk_process_6d_tilt_status:: X- direction");
        int_value |= STK83XX_SIXDCFG_6D_CTRL_LOW_X;
        break;
    case STK83XX_SIXDSTS_X_H_MASK:
        STK_ACC_ERR("stk_process_6d_tilt_status:: X+ direction");
        int_value |= STK83XX_SIXDCFG_6D_CTRL_HIGH_X;
        break;
    case STK83XX_SIXDSTS_Y_L_MASK:
        STK_ACC_ERR("stk_process_6d_tilt_status:: Y- direction");
        int_value |= STK83XX_SIXDCFG_6D_CTRL_LOW_Y;
        break;
    case STK83XX_SIXDSTS_Y_H_MASK:
        STK_ACC_ERR("stk_process_6d_tilt_status:: Y+ direction");
        int_value |= STK83XX_SIXDCFG_6D_CTRL_HIGH_Y;
        break;
    case STK83XX_SIXDSTS_Z_L_MASK:
        STK_ACC_ERR("stk_process_6d_tilt_status:: Z- direction");
        int_value |= STK83XX_SIXDCFG_6D_CTRL_LOW_Z;
        break;
    case STK83XX_SIXDSTS_Z_H_MASK:
        STK_ACC_ERR("stk_process_6d_tilt_status:: Z+ direction");
        int_value |= STK83XX_SIXDCFG_6D_CTRL_HIGH_Z;
        break;
    default:
        STK_ACC_ERR("stk_process_6d_tilt_status:: Two direction, wait next trigger");
        break;
    }

    //disable current int
    int_value = ~int_value;
    STK_ACC_ERR("stk_process_6d_tilt_status:: 0x1B=0x%x", int_value);
    STK_REG_WRITE(stk, STK83XX_REG_SIXDCFG, int_value);
}
#endif // STK_6D_TILT

/*
 * @brief: Get sensitivity. Set result to stk_data.sensitivity.
 *          sensitivity = number bit per G (LSB/g)
 *          Example: RANGESEL=8g, 12 bits for STK8xxx full resolution
 *          Ans: number bit per G = 2^12 / (8x2) = 256 (LSB/g)
 *
 * @param[in/out] stk: struct stk_data *
 */
static void stk_get_sensitivity(struct stk_data *stk)
{
    u8 val = 0;
    int err = 0;
    stk->sensitivity = 0;
    err = STK_REG_READ(stk, STK83XX_REG_RANGESEL, &val);

    if (err >= 0)
    {
        val = val & STK83XX_RANGESEL_BW_MASK;

        switch (val)
        {
        case STK83XX_RANGESEL_2G:
            if (stk->pid == STK8327_ID ||
                stk->pid == STK8329_ID)
                stk->sensitivity = 16384;
            else if (stk->pid == STK8BA50_R_ID)
                stk->sensitivity = 256;
            else
                stk->sensitivity = 1024;

            break;

        case STK83XX_RANGESEL_4G:
            if (stk->pid == STK8327_ID ||
                stk->pid == STK8329_ID)
                stk->sensitivity = 8192;
            else if (stk->pid == STK8BA50_R_ID)
                stk->sensitivity = 128;
            else
                stk->sensitivity = 512;

            break;

        case STK83XX_RANGESEL_8G:
            if (stk->pid == STK8327_ID ||
                stk->pid == STK8329_ID)
                stk->sensitivity = 4096;
            else if (stk->pid == STK8BA50_R_ID)
                stk->sensitivity = 64;
            else
                stk->sensitivity = 256;

            break;

        case STK83XX_RANGESEL_16G:
            stk->sensitivity = 2048;
            break;

        default:
            STK_ACC_ERR("got wrong RANGE: 0x%X", val);
            break;
        }
    }
}

/*
 * @brief: Set range
 *          1. Setting STK83XX_REG_RANGESEL
 *          2. Calculate sensitivity and store to stk_data.sensitivity
 *
 * @param[in/out] stk: struct stk_data *
 * @param[in] range: range for STK83XX_REG_RANGESEL
 *              STK_2G
 *              STK_4G
 *              STK_8G
 *
 * @return: Success or fail
 *          0: Success
 *          others: Fail
 */
int stk_range_selection(struct stk_data *stk, stk_rangesel range)
{
    int err = 0;
    err = STK_REG_WRITE(stk, STK83XX_REG_RANGESEL, range);

    if (err < 0)
        return err;

    stk_get_sensitivity(stk);
    return 0;
}

/*
 * @brief: Change power mode
 *
 * @param[in/out] stk: struct stk_data *
 * @param[in] pwd_md: power mode for STK83XX_REG_POWMODE
 *              STK83XX_PWMD_SUSPEND
 *              STK83XX_PWMD_LOWPOWER
 *              STK83XX_PWMD_NORMAL
 *
 * @return: Success or fail
 *          0: Success
 *          others: Fail
 */
static int stk_change_power_mode(struct stk_data *stk, u8 pwd_md)
{
    if (pwd_md != stk->power_mode)
    {
        int err = 0;
        u8 reg = STK83XX_REG_POWMODE, val = 0;
        err = STK_REG_READ(stk, reg, &val);

        if (err < 0)
            return err;

        val = val & STK83XX_PWMD_SLP_MASK;
        err = STK_REG_WRITE(stk, reg, (val | pwd_md));

        if (err < 0)
            return err;

        stk->power_mode = pwd_md;
    }
    else
    {
        STK_ACC_ERR("Same as original power mode: 0x%X", stk->power_mode);
    }

    return 0;
}

/*
 * stk_set_enable
 * @brief: Turn ON/OFF the power state of stk83xx.
 *
 * @param[in/out] stk: struct stk_data *
 * @param[in] en: turn ON/OFF
 *              0 for suspend mode;
 *              1 for normal mode.
 */
void stk_set_enable(struct stk_data *stk, char en)
{
    if (en == atomic_read(&stk->enabled))
        return;

    if (en)
    {
        if (stk_change_power_mode(stk, stk_odr_map[stk->sr_no].regPwsel))
        {
            STK_ACC_ERR("failed to change power mode, en:%d", en);
            return;
        }
#ifdef STK_AXES_SIM
        stk_axes_sim_reset();
#endif // STK_AXES_SIM

#ifdef STK_INTERRUPT_MODE
        /* do nothing */
#elif defined STK_POLLING_MODE
        STK_TIMER_START(stk, &stk->stk_timer_info);
#endif /* STK_INTERRUPT_MODE, STK_POLLING_MODE */
    }
    else
    {
        if (stk_change_power_mode(stk, STK83XX_PWMD_SUSPEND))
        {
            STK_ACC_ERR("failed to change power mode, en:%d", en);
            return;
        }

#ifdef STK_INTERRUPT_MODE
        /* do nothing */
#elif defined STK_POLLING_MODE
        STK_TIMER_STOP(stk, &stk->stk_timer_info);
#endif /* STK_INTERRUPT_MODE, STK_POLLING_MODE */
    }

#ifdef STK_AMD
    STK83XX_AMD_REPORT(stk, false);
#endif /* STK_AMD */
    atomic_set(&stk->enabled, en);
}

/*
 * @brief: Get delay
 *
 * @param[in/out] stk: struct stk_data *
 *
 * @return: delay in usec
 */
int stk_get_delay(struct stk_data *stk)
{
    return stk_odr_map[stk->sr_no].sample_rate_us;
}

/*
 * @brief: Set delay
 *
 * @param[in/out] stk: struct stk_data *
 * @param[in] delay_us: delay in usec
 *
 * @return: Success or fail
 *          0: Success
 *          others: Fail
 */
int stk_set_delay(struct stk_data *stk, int delay_us)
{
    int err = 0;
    bool enable = false;
    int sr_no;

    for (sr_no = 0; sr_no < stk->odr_table_count; sr_no++)
    {
        if (delay_us >= stk_odr_map[sr_no].sample_rate_us)
            break;
    }

    if (sr_no >= stk->odr_table_count)
    {
        sr_no = stk->odr_table_count - 1;
    }

    stk->sr_no = sr_no;

    if (atomic_read(&stk->enabled))
    {
        stk_set_enable(stk, 0);
        enable = true;
    }

    if (stk->is_esm_mode)
    {
        err = STK_REG_WRITE(stk, STK83XX_REG_ODRMODE, STK83XX_ODR_ESMMODE);

        if (err < 0)
        {
            STK_ACC_ERR("failed to set ESM mode");
        }
    }

    err = STK_REG_WRITE(stk, STK83XX_REG_POWMODE, stk_odr_map[sr_no].regPwsel);

    if (err < 0)
    {
        STK_ACC_ERR("failed to set Powmode");
    }

    err = STK_REG_WRITE(stk, STK83XX_REG_BWSEL, stk_odr_map[sr_no].regBwsel);

    if (err < 0)
    {
        STK_ACC_ERR("failed to change ODR");
    }

#ifdef STK_INTERRUPT_MODE
    /* do nothing */
#elif defined STK_POLLING_MODE
    stk->stk_timer_info.interval_time = stk_odr_map[sr_no].sample_rate_us;
#endif /* STK_INTERRUPT_MODE, STK_POLLING_MODE */

    if (enable)
    {
        stk_set_enable(stk, 1);
    }

    return err;
}

/*
 * @brief: Get offset
 *
 * @param[in/out] stk: struct stk_data *
 * @param[out] offset: offset value read from register
 *                  STK83XX_REG_OFSTX,  STK83XX_REG_OFSTY, STK83XX_REG_OFSTZ
 *
 * @return: Success or fail
 *          0: Success
 *          -1: Fail
 */
int stk_get_offset(struct stk_data *stk, u8 offset[3])
{
    int err = 0;
    int idx = 0;
    uint8_t REG_TABLE[] =
    {
        STK83XX_REG_OFSTX,
        STK83XX_REG_OFSTY,
        STK83XX_REG_OFSTZ
    };

    for (idx = 0; idx < sizeof(REG_TABLE) / sizeof(uint8_t); idx++)
    {
        err = STK_REG_READ(stk, REG_TABLE[idx], &offset[idx]);

        if (err < 0)
        {
            return -1;
        }
    }

    return 0;
}

/*
 * @brief: Set offset
 *
 * @param[in/out] stk: struct stk_data *
 * @param[in] offset: offset value write to register
 *                  STK83XX_REG_OFSTX,  STK83XX_REG_OFSTY, STK83XX_REG_OFSTZ
 *
 * @return: Success or fail
 *          0: Success
 *          -1: Fail
 */
int stk_set_offset(struct stk_data *stk, u8 offset[3])
{
    int err = 0;
    bool enable = false;

    if (!atomic_read(&stk->enabled))
        stk_set_enable(stk, 1);
    else
        enable = true;

    err = STK_REG_WRITE(stk, STK83XX_REG_OFSTX, offset[0]);

    if (err < 0)
    {
        goto exit;
    }

    err = STK_REG_WRITE(stk, STK83XX_REG_OFSTY, offset[1]);

    if (err < 0)
    {
        goto exit;
    }

    err = STK_REG_WRITE(stk, STK83XX_REG_OFSTZ, offset[2]);

    if (err < 0)
    {
        goto exit;
    }

exit:

    if (!enable)
        stk_set_enable(stk, 0);

    return err;
}

/*
 * @brief: Read FIFO data
 *
 * @param[in/out] stk: struct stk_data *
 * @param[out] fifo: FIFO data
 * @param[in] len: FIFO size what you want to read
 */
void stk_fifo_reading(struct stk_data *stk, u8 fifo[], int len)
{
    if (!stk->fifo)
    {
        STK_ACC_ERR("No fifo data");
        return;
    }

    /* Reject all register R/W to protect FIFO data reading */
    STK_ACC_LOG("Start to read FIFO data");

    if (0 > STK_REG_READ_BLOCK(stk, STK83XX_REG_FIFOOUT, len, fifo))
    {
        STK_ACC_ERR("Break to read FIFO data");
    }

    STK_ACC_LOG("Done for reading FIFO data");
}


/*
 * @brief: Change FIFO status
 *          If wm = 0, change FIFO to bypass mode.
 *          STK83XX_CFG1_XYZ_FRAME_MAX >= wm, change FIFO to FIFO mode +
 *                                          STK83XX_CFG2_FIFO_DATA_SEL_XYZ.
 *          Do nothing if STK83XX_CFG1_XYZ_FRAME_MAX < wm.
 *
 * @param[in/out] stk: struct stk_data *
 * @param[in] wm: water mark
 *
 * @return: Success or fail
 *          0: Success
 *          Others: Fail
 */
int stk_change_fifo_status(struct stk_data *stk, u8 wm)
{
    int err = 0;
#ifdef STK_INTERRUPT_MODE
    u8 regIntmap2 = 0, regInten2 = 0;
#endif

    if (!stk->fifo)
    {
        return -1;
    }

    if (STK83XX_CFG1_XYZ_FRAME_MAX < wm)
    {
        STK_ACC_ERR("water mark out of range(%d)", wm);
        return -1;
    }

#ifdef STK_INTERRUPT_MODE
    err = STK_REG_READ(stk, STK83XX_REG_INTMAP2, &regIntmap2);

    if (err < 0)
        return err;

    err = STK_REG_READ(stk, STK83XX_REG_INTEN2, &regInten2);

    if (err < 0)
        return err;

#endif /* STK_INTERRUPT_MODE */

    if (wm)
    {
        /* FIFO settings: FIFO mode + XYZ per frame */
        err = STK_REG_WRITE(stk, STK83XX_REG_CFG2,
                            (STK83XX_CFG2_FIFO_MODE_FIFO << STK83XX_CFG2_FIFO_MODE_SHIFT)
                            | STK83XX_CFG2_FIFO_DATA_SEL_XYZ);

        if (err < 0)
            return err;

#ifdef STK_INTERRUPT_MODE
        err = STK_REG_WRITE(stk, STK83XX_REG_INTMAP2, regIntmap2 | STK83XX_INTMAP2_FWM2INT1);

        if (err < 0)
            return err;

        err = STK_REG_WRITE(stk, STK83XX_REG_INTEN2, regInten2 | STK83XX_INTEN2_FWM_EN);

        if (err < 0)
            return err;

#endif /* STK_INTERRUPT_MODE */
    }
    else
    {
        /* FIFO settings: bypass mode */
        err = STK_REG_WRITE(stk, STK83XX_REG_CFG2,
                            STK83XX_CFG2_FIFO_MODE_BYPASS << STK83XX_CFG2_FIFO_MODE_SHIFT);

        if (err < 0)
            return err;

#ifdef STK_INTERRUPT_MODE
        err = STK_REG_WRITE(stk, STK83XX_REG_INTMAP2, regIntmap2 & (~STK83XX_INTMAP2_FWM2INT1));

        if (err < 0)
            return err;

        err = STK_REG_WRITE(stk, STK83XX_REG_INTEN2, regInten2 & (~STK83XX_INTEN2_FWM_EN));

        if (err < 0)
            return err;

#endif /* STK_INTERRUPT_MODE */
    }

    err = STK_REG_WRITE(stk, STK83XX_REG_CFG1, wm);

    if (err < 0)
        return err;

    return 0;
}

/**
 * @brief: read accel raw data from register.
 *
 * @param[in/out] stk: struct stk_data *
 *
 * @return: Success or fail
 *          0: Success
 *          others: Fail
 */
int stk_read_accel_rawdata(struct stk_data *stk)
{
    u8 data[6] = { 0 };
    int err = 0;
    err = STK_REG_READ_BLOCK(stk, STK83XX_REG_XOUT1, 6, data);

    if (err < 0)
        return err;

    stk->xyz[0] = (data[1] << 8) | data[0];
    stk->xyz[1] = (data[3] << 8) | data[2];
    stk->xyz[2] = (data[5] << 8) | data[4];

    if (stk->data_shift > 0)
    {
        stk->xyz[0] >>= stk->data_shift;
        stk->xyz[1] >>= stk->data_shift;
        stk->xyz[2] >>= stk->data_shift;
    }

#if defined(STK_FIR) || defined(STK_AUTO_K)
    stk_data_log_process(stk, stk->xyz);
#endif // STK_FIR or STK_AUTO_K

#ifdef STK_AUTO_K
    stk_auto_cali(stk, stk->xyz);
#endif // STK_AUTO_K

#ifdef STK_AXES_SIM
    stk_axes_sim(stk->xyz);
#endif // STK_AXES_SIM
    return 0;
}

/*
 * @brief: stk83xx register initialize
 *
 * @param[in/out] stk: struct stk_data *
 * @param[in] range: range for STK83XX_REG_RANGESEL
 *              STK_2G
 *              STK_4G
 *              STK_8G
 * @param[in] sr_no: Serial number of stk_odr_map.
 *
 * @return: Success or fail.
 *          0: Success
 *          others: Fail
 */
int stk_reg_init(struct stk_data *stk, stk_rangesel range, int sr_no)
{
    int err = 0;
    /* SW reset */
    err = stk_sw_reset(stk);

    if (err)
        return err;

    /* INT1, push-pull, active high. */
    err = STK_REG_WRITE(stk, STK83XX_REG_INTCFG1,
                        STK83XX_INTCFG1_INT1_ACTIVE_H | STK83XX_INTCFG1_INT1_OD_PUSHPULL);

    if (err < 0)
        return err;

#ifdef STK_INTERRUPT_MODE

#ifdef STK_6D_TILT

    err = STK_REG_WRITE(stk, STK83XX_REG_INTMAP3, STK83XX_INTMAP3_6D_2INT1);

    if (err < 0)
        return err;

    err = STK_REG_WRITE(stk, STK83XX_REG_SIXDCFG,
                        (STK83XX_SIXDCFG_6D_CTRL_LOW_X |
                        STK83XX_SIXDCFG_6D_CTRL_HIGH_X |
                        STK83XX_SIXDCFG_6D_CTRL_LOW_Y |
                        STK83XX_SIXDCFG_6D_CTRL_HIGH_Y |
                        STK83XX_SIXDCFG_6D_CTRL_LOW_Z |
                        STK83XX_SIXDCFG_6D_CTRL_HIGH_Z |
                        STK83XX_SIXDCFG_6D_MODE_MASK));

    if (err < 0)
        return err;

    switch (range)
    {
    case STK83XX_RANGESEL_2G:
    default:
        err = STK_REG_WRITE(stk, STK83XX_REG_SIXDTHD, STK83XX_SIXDTHD_2G_DEF);
        break;

    case STK83XX_RANGESEL_4G:
        err = STK_REG_WRITE(stk, STK83XX_REG_SIXDTHD, STK83XX_SIXDTHD_4G_DEF);
        break;

    case STK83XX_RANGESEL_8G:
        err = STK_REG_WRITE(stk, STK83XX_REG_SIXDTHD, STK83XX_SIXDTHD_8G_DEF);
        break;
    }

    if (err < 0)
        return err;

    err = STK_REG_WRITE(stk, STK83XX_REG_SIXDDLY, STK83XX_SIXDDLY_DEF);

    if (err < 0)
        return err;
#endif /* STK_6D_TILT */

    /* map new accel data interrupt to int1 */
    err = STK_REG_WRITE(stk, STK83XX_REG_INTMAP2, STK83XX_INTMAP2_DATA2INT1);

    if (err < 0)
        return err;

    /* enable new data interrupt for both new accel data */
    err = STK_REG_WRITE(stk, STK83XX_REG_INTEN2, STK83XX_INTEN2_DATA_EN);

    if (err < 0)
        return err;

#endif /* STK_INTERRUPT_MODE */
#ifdef STK_AMD
    /* enable new data interrupt for any motion */
    err = STK_REG_WRITE(stk, STK83XX_REG_INTEN1, STK83XX_INTEN1_SLP_EN_XYZ);

    if (err < 0)
        return err;

    /* map any motion interrupt to int1 */
    err = STK_REG_WRITE(stk, STK83XX_REG_INTMAP1, STK83XX_INTMAP1_ANYMOT2INT1);

    if (err < 0)
        return err;

    /* SIGMOT2 */
    err = STK_REG_WRITE(stk, STK83XX_REG_SIGMOT2, STK83XX_SIGMOT2_ANY_MOT_EN);

    if (err < 0)
        return err;

    /*
     * latch int
     * In interrupt mode + significant mode, both of them share the same INT.
     * Set latched to make sure we can get ANY data(ANY_MOT_STS) before signal fall down.
     * Read ANY flow:
     * Get INT --> check INTSTS1.ANY_MOT_STS status -> INTCFG2.INT_RST(relese all latched INT)
     * Read FIFO flow:
     * Get INT --> check INTSTS2.FWM_STS status -> INTCFG2.INT_RST(relese all latched INT)
     * In latch mode, echo interrupt(SIT_MOT_STS/FWM_STS) will cause all INT(INT1/INT2)
     * rising up.
     */
    err = STK_REG_WRITE(stk, STK83XX_REG_INTCFG2, STK83XX_INTCFG2_LATCHED);

    if (err < 0)
        return err;

#else /* STK_AMD */
    /* non-latch int */
    err = STK_REG_WRITE(stk, STK83XX_REG_INTCFG2, STK83XX_INTCFG2_NOLATCHED);

    if (err < 0)
        return err;

    /* SIGMOT2 */
    err = STK_REG_WRITE(stk, STK83XX_REG_SIGMOT2, 0);

    if (err < 0)
        return err;

#endif /* STK_AMD */
    /* SLOPE DELAY */
    err = STK_REG_WRITE(stk, STK83XX_REG_SLOPEDLY, 0x00);

    if (err < 0)
        return err;

    /* SLOPE THRESHOLD */
    err = STK_REG_WRITE(stk, STK83XX_REG_SLOPETHD, STK83XX_SLOPETHD_DEF);

    if (err < 0)
        return err;

    /* SIGMOT1 */
    err = STK_REG_WRITE(stk, STK83XX_REG_SIGMOT1, STK83XX_SIGMOT1_SKIP_TIME_3SEC);

    if (err < 0)
        return err;

    /* SIGMOT3 */
    err = STK_REG_WRITE(stk, STK83XX_REG_SIGMOT3, STK83XX_SIGMOT3_PROOF_TIME_1SEC);

    if (err < 0)
        return err;

    /* According to STK_DEF_DYNAMIC_RANGE */
    err = stk_range_selection(stk, range);

    if (err)
        return err;

    /* ODR */
    if (stk->is_esm_mode)
    {
        err = STK_REG_WRITE(stk, STK83XX_REG_ODRMODE, STK83XX_ODR_ESMMODE);

        if (err < 0)
        {
            STK_ACC_ERR("failed to set ESM mode");
        }
    }

    err = STK_REG_WRITE(stk, STK83XX_REG_POWMODE, stk_odr_map[sr_no].regPwsel);

    if (err < 0)
    {
        STK_ACC_ERR("failed to set Powmode");
    }

    err = STK_REG_WRITE(stk, STK83XX_REG_BWSEL, stk_odr_map[sr_no].regBwsel);

    if (err < 0)
        return err;

    stk->sr_no = sr_no;

    if (stk->fifo)
    {
        stk_change_fifo_status(stk, 0);
    }

    /* i2c watchdog enable */
    err = STK_REG_WRITE(stk, STK83XX_REG_INTFCFG, STK83XX_INTFCFG_I2C_WDT_EN);

    if (err < 0)
        return err;

    /* power to suspend mode */
    err = STK_REG_WRITE(stk, STK83XX_REG_POWMODE, STK83XX_PWMD_SUSPEND);

    if (err < 0)
        return err;

    stk->power_mode = STK83XX_PWMD_SUSPEND;
    atomic_set(&stk->enabled, 0);
    return 0;
}

/**
 * @brief: Selftest for XYZ offset and noise.
 *
 * @param[in/out] stk: struct stk_data *
 *
 * @return: Success or fail
 *          0: Success
 *          others: Fail
 */
static int stk_testOffsetNoise(struct stk_data *stk)
{
    int read_delay_ms = 8; /* 125Hz = 8ms */
    int acc_ave[3] = { 0, 0, 0 };
    int acc_min[3] = { INT_MAX, INT_MAX, INT_MAX };
    int acc_max[3] = { INT_MIN, INT_MIN, INT_MIN };
    int noise[3] = { 0, 0, 0 };
    int sn = 0, axis = 0;
    int thresholdOffset, thresholdNoise;
    u8 localResult = 0;

    if (stk_sw_reset(stk))
        return -1;

    if (0 > STK_REG_WRITE(stk, STK83XX_REG_BWSEL, 0x0B)) /* ODR = 125Hz */
        return -1;

    if (stk_range_selection(stk, STK83XX_RANGESEL_2G))
        return -1;

    thresholdOffset = stk_selftest_offset_factor(stk->sensitivity);
    thresholdNoise = stk_selftest_noise_factor(stk->sensitivity);

    for (sn = 0; sn < STK_SELFTEST_SAMPLE_NUM; sn++)
    {
        STK_TIMER_BUSY_WAIT(stk, read_delay_ms, read_delay_ms, MS_DELAY);
        stk_read_accel_rawdata(stk);
        STK_ACC_LOG("acc = %d, %d, %d", stk->xyz[0], stk->xyz[1], stk->xyz[2]);

        for (axis = 0; axis < 3; axis++)
        {
            acc_ave[axis] += stk->xyz[axis];

            if (stk->xyz[axis] > acc_max[axis])
                acc_max[axis] = stk->xyz[axis];

            if (stk->xyz[axis] < acc_min[axis])
                acc_min[axis] = stk->xyz[axis];
        }
    }

    for (axis = 0; axis < 3; axis++)
    {
        acc_ave[axis] /= STK_SELFTEST_SAMPLE_NUM;
        noise[axis] = acc_max[axis] - acc_min[axis];
    }

    STK_ACC_LOG("acc_ave=%d, %d, %d, noise=%d, %d, %d",
                acc_ave[0], acc_ave[1], acc_ave[2], noise[0], noise[1], noise[2]);
    STK_ACC_LOG("offset threshold=%d, noise threshold=%d",
                thresholdOffset, thresholdNoise);

    if (0 < acc_ave[2])
        acc_ave[2] -= stk->sensitivity;
    else
        acc_ave[2] += stk->sensitivity;

    if (0 == acc_ave[0] && 0 == acc_ave[1] && 0 == acc_ave[2])
        localResult |= STK_SELFTEST_RESULT_NO_OUTPUT;

    if (thresholdOffset <= abs(acc_ave[0])
        || 0 == noise[0] || thresholdNoise <= noise[0])
        localResult |= STK_SELFTEST_RESULT_FAIL_X;

    if (thresholdOffset <= abs(acc_ave[1])
        || 0 == noise[1] || thresholdNoise <= noise[1])
        localResult |= STK_SELFTEST_RESULT_FAIL_Y;

    if (thresholdOffset <= abs(acc_ave[2])
        || 0 == noise[2] || thresholdNoise <= noise[2])
        localResult |= STK_SELFTEST_RESULT_FAIL_Z;

    if (0 == localResult)
        atomic_set(&stk->selftest, STK_SELFTEST_RESULT_NO_ERROR);
    else
        atomic_set(&stk->selftest, localResult);

    return 0;
}

/**
 * @brief: SW selftest function.
 *
 * @param[in/out] stk: struct stk_data *
 */
void stk_selftest(struct stk_data *stk)
{
    int err = 0;
    int i = 0;
    uint8_t data = 0;
    STK_ACC_FUN();
    atomic_set(&stk->selftest, STK_SELFTEST_RESULT_RUNNING);

    /* Check PID */
    if (stk_get_pid(stk))
    {
        atomic_set(&stk->selftest, STK_SELFTEST_RESULT_DRIVER_ERROR);
        return;
    }

    STK_ACC_LOG("PID 0x%x", stk->pid);

    for (i = 0; i < (sizeof(STK_ID) / sizeof(STK_ID[0])); i++)
    {
        if (STK_ID[i] == stk->pid)
        {
            break;
        }

        if (sizeof(STK_ID) / sizeof(STK_ID[0]) - 1 == i)
        {
            atomic_set(&stk->selftest, STK_SELFTEST_RESULT_DRIVER_ERROR);
            return;
        }
    }

    /* Touch all register */
    for (i = 0; i <= 0x3F; i++)
    {
        err = STK_REG_READ(stk, i, &data);

        if (err < 0)
        {
            atomic_set(&stk->selftest, STK_SELFTEST_RESULT_DRIVER_ERROR);
            return;
        }

        STK_ACC_LOG("[0x%2X]=0x%2X", i, data);
    }

    if (stk_testOffsetNoise(stk))
    {
        atomic_set(&stk->selftest, STK_SELFTEST_RESULT_DRIVER_ERROR);
    }

    stk_reg_init(stk, STK83XX_RANGESEL_DEF, (stk->odr_table_count - 1));
}

/**
 * @brief: read accel data from register.
 *          Store result to stk832x_data.xyz[].
 *
 * @param[in/out] stk: struct stk_data *
 */
void stk_read_accel_data(struct stk_data *stk)
{
    stk_read_accel_rawdata(stk);
    STK83XX_READ_DATA_CB(stk);
}

#if defined STK_INTERRUPT_MODE || defined STK_POLLING_MODE
/*
 * @brief: 1. Read FIFO data.
 *         2. Report to /sys/class/input/inputX/capabilities/rel
 *
 * @param[in/out] stk: struct stk_data *
 */
void stk_read_then_report_fifo_data(struct stk_data *stk)
{
    int err = 0;
    u8 wm = 0;

    if (!stk->fifo)
    {
        return;
    }

    err = STK_REG_READ(stk, STK83XX_REG_FIFOSTS, &wm);

    if (err < 0)
        return;

    wm = wm & STK83XX_FIFOSTS_FIFO_FRAME_CNT_MASK;

    if (wm)
    {
        u8 *fifo = NULL;
        int i, fifo_len;
        fifo_len = (int)wm * 6; /* xyz * 2 bytes/axis */
        /* kzalloc: allocate memory and set to zero. */
        fifo = kzalloc(sizeof(u8) * fifo_len, GFP_KERNEL);

        if (!fifo)
        {
            STK_ACC_ERR("memory allocation error");
            return;
        }

        stk_fifo_reading(stk, fifo, fifo_len);

        for (i = 0; i < (int)wm; i++)
        {
            s16 *xyz;
            int ii;
            s16 coor_trans[3] = { 0 };
            xyz = kzalloc(sizeof(s16) * 3, GFP_KERNEL);
            xyz[0] = fifo[i * 6 + 1] << 8 | fifo[i * 6];
            xyz[0] >>= 4;
            xyz[1] = fifo[i * 6 + 3] << 8 | fifo[i * 6 + 2];
            xyz[1] >>= 4;
            xyz[2] = fifo[i * 6 + 5] << 8 | fifo[i * 6 + 4];
            xyz[2] >>= 4;

            for (ii = 0; ii < 3; ii++)
            {
                coor_trans[0] += xyz[ii] * coordinate_trans[stk->direction][0][ii];
                coor_trans[1] += xyz[ii] * coordinate_trans[stk->direction][1][ii];
                coor_trans[2] += xyz[ii] * coordinate_trans[stk->direction][2][ii];
            }

            xyz[0] = coor_trans[0];
            xyz[1] = coor_trans[1];
            xyz[2] = coor_trans[2];
#if defined(STK_FIR) || defined(STK_AUTO_K)
            stk_data_log_process(stk, xyz);
#endif // STK_FIR or STK_AUTO_K

#ifdef STK_AUTO_K
            stk_auto_cali(stk, xyz);
#endif // STK_AUTO_K

#ifdef STK_AXES_SIM
            stk_axes_sim(xyz);
#endif // STK_AXES_SIM
            STK83XX_FIFO_REPORT(stk, xyz, 3);
            kfree(xyz);
        }

        kfree(fifo);
    }

    STK83XX_READ_FIFO_CB(stk);
}

/*
 * @brief: Queue work list.
 *          1. Read accel data, then report to userspace.
 *          2. Read FIFO data.
 *          3. Read ANY MOTION data.
 *          4. Reset latch status.
 *          5. Enable IRQ.
 *
 * @param[in] work: struct work_struct *
 */
#ifdef STK_INTERRUPT_MODE
void stk_work_queue(stk_gpio_info *gpio_info)
{
    struct stk_data *stk = (struct stk_data*)gpio_info->any;
#elif defined STK_POLLING_MODE
void stk_work_queue(stk_timer_info * t_info)
{
    struct stk_data *stk = (struct stk_data*)t_info->any;
#endif
#ifdef STK_INTERRUPT_MODE
    STK_ACC_ERR("stk_work_queue:: Interrupt mode");
#elif defined STK_POLLING_MODE
//    STK_ACC_ERR("stk_work_queue:: Polling mode");
#endif // STK_INTERRUPT_MODE
    stk_read_accel_data(stk);
    STK83XX_ACCEL_REPORT(stk);

    if (stk->fifo)
    {
        int err = 0;
        u8 data = 0;
        err = STK_REG_READ(stk, STK83XX_REG_INTSTS2, &data);

        if (err < 0)
        {
            STK_ACC_ERR("cannot read register INTSTS2");
        }
        else
        {
            if (data & STK83XX_INTSTS2_FWM_STS_MASK)
            {
                stk_read_then_report_fifo_data(stk);
            }
        }
    }

#ifdef STK_AMD
    stk_read_anymotion_data(stk);
    stk_reset_latched_int(stk);
#endif /* STK_AMD */

#ifdef STK_6D_TILT
    stk_process_6d_tilt_status(stk);
#endif /* STK_6D_TILT */
}
#endif /* defined STK_INTERRUPT_MODE || defined STK_POLLING_MODE */
#ifdef STK_HW_STEP_COUNTER
/**
 * @brief: read step counter value from register.
 *          Store result to stk_data.steps.
 *
 * @param[in/out] stk: struct stk_data *
 */
void stk_read_step_data(struct stk_data * stk)
{
    int err = 0;
    u8 dataL = 0;
    u8 dataH = 0;

    if (STK8323_ID != stk->pid)
    {
        STK_ACC_ERR("not support step counter");
        return;
    }

    err = STK_REG_READ(stk, STK83XX_REG_STEPOUT1, &dataL);

    if (err < 0)
        return;

    err = STK_REG_READ(stk, STK83XX_REG_STEPOUT2, &dataH);

    if (err < 0)
        return;

    stk->steps = dataH << 8 | dataL;
}
/**
 * @brief: Turn ON/OFF step count.
 *
 * @param[in/out] stk: struct stk_data *
 * @param[in] turn: true to turn ON step count; false to turn OFF.
 */
void stk_turn_step_counter(struct stk_data * stk, bool turn)
{
    if (STK8323_ID != stk->pid)
    {
        STK_ACC_ERR("not support step counter");
        return;
    }

    if (turn)
    {
        if (0 > STK_REG_WRITE(stk, STK83XX_REG_STEPCNT2,
            STK83XX_STEPCNT2_RST_CNT | STK83XX_STEPCNT2_STEP_CNT_EN))
            return;
    }
    else
    {
        if (0 > STK_REG_WRITE(stk, STK83XX_REG_STEPCNT2, 0))
            return;
    }

    stk->steps = 0;
}
#endif /* STK_HW_STEP_COUNTER */
#if defined(STK_FIR) || defined(STK_AUTO_K)
/// <summary>
/// process FIR and AUTO_K method
/// </summary>
void stk_data_log_process(struct stk_data* stk, signed short* xyz)
{
    int i;
#ifdef STK_ZG
    signed short avg;
    int jitter_boundary = stk->sensitivity / 128;
#endif /* STK_ZG */
#ifdef STK_AUTO_K
    signed short min[3] = { 0 };
    signed short max[3] = { 0 };
    unsigned short max_min_diff = STK_SPEC_MAX_MIN_DEF * stk->sensitivity / 1000;
    int j;
#endif // STK_AUTO_K

    if (0 == stk->data_log_len)
    {
#ifdef STK_AUTO_K
        stk->is_stationary = false;
#endif // STK_AUTO_K
        return;
    }

    if (stk->data_log_len > stk->data_log.count)
    {
        stk->data_log.xyz[0][stk->data_log.idx] = xyz[0];
        stk->data_log.xyz[1][stk->data_log.idx] = xyz[1];
        stk->data_log.xyz[2][stk->data_log.idx] = xyz[2];
        stk->data_log.sum[0] += xyz[0];
        stk->data_log.sum[1] += xyz[1];
        stk->data_log.sum[2] += xyz[2];
        stk->data_log.count++;
        stk->data_log.idx++;
    }
    else
    {
        if (stk->data_log_len <= stk->data_log.idx)
            stk->data_log.idx = 0;

        for (i = 0; i < 3; i++)
        {
            stk->data_log.sum[i] -= stk->data_log.xyz[i][stk->data_log.idx];
            stk->data_log.xyz[i][stk->data_log.idx] = xyz[i];
            stk->data_log.sum[i] += xyz[i];

#ifdef STK_AUTO_K
            // find max & min
            min[i] = 9999.0f;
            max[i] = -9999.0f;
            for (j = 0; j < stk->data_log_len; j++)
            {
                if (min[i] > stk->data_log.xyz[i][j])
                {
                    min[i] = stk->data_log.xyz[i][j];
                }

                if (max[i] < stk->data_log.xyz[i][j])
                {
                    max[i] = stk->data_log.xyz[i][j];
                }
            }
#endif // STK_AUTO_K
#ifdef STK_ZG
            avg = stk->data_log.sum[i] / stk->data_log_len;

            if (abs(avg) <= jitter_boundary)
                xyz[i] = avg * ZG_FACTOR;
            else
                xyz[i] = avg;
#elif defined(STK_FIR) //FIR
            xyz[i] = stk->data_log.sum[i] / stk->data_log_len;
#endif /* STK_ZG */
        }
        stk->data_log.idx++;

#ifdef STK_AUTO_K
        stk->is_stationary = true;

        if (STK_ABS(max[0] - min[0]) >= max_min_diff)
        {
            stk->is_stationary = false;
            STK_ACC_ERR("stk_data_log_process:: is_stationary X=false(%d)\n",
                        STK_ABS(max[0] - min[0]));
        }
        else if (STK_ABS(max[1] - min[1]) >= max_min_diff)
        {
            stk->is_stationary = false;
            STK_ACC_ERR("stk_data_log_process:: is_stationary Y=false(%d)\n",
                        STK_ABS(max[1] - min[1]));
        }
        else if (STK_ABS(max[2] - min[2]) >= max_min_diff)
        {
            stk->is_stationary = false;
            STK_ACC_ERR("stk_data_log_process:: is_stationary Z=false(%d)\n",
                        STK_ABS(max[2] - min[2]));
        }
#endif // STK_AUTO_K
        }
    }
#endif // STK_FIR or STK_AUTO_K

#ifdef STK_AUTO_K

void stk_auto_cali(struct stk_data* stk, signed short acc_data[3])
{
    bool do_cali = true;
    signed short lsb_thd[3] = { 0 };
    STK_ACC_ERR("stk_auto_cali::1 x=%d, y=%d, z=%d\n", acc_data[0], acc_data[1], acc_data[2]);

    if (stk->is_stationary)
    {
        lsb_thd[0] = stk->sensitivity * STK_SPEC_X_OFFSEET / 1000; // mg to LSB(8G)
        lsb_thd[1] = stk->sensitivity * STK_SPEC_Y_OFFSEET / 1000; // mg to LSB(8G)
        lsb_thd[2] = stk->sensitivity * STK_SPEC_Z_OFFSEET / 1000; // mg to LSB(8G)

        if (STK_ABS(acc_data[0]) > lsb_thd[0])
        {
            do_cali = false;
            STK_ACC_ERR("stk_auto_cali:: X offset=%d, do_cali=%d\n",
                        STK_ABS(acc_data[0]),
                        do_cali);
        }

        if (STK_ABS(acc_data[1]) > lsb_thd[1])
        {
            do_cali = false;
            STK_ACC_ERR("stk_auto_cali:: Y offset=%d, do_cali=%d\n",
                        STK_ABS(acc_data[1]),
                        do_cali);
        }
        if (acc_data[2] > 0)
        {
            if (STK_ABS(acc_data[2] - stk->sensitivity) > lsb_thd[2])
            {
                do_cali = false;
                STK_ACC_ERR("stk_auto_cali:: Z offset=%d, do_cali=%d\n",
                            STK_ABS(acc_data[2]),
                            do_cali);
            }
        }
        else if (acc_data[2] < 0)
        {
            if (STK_ABS(STK_ABS(acc_data[2]) - stk->sensitivity) > lsb_thd[2])
            {
                do_cali = false;
                STK_ACC_ERR("stk_auto_cali:: Z offset=%d, do_cali=%d\n",
                            STK_ABS(acc_data[3]),
                            do_cali);
            }
        }

        if (do_cali)
        {
            stk->auto_cali_bias[0] = -acc_data[0];
            stk->auto_cali_bias[1] = -acc_data[1];
            if (acc_data[2] < 0)
                stk->auto_cali_bias[2] = -(acc_data[2] + stk->sensitivity);
            else
                stk->auto_cali_bias[2] = -(acc_data[2] - stk->sensitivity);
        }
    }
    else
    {
        do_cali = false;
    }
    STK_ACC_ERR("stk_auto_cali:: bias_x=%d, bias_x=%d, bias_x=%d, bias_x=%d\n", stk->auto_cali_bias[0], stk->auto_cali_bias[1], stk->auto_cali_bias[2], do_cali);
    STK_ACC_ERR("stk_auto_cali:: lsb_thd[0]=%d, lsb_thd[1]=%d, lsb_thd[2]=%d, is_stationary = %d\n", lsb_thd[0], lsb_thd[1], lsb_thd[2], stk->is_stationary);
    acc_data[0] += stk->auto_cali_bias[0];
    acc_data[1] += stk->auto_cali_bias[1];
    acc_data[2] += stk->auto_cali_bias[2];
    STK_ACC_ERR("stk_auto_cali:: after cali x=%d, y=%d, z=%d\n", acc_data[0], acc_data[1], acc_data[2]);
}
#endif // STK_AUTO_K



#ifdef STK_CALI
/*
 * @brief: Get sample_no of samples then calculate average
 *
 * @param[in/out] stk: struct stk_data *
 * @param[in] delay_ms: delay in msec
 * @param[in] sample_no: amount of sample
 * @param[out] acc_ave: XYZ average
 */
static void stk_calculate_average(struct stk_data * stk,
                                  unsigned int delay_ms, int sample_no, int acc_ave[3])
{
    int i;

    for (i = 0; i < sample_no; i++)
    {
        STK_TIMER_BUSY_WAIT(stk, delay_ms, delay_ms, MS_DELAY);
        stk_read_accel_data(stk);
        acc_ave[0] += stk->xyz[0];
        acc_ave[1] += stk->xyz[1];
        acc_ave[2] += stk->xyz[2];
    }

    /*
     * Take ceiling operation.
     * ave = (ave + SAMPLE_NO/2) / SAMPLE_NO
     *     = ave/SAMPLE_NO + 1/2
     * Example: ave=7, SAMPLE_NO=10
     * Ans: ave = 7/10 + 1/2 = (int)(1.2) = 1
     */
    for (i = 0; i < 3; i++)
    {
        if (0 <= acc_ave[i])
            acc_ave[i] = (acc_ave[i] + sample_no / 2) / sample_no;
        else
            acc_ave[i] = (acc_ave[i] - sample_no / 2) / sample_no;
    }

    /*
     * For Z-axis
     * Pre-condition: Sensor be put on a flat plane, with +z face up.
     */
    if (0 < acc_ave[2])
        acc_ave[2] -= stk->sensitivity;
    else
        acc_ave[2] += stk->sensitivity;
}
/*
 * @brief: Align STK83XX_REG_OFSTx sensitivity with STK83XX_REG_RANGESEL
 *  Description:
 *  Example:
 *      RANGESEL=0x3 -> +-2G / 12bits for STK8xxx full resolution
 *              number bit per G = 2^12 / (2x2) = 1024 (LSB/g)
 *              (2x2) / 2^12 = 0.97 mG/bit
 *      OFSTx: There are 8 bits to describe OFSTx for +-1G
 *              number bit per G = 2^8 / (1x2) = 128 (LSB/g)
 *              (1x2) / 2^8 = 7.8125mG/bit
 *      Align: acc_OFST = acc * 128 / 1024
 *
 * @param[in/out] stk: struct stk_data *
 * @param[in/out] acc: accel data
 *
 */
static void stk_align_offset_sensitivity(struct stk_data * stk, int acc[3])
{
    int axis;

    /*
     * Take ceiling operation.
     * ave = (ave + SAMPLE_NO/2) / SAMPLE_NO
     *     = ave/SAMPLE_NO + 1/2
     * Example: ave=7, SAMPLE_NO=10
     * Ans: ave = 7/10 + 1/2 = (int)(1.2) = 1
     */
    for (axis = 0; axis < 3; axis++)
    {
        if (acc[axis] > 0)
        {
            acc[axis] = (acc[axis] * STK83XX_OFST_LSB + stk->sensitivity / 2)
                / stk->sensitivity;
        }
        else
        {
            acc[axis] = (acc[axis] * STK83XX_OFST_LSB - stk->sensitivity / 2)
                / stk->sensitivity;
        }
    }
}
/*
 * @brief: Verify offset.
 *          Read register of STK83XX_REG_OFSTx, then check data are the same as
 *          what we wrote or not.
 *
 * @param[in/out] stk: struct stk_data *
 * @param[in] offset: offset value to compare with the value in register
 *
 * @return: Success or fail
 *          0: Success
 *          STK_K_FAIL_I2C: I2C error
 *          STK_K_FAIL_WRITE_OFSET: offset value not the same as the value in
 *                                  register
 */
static int stk_verify_offset(struct stk_data * stk, u8 offset[3])
{
    int axis;
    u8 offset_from_reg[3] = { 0, 0, 0 };

    if (stk_get_offset(stk, offset_from_reg))
        return STK_K_FAIL_I2C;

    for (axis = 0; axis < 3; axis++)
    {
        if (offset_from_reg[axis] != offset[axis])
        {
            STK_ACC_ERR("set OFST failed! offset[%d]=%d, read from reg[%d]=%d",
                        axis, offset[axis], axis, offset_from_reg[axis]);
            atomic_set(&stk->cali_status, STK_K_FAIL_WRITE_OFST);
            return STK_K_FAIL_WRITE_OFST;
        }
    }

    return 0;
}
#ifdef STK_CALI_FILE
/*
 * @brief: Write calibration data to config file
 *
 * @param[in/out] stk: struct stk_data *
 * @param[in] offset: offset value
 * @param[in] status: status
 *                  STK_K_SUCCESS_FILE
 *
 * @return: Success or fail
 *          0: Success
 *          -1: Fail
 */
int stk_write_cali_to_file(struct stk_data * stk,
                           u8 offset[3], u8 status)
{
    char file_buf[STK_CALI_FILE_SIZE];
    memset(file_buf, 0, sizeof(file_buf));
    file_buf[0] = STK_CALI_VER0;
    file_buf[1] = STK_CALI_VER1;
    file_buf[3] = offset[0];
    file_buf[5] = offset[1];
    file_buf[7] = offset[2];
    file_buf[8] = status;
    file_buf[STK_CALI_FILE_SIZE - 2] = '\0';
    file_buf[STK_CALI_FILE_SIZE - 1] = STK_CALI_END;

    if (stk->sops->write_to_storage(STK_CALI_FILE_PATH, file_buf, STK_CALI_FILE_SIZE))
        return -1;

    return 0;
}
/*
 * @brief: Get calibration data and status.
 *          Set cali status to stk_data.cali_status.
 *
 * @param[in/out] stk: struct stk_data *
 * @param[out] r_buf: cali data what want to read from STK_CALI_FILE_PATH.
 * @param[in] buf_size: size of r_buf.
 *
 * @return: Success or fail
 *          0: Success
 *          others: Fail
 */
void stk_get_cali(struct stk_data * stk)
{
    char stk_file[STK_CALI_FILE_SIZE];

    if (stk->sops->read_from_storage(STK_CALI_FILE_PATH, stk_file, STK_CALI_FILE_SIZE) == 0)
    {
        if (STK_CALI_VER0 == stk_file[0]
            && STK_CALI_VER1 == stk_file[1]
            && STK_CALI_END == stk_file[STK_CALI_FILE_SIZE - 1])
        {
            atomic_set(&stk->cali_status, (int)stk_file[8]);
            stk->offset[0] = (u8)stk_file[3];
            stk->offset[1] = (u8)stk_file[5];
            stk->offset[2] = (u8)stk_file[7];
            STK_ACC_LOG("offset:%d,%d,%d, mode=0x%X",
                        stk_file[3], stk_file[5], stk_file[7], stk_file[8]);
            STK_ACC_LOG("variance=%u,%u,%u",
                        (stk_file[9] << 24 | stk_file[10] << 16 | stk_file[11] << 8 | stk_file[12]),
                        (stk_file[13] << 24 | stk_file[14] << 16 | stk_file[15] << 8 | stk_file[16]),
                        (stk_file[17] << 24 | stk_file[18] << 16 | stk_file[19] << 8 | stk_file[20]));
        }
        else
        {
            int i;
            STK_ACC_ERR("wrong cali version number");

            for (i = 0; i < STK_CALI_FILE_SIZE; i++)
            {
                STK_ACC_LOG("cali_file[%d]=0x%X", i, stk_file[i]);
            }
        }
    }
}
#endif /* STK_CALI_FILE */
/*
 * @brief: Calibration action
 *          1. Calculate calibration data
 *          2. Write data to STK83XX_REG_OFSTx
 *          3. Check calibration well-done with chip register
 *          4. Write calibration data to file
 *          Pre-condition: Sensor be put on a flat plane, with +z face up.
 *
 * @param[in/out] stk: struct stk_data *
 * @param[in] delay_us: delay in usec
 *
 * @return: Success or fail
 *          0: Success
 *          STK_K_FAIL_I2C: I2C error
 *          STK_K_FAIL_WRITE_OFSET: offset value not the same as the value in
 *                                  register
 *          STK_K_FAIL_W_FILE: fail during writing cali to file
 */
static int stk_cali_do(struct stk_data * stk, int delay_us)
{
    int err = 0;
    int acc_ave[3] = { 0, 0, 0 };
    unsigned int delay_ms = delay_us / 1000;
    u8 offset[3] = { 0, 0, 0 };
    int acc_verify[3] = { 0, 0, 0 };
    const unsigned char verify_diff = stk->sensitivity / 10;
    int axis;
    stk_calculate_average(stk, delay_ms, STK_CALI_SAMPLE_NO, acc_ave);
    stk_align_offset_sensitivity(stk, acc_ave);
    stk->cali_sw[0] = (s16)acc_ave[0];
    stk->cali_sw[1] = (s16)acc_ave[1];
    stk->cali_sw[2] = (s16)acc_ave[2];

    for (axis = 0; axis < 3; axis++)
        offset[axis] = -acc_ave[axis];

    STK_ACC_LOG("New offset for XYZ: %d, %d, %d", acc_ave[0], acc_ave[1], acc_ave[2]);
    err = stk_set_offset(stk, offset);

    if (err)
        return STK_K_FAIL_I2C;

    /* Read register, then check OFSTx are the same as we wrote or not */
    err = stk_verify_offset(stk, offset);

    if (err)
        return err;

    /* verify cali */
    stk_calculate_average(stk, delay_ms, 3, acc_verify);

    if (verify_diff < abs(acc_verify[0]) || verify_diff < abs(acc_verify[1])
        || verify_diff < abs(acc_verify[2]))
    {
        STK_ACC_ERR("Check data x:%d, y:%d, z:%d. Check failed!",
                    acc_verify[0], acc_verify[1], acc_verify[2]);
        return STK_K_FAIL_VERIFY_CALI;
    }

#ifdef STK_CALI_FILE
    /* write cali to file */
    err = stk_write_cali_to_file(stk, offset, STK_K_SUCCESS_FILE);

    if (err)
    {
        STK_ACC_ERR("failed to stk_write_cali_to_file, err=%d", err);
        return STK_K_FAIL_W_FILE;
    }

#endif /* STK_CALI_FILE */
    atomic_set(&stk->cali_status, STK_K_SUCCESS_FILE);
    return 0;
}
/*
 * @brief: Set calibration
 *          1. Change delay to 8000usec
 *          2. Reset offset value by trigger OFST_RST
 *          3. Calibration action
 *          4. Change delay value back
 *
 * @param[in/out] stk: struct stk_data *
 */
void stk_set_cali(struct stk_data * stk)
{
    int err = 0;
    bool enable;
    int org_delay_us, real_delay_us;
    atomic_set(&stk->cali_status, STK_K_RUNNING);
    org_delay_us = stk_get_delay(stk);
    /* Use several samples (with ODR:125) for calibration data base */
    err = stk_set_delay(stk, 8000);

    if (err)
    {
        STK_ACC_ERR("failed to stk_set_delay, err=%d", err);
        atomic_set(&stk->cali_status, STK_K_FAIL_I2C);
        goto err_for_set_delay;
    }

    real_delay_us = stk_get_delay(stk);

    /* SW reset before getting calibration data base */
    if (atomic_read(&stk->enabled))
    {
        enable = true;
        stk_set_enable(stk, 0);
    }
    else
    {
        enable = false;
    }

    stk_set_enable(stk, 1);
    err = STK_REG_WRITE(stk, STK83XX_REG_OFSTCOMP1,
                        STK83XX_OFSTCOMP1_OFST_RST);

    if (err < 0)
    {
        atomic_set(&stk->cali_status, STK_K_FAIL_I2C);
        goto exit_for_OFST_RST;
    }

    /* Action for calibration */
    err = stk_cali_do(stk, real_delay_us);

    if (err)
    {
        STK_ACC_ERR("failed to stk_cali_do, err=%d", err);
        atomic_set(&stk->cali_status, err);
        goto exit_for_OFST_RST;
    }

    STK_ACC_LOG("successful calibration");
exit_for_OFST_RST:

    if (!enable)
    {
        stk_set_enable(stk, 0);
    }

err_for_set_delay:
    stk_set_delay(stk, org_delay_us);
}
/**
 * @brief: Reset calibration
 *          1. Reset offset value by trigger OFST_RST
 *          2. Calibration action
 */
void stk_reset_cali(struct stk_data* stk)
{
    STK_REG_WRITE(stk, STK83XX_REG_OFSTCOMP1,
                  STK83XX_OFSTCOMP1_OFST_RST);
    atomic_set(&stk->cali_status, STK_K_NO_CALI);
    memset(stk->cali_sw, 0x0, sizeof(stk->cali_sw));
}
#endif /* STK_CALI */
