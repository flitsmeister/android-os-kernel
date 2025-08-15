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
#ifndef __DEFINE_COMMON_H__
#define __DEFINE_COMMON_H__

#include "platform_config.h"

typedef struct stk_timer_info stk_timer_info;
typedef struct stk_gpio_info stk_gpio_info;

struct stk_bus_ops
{
    int bustype;
    int (*init)(void *);
    int (*read)(int, unsigned int, unsigned char *);
    int (*read_block)(int, unsigned int, int, void *);
    int (*write)(int, unsigned int, unsigned char);
    int (*write_block)(int, unsigned int, void *, int);
    int (*read_modify_write)(int, unsigned int, unsigned char, unsigned char);
    int (*remove)(void *);
};

typedef enum
{
    SECOND,
    M_SECOND,
    U_SECOND,
    N_SECOND,
} TIMER_UNIT;

typedef enum
{
    US_RANGE_DELAY,
    MS_DELAY,
} BUSY_WAIT_TYPE;

struct stk_timer_info
{
    char            wq_name[4096];
    uint32_t        interval_time;
    TIMER_UNIT      timer_unit;
    void            (*timer_cb)(stk_timer_info *t_info);
    bool            is_active;
    bool            is_exist;
    bool            is_periodic;
    bool            change_interval_time;
    void            *any;
} ;

struct stk_timer_ops
{
    int (*register_timer)(stk_timer_info *);
    int (*start_timer)(stk_timer_info *);
    int (*stop_timer)(stk_timer_info *);
    int (*remove)(stk_timer_info *);
    void (*busy_wait)(unsigned long, unsigned long, BUSY_WAIT_TYPE);
};

typedef enum
{
    TRIGGER_RISING,
    TRIGGER_FALLING,
    TRIGGER_HIGH,
    TRIGGER_LOW,
} GPIO_TRIGGER_TYPE;

struct stk_gpio_info
{
    char                wq_name[4096];
    char                device_name[4096];
    void                (*gpio_cb)(stk_gpio_info *gpio_info);
    GPIO_TRIGGER_TYPE   trig_type;
    int                 int_pin;
    int32_t             irq;
    bool                is_active;
    bool                is_exist;
    void                *any;
} ;

struct stk_gpio_ops
{
    int (*register_gpio_irq)(stk_gpio_info *);
    int (*start_gpio_irq)(stk_gpio_info *);
    int (*stop_gpio_irq)(stk_gpio_info *);
    int (*remove)(stk_gpio_info *);
};

struct stk_storage_ops
{
    int (*init_storage)(void);
    int (*write_to_storage)(char *, char *, int);
    int (*read_from_storage)(char *, char *, int);
    int (*remove)(void);
};

struct common_function
{
    const struct stk_bus_ops *bops;
    const struct stk_timer_ops *tops;
    const struct stk_gpio_ops *gops;
    const struct stk_storage_ops *sops;
};

typedef struct stk_register_table
{
    uint8_t address;
    uint8_t value;
    uint8_t mask_bit;
} stk_register_table;

#endif // __DEFINE_COMMON_H__
