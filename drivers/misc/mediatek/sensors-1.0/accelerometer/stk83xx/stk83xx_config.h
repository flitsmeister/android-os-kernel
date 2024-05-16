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

/*
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
/*
 * Definitions for stk83xx accelerometer sensor chip.
 */
#ifndef __STK83XX_CONFIG_H__
#define __STK83XX_CONFIG_H__

/*****************************************************************************
 * Global variable
 *****************************************************************************/
//#define STK_INTERRUPT_MODE
#define STK_POLLING_MODE

//#define STK_AMD /* Turn ON any motion */
//#define STK_HW_STEP_COUNTER /* Turn on step counter */
#define STK_CALI /* Turn on sensortek calibration feature */
//#define STK_CALI_FILE /* Write cali data to file */
#define STK_FIR /* low-pass mode */
//#define STK_ZG
/* enable Zero-G simulation.
 * This feature only works when both of STK_FIR and STK_ZG are turn ON. */
//#define STK_6D_TILT /* only for STK8331! */
#define STK_AXES_SIM // for checking and simulation axes
//#define STK_AUTO_K

#ifdef STK_QUALCOMM
    #include <linux/sensors.h>
    #undef STK_SPREADTRUM
#elif defined STK_MTK
    //#undef STK_INTERRUPT_MODE
    //#undef STK_POLLING_MODE
    #undef STK_AMD
    #define STK_CALI
#elif defined STK_SPREADTRUM
    #include <linux/limits.h>
    #include <linux/version.h>
    //#undef STK_INTERRUPT_MODE
    //#define STK_POLLING_MODE
#elif defined STK_ALLWINNER
    #undef STK_INTERRUPT_MODE
    #define STK_POLLING_MODE
    #undef STK_AMD
#endif /* STK_QUALCOMM, STK_MTK, STK_SPREADTRUM, or STK_ALLWINNER */

#ifdef STK_AXES_SIM
#include "stk_axes_sim.h"
#endif // STK_AXES_SIM

#endif /* __STK83XX_CONFIG_H__ */
