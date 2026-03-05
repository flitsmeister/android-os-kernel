/*
 * Sitronix Touchscreen Controller Driver
 *
 * Copyright (C) 2018 Sitronix Technology Co., Ltd.
 *	CT Chen <ct_chen@sitronix.com.tw>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "sitronix_ts.h"
#include "sitronix_st7123.h"
#include <linux/i2c.h>

//#define CONFIG_MTK_I2C_EXTENSION
#ifdef CONFIG_MTK_I2C_EXTENSION
#include <linux/dma-mapping.h>
#endif

#define I2C_RETRY_COUNT 1
//#define I2C_MASTER_SEND_RECV

#ifdef CONFIG_MTK_I2C_EXTENSION
static u8 *g_dma_buff_va = NULL;
static dma_addr_t g_dma_buff_pa = 0;
#endif

static int sitronix_ts_i2c_parse_dt(struct device *dev, struct sitronix_ts_host_interface *host_if)
{
#ifdef CONFIG_OF
	struct device_node *np = dev->of_node;
	int rc;
	host_if->irq_gpio = of_get_named_gpio_flags(np, "irq-gpio", 0, &host_if->irq_gpio_flags);
	if (host_if->irq_gpio < 0)
		sterr("%s: Failed to read interrupt GPIO!(%d)\n", __func__, host_if->irq_gpio);
	else{
		rc = gpio_request(host_if->irq_gpio, "st_irq_gpio");
		stmsg("%s: Interrupt GPIO = %d. (flag=%d) , request ret = %d\n", __func__, host_if->irq_gpio, host_if->irq_gpio_flags, rc);
	}

	host_if->rst_gpio = of_get_named_gpio_flags(np, "rst-gpio", 0, &host_if->rst_gpio_flags);
	if (host_if->rst_gpio < 0)
		sterr("%s: Failed to read Reset GPIO!(%d)\n", __func__, host_if->rst_gpio);
	else {
		rc = gpio_request(host_if->rst_gpio, "st_rst_gpio");
		stmsg("%s: Reset GPIO = %d. (flag=%d), request ret = %d\n", __func__, host_if->rst_gpio, host_if->rst_gpio_flags, rc);
	}

#endif
	return 0;
}

static int sitronix_ts_i2c_read(uint16_t addr, uint8_t *data, uint16_t length, void *if_data)
{
	int ret;
	uint8_t retry;
	unsigned char buf[2];
	struct sitronix_ts_i2c_data *i2c_data = (struct sitronix_ts_i2c_data *)if_data;
	struct i2c_client *client = i2c_data->client;
#ifdef I2C_MASTER_SEND_RECV
	retry = 0; 
	buf[0] = (addr >> 8) & 0xFF;
	buf[1] = (addr) & 0xFF;

#ifdef CONFIG_MTK_I2C_EXTENSION
	client->addr = ((client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG);
	//stmsg("%s:%d DMA start\n", __func__, __LINE__);
	memcpy(g_dma_buff_va, buf, 2);
	//g_dma_buff_va[0] = (addr >> 8) & 0xFF;
	//g_dma_buff_va[1] = (addr) & 0xFF;
	ret = i2c_master_send(client, (u8 *)g_dma_buff_pa, 2);
	if (ret < 0 ) {
		sterr("%s: i2c_master_send DMA returns error (%d)\n", __func__, ret);
		client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
		return ret;
	}
	ret = i2c_master_recv(client, (u8 *)g_dma_buff_pa, length);
	if (ret < 0 ) {
		sterr("%s: i2c_master_recv DMA returns error (%d)\n", __func__, ret);
		client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
		return ret;
	}
	client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
    memcpy(data, g_dma_buff_va, length);
#else
	ret = i2c_master_send(client, buf, 2);
	if (ret < 0 ) {
		sterr("%s: i2c_master_send returns error (%d)\n", __func__, ret);
		return ret;
	}
	ret = i2c_master_recv(client, data, length);
	if (ret < 0 ) {
		sterr("%s: i2c_master_recv returns error (%d)\n", __func__, ret);
		return ret;
	}
#endif
#else
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 2,
			.buf = buf,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		},
	};
	buf[0] = (addr >> 8) & 0xFF;
	buf[1] = (addr) & 0xFF;

	for (retry = 1; retry <= I2C_RETRY_COUNT; retry++) {
		if (i2c_transfer(client->adapter, msg, 2) == 2) {
			ret = length;
			break;
		}
		sterr("%s: I2C retry %d\n", __func__, retry);
		msleep(20);
	}

	if (retry > I2C_RETRY_COUNT) {
		sterr("%s: I2C read over retry limit\n", __func__);
		ret = -EIO;
	}
#endif
	return ret;
}

static int sitronix_ts_i2c_write(uint16_t addr, uint8_t *data, uint16_t length, void *if_data)
{
	int ret;
	unsigned char retry;
	unsigned char *buf = NULL;
	struct sitronix_ts_i2c_data *i2c_data = (struct sitronix_ts_i2c_data *)if_data;
	struct i2c_client *client = i2c_data->client;

#ifdef I2C_MASTER_SEND_RECV
	buf = kzalloc((length+2), GFP_KERNEL);
	if (!buf) {
		sterr("%s: Alloc memory for buf failed!\n", __func__);
		return -ENOMEM;
	}

	retry = 0;
	buf[0] = (addr >> 8) & 0xFF;
	buf[1] = (addr) & 0xFF;
#ifdef CONFIG_MTK_I2C_EXTENSION
	client->addr = ((client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG);
	memcpy(g_dma_buff_va, buf, 2);	//write address
	memcpy(&g_dma_buff_va[2], &data[0], length);	//write data
	ret = i2c_master_send(client, (u8 *)g_dma_buff_pa, length+2);
	if (ret < 0 ) {
		sterr("%s: i2c_master_send DMA returns error (%d)\n", __func__, ret);
		client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
		goto err_return;
	}	
	client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
#else
	memcpy(&buf[2], &data[0], length);
	ret = i2c_master_send(client, buf, length+2);
	if (ret < 0 ) {
		sterr("%s: i2c_master_send returns error (%d)\n", __func__, ret);
		goto err_return;
	}
#endif
#else
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length + 2,
			//.buf = buf,
		}
	};
	
	buf = kzalloc((length+2), GFP_KERNEL);
	if (!buf) {
		sterr("%s: Alloc memory for buf failed!\n", __func__);
		return -ENOMEM;
	}
	msg[0].buf = buf;
	buf[0] = (addr >> 8) & 0xFF;
	buf[1] = (addr) & 0xFF;
	memcpy(&buf[2], &data[0], length);

	for (retry = 1; retry <= I2C_RETRY_COUNT; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1) {
			ret = length;
			break;
		}
		sterr("%s: I2C retry %d\n", __func__, retry);
		msleep(8);
	}

	if (retry > I2C_RETRY_COUNT) {
		sterr("%s: I2C write over retry limit\n", __func__);
		ret = -EIO;
		goto err_return;
	}
#endif
	
err_return:
	if (buf)
		kfree(buf);

	return ret;
}

static int sitronix_ts_i2c_dread(uint8_t *data, uint16_t length, void *if_data)
{
	int ret;
	uint8_t retry;
	struct sitronix_ts_i2c_data *i2c_data = (struct sitronix_ts_i2c_data *)if_data;
	struct i2c_client *client = i2c_data->client;
#ifdef I2C_MASTER_SEND_RECV
#ifdef CONFIG_MTK_I2C_EXTENSION
	client->addr = ((client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG);	
	retry = 0;
	ret = i2c_master_recv(client, (u8 *)g_dma_buff_pa, length);
	if (ret < 0 ) {
		sterr("%s: i2c_master_recv returns error (%d)\n", __func__, ret);
		client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
		return ret;
	}
	client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
	memcpy(data, g_dma_buff_va, length);
#else
	retry = 0;
	ret = i2c_master_recv(client, data, length);
	if (ret < 0 ) {
		sterr("%s: i2c_master_recv returns error (%d)\n", __func__, ret);
		return ret;
	}
#endif
#else
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		},
	};

	for (retry = 1; retry <= I2C_RETRY_COUNT; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1) {
			ret = length;
			break;
		}
		sterr("%s: I2C retry %d\n", __func__, retry);
		msleep(20);
	}

	if (retry > I2C_RETRY_COUNT) {
		sterr("%s: I2C read over retry limit\n", __func__);
		ret = -EIO;
	}
#endif
	return ret;
}

static int sitronix_ts_i2c_dwrite(uint8_t *data, uint16_t length, void *if_data)
{
	int ret;
	unsigned char retry;
	struct sitronix_ts_i2c_data *i2c_data = (struct sitronix_ts_i2c_data *)if_data;
	struct i2c_client *client = i2c_data->client;
#ifdef I2C_MASTER_SEND_RECV
	retry = 0;
#ifdef CONFIG_MTK_I2C_EXTENSION
	client->addr = ((client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG);
	memcpy(g_dma_buff_va, data, length);	//write data
	ret = i2c_master_send(client,  (u8 *)g_dma_buff_pa, length);
	if (ret < 0 ) {
		sterr("%s: i2c_master_send DMA returns error (%d)\n", __func__, ret);
		client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
		return ret;
	}
	client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
#else
	ret = i2c_master_send(client, data, length);
	if (ret < 0 ) {
		sterr("%s: i2c_master_send returns error (%d)\n", __func__, ret);
		return ret;
	}
#endif
#else
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length,
			.buf = data,
		}
	};

	for (retry = 1; retry <= I2C_RETRY_COUNT; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1) {
			ret = length;
			break;
		}
		sterr("%s: I2C retry %d\n", __func__, retry);
		msleep(20);
	}

	if (retry > I2C_RETRY_COUNT) {
		sterr("%s: I2C write over retry limit\n", __func__);
		ret = -EIO;
	}
#endif
	return ret;
}

static int sitronix_ts_i2c_aread(uint8_t *tx_buf, uint16_t tx_len, uint8_t *rx_buf, uint16_t rx_len, void *if_data)
{
	int ret;
	uint8_t retry;
	struct sitronix_ts_i2c_data *i2c_data = (struct sitronix_ts_i2c_data *)if_data;
	struct i2c_client *client = i2c_data->client;
#ifdef I2C_MASTER_SEND_RECV
	unsigned short fw_slave_addr = client->addr;
	client->addr = ST_ADDR_MODE_I2C_ADDR;
	retry = 0;
	tx_buf[0] = 0xA0 | (tx_len - 1);
#ifdef CONFIG_MTK_I2C_EXTENSION
	client->addr = ((client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG);
	memcpy(g_dma_buff_va, tx_buf, (tx_len+1));
	ret = i2c_master_send(client, (u8 *)g_dma_buff_pa, (tx_len+1));
	if (ret < 0 ) {
		sterr("%s: i2c_master_send DMA returns error (%d)\n", __func__, ret);
		client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
		client->addr = fw_slave_addr;
		return ret;
	}
	ret = i2c_master_recv(client, (u8 *)g_dma_buff_pa, rx_len);
	if (ret < 0 ) {
		sterr("%s: i2c_master_recv DMA returns error (%d)\n", __func__, ret);
		client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
		client->addr = fw_slave_addr;
		return ret;
	}
	client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
	memcpy(rx_buf, g_dma_buff_va, rx_len);
#else
	ret = i2c_master_send(client, tx_buf, (tx_len+1));
	if (ret < 0 ) {
		sterr("%s: i2c_master_send returns error (%d)\n", __func__, ret);
		client->addr = fw_slave_addr;
		return ret;
	}
	ret = i2c_master_recv(client, rx_buf, rx_len);
	if (ret < 0 ) {
		sterr("%s: i2c_master_recv returns error (%d)\n", __func__, ret);
		client->addr = fw_slave_addr;
		return ret;
	}
#endif /* CONFIG_MTK_I2C_EXTENSION */
	client->addr = fw_slave_addr;
