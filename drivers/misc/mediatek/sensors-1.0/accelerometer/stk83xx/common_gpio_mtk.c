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

/* common_gpio_mtk.c - stk83xx accelerometer (Common function)
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
#include <linux/input.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/pm_wakeup.h>
#include <common_define.h>

typedef struct gpio_manager gpio_manager;

struct gpio_manager
{
    struct work_struct          stk_work;
    struct workqueue_struct     *stk_wq;

    stk_gpio_info                *gpio_info;
} gpio_mgr_default = {.gpio_info = 0};

#define MAX_LINUX_GPIO_MANAGER_NUM      5

gpio_manager linux_gpio_mgr[MAX_LINUX_GPIO_MANAGER_NUM];

static gpio_manager* parser_work(struct work_struct *work)
{
    int gpio_idx = 0;

    if (!work)
    {
        return NULL;
    }

    for (gpio_idx = 0; gpio_idx < MAX_LINUX_GPIO_MANAGER_NUM; gpio_idx ++)
    {
        if (&linux_gpio_mgr[gpio_idx].stk_work == work)
        {
            return &linux_gpio_mgr[gpio_idx];
        }
    }

    return NULL;
}

static void gpio_callback(struct work_struct *work)
{
    gpio_manager *gpio_mgr = parser_work(work);

    if (!gpio_mgr)
    {
        return;
    }

    gpio_mgr->gpio_info->gpio_cb(gpio_mgr->gpio_info);
    enable_irq(gpio_mgr->gpio_info->irq);
}

static irqreturn_t stk_gpio_irq_handler(int irq, void *data)
{
    gpio_manager *pData = data;
    disable_irq_nosync(irq);
    schedule_work(&pData->stk_work);
    return IRQ_HANDLED;
}
int register_gpio_irq(stk_gpio_info *gpio_info)
{
    int gpio_idx = 0;
    int irq;
    int err = 0;

    if (!gpio_info)
    {
        return -1;
    }

    for (gpio_idx = 0; gpio_idx < MAX_LINUX_GPIO_MANAGER_NUM; gpio_idx ++)
    {
        if (!linux_gpio_mgr[gpio_idx].gpio_info)
        {
            linux_gpio_mgr[gpio_idx].gpio_info = gpio_info;
            break;
        }
        else
        {
            if (linux_gpio_mgr[gpio_idx].gpio_info == gpio_info)
            {
                //already register
                return -1;
            }
        }
    }

    printk(KERN_INFO "%s: irq num = %d \n", __func__, gpio_info->int_pin);

    if (err < 0)
    {
        printk(KERN_ERR "%s: gpio_request, err=%d", __func__, err);
        return err;
    }

    linux_gpio_mgr[gpio_idx].stk_wq = create_singlethread_workqueue(linux_gpio_mgr[gpio_idx].gpio_info->wq_name);
    INIT_WORK(&linux_gpio_mgr[gpio_idx].stk_work, gpio_callback);
    err = gpio_direction_input(linux_gpio_mgr[gpio_idx].gpio_info->int_pin);

    if (err < 0)
    {
        printk(KERN_ERR "%s: gpio_direction_input, err=%d", __func__, err);
        return err;
    }

    switch (linux_gpio_mgr[gpio_idx].gpio_info->trig_type)
    {
        case TRIGGER_RISING:
            err = request_irq(linux_gpio_mgr[gpio_idx].gpio_info->irq, stk_gpio_irq_handler, \
                              IRQF_TRIGGER_RISING, linux_gpio_mgr[gpio_idx].gpio_info->device_name, &linux_gpio_mgr[gpio_idx]);
            break;

        case TRIGGER_FALLING:
            err = request_irq(linux_gpio_mgr[gpio_idx].gpio_info->irq, stk_gpio_irq_handler, \
                              IRQF_TRIGGER_FALLING, linux_gpio_mgr[gpio_idx].gpio_info->device_name, &linux_gpio_mgr[gpio_idx]);
            break;

        case TRIGGER_HIGH:
        case TRIGGER_LOW:
            err = request_irq(linux_gpio_mgr[gpio_idx].gpio_info->irq, stk_gpio_irq_handler, \
                              IRQF_TRIGGER_LOW, linux_gpio_mgr[gpio_idx].gpio_info->device_name, &linux_gpio_mgr[gpio_idx]);
            break;
    }

    if (err < 0)
    {
        printk(KERN_WARNING "%s: request_any_context_irq(%d) failed for (%d)\n", __func__, irq, err);
        goto err_request_any_context_irq;
    }

    linux_gpio_mgr[gpio_idx].gpio_info->is_exist = true;
    return 0;
err_request_any_context_irq:
    gpio_free(linux_gpio_mgr[gpio_idx].gpio_info->int_pin);
    return err;
}

int start_gpio_irq(stk_gpio_info *gpio_info)
{
    int gpio_idx = 0;

    for (gpio_idx = 0; gpio_idx < MAX_LINUX_GPIO_MANAGER_NUM; gpio_idx ++)
    {
        if (linux_gpio_mgr[gpio_idx].gpio_info == gpio_info)
        {
            if (linux_gpio_mgr[gpio_idx].gpio_info->is_exist)
            {
                if (!linux_gpio_mgr[gpio_idx].gpio_info->is_active)
                {
                    linux_gpio_mgr[gpio_idx].gpio_info->is_active = true;
                }
            }

            return 0;
        }
    }

    return -1;
}

int stop_gpio_irq(stk_gpio_info *gpio_info)
{
    int gpio_idx = 0;

    for (gpio_idx = 0; gpio_idx < MAX_LINUX_GPIO_MANAGER_NUM; gpio_idx ++)
    {
        if (linux_gpio_mgr[gpio_idx].gpio_info == gpio_info)
        {
            if (linux_gpio_mgr[gpio_idx].gpio_info->is_exist)
            {
                if (linux_gpio_mgr[gpio_idx].gpio_info->is_active)
                {
                    linux_gpio_mgr[gpio_idx].gpio_info->is_active = false;
                }
            }

            return 0;
        }
    }

    return -1;
}

int remove_gpio_irq(stk_gpio_info *gpio_info)
{
    int gpio_idx = 0;

    for (gpio_idx = 0; gpio_idx < MAX_LINUX_GPIO_MANAGER_NUM; gpio_idx ++)
    {
        if (linux_gpio_mgr[gpio_idx].gpio_info == gpio_info)
        {
            if (linux_gpio_mgr[gpio_idx].gpio_info->is_exist)
            {
                if (linux_gpio_mgr[gpio_idx].gpio_info->is_active)
                {
                    linux_gpio_mgr[gpio_idx].gpio_info->is_active = false;
                    linux_gpio_mgr[gpio_idx].gpio_info->is_exist = false;
                    free_irq(linux_gpio_mgr[gpio_idx].gpio_info->irq, &linux_gpio_mgr[gpio_idx]);
                    gpio_free(linux_gpio_mgr[gpio_idx].gpio_info->int_pin);
                    cancel_work_sync(&linux_gpio_mgr[gpio_idx].stk_work);
                    linux_gpio_mgr[gpio_idx].gpio_info = NULL;
                }
            }

            return 0;
        }
    }

    return -1;
}

const struct stk_gpio_ops stk_g_ops =
{
    .register_gpio_irq      = register_gpio_irq,
    .start_gpio_irq         = start_gpio_irq,
    .stop_gpio_irq          = stop_gpio_irq,
    .remove                 = remove_gpio_irq,

};
