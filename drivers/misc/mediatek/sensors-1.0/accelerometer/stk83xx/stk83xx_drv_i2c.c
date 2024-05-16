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

/* stk83xx_drv_i2c.c - stk83xx accelerometer (I2C Interface)
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
#include <stk83xx_mtk_i2c.h>

#ifdef CONFIG_OF
static struct of_device_id stk83xx_match_table[] =
{
    { .compatible = "mediatek,gsensor", },
    {}
};
#endif /* CONFIG_OF */

static int stk_acc_init(void);
static int stk_acc_uninit(void);

struct acc_init_info stk_acc_init_info =
{
    .name = STK83XX_NAME,
    .init = stk_acc_init,
    .uninit = stk_acc_uninit,
};

/*
 * @brief: Proble function for i2c_driver.
 *
 * @param[in] client: struct i2c_client *
 * @param[in] id: struct i2c_device_id *
 *
 * @return: Success or fail
 *          0: Success
 *          others: Fail
 */
static int stk83xx_i2c_probe(struct i2c_client *client,
                             const struct i2c_device_id *id)
{
    struct common_function common_fn =
    {
        .bops = &stk_i2c_bops,
        .tops = &stk_t_ops,
        .gops = &stk_g_ops,
        .sops = &stk_s_ops,
    };
    return stk_i2c_probe(client, &common_fn);
}

/*
 * @brief: Remove function for i2c_driver.
 *
 * @param[in] client: struct i2c_client *
 *
 * @return: 0
 */
static int stk83xx_i2c_remove(struct i2c_client *client)
{
    return stk_i2c_remove(client);
}

/**
 * @brief:
 */
static int stk83xx_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    strcpy(info->type, STK83XX_NAME);
    return 0;
}

#ifdef CONFIG_PM_SLEEP
/*
 * @brief: Suspend function for dev_pm_ops.
 *
 * @param[in] dev: struct device *
 *
 * @return: 0
 */
static int stk83xx_i2c_suspend(struct device *dev)
{
    return stk83xx_suspend(dev);
}

/*
 * @brief: Resume function for dev_pm_ops.
 *
 * @param[in] dev: struct device *
 *
 * @return: 0
 */
static int stk83xx_i2c_resume(struct device *dev)
{
    return stk83xx_resume(dev);
}

static const struct dev_pm_ops stk83xx_pm_ops =
{
    .suspend = stk83xx_i2c_suspend,
    .resume = stk83xx_i2c_resume,
};
#endif /* CONFIG_PM_SLEEP */

#ifdef CONFIG_ACPI
static const struct acpi_device_id stk83xx_acpi_id[] =
{
    {"STK83XX", 0},
    {}
};
MODULE_DEVICE_TABLE(acpi, stk83xx_acpi_id);
#endif /* CONFIG_ACPI */

static const struct i2c_device_id stk83xx_i2c_id[] =
{
    {STK83XX_NAME, 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, stk83xx_i2c_id);

static struct i2c_driver stk83xx_i2c_driver =
{
    .probe      = stk83xx_i2c_probe,
    .remove     = stk83xx_i2c_remove,
    .detect     = stk83xx_i2c_detect,
    .id_table   = stk83xx_i2c_id,
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .owner  = THIS_MODULE,
        .name   = STK83XX_NAME,
#ifdef CONFIG_PM_SLEEP
        .pm     = &stk83xx_pm_ops,
#endif
#ifdef CONFIG_ACPI
        .acpi_match_table = ACPI_PTR(stk83xx_acpi_id),
#endif /* CONFIG_ACPI */
#ifdef CONFIG_OF
        .of_match_table = stk83xx_match_table,
#endif /* CONFIG_OF */
    }
};

/**
 * @brief:
 *
 * @return: Success or fail.
 *          0: Success
 *          others: Fail
 */
static int stk_acc_init(void)
{
    STK_ACC_FUN();

    if (i2c_add_driver(&stk83xx_i2c_driver))
    {
        STK_ACC_ERR("i2c_add_driver fail");
        return -1;
    }

    if (0 != stk_init_flag)
    {
        STK_ACC_ERR("stk83xx init error");
        return -1;
    }

    return 0;
}

/**
 * @brief:
 *
 * @return: Success or fail.
 *          0: Success
 *          others: Fail
 */
static int stk_acc_uninit(void)
{
    i2c_del_driver(&stk83xx_i2c_driver);
    return 0;
}

/**
 * @brief:
 *
 * @return: Success or fail.
 *          0: Success
 *          others: Fail
 */
static int __init stk83xx_init(void)
{
    STK_ACC_FUN();
    acc_driver_add(&stk_acc_init_info);
    return 0;
}

static void __exit stk83xx_exit(void)
{
    STK_ACC_FUN();
}

module_init(stk83xx_init);
module_exit(stk83xx_exit);

MODULE_AUTHOR("Sensortek");
MODULE_DESCRIPTION("stk83xx 3-Axis accelerometer driver");
MODULE_LICENSE("GPL");
//MODULE_VERSION(STK_MTK_VERSION);