#else	
	struct i2c_msg msg[] = {
		{
			.addr = ST_ADDR_MODE_I2C_ADDR,
			.flags = 0,
			.len = tx_len+1,
			.buf = tx_buf,
		},
		{
			.addr = ST_ADDR_MODE_I2C_ADDR,
			.flags = I2C_M_RD,
			.len = rx_len,
			.buf = rx_buf,
		},
	};

	tx_buf[0] = 0xA0 | (tx_len - 1);


	for (retry = 1; retry <= I2C_RETRY_COUNT; retry++) {
		if (i2c_transfer(client->adapter, msg, 2) == 2) {
			ret = rx_len;
			break;
		}
		sterr("%s: I2C retry %d\n", __func__, retry);
		msleep(20);
	}

	if (retry > I2C_RETRY_COUNT) {
		sterr("%s: I2C read over retry limit\n", __func__);
		ret = -EIO;
	}
#endif /* I2C_MASTER_SEND_RECV */
	return 0;
}

static int sitronix_ts_i2c_awrite(uint8_t *tx_buf, uint16_t tx_len, void *if_data)
{
	int ret;
	unsigned char retry;
	struct sitronix_ts_i2c_data *i2c_data = (struct sitronix_ts_i2c_data *)if_data;
	struct i2c_client *client = i2c_data->client;
#ifdef I2C_MASTER_SEND_RECV
	unsigned short fw_slave_addr = client->addr;
	client->addr = ST_ADDR_MODE_I2C_ADDR;
	retry = 0;
#ifdef SITRONIX_UPGRADE_TEST
	tx_buf[0] = 0x10;
#else
	tx_buf[0] = 0x00;
#endif

#ifdef CONFIG_MTK_I2C_EXTENSION
	client->addr = ((client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG);
	memcpy(g_dma_buff_va, tx_buf, (tx_len+1));
	ret = i2c_master_send(client, (u8 *)g_dma_buff_pa, (tx_len+1));

	client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
	client->addr = fw_slave_addr;	//restore slave address
	if (ret < 0 ) {
		sterr("%s: i2c_master_send DMA returns error (%d)\n", __func__, ret);
		return ret;
	}
#else
	ret = i2c_master_send(client, tx_buf, (tx_len+1));

	client->addr = fw_slave_addr;	//restore slave address
	if (ret < 0 ) {
		sterr("%s: i2c_master_send returns error (%d)\n", __func__, ret);
		return ret;
	}
#endif 
#else
	struct i2c_msg msg[] = {
		{
			.addr = ST_ADDR_MODE_I2C_ADDR,
			.flags = 0,
			.len = tx_len + 1,
			.buf = tx_buf,
		}
	};
#ifdef SITRONIX_UPGRADE_TEST
	tx_buf[0] = 0x10;
#else
	tx_buf[0] = 0x00;
#endif


	for (retry = 1; retry <= I2C_RETRY_COUNT; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1) {
			ret = tx_len;
			break;
		}
		sterr("%s: I2C retry %d\n", __func__, retry);
		msleep(20);
	}

	if (retry > I2C_RETRY_COUNT) {
		sterr("%s: I2C write over retry limit\n", __func__);
		ret = -EIO;
	}
#endif
	return ret;
}

