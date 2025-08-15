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
#ifndef __PLATFORM_CONFIG_H__
#define __PLATFORM_CONFIG_H__

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#ifdef CONFIG_IIO
    #include <linux/iio/events.h>
    #include <linux/iio/buffer.h>
    #include <linux/iio/iio.h>
    #include <linux/iio/trigger.h>
    #include <linux/iio/trigger_consumer.h>
    #include <linux/iio/triggered_buffer.h>
    #include <linux/iio/sysfs.h>

#endif

typedef enum
{
    ADDR_8BIT,
    ADDR_16BIT,
} I2C_REG_ADDR_TYPE;

typedef enum
{
    SPI_MODE0,
    SPI_MODE1,
    SPI_MODE2,
    SPI_MODE3,
} SPI_TRANSFER_MODE;

struct spi_manager
{
    struct spi_device *spi;
    struct mutex lock;
    SPI_TRANSFER_MODE trans_mode;
    void *any;
    u8 *spi_buffer;        /* SPI buffer, used for SPI transfer. */
} ;

struct i2c_manager
{
    struct i2c_client *client;
    struct mutex lock;
    I2C_REG_ADDR_TYPE addr_type;
    void *any;
} ;

#define kzalloc(size, mode) kzalloc(size, mode)
#define kfree(ptr) kfree(ptr)

extern const struct stk_bus_ops stk_spi_bops;
extern const struct stk_bus_ops stk_i2c_bops;
extern const struct stk_timer_ops stk_t_ops;
extern const struct stk_gpio_ops stk_g_ops;
extern const struct stk_storage_ops stk_s_ops;
#endif // __KERNEL__

#endif // __PLATFORM_CONFIG_H__
