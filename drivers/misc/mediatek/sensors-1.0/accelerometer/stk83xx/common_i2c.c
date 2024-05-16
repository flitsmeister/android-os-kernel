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

/* common_i2c.c - stk83xx accelerometer (Common function)
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
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/pm.h>
#include <common_define.h>

#define MAX_I2C_MANAGER_NUM     5

struct i2c_manager *pi2c_mgr[MAX_I2C_MANAGER_NUM] = {NULL};

int i2c_init(void* st)
{
    int i2c_idx = 0;

    if (!st)
    {
        return -1;
    }

    for (i2c_idx = 0; i2c_idx < MAX_I2C_MANAGER_NUM; i2c_idx ++)
    {
        if (pi2c_mgr[i2c_idx] == (struct i2c_manager*)st)
        {
            printk(KERN_INFO "%s: i2c is exist\n", __func__);
            break;
        }
        else if (pi2c_mgr[i2c_idx] == NULL)
        {
            pi2c_mgr[i2c_idx] = (struct i2c_manager*)st;
            break;
        }
    }

    return i2c_idx;
}

int i2c_reg_read(int i2c_idx, unsigned int reg, unsigned char *val)
{
    int error = 0;
    struct i2c_manager *_pi2c = pi2c_mgr[i2c_idx];
    I2C_REG_ADDR_TYPE addr_type = _pi2c->addr_type;
    mutex_lock(&_pi2c->lock);

    if (addr_type == ADDR_8BIT)
    {
        unsigned char reg_ = (unsigned char)(reg & 0xFF);
        error = i2c_smbus_read_byte_data(_pi2c->client, reg_);

        if (error < 0)
        {
            dev_err(&_pi2c->client->dev,
                    "%s: failed to read reg:0x%x\n",
                    __func__, reg);
        }
        else
        {
            *(unsigned char *)val = error & 0xFF;
        }
    }
    else if (addr_type == ADDR_16BIT)
    {
    }

    mutex_unlock(&_pi2c->lock);
    return error;
}

int i2c_reg_write(int i2c_idx, unsigned int reg, unsigned char val)
{
    int error = 0;
    struct i2c_manager *_pi2c = pi2c_mgr[i2c_idx];
    I2C_REG_ADDR_TYPE addr_type = _pi2c->addr_type;
    mutex_lock(&_pi2c->lock);

    if (addr_type == ADDR_8BIT)
    {
        unsigned char reg_ = (unsigned char)(reg & 0xFF);
        error = i2c_smbus_write_byte_data(_pi2c->client, reg_, val);
    }
    else if (addr_type == ADDR_16BIT)
    {
    }

    mutex_unlock(&_pi2c->lock);

    if (error < 0)
    {
        dev_err(&_pi2c->client->dev,
                "%s: failed to write reg:0x%x with val:0x%x\n",
                __func__, reg, val);
    }

    return error;
}

int i2c_reg_write_block(int i2c_idx, unsigned int reg, void *val, int length)
{
    int error = 0;
    struct i2c_manager *_pi2c = pi2c_mgr[i2c_idx];
    I2C_REG_ADDR_TYPE addr_type = _pi2c->addr_type;
    mutex_lock(&_pi2c->lock);

    if (addr_type == ADDR_8BIT)
    {
        unsigned char reg_ = (unsigned char)(reg & 0xFF);
        error = i2c_smbus_write_i2c_block_data(_pi2c->client, reg_, length, val);
    }
    else if (addr_type == ADDR_16BIT)
    {
        int i = 0;
        unsigned char *buffer_inverse;
        struct i2c_msg msgs;
        buffer_inverse = kzalloc((sizeof(unsigned char) * (length + 2)), GFP_KERNEL);
        buffer_inverse[0] = reg >> 8;
        buffer_inverse[1] = reg & 0xff;

        for (i = 0; i < length; i ++)
        {
            buffer_inverse[2 + i] = *(u8*)((u8*)val + ((length - 1) - i));
        }

        msgs.addr = _pi2c->client->addr;
        msgs.flags = _pi2c->client->flags & I2C_M_TEN;
        msgs.len = length + 2;
        msgs.buf = buffer_inverse;
#ifdef STK_RETRY_I2C
        i = 0;

        do
        {
            error = i2c_transfer(_pi2c->client->adapter, &msgs, 1);
        }
        while (error != 1 && ++i < 3);

#else
        error = i2c_transfer(_pi2c->client->adapter, &msgs, 1);
#endif //  STK_RETRY_I2C
        kfree(buffer_inverse);
    }

    mutex_unlock(&_pi2c->lock);

    if (error < 0)
    {
        dev_err(&_pi2c->client->dev,
                "%s: failed to write reg:0x%x\n",
                __func__, reg);
    }

    return error;
}

int i2c_reg_read_modify_write(int i2c_idx, unsigned int reg, unsigned char val, unsigned char mask)
{
    uint8_t rw_buffer = 0;
    int error = 0;
    struct i2c_manager *_pi2c = pi2c_mgr[i2c_idx];

    if ((mask == 0xFF) || (mask == 0x0))
    {
        error = i2c_reg_write(i2c_idx, reg, val);

        if (error < 0)
        {
            dev_err(&_pi2c->client->dev,
                    "%s: failed to write reg:0x%x with val:0x%x\n",
                    __func__, reg, val);
        }
    }
    else
    {
        error = (uint8_t)i2c_reg_read(i2c_idx, reg, &rw_buffer);

        if (error < 0)
        {
            dev_err(&_pi2c->client->dev,
                    "%s: failed to read reg:0x%x\n",
                    __func__, reg);
            return error;
        }
        else
        {
            rw_buffer = (rw_buffer & (~mask)) | (val & mask);
            error = i2c_reg_write(i2c_idx, reg, rw_buffer);

            if (error < 0)
            {
                dev_err(&_pi2c->client->dev,
                        "%s: failed to write reg(mask):0x%x with val:0x%x\n",
                        __func__, reg, val);
            }
        }
    }

    return error;
}

int i2c_reg_read_block(int i2c_idx, unsigned int reg, int count, void *buf)
{
    int ret = 0;
    int loop_cnt = 0;
    struct i2c_manager *_pi2c = pi2c_mgr[i2c_idx];
    I2C_REG_ADDR_TYPE addr_type = _pi2c->addr_type;
    mutex_lock(&_pi2c->lock);

    if (addr_type == ADDR_8BIT)
    {
        unsigned char reg_ = (unsigned char)(reg & 0xFF);

        while (count)
        {
            ret = i2c_smbus_read_i2c_block_data(_pi2c->client, reg_, \
                                                (count > I2C_SMBUS_BLOCK_MAX) ? I2C_SMBUS_BLOCK_MAX : count, \
                                                (buf + (loop_cnt * I2C_SMBUS_BLOCK_MAX)) \
                                               );
            (count > I2C_SMBUS_BLOCK_MAX) ? (count -= I2C_SMBUS_BLOCK_MAX) : (count -= count);
            loop_cnt ++;
        }
    }
    else if (addr_type == ADDR_16BIT)
    {
        int i = 0;
        u16 reg_inverse = (reg & 0x00FF) << 8 | (reg & 0xFF00) >> 8;
        int read_length = count;
        u8 buffer_inverse[99] = { 0 };
        struct i2c_msg msgs[2] =
        {
            {
                .addr = _pi2c->client->addr,
                .flags = 0,
                .len = 2,
                .buf = (u8*)&reg_inverse
            },
            {
                .addr = _pi2c->client->addr,
                .flags = I2C_M_RD,
                .len = read_length,
                .buf = buffer_inverse
            }
        };
#ifdef STK_RETRY_I2C
        i = 0;

        do
        {
            ret = i2c_transfer(_pi2c->client->adapter, msgs, 2);
        }
        while (ret != 2 && ++i < 3);

#else
        ret = i2c_transfer(_pi2c->client->adapter, msgs, 2);
#endif //  STK_RETRY_I2C

        if (2 == ret)
        {
            ret = 0;

            for (i = 0; i < read_length; i ++)
            {
                *(u8*)((u8*)buf + i) = ((buffer_inverse[read_length - 1 - i]));
            }
        }
    }

    mutex_unlock(&_pi2c->lock);
    return ret;
}

int32_t i2c_remove(void* st)
{
    int i2c_idx = 0;

    if (!st)
    {
        return -1;
    }

    for (i2c_idx = 0; i2c_idx < MAX_I2C_MANAGER_NUM; i2c_idx ++)
    {
        printk(KERN_INFO "%s: i2c_idx = %d\n", __func__, i2c_idx);

        if (pi2c_mgr[i2c_idx] == (struct i2c_manager*)st)
        {
            printk(KERN_INFO "%s: release i2c_idx = %d\n", __func__, i2c_idx);
            pi2c_mgr[i2c_idx] = NULL;
            break;
        }
    }

    return 0;
}

const struct stk_bus_ops stk_i2c_bops =
{
    .bustype            = BUS_I2C,
    .init               = i2c_init,
    .write              = i2c_reg_write,
    .write_block        = i2c_reg_write_block,
    .read               = i2c_reg_read,
    .read_block         = i2c_reg_read_block,
    .read_modify_write  = i2c_reg_read_modify_write,
    .remove             = i2c_remove,
};