static void sitronix_ts_i2c_dev_release(struct device *dev)
{
#ifdef CONFIG_MTK_I2C_EXTENSION
	if (g_dma_buff_va) {
        dma_free_coherent(NULL, ST_PLATFORM_WRITE_LEN_MAX, g_dma_buff_va, g_dma_buff_pa);
        g_dma_buff_va = NULL;
        g_dma_buff_pa = 0;
    }
#endif
	return;
}

static struct sitronix_ts_i2c_data g_i2c_data;

static struct sitronix_ts_host_interface i2c_host_if = {
	.bus_type = BUS_I2C,
#ifdef	SITRONIX_TP_WITH_FLASH
	.is_use_flash = 1,
#else
	.is_use_flash = 0,
#endif
	.if_data = &g_i2c_data,
	.read = sitronix_ts_i2c_read,
	.write = sitronix_ts_i2c_write,
	.dread = sitronix_ts_i2c_dread,
	.dwrite = sitronix_ts_i2c_dwrite,
	.aread = sitronix_ts_i2c_aread,
	.awrite = sitronix_ts_i2c_awrite,
	.sread = sitronix_ts_i2c_aread,
};

static struct platform_device sitronix_ts_i2c_device = {
	.name = SITRONIX_TS_I2C_DRIVER_NAME,
	.id = 0,
	.num_resources = 0,
	.dev = {
		.release = sitronix_ts_i2c_dev_release,
	},
};

