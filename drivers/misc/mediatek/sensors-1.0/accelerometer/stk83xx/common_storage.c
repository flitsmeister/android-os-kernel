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

/* common_storage.c - stk83xx accelerometer (Common function)
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
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "common_define.h"

int init_storage(void)
{
    return 0;
}

int write_to_storage(char *name, char *w_buf, int buf_size)
{
    struct file  *cali_file = NULL;
    mm_segment_t fs;
    loff_t pos;
    int err;
    cali_file = filp_open(name, O_CREAT | O_RDWR, 0666);

    if (IS_ERR(cali_file))
    {
        err = PTR_ERR(cali_file);
        printk(KERN_ERR "%s: filp_open error!err=%d,path=%s\n", __func__, err, name);
        return -1;
    }
    else
    {
        fs = get_fs();
        set_fs(KERNEL_DS);
        pos = 0;
        vfs_write(cali_file, w_buf, buf_size, &pos);
        set_fs(fs);
    }

    filp_close(cali_file, NULL);
    return 0;
}

int read_from_storage(char *name, char *r_buf, int buf_size)
{
    struct file  *cali_file = NULL;
    mm_segment_t fs;
    loff_t pos;
    int err;
    cali_file = filp_open(name, O_CREAT | O_RDWR, 0644);

    if (IS_ERR(cali_file))
    {
        err = PTR_ERR(cali_file);
        printk(KERN_ERR "%s: filp_open error!err=%d,path=%s\n", __func__, err, name);
        return -1;
    }
    else
    {
        fs = get_fs();
        set_fs(KERNEL_DS);
        pos = 0;
        vfs_read(cali_file, r_buf, buf_size, &pos);
        set_fs(fs);
    }

    filp_close(cali_file, NULL);
    return 0;
}

int remove_storage(void)
{
    return 0;
}

const struct stk_storage_ops stk_s_ops =
{
    .init_storage           = init_storage,
    .write_to_storage       = write_to_storage,
    .read_from_storage      = read_from_storage,
    .remove                 = remove_storage,
};