static int sitronix_ts_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;

	//client->addr = 0x66;	//for debug
	stmsg("i2c addr: 0x%x\n",client->addr);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		sterr("%s: I2C function not supported by host!\n", __func__);
		return -EIO;
	}

	if (client->dev.of_node)
		ret = sitronix_ts_i2c_parse_dt(&client->dev, &i2c_host_if);

	g_i2c_data.client = client;

	sitronix_ts_i2c_device.dev.parent = &client->dev;
	sitronix_ts_i2c_device.dev.platform_data = &i2c_host_if;
#ifdef SITRONIX_PLATFORM_QUALCOMM_DRM
	gnp = client->dev.of_node;
#endif

	ret = platform_device_register(&sitronix_ts_i2c_device);
	if (ret) {
		sterr("%s: Failed to register platform device\n", __func__);
		return -ENODEV;
	}
	
#ifdef CONFIG_MTK_I2C_EXTENSION
	 if (NULL == g_dma_buff_va) {
        client->dev.coherent_dma_mask = DMA_BIT_MASK(32);
        g_dma_buff_va = (u8 *)dma_alloc_coherent(&client->dev,
                        ST_PLATFORM_WRITE_LEN_MAX, &g_dma_buff_pa, GFP_KERNEL);
        if (!g_dma_buff_va) {
            sterr("Allocate I2C DMA Buffer fail!!");
			return -ENOMEM;
		}
    }
#endif
	return 0;
}
#ifdef SITRONIX_INTERFACE_I2C
static int sitronix_ts_i2c_remove(struct i2c_client *client)
{
	struct sitronix_ts_host_interface *host_if;

	host_if = (struct sitronix_ts_host_interface*)sitronix_ts_i2c_device.dev.platform_data;

	if (!host_if)
		return 0;

	if (gpio_is_valid(host_if->irq_gpio))
		gpio_free(host_if->irq_gpio);

	if (gpio_is_valid(host_if->rst_gpio))
		gpio_free(host_if->rst_gpio);

	platform_device_unregister(&sitronix_ts_i2c_device);

	return 0;
}
#endif

static const struct i2c_device_id i2c_id_table[] = {
	{ SITRONIX_TS_I2C_DRIVER_NAME, 0, },
	{ },
};
/* MODULE_DEVICE_TABLE(i2c, i2c_id_table); */

#ifdef CONFIG_OF
static struct of_device_id i2c_match_table[] = {
	{ .compatible = "sitronix_ts", },
	{ },
};
/* MODULE_DEVICE_TABLE(of, i2c_match_table); */
#else /* CONFIG_OF */
#define i2c_match_table		NULL
#endif /* CONFIG_OF */

#ifdef SITRONIX_I2C_ADDRESS_DETECT
static int sitronix_ts_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	uint8_t data[8];
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = sizeof(data),
			.buf = data,
		},
	};
	stmsg("%s:%d bus = %d, addr = 0x%02X: ", __func__, __LINE__, client->adapter->nr, client->addr);
	if (i2c_transfer(client->adapter, msg, 1) == 1) {
		stmsg("Detected.\n");
		/* strlcpy(info->type, SITRONIX_I2C_TOUCH_DRV_NAME, strlen(SITRONIX_I2C_TOUCH_DRV_NAME)+1); */
	} else {
		stdbg("Not detected.\n");
		return -ENODEV;
	}

	return 0;
}

const unsigned short sitronix_i2c_address_list[] = {0x55, 0x66, 0x38, 0x70, I2C_CLIENT_END};
#endif /* SITRONIX_I2C_ADDRESS_DETECT */

static struct i2c_driver sitronix_ts_i2c_driver = {
	.driver = {
		.name = SITRONIX_TS_I2C_DRIVER_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = i2c_match_table,
#endif
	},
	.probe = sitronix_ts_i2c_probe,
#ifdef SITRONIX_INTERFACE_I2C
	/*.remove = __exit_p(sitronix_ts_i2c_remove),*/
	.remove = sitronix_ts_i2c_remove,
#endif
	.id_table = i2c_id_table,
#ifdef SITRONIX_I2C_ADDRESS_DETECT
	.address_list = sitronix_i2c_address_list,
	.detect = sitronix_ts_i2c_detect,
#endif /* SITRONIX_I2C_ADDRESS_DETECT */
};

int sitronix_ts_i2c_init(void)
{
	return i2c_add_driver(&sitronix_ts_i2c_driver);
}
EXPORT_SYMBOL(sitronix_ts_i2c_init);

void sitronix_ts_i2c_exit(void)
{
	i2c_del_driver(&sitronix_ts_i2c_driver);

	return;
}
EXPORT_SYMBOL(sitronix_ts_i2c_exit);

MODULE_AUTHOR("Sitronix Technology Co., Ltd.");
MODULE_DESCRIPTION("Sitronix Touchscreen Controller Driver");
MODULE_LICENSE("GPL");
