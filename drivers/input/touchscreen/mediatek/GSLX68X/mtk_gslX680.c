/* drivers/input/touchscreen/mediatek/gslX680/
 *
 * 2010 - 2016 silead inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the sileadinc's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <uapi/linux/sched/types.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include "tpd.h"
#include "upmu_common.h"
#include <linux/hrtimer.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/jiffies.h>
#include <linux/bitops.h>
#include <linux/byteorder/generic.h>
#include <linux/interrupt.h>
#include <linux/time.h>

#ifdef CONFIG_MID_ITEMS_SUPPORT
#define TP_COMPATIBLE
#endif

#ifdef TP_COMPATIBLE
#define GSL_NOID_VERSION
#define ATA_TP_ADDR   0x5f80
#include "gslx680_config.h"
#else
#include "mtk_gslX680.h"
#endif

#include <linux/device.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
//#include <mt-plat/mtk_boot_common.h>

#if !defined(CONFIG_MTK_I2C_EXTENSION) || defined(GSLTP_ENABLE_I2C_DMA)
#include <linux/dma-mapping.h>
#endif
#define GSL_MONITOR
#define GSLX680_NAME	"gslX680"
#define GSLX680_ADDR	0x40
#define MAX_FINGERS		10
#define MAX_CONTACTS	10
#define DMA_TRANS_LEN	0x20
#define SMBUS_TRANS_LEN	0x01
#define GSL_PAGE_REG		0xf0
/*#define GREEN_MODE*/  /*IF use this, pls close esd check*/
#ifdef GREEN_MODE
#define MODE_ON 1
#define MODE_OFF 0
#endif
#ifndef GREEN_MODE
/*#define GSL_MONITOR*/ /*if enable ESD, please close GREEN_MODE*/
#endif
#ifndef TP_COMPATIBLE
extern unsigned int DISP_GetScreenHeight(void);
extern unsigned int DISP_GetScreenWidth(void);
#endif
enum check_meun {
	power_status = 1,
	interrupt_status = 2,
	esd_scanning = 4
}; /* select check mode  1,2,4 */

enum check_err {
	power_shutdowned = 2,
	interrupt_fail,
	esd_protected
}; /* check mode err info  2,3,4 */

//#define GSL_LATE_INIT_CHIP
#define TPD_PROC_DEBUG
/* #define ADD_I2C_DEVICE_ANDROID_4_0 */
/* #define HIGH_SPEED_I2C */

#ifdef TPD_PROC_DEBUG
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>  /* lzk */
/* static struct proc_dir_entry *gsl_config_proc = NULL; */
#define GSL_CONFIG_PROC_FILE "gsl_config"
#define CONFIG_LEN 31
static char gsl_read[CONFIG_LEN];
static u8 gsl_data_proc[8] = {0};
static u8 gsl_proc_flag;
#endif

static int tpd_flag;
static int tpd_halt;
/*static char eint_flag;*/
static int touch_irq;
static struct i2c_client *i2c_client;
static struct task_struct *thread;

#ifdef GSL_LATE_INIT_CHIP
static struct delayed_work gsl_late_init_work;
static struct workqueue_struct *gsl_late_init_workqueue;
#define LATE_INIT_CYCLE_BY_REG_CHECK 10
#endif
#ifdef TP_COMPATIBLE
unsigned int *PTR_GSL_CONFIG_DATA_ID;
const struct fw_data *PTR_FW;
char name_tp_mode[64];
char *NAME_TP;
unsigned int TP_SOURCE_LEN;

int KEY_NUM;
int TP_NUM;
int LED_PIN;
int (*PTR_TPD_KEYS_DIM_LOCAL)[4];

TP_PROJECT_DATA *PTR_TP_PROJECT_DATA;
#endif

#ifdef GSL_MONITOR
static struct delayed_work gsl_monitor_work;
static struct workqueue_struct *gsl_monitor_workqueue = NULL;
static u8 int_1st[4] = {0};
static u8 int_2nd[4] = {0};
static char b0_counter = 0;
static char bc_counter = 0;
/* i2c_lock_flag mean 0:do checking 1:skip once checking; 2:skip anyway*/
static char i2c_lock_flag = 0;
#endif

/* #define TPD_HAVE_BUTTON */
#define TPD_KEY_COUNT	4
#define TPD_KEYS		{KEY_MENU, KEY_HOMEPAGE, KEY_BACK, KEY_SEARCH}
/* {button_center_x, button_center_y, button_width, button_height*/
#define TPD_KEYS_DIM	{{70, 2048, 60, 50}, {210, 2048, 60, 50}, {340, 2048, 60, 50}, {470, 2048, 60, 50} }


static DECLARE_WAIT_QUEUE_HEAD(waiter);
static int init_chip(struct i2c_client *client);
/*static void green_mode(struct i2c_client *client, int mode);*/

#define GSLTP_REG_ADDR_LEN	1
#ifdef CONFIG_MTK_I2C_EXTENSION
/*for ARCH_MT6735,ARCH_MT6735M, ARCH_MT6753,ARCH_MT6580,ARCH_MT6755*/
#define GSLTP_ENABLE_WRRD_MODE
#ifdef GSLTP_ENABLE_I2C_DMA
#define GSLTP_DMA_MAX_TRANSACTION_LEN  255	/* for DMA mode */
#define GSLTP_DMA_MAX_WR_SIZE	(GSLTP_DMA_MAX_TRANSACTION_LEN - GSLTP_REG_ADDR_LEN)
#ifdef GSLTP_ENABLE_WRRD_MODE /*for WRRD(write and read) mode */
#define GSLTP_DMA_MAX_RD_SIZE	31
#else
#define GSLTP_DMA_MAX_RD_SIZE	GSLTP_DMA_MAX_TRANSACTION_LEN
#endif
#endif
#else
#define GSLTP_DMA_MAX_TRANSACTION_LEN  255	/* for DMA mode */
#define GSLTP_DMA_MAX_RD_SIZE	GSLTP_DMA_MAX_TRANSACTION_LEN
#define GSLTP_DMA_MAX_WR_SIZE	(GSLTP_DMA_MAX_TRANSACTION_LEN - GSLTP_REG_ADDR_LEN)
#endif

#ifdef CONFIG_MTK_I2C_EXTENSION
#define GSLTP_I2C_MASTER_CLOCK              100
#ifdef GSLTP_ENABLE_I2C_DMA
static u8 *g_dma_buff_va;
static u8 *g_dma_buff_pa;
#endif
#else
static u8 *g_i2c_buff;
static u8 *g_i2c_addr;
#endif
#if !defined(CONFIG_MTK_I2C_EXTENSION) || defined(GSLTP_ENABLE_I2C_DMA)
static int msg_dma_alloc(void);
static void msg_dma_release(void);
#endif

#define GSL_DEBUG (0)
#if GSL_DEBUG
#define GSL_LOGD(fmt, args...)		\
		printk(GSLX680_NAME	"<-dbg-> [%04d] [@%s]" fmt, \
		__LINE__, __func__, ##args)
#define GSL_LOGF() \
		pr_debug(GSLX680_NAME	"<-func-> [%04d] [@%s] is call!\n", \
		__LINE__, __func__)

#else
#define GSL_LOGD(fmt, args...) do {} while (0)
#define GSL_LOGF() do {} while (0)
#endif /* end #if GSL_DEBUG */

#define GSL_LOGE(fmt, args...)   \
		pr_debug(GSLX680_NAME "<-err->[%04d] [@%s]" fmt, __LINE__, __func__, ##args)  \

#ifdef TPD_HAVE_BUTTON
static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT]   = TPD_WARP_END;
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
static int tpd_calmat_local[8]     = TPD_CALIBRATION_MATRIX;
static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX;
#endif

#ifdef CONFIG_MTK_I2C_EXTENSION
#ifdef GSLTP_ENABLE_I2C_DMA
static int msg_dma_alloc(void)
{
	g_dma_buff_va = (u8 *) dma_alloc_coherent(NULL, GSLTP_DMA_MAX_TRANSACTION_LEN,
		(dma_addr_t *) (&g_dma_buff_pa), GFP_KERNEL | GFP_DMA);
	if (!g_dma_buff_va) {
		GSL_LOGE("[DMA][Error] Allocate DMA I2C Buffer failed!\n");
		return -1;
	}
	return 0;
}

static void msg_dma_release(void)
{
	if (g_dma_buff_va) {
		dma_free_coherent(NULL, GSLTP_DMA_MAX_TRANSACTION_LEN,
			g_dma_buff_va, (dma_addr_t) g_dma_buff_pa);
		g_dma_buff_va = NULL;
		g_dma_buff_pa = NULL;
		GSL_LOGD("[DMA][release]I2C Buffer release!\n");
	}
}

#ifdef GSLTP_ENABLE_WRRD_MODE
/*WRRD(write and read) mode, no stop condition after write reg addr*/
/*max DMA read len  31  bytes */
static s32 i2c_dma_read(struct i2c_client *client, u8 addr, u8 *rxbuf, s32 len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg;

	if (rxbuf == NULL)
		return -1;
	memset(&msg, 0, sizeof(struct i2c_msg));

	*g_dma_buff_va = addr;
	msg.addr = client->addr & I2C_MASK_FLAG;
	msg.flags = 0;
	msg.len = (len << 8) | GSLTP_REG_ADDR_LEN;
	msg.buf = g_dma_buff_pa;
	msg.ext_flag = client->ext_flag | I2C_ENEXT_FLAG |
		I2C_WR_FLAG | I2C_RS_FLAG | I2C_DMA_FLAG;
	msg.timing = GSLTP_I2C_MASTER_CLOCK;

	/* GSL_LOGD("dma i2c read: 0x%04X, %d bytes(s)", addr, len); */
	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0)
			continue;
		memcpy(rxbuf, g_dma_buff_va, len);
		return 0;
	}
	GSL_LOGE("Dma I2C Read Error: 0x%04X, %d byte(s), err-code: %d", addr, len, ret);
	return ret;
}
#else
/*read only mode, max read length is 65532bytes*/
static s32 i2c_dma_read(struct i2c_client *client, u8 addr, u8 *rxbuf, s32 len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg[2];

	if (rxbuf == NULL)
		return -1;
	memset(&msg, 0, sizeof(struct i2c_msg));

	*g_dma_buff_va = addr;
	msg[0].addr = client->addr & I2C_MASK_FLAG;
	msg[0].flags = 0;
	msg[0].len = GSLTP_REG_ADDR_LEN;
	msg[0].buf = g_dma_buff_pa;
	msg[0].ext_flag = I2C_DMA_FLAG;
	msg[0].timing = GSLTP_I2C_MASTER_CLOCK;

	msg[1].addr = client->addr & I2C_MASK_FLAG;
	msg[1].flags = I2C_M_RD;
	msg[1].len = GSLTP_DMA_MAX_RD_SIZE;
	msg[1].buf = g_dma_buff_pa;
	msg[1].ext_flag = client->ext_flag | I2C_ENEXT_FLAG
			| I2C_DMA_FLAG;
	msg[1].timing = GSLTP_I2C_MASTER_CLOCK;

	/* GSL_LOGD("dma i2c read: 0x%04X, %d bytes(s)", addr, len); */
	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg[0], 2);
		if (ret < 0)
			continue;
		memcpy(rxbuf, g_dma_buff_va, len);
		return 0;
	}
	GSL_LOGE("Dma I2C Read Error: 0x%04X, %d byte(s), err-code: %d", addr, len, ret);
	return ret;
}
#endif
static s32 i2c_dma_write(struct i2c_client *client, u8 addr, u8 *txbuf, s32 len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg;

	if (txbuf == NULL)
		return -1;

	memset(&msg, 0, sizeof(struct i2c_msg));
	*g_dma_buff_va = addr;

	msg.addr = (client->addr & I2C_MASK_FLAG);
	msg.flags = 0;
	msg.buf = g_dma_buff_pa;
	msg.len = 1 + len;
	msg.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG
			| I2C_DMA_FLAG);
	msg.timing = GSLTP_I2C_MASTER_CLOCK;

	/* GSL_LOGD("dma i2c write: 0x%04X, %d bytes(s)", addr, len); */
	memcpy(g_dma_buff_va + 1, txbuf, len);
	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0)
			continue;
		return 0;
	}
	GSL_LOGE("Dma I2C Write Error: 0x%04X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}

#else /*GSLTP_ENABLE_I2C_DMA*/
static s32 i2c_read_nondma(struct i2c_client *client, u8 addr, u8 *rxbuf, int len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg;

	if (rxbuf == NULL)
		return -1;
	memset(&msg, 0, sizeof(struct i2c_msg));

	rxbuf[0] = addr;
	msg.addr = client->addr & I2C_MASK_FLAG;
	msg.flags = 0;
	msg.len = (len << 8) | GSLTP_REG_ADDR_LEN;
	msg.buf = rxbuf;
	msg.ext_flag = I2C_WR_FLAG | I2C_RS_FLAG;
	msg.timing = GSLTP_I2C_MASTER_CLOCK;


	/* GSL_LOGD("dma i2c read: 0x%04X, %d bytes(s)", addr, len); */
	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0)
			continue;
		return 0;
	}
	GSL_LOGE("Dma I2C Read Error: 0x%4X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}

static s32 i2c_write_nondma(struct i2c_client *client, u8 addr, u8 *txbuf, int len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg;
	u8 wrBuf[C_I2C_FIFO_SIZE + 1];

	if (txbuf == NULL)
		return -1;

	memset(&msg, 0, sizeof(struct i2c_msg));
	memset(wrBuf, 0, C_I2C_FIFO_SIZE + 1);
	wrBuf[0] = addr;
	memcpy(wrBuf + 1, txbuf, len);

	msg.flags = 0;
	msg.buf = wrBuf;
	msg.len = 1 + len;
	msg.addr = (client->addr & I2C_MASK_FLAG);
	msg.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG);
	msg.timing = GSLTP_I2C_MASTER_CLOCK;

	/* GSL_LOGD("dma i2c write: 0x%04X, %d bytes(s)", addr, len); */
	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0)
			continue;
		return 0;
	}
	GSL_LOGE("Dma I2C Write Error: 0x%04X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}
#endif
#else /*CONFIG_MTK_I2C_EXTENSION*/
static int msg_dma_alloc(void)
{
	g_i2c_buff = kzalloc(GSLTP_DMA_MAX_TRANSACTION_LEN, GFP_KERNEL);
	if (!g_i2c_buff) {
		GSL_LOGE("[DMA][Error] Allocate DMA I2C Buffer failed!\n");
		return -1;
	}

	g_i2c_addr = kzalloc(GSLTP_REG_ADDR_LEN, GFP_KERNEL);
	if (!g_i2c_addr) {
		GSL_LOGE("[DMA]Allocate DMA I2C addr buf failed!\n");
		kfree(g_i2c_buff);
		g_i2c_buff = NULL;
		return -1;
	}

	return 0;
}

static void msg_dma_release(void)
{
	kfree(g_i2c_buff);
	g_i2c_buff = NULL;

	kfree(g_i2c_addr);
	g_i2c_addr = NULL;

	GSL_LOGD("[DMA][release]I2C Buffer release!\n");
}
static s32 i2c_dma_read(struct i2c_client *client, u8 addr, u8 *rxbuf, int len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg[2];

	if (rxbuf == NULL)
		return -1;

	memset(&msg, 0, 2 * sizeof(struct i2c_msg));
	memcpy(g_i2c_addr, &addr, GSLTP_REG_ADDR_LEN);
	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].buf = g_i2c_addr;
	msg[0].len = 1;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = g_i2c_buff;
	msg[1].len = len;

	/* GSL_LOGD("dma i2c read: 0x%04X, %d bytes(s)", addr, len); */
	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg[0], 2);
		if (ret < 0)
			continue;
		memcpy(rxbuf, g_i2c_buff, len);
		return 0;
	}
	GSL_LOGE("Dma I2C Read Error: 0x%4X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}
#if 0
static s32 i2c_dma_write(struct i2c_client *client, u8 addr, u8 *txbuf, s32 len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg;

	if (txbuf == NULL)
		return -1;

	memset(&msg, 0, sizeof(struct i2c_msg));
	*g_i2c_buff = addr;

	msg.addr = (client->addr);
	msg.flags = 0;
	msg.buf = g_i2c_buff;
	msg.len = 1 + len;

	/* GSL_LOGD("dma i2c write: 0x%04X, %d bytes(s)", addr, len); */
	memcpy(g_i2c_buff + 1, txbuf, len);
	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0)
			continue;
		return 0;
	}
	GSL_LOGE("Dma I2C Write Error: 0x%04X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}
#endif
#endif
static int gsl_i2c_read_bytes(struct i2c_client *client, u8 addr, u8 *rxbuf, int len)
{
	int left = len;
	int readLen = 0;
	u8 *rd_buf = rxbuf;
	int ret = 0;

	/* GSL_LOGD("Read bytes dma: 0x%04X, %d byte(s)", addr, len); */
	while (left > 0) {
#if !defined(CONFIG_MTK_I2C_EXTENSION) || defined(GSLTP_ENABLE_I2C_DMA)
		readLen = left > GSLTP_DMA_MAX_RD_SIZE ?
			GSLTP_DMA_MAX_RD_SIZE : left;
		ret = i2c_dma_read(client, addr, rd_buf, readLen);
#else
		readLen = left > C_I2C_FIFO_SIZE ? C_I2C_FIFO_SIZE : left;
		ret = i2c_read_nondma(client, addr, rd_buf, readLen);
#endif

		if (ret < 0) {
			GSL_LOGE("dma read failed!\n");
			return -1;
		}

		left -= readLen;
		if (left > 0) {
			addr += readLen;
			rd_buf += readLen;
		}
	}
	return 0;
}
#if 0
static s32 gsl_i2c_write_bytes(struct i2c_client *client, u8 addr, u8 *txbuf, int len)
{

	int ret = 0;
	int write_len = 0;
	int left = len;
	u8 *wr_buf = txbuf;
	u8 offset = 0;
	u8 wrAddr = addr;

	/* GSL_LOGD("Write bytes dma: 0x%04X, %d byte(s)", addr, len); */
	while (left > 0) {
#if !defined(CONFIG_MTK_I2C_EXTENSION) || defined(GSLTP_ENABLE_I2C_DMA)
		write_len = left > GSLTP_DMA_MAX_WR_SIZE ?
			GSLTP_DMA_MAX_WR_SIZE : left;
		ret = i2c_dma_write(client, wrAddr, wr_buf, write_len);
#else
		write_len = left > C_I2C_FIFO_SIZE ? C_I2C_FIFO_SIZE : left;
		ret = i2c_write_nondma(client, wrAddr, wr_buf, write_len);
#endif

		if (ret < 0) {
			GSL_LOGE("dma i2c write failed!\n");
			return -1;
		}
		offset += write_len;
		left -= write_len;
		if (left > 0) {
			wrAddr = addr + offset;
			wr_buf = txbuf + offset;
		}
	}
	return 0;
}
#endif

static void startup_chip(struct i2c_client *client)
{
	u8 write_buf = 0x00;

	i2c_smbus_write_i2c_block_data(client, 0xe0,1, &write_buf);
#ifdef GSL_NOID_VERSION
#ifdef TP_COMPATIBLE
	gsl_DataInit(PTR_GSL_CONFIG_DATA_ID);
#else
	gsl_DataInit(gsl_config_data_id);
#endif
#endif

	usleep_range(10000, 11000);
}

#ifdef GSL9XX_CHIP
static void gsl_io_control(struct i2c_client *client)
{
	u8 buf[4] = {0};
	int i;

	for (i = 0; i < 5; i++) {
		buf[0] = 0;
		buf[1] = 0;
		buf[2] = 0xfe;
		buf[3] = 0x1;
		i2c_smbus_write_i2c_block_data(client, 0xf0,4,buf);
		buf[0] = 0x5;
		buf[1] = 0;
		buf[2] = 0;
		buf[3] = 0x80;
		i2c_smbus_write_i2c_block_data(client, 0x78,4,buf);
		usleep_range(5000, 5100);
	}
	msleep(50);

}
#endif

static int reset_chip(struct i2c_client *client)
{
	u8 write_buf[4]	= {0};
	int ret = 0;

	write_buf[0] = 0x88;
	ret = i2c_smbus_write_i2c_block_data(client, 0xe0,1,&write_buf[0]);
	msleep(20);

	write_buf[0] = 0x04;
	ret += i2c_smbus_write_i2c_block_data(client, 0xe4,1,&write_buf[0]);
	usleep_range(10000, 11000);

	write_buf[0] = 0x00;
	write_buf[1] = 0x00;
	write_buf[2] = 0x00;
	write_buf[3] = 0x00;
	ret += i2c_smbus_write_i2c_block_data(client, 0xbc,4,&write_buf[0]);
	usleep_range(10000, 11000);
#ifdef GSL9XX_CHIP
	gsl_io_control(client);
#endif

	if (ret < 0)
		GSL_LOGE("reset_chip fail!\n");

	return ret;
}

static void clr_reg(struct i2c_client *client)
{
	u8 write_buf[4]	= {0};

	write_buf[0] = 0x88;
	i2c_smbus_write_i2c_block_data(client, 0xe0,1, &write_buf[0]);
	msleep(20);

	write_buf[0] = 0x03;
	i2c_smbus_write_i2c_block_data(client, 0x80,1, &write_buf[0]);
	usleep_range(5000, 5100);

	write_buf[0] = 0x04;
	i2c_smbus_write_i2c_block_data(client, 0xe4,1, &write_buf[0]);
	usleep_range(5000, 5100);

	write_buf[0] = 0x00;
	i2c_smbus_write_i2c_block_data(client, 0xe0,1, &write_buf[0]);
	msleep(20);
}

#ifdef HIGH_SPEED_I2C
static u32 gsl_read_interface(struct i2c_client *client, u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[2];

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = &reg;
	xfer_msg[0].timing = 400;

	xfer_msg[1].addr = client->addr;
	xfer_msg[1].len = num;
	xfer_msg[1].flags |= I2C_M_RD;
	xfer_msg[1].buf = buf;
	xfer_msg[1].timing = 400;

	if (reg < 0x80) {
		i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg));
		usleep_range(5000, 5100);
	}

	return i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg)) == ARRAY_SIZE(xfer_msg) ? 0 : -EFAULT;
}

static u32 gsl_write_interface(struct i2c_client *client, const u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[1];

	buf[0] = reg;

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = num + 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = buf;
	xfer_msg[0].timing = 400;

	return i2c_transfer(client->adapter, xfer_msg, 1) == 1 ? 0 : -EFAULT;
}

static inline void fw2buf(u8 *buf, const u32 *fw)
{
	u32 *u32_buf = (int *)buf;
	*u32_buf = *fw;
}

static void gsl_load_fw(struct i2c_client *client)
{
	u8 buf[DMA_TRANS_LEN*4 + 1] = {0};
	u8 send_flag = 1;
	u8 *cur = buf + 1;
	u32 source_line = 0;
	u32 source_len;
	struct fw_data *ptr_fw;

	GSL_LOGD("===gsl_load_fw start===\n");
#ifdef TP_COMPATIBLE
	ptr_fw = PTR_FW;
	source_len = TP_SOURCE_LEN;
#else
   	ptr_fw = GSLX680_FW;
   	source_len = ARRAY_SIZE(GSLX680_FW);
#endif
	for (source_line = 0; source_line < source_len; source_line++) {
		/* init page trans, set the page val */
		if (ptr_fw[source_line].offset == GSL_PAGE_REG) {
			fw2buf(cur, &ptr_fw[source_line].val);
			gsl_write_interface(client, GSL_PAGE_REG, buf, 4);
			send_flag = 1;
		} else {
			if (send_flag == 1 % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20))
				buf[0] = (u8)ptr_fw[source_line].offset;

			fw2buf(cur, &ptr_fw[source_line].val);
			cur += 4;

			if (send_flag == 0 % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20)) {
				gsl_write_interface(client, buf[0], buf, cur - buf - 1);
				cur = buf + 1;
			}

			send_flag++;
		}
	}

	GSL_LOGD("===gsl_load_fw end===\n");
}
#else
#ifdef TP_COMPATIBLE
static void gsl_load_fw(struct i2c_client *client)
{
	u8 buf[SMBUS_TRANS_LEN*4] = {0};
	u8 reg = 0, send_flag = 1, cur = 0;
	const struct fw_data *ptr_fw;
	unsigned int source_line = 0;
	unsigned int source_len = TP_SOURCE_LEN;
    	ptr_fw = PTR_FW;
		
	GSL_LOGD("===gsl_load_fw start===\n");
	for (source_line = 0; source_line < source_len; source_line++) {
		if (1 == SMBUS_TRANS_LEN) {
			reg = ptr_fw[source_line].offset;
			memcpy(&buf[0], &ptr_fw[source_line].val, 4);
			i2c_smbus_write_i2c_block_data(client, reg,4,buf);
		} else {
			/* init page trans, set the page val */
			if (ptr_fw[source_line].offset == GSL_PAGE_REG) {
				buf[0] = (u8)(ptr_fw[source_line].val & 0x000000ff);
				i2c_smbus_write_i2c_block_data(client, GSL_PAGE_REG,
						    1,&buf[0]);
				send_flag = 1;
			} else {
				if (send_flag == 1 % (SMBUS_TRANS_LEN < 0x08 ? SMBUS_TRANS_LEN : 0x08))
					reg = ptr_fw[source_line].offset;

				memcpy(&buf[cur], &ptr_fw[source_line].val, 4);
				cur += 4;

				if (send_flag == 0 % (SMBUS_TRANS_LEN < 0x08 ? SMBUS_TRANS_LEN : 0x08)) {
					i2c_smbus_write_i2c_block_data(client, reg, SMBUS_TRANS_LEN *4,buf);
					cur = 0;
				}

				send_flag++;
			}
		}
	}

	GSL_LOGD("===gsl_load_fw end===\n");
}
#else
static void gsl_load_fw(struct i2c_client *client)
{
	u8 buf[SMBUS_TRANS_LEN*4] = {0};
	u8 reg = 0, send_flag = 1, cur = 0;
	
	unsigned int source_line = 0;
	unsigned int source_len = ARRAY_SIZE(GSLX680_FW);

	GSL_LOGD("===gsl_load_fw start===\n");
	for (source_line = 0; source_line < source_len; source_line++) {
		if (1 == SMBUS_TRANS_LEN) {
			reg = GSLX680_FW[source_line].offset;
			memcpy(&buf[0], &GSLX680_FW[source_line].val, 4);
			i2c_smbus_write_i2c_block_data(client, reg,4,buf);
		} else {
			/* init page trans, set the page val */
			if (GSLX680_FW[source_line].offset == GSL_PAGE_REG) {
				buf[0] = (u8)(GSLX680_FW[source_line].val & 0x000000ff);
				i2c_smbus_write_i2c_block_data(client, GSL_PAGE_REG,
						    1,&buf[0]);
				send_flag = 1;
			} else {
				if (send_flag == 1 % (SMBUS_TRANS_LEN < 0x08 ? SMBUS_TRANS_LEN : 0x08))
					reg = GSLX680_FW[source_line].offset;

				memcpy(&buf[cur], &GSLX680_FW[source_line].val, 4);
				cur += 4;

				if (send_flag == 0 % (SMBUS_TRANS_LEN < 0x08 ? SMBUS_TRANS_LEN : 0x08)) {
					i2c_smbus_write_i2c_block_data(client, reg, SMBUS_TRANS_LEN *4,buf);
					cur = 0;
				}

				send_flag++;
			}
		}
	}

	GSL_LOGD("===gsl_load_fw end===\n");
}
#endif
#endif

/*   ----------------------check_memdata start-------------*/
static int arry_compare(u8 *arry_1st, u8 *arry_2nd, int num)
{
	int i;
	int result = 0;

	for (i = 0; i < num; i++) {
		if (*(arry_1st + i) != *(arry_2nd + i))
			result++;
	}
	return result;
}

static int arry_copy(u8 *arry_new, u8 *arry_old, int num)
{
	int i;

	for (i = 0; i < num; i++)
		*(arry_new + i) = *(arry_old + i);

	return 0;
}

static int power_check(struct i2c_client *client)
{
	int result = 0;
	u8 read_buf[4] = {0x00};

	i2c_smbus_read_i2c_block_data(client,0xb0, sizeof(read_buf),read_buf );
	if (read_buf[3] != 0x5a || read_buf[2] != 0x5a
			|| read_buf[1] != 0x5a || read_buf[0] != 0x5a)
		result = power_shutdowned;

	return result;
}

static int interrupt_check(struct i2c_client *client)
{
	int i, num;
	int result = 0;
	u8 read_buf[4] = {0x00};
	u8 arry_1st[4] = {0x00};
	u8 arry_2nd[4] = {0x00};

	num = sizeof(read_buf);
	for (i = 0; i < (num * num); i++) {
		i2c_smbus_read_i2c_block_data(client,0xb4, sizeof(read_buf),read_buf );
		usleep_range(10000, 11000);
		if (!(i % num))
			arry_copy(arry_1st, read_buf, num);
		else
			arry_copy(arry_2nd, read_buf, num);

	}
	result = arry_compare(arry_1st, arry_2nd, num);
	if (result)
		result = interrupt_status;
	return result;
}

static int esd_check(struct i2c_client *client)
{
	int result = 0;
	u8 read_buf[4] = {0x00};

	i2c_smbus_read_i2c_block_data(client,0xbc, sizeof(read_buf),read_buf );
	if (read_buf[3] != 0x00 || read_buf[2] != 0x00
			|| read_buf[1] != 0x00 || read_buf[0] != 0x00)
		result = esd_protected;
	return result;
}

static int check_mode(struct i2c_client *client, int mode_set)
{
	int result = 0;

	switch (mode_set) {
	case power_status:
		result = power_check(client);
		break;
	case interrupt_status:
		result = interrupt_check(client);
		break;
	case esd_scanning:
		result = esd_check(client);
		break;
	case (power_status + interrupt_status):
		result = power_check(client);
		result += interrupt_check(client);
		break;
	case (power_status + esd_scanning):
		result = power_check(client);
		result += esd_check(client);
		break;
	case (interrupt_status + esd_scanning):
		result = interrupt_check(client);
		result += esd_check(client);
		break;
	case (power_status + interrupt_status + esd_scanning):
		result = power_check(client);
		result += interrupt_check(client);
		result += esd_check(client);
		break;
	default:
		result = mode_set;
		GSL_LOGE("mode_set[%d] not valid!\n", mode_set);
	}
	return result;
}

static int check_mem_data(struct i2c_client *client)
{
	int result = 0;

	result = check_mode(client, power_status);
	GSL_LOGD("---result num is[%d] ", result);
	GSL_LOGD("power_shutdowned[%d]\n", power_shutdowned);
	if (result)
		result = init_chip(client);

	return result;
}
/*   ----------------------check_memdata end-------------*/


static int test_i2c(struct i2c_client *client)
{
	u8 read_buf = 0;
	u8 write_buf = 0x12;
	int ret, rc = 1;

	ret = i2c_smbus_read_i2c_block_data( client, 0xf0, sizeof(read_buf),&read_buf);
	if(ret < 0)
		rc --;
	else
		printk("I read reg 0xf0 is %x\n", read_buf);

	msleep(2);
	ret = i2c_smbus_write_i2c_block_data(client, 0xf0,  sizeof(write_buf),&write_buf);
	if(ret	>= 0)
		printk("I write reg 0xf0 0x12\n");

	msleep(2);
	ret = i2c_smbus_read_i2c_block_data( client, 0xf0, sizeof(read_buf),&read_buf);
	if(ret < 0)
		rc --;
	else
		printk("I read reg 0xf0 is 0x%x\n", read_buf);

	return rc;
}

static int init_chip(struct i2c_client *client)
{
	int rc;
//#ifdef GSL_MONITOR
//       i2c_lock_flag = 2;
//#endif
	tpd_gpio_output(GTP_RST_PORT, 0);
	msleep(20);
	tpd_gpio_output(GTP_RST_PORT, 1);
	msleep(20);

	rc = test_i2c(client);
	if (rc < 0) {
		GSL_LOGE("------gslX680 test_i2c error------\n");
		return -1;
	}
#ifndef TP_COMPATIBLE
	clr_reg(client);
	reset_chip(client);
	clr_reg(client);
	rc = reset_chip(client);
	gsl_load_fw(client);
	startup_chip(client);
	rc += reset_chip(client);
	startup_chip(client);
#endif

//#ifdef GSL_MONITOR
//       i2c_lock_flag = 0;
//#endif
	return rc;
}

#ifdef TPD_PROC_DEBUG
static int char_to_int(char ch)
{
	if (ch >= '0' && ch <= '9')
		return (ch-'0');
	else
		return (ch-'a'+10);
}

/* static int gsl_config_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data) */
static int gsl_config_read_proc(struct seq_file *m, void *v)
{
	char temp_data[5] = {0};
	unsigned int tmp = 0;
#ifdef TP_COMPATIBLE	
	unsigned int *ptr_fw;
#endif	

	if ('v' == gsl_read[0] && 's' == gsl_read[1]) {
#ifdef GSL_NOID_VERSION
		tmp = gsl_version_id();
#else
		tmp = 0x20121215;
#endif
		seq_printf(m, "version:%x\n", tmp);
	} else if ('r' == gsl_read[0] && 'e' == gsl_read[1]) {
		if ('i' == gsl_read[3]) {
#ifdef GSL_NOID_VERSION
//--------------------------------------------------
#ifdef TP_COMPATIBLE
	ptr_fw = PTR_GSL_CONFIG_DATA_ID;
#endif
//--------------------------------------------------
			tmp = (gsl_data_proc[5]<<8) | gsl_data_proc[4];
			seq_printf(m, "gsl_config_data_id[%d] = ", tmp);

#ifdef TP_COMPATIBLE
			if (tmp >= 0 && tmp < 512/*ARRAY_SIZE(gsl_config_data_id)*/)
				seq_printf(m,"%d\n",ptr_fw[tmp]);	
#else	
			if (tmp >= 0 && tmp < ARRAY_SIZE(gsl_config_data_id))
				seq_printf(m, "%d\n", gsl_config_data_id[tmp]);
#endif
#endif
		} else {
			i2c_smbus_write_i2c_block_data(i2c_client, 0Xf0,4, &gsl_data_proc[4]);
			if (gsl_data_proc[0] < 0x80)
				i2c_smbus_read_i2c_block_data(i2c_client, gsl_data_proc[0],4,temp_data);
			i2c_smbus_read_i2c_block_data(i2c_client, gsl_data_proc[0],4,temp_data);
			seq_printf(m, "offset : {0x%02x,0x", gsl_data_proc[0]);
			seq_printf(m, "%02x", temp_data[3]);
			seq_printf(m, "%02x", temp_data[2]);
			seq_printf(m, "%02x", temp_data[1]);
			seq_printf(m, "%02x};\n", temp_data[0]);
		}
	}
/* *eof = 1; */
	return 0;
}

static ssize_t gsl_config_write_proc(struct file *file, const char __user  *buffer, size_t  count, loff_t *data)
{
	u8 buf[8] = {0};
	char temp_buf[CONFIG_LEN];
	char *path_buf;
#ifdef GSL_NOID_VERSION
	int tmp = 0;
	int tmp1 = 0;
#endif

	GSL_LOGD("[tp-gsl]\n");
	if (count > 512) {
		GSL_LOGE("size not match [%d:%d]\n", CONFIG_LEN, (int)count);
	return -EFAULT;
	}
	path_buf = kzalloc(count, GFP_KERNEL);
	if (!path_buf) {
		GSL_LOGE("alloc path_buf memory error\n");
		return -1;
	}
	if (copy_from_user(path_buf, buffer, count)) {
		GSL_LOGE("copy from user fail\n");
		goto exit_write_proc_out;
	}
	memcpy(temp_buf, path_buf, (count < CONFIG_LEN?count:CONFIG_LEN));
	GSL_LOGD("[tp-gsl][%s][%s]\n", __func__, temp_buf);

	buf[3] = char_to_int(temp_buf[14])<<4 | char_to_int(temp_buf[15]);
	buf[2] = char_to_int(temp_buf[16])<<4 | char_to_int(temp_buf[17]);
	buf[1] = char_to_int(temp_buf[18])<<4 | char_to_int(temp_buf[19]);
	buf[0] = char_to_int(temp_buf[20])<<4 | char_to_int(temp_buf[21]);

	buf[7] = char_to_int(temp_buf[5])<<4 | char_to_int(temp_buf[6]);
	buf[6] = char_to_int(temp_buf[7])<<4 | char_to_int(temp_buf[8]);
	buf[5] = char_to_int(temp_buf[9])<<4 | char_to_int(temp_buf[10]);
	buf[4] = char_to_int(temp_buf[11])<<4 | char_to_int(temp_buf[12]);
	if ('v' == temp_buf[0] && 's' == temp_buf[1]) {
		memcpy(gsl_read, temp_buf, 4);
		GSL_LOGD("gsl version\n");
	} else if ('s' == temp_buf[0] && 't' == temp_buf[1]) {
//	#ifdef GSL_MONITOR
//		cancel_delayed_work_sync(&gsl_monitor_work);
//		i2c_lock_flag = 2;
//	#endif
		gsl_proc_flag = 1;
		reset_chip(i2c_client);
	} else if ('e' == temp_buf[0] && 'n' == temp_buf[1]) {
		msleep(20);
		reset_chip(i2c_client);
		startup_chip(i2c_client);
		gsl_proc_flag = 0;
	} else if ('r' == temp_buf[0] && 'e' == temp_buf[1]) {
		memcpy(gsl_read, temp_buf, 4);
		memcpy(gsl_data_proc, buf, 8);
	} else if ('w' == temp_buf[0] && 'r' == temp_buf[1]) {
		i2c_smbus_write_i2c_block_data(i2c_client, buf[4],4, buf);
	}
#ifdef GSL_NOID_VERSION
	else if ('i' == temp_buf[0] && 'd' == temp_buf[1]) {
		tmp1 = (buf[7]<<24)|(buf[6]<<16)|(buf[5]<<8)|buf[4];
		tmp = (buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0];
//--------------------------------------------------
#ifdef TP_COMPATIBLE
		if (tmp1 >= 0 && tmp1 < 128/*ARRAY_SIZE(gsl_config_data_id)*/)  //xuchen modify 20180410
		{
		PTR_GSL_CONFIG_DATA_ID[tmp1] = tmp;
#else
		if (tmp1 >= 0 && tmp1 < ARRAY_SIZE(gsl_config_data_id))
		{
		gsl_config_data_id[tmp1] = tmp;
#endif
//--------------------------------------------------			
		}
	}
#endif
exit_write_proc_out:
	kfree(path_buf);
	return count;
}
static int gsl_server_list_open(struct inode *inode, struct file *file)
{
	return single_open(file, gsl_config_read_proc, NULL);
}
static const struct file_operations gsl_seq_fops = {
	.open = gsl_server_list_open,
	.read = seq_read,
	.release = single_release,
	.write = gsl_config_write_proc,
	.owner = THIS_MODULE,
};
#endif

#ifdef TPD_ROTATION_SUPPORT
static void tpd_swap_xy(int *x, int *y)
{
	int temp = 0;

	temp = *x;
	*x = *y;
	*y = temp;
}

static void tpd_rotate_90(int *x, int *y)
{
	*x = SCREEN_MAX_X + 1 - *x;

	*x = (*x * SCREEN_MAX_Y) / SCREEN_MAX_X;
	*y = (*y * SCREEN_MAX_X) / SCREEN_MAX_Y;

	tpd_swap_xy(x, y);
}
static void tpd_rotate_180(int *x, int *y)
{
	*y = SCREEN_MAX_Y + 1 - *y;
	*x = SCREEN_MAX_X + 1 - *x;
}
static void tpd_rotate_270(int *x, int *y)
{
	*y = SCREEN_MAX_Y + 1 - *y;

	*x = (*x * SCREEN_MAX_Y) / SCREEN_MAX_X;
	*y = (*y * SCREEN_MAX_X) / SCREEN_MAX_Y;

	tpd_swap_xy(x, y);
}
#endif
#ifndef TP_COMPATIBLE
int control_input_orientation_c(int *x, int *y, int screenwidth, int screenheight)
{
	int tpd_x_res=0, tpd_y_res=0;
	int z;

	if(true)
	{	
		tpd_y_res = screenwidth;
		tpd_x_res = screenheight;
	}

	if(true)
	{	
		z=*y; *y=*x; *x=z;
	}

	if(true)
		*x=tpd_x_res-*x;

	// if(1 == y_reverse)	
	// 	*y=tpd_y_res-*y;

	// if(1 == xy_deal)
	// {
	// 	*x=(*x)*tpd_y_res/tpd_x_res;
	// 	*y=(*y)*tpd_x_res/tpd_y_res; 
	// }

	// if(1 == xy_deal_minor)
	// {
	// 	*x=(*x)*tpd_x_res/tpd_y_res;
	// 	*y=(*y)*tpd_y_res/tpd_x_res; 
	// }

	return 0;
}
#endif
u8 rs_value1;
#ifdef CONFIG_DISABLE_TOUCHPANEL
static bool b_pen_request_tp_to_disable = false;
static unsigned long current_jiffies;
void pen_request_tp_to_disable(void)
{
	current_jiffies = jiffies;
	b_pen_request_tp_to_disable = true;
}
EXPORT_SYMBOL(pen_request_tp_to_disable);
#endif

static void tpd_down(int id, int x, int y, int p)
{
//	printk("----tpd_down id: %d, x:%d, y:%d----\n", id, x, y);
#ifdef CONFIG_DISABLE_TOUCHPANEL
    if (b_pen_request_tp_to_disable) {
        unsigned long new_jiffies = jiffies;
		if(jiffies_to_msecs(new_jiffies - current_jiffies) > 50)
			b_pen_request_tp_to_disable = false;
    }
	if (b_pen_request_tp_to_disable)
		return;
#endif
#ifdef TP_COMPATIBLE
	control_input_orientation(&x, &y, DISP_GetScreenWidth(), DISP_GetScreenHeight());
	control_key_light(x, y); // gslx_config.c 
#else
	control_input_orientation_c(&x, &y, DISP_GetScreenWidth(), DISP_GetScreenHeight());	
#endif
#ifdef TPD_ROTATION_SUPPORT
	switch (tpd_rotation_type) {
	case TPD_ROTATION_90:
			tpd_rotate_90(&x, &y);
			break;
	case TPD_ROTATION_270:
			tpd_rotate_270(&x, &y);
			break;
	case TPD_ROTATION_180:
			tpd_rotate_180(&x, &y);
			break;
	default:
			break;
	}
#endif

	input_report_key(tpd->dev, BTN_TOUCH, 1);
	input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 1);
	input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
	input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
	input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, id);
	input_mt_sync(tpd->dev);
}

static void tpd_up(void)
{
	GSL_LOGD("------tpd_up------\n");
//--------------------------------------------------
#ifdef TP_COMPATIBLE
	key_light_mod_timer(); //gslx_config.c
#endif
//--------------------------------------------------	
	input_report_key(tpd->dev, BTN_TOUCH, 0);
	input_mt_sync(tpd->dev);
}
static void gsl_report_point(struct gsl_touch_info *ti)
{
	int tmp = 0;
	static int gsl_up_flag; /*prevent more up event*/

	GSL_LOGD("gsl_report_point %d\n", ti->finger_num);
	if (unlikely(ti->finger_num == 0)) {
		if (gsl_up_flag == 0)
			return;
		gsl_up_flag = 0;
		tpd_up();

	//	if ((get_boot_mode() == FACTORY_BOOT) ||
	//			(get_boot_mode() == RECOVERY_BOOT))
	//		tpd_button(ti->x[tmp], ti->y[tmp], 0);

	} else {
		gsl_up_flag = 1;
		for (tmp = 0; ti->finger_num > tmp; tmp++) {
			/* GSL_LOGD("[gsl_report_point](x[%d],y[%d]) = (%d,%d);\n",
			* ti->id[tmp], ti->id[tmp], ti->x[tmp], ti->y[tmp]);
			*/

			tpd_down(ti->id[tmp] - 1, ti->x[tmp], ti->y[tmp], 0);
	//	if ((get_boot_mode() == FACTORY_BOOT) ||
	//			(get_boot_mode() == RECOVERY_BOOT))
	//		tpd_button(ti->x[tmp], ti->y[tmp], 1);
		}
	}
	input_sync(tpd->dev);
}

static void report_data_handle(void)
{
	u8 touch_data[44] = {0};
	unsigned char point_num = 0;
	unsigned int temp_a, temp_b, i;

#ifdef GSL_NOID_VERSION
	u8 buf[4] = {0};
	struct gsl_touch_info cinfo = {{0} };
	int tmp1 = 0;
#endif

#ifdef GSL_MONITOR
	if (i2c_lock_flag != 0)
		return;

	i2c_lock_flag = 1;
#endif

#ifdef TPD_PROC_DEBUG
	if (gsl_proc_flag == 1)
		return;
#endif

	i2c_smbus_read_i2c_block_data(i2c_client, 0x80,4,&touch_data[0]);
	point_num = touch_data[0];
	if (point_num > 0)
		i2c_smbus_read_i2c_block_data(i2c_client, 0x84,4,&touch_data[4]);
	if (point_num > 1)
		i2c_smbus_read_i2c_block_data(i2c_client, 0x88,4,&touch_data[8]);
	if (point_num > 2)
		i2c_smbus_read_i2c_block_data(i2c_client, 0x8c,4,&touch_data[12]);
	if (point_num > 3)
		i2c_smbus_read_i2c_block_data(i2c_client, 0x90,4,&touch_data[16]);
	if (point_num > 4)
		i2c_smbus_read_i2c_block_data(i2c_client, 0x94,4,&touch_data[20]);
	if (point_num > 5)
		i2c_smbus_read_i2c_block_data(i2c_client, 0x98,4,&touch_data[24]);
	if (point_num > 6)
		i2c_smbus_read_i2c_block_data(i2c_client, 0x9c,4,&touch_data[28]);
	if (point_num > 7)
		i2c_smbus_read_i2c_block_data(i2c_client, 0xa0,4,&touch_data[32]);
	if (point_num > 8)
		i2c_smbus_read_i2c_block_data(i2c_client, 0xa4,4,&touch_data[36]);
	if (point_num > 9)
		i2c_smbus_read_i2c_block_data(i2c_client, 0xa8,4,&touch_data[40]);

#ifdef GSL_NOID_VERSION
	cinfo.finger_num = point_num;
	GSL_LOGD("tp-gsl  finger_num = %d\n", cinfo.finger_num);
	for (i = 0; i < (point_num < MAX_CONTACTS ? point_num : MAX_CONTACTS); i++) {
		temp_a = touch_data[(i + 1) * 4 + 3] & 0x0f;
		temp_b = touch_data[(i + 1) * 4 + 2];
		cinfo.x[i] = temp_a << 8 | temp_b;
		temp_a = touch_data[(i + 1) * 4 + 1];
		temp_b = touch_data[(i + 1) * 4 + 0];
		cinfo.y[i] = temp_a << 8 | temp_b;
		cinfo.id[i] = ((touch_data[(i + 1) * 4 + 3] & 0xf0)>>4);
		GSL_LOGD("tp-gsl  before: x[%d] = %d, y[%d] = %d, id[%d] = %d\n",
			i, cinfo.x[i], i, cinfo.y[i], i, cinfo.id[i]);
	}
	cinfo.finger_num = (touch_data[3]<<24)|(touch_data[2]<<16)|
		(touch_data[1]<<8)|touch_data[0];
	gsl_alg_id_main(&cinfo);
	tmp1 = gsl_mask_tiaoping();
	GSL_LOGD("[tp-gsl] tmp1=%x\n", tmp1);
	if (tmp1 > 0 && tmp1 < 0xffffffff) {
		buf[0] = 0xa; buf[1] = 0; buf[2] = 0; buf[3] = 0;
		i2c_smbus_write_i2c_block_data(i2c_client, 0xf0, 4,buf);
		buf[0] = (u8)(tmp1 & 0xff);
		buf[1] = (u8)((tmp1>>8) & 0xff);
		buf[2] = (u8)((tmp1>>16) & 0xff);
		buf[3] = (u8)((tmp1>>24) & 0xff);
		GSL_LOGD("tmp1=%08x,buf[0]=%02x,buf[1]=%02x,buf[2]=%02x,buf[3]=%02x\n",
			tmp1, buf[0], buf[1], buf[2], buf[3]);
		i2c_smbus_write_i2c_block_data(i2c_client, 0x8, 4,buf);
	}
	point_num = cinfo.finger_num;
#endif
	gsl_report_point(&cinfo);
#ifdef GSL_MONITOR
	i2c_lock_flag = 0;
#endif
}

#ifdef GSL_MONITOR
static void gsl_monitor_worker(struct work_struct *work)
{
	u8 read_buf[4]  = {0};
	char init_chip_flag = 0;

	GSL_LOGD("---------gsl_monitor_worker-------\n");

#ifdef TPD_PROC_DEBUG
	if (gsl_proc_flag == 1)
		return;
#endif
	i2c_smbus_read_i2c_block_data(i2c_client, 0xb0, 4, read_buf);
	if(read_buf[3] != 0x5a || read_buf[2] != 0x5a || read_buf[1] != 0x5a || read_buf[0] != 0x5a)
		b0_counter ++;
	else
		b0_counter = 0;

	if(b0_counter > 1)
	{
		printk("======read 0xb0: %x %x %x %x ======\n",read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip_flag = 1;
		b0_counter = 0;
		goto queue_monitor_init_chip;
	}

	i2c_smbus_read_i2c_block_data(i2c_client, 0xb4, 4, read_buf);

	int_2nd[3] = int_1st[3];
	int_2nd[2] = int_1st[2];
	int_2nd[1] = int_1st[1];
	int_2nd[0] = int_1st[0];
	int_1st[3] = read_buf[3];
	int_1st[2] = read_buf[2];
	int_1st[1] = read_buf[1];
	int_1st[0] = read_buf[0];

	if (int_1st[3] == int_2nd[3] && int_1st[2] == int_2nd[2] &&int_1st[1] == int_2nd[1] && int_1st[0] == int_2nd[0]) 
	{
		printk("======int_1st: %x %x %x %x , int_2nd: %x %x %x %x ======\n",int_1st[3], int_1st[2], int_1st[1], int_1st[0], int_2nd[3], int_2nd[2],int_2nd[1],int_2nd[0]);
		init_chip_flag = 1;
		goto queue_monitor_init_chip;
	}

	i2c_smbus_read_i2c_block_data(i2c_client, 0xbc, 4, read_buf);
	if(read_buf[3] != 0 || read_buf[2] != 0 || read_buf[1] != 0 || read_buf[0] != 0)
		bc_counter ++;
	else
		bc_counter = 0;

	if(bc_counter > 1)
	{
		printk("======read 0xbc: %x %x %x %x ======\n",read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip_flag = 1;
		bc_counter = 0;
	}

queue_monitor_init_chip:

    if(i2c_lock_flag != 0)
		goto queue_monitor_work;
	else
		i2c_lock_flag = 1;

	if(init_chip_flag) {
		tpd_gpio_output(GTP_RST_PORT, 0);
		msleep(20);
		tpd_gpio_output(GTP_RST_PORT, 1);
		msleep(20);

		clr_reg(i2c_client);
		reset_chip(i2c_client);
		gsl_load_fw(i2c_client);
		startup_chip(i2c_client);
		reset_chip(i2c_client);
		startup_chip(i2c_client);
	}

queue_monitor_work:
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 100);
	i2c_lock_flag = 0;
}
#endif


#define SUPPORT_TP_KERNEL_CHECK
#ifdef SUPPORT_TP_KERNEL_CHECK

#if defined(ATA_TP_ADDR)
#define RAWDATA_ADDR	ATA_TP_ADDR
#endif

#define DRV_NUM			15
#define SEN_NUM			10
#define RAWDATA_THRESHOLD		6000
#define DAC_THRESHOLD		20
#define MAX_SEN_NUM 15
static const u8 sen_order[SEN_NUM] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

int ctp_factory_test(void)
{
	u8 buf[4], i, offset;
	u32 rawdata_value, dac_value;
	struct i2c_client *client = i2c_client;

	if (!client) {
		GSL_LOGE("err ,client is NULL,ctp_factory_test\n");
		return -1;
	}

	msleep(800);
	/* msleep(20000); */
	for (i = 0; i < DRV_NUM; i++) {
		buf[3] = 0;
		buf[2] = 0;
		buf[1] = 0;
		buf[0] = (RAWDATA_ADDR + SEN_NUM*2*i)/0x80;
		offset = (RAWDATA_ADDR + SEN_NUM*2*i)%0x80;
		i2c_smbus_write_i2c_block_data(client, 0xf0,4, buf);
		i2c_smbus_read_i2c_block_data(client, offset,4,buf);
		i2c_smbus_read_i2c_block_data(client, offset,4,buf);
		rawdata_value = (buf[1]<<8) + buf[0];
		GSL_LOGD("rawdata_value = %d\n", rawdata_value);
		if (rawdata_value > RAWDATA_THRESHOLD) {
			rawdata_value = (buf[3]<<8) + buf[2];
			GSL_LOGD("===>rawdata_value = %d\n", rawdata_value);
			if (rawdata_value > RAWDATA_THRESHOLD) {
				GSL_LOGE("###>rawdata_value = %d\n", rawdata_value);
				return -1; /* fail */
			}
		}
	}

	for (i = 0; i < SEN_NUM; i++) {
		buf[3] = 0x01;
		buf[2] = 0xfe;
		buf[1] = 0x10;
		buf[0] = 0x00;
		offset = 0x10 + (sen_order[i]/4)*4;
		i2c_smbus_write_i2c_block_data(client, 0xf0,4, buf);
		i2c_smbus_read_i2c_block_data(client, offset,4,buf);
		i2c_smbus_read_i2c_block_data(client, offset,4,buf);

		dac_value = buf[sen_order[i]%4];
		GSL_LOGD("===dac_value = %d DAC_THRESHOLD = %d===\n",
		dac_value, DAC_THRESHOLD);
		if (dac_value < DAC_THRESHOLD) {
			GSL_LOGE("dac_value %d < thres %d\n", dac_value, DAC_THRESHOLD);
			return -1; /* fail */
		}
	}

	return 0; /* pass */
}
#endif
static int touch_event_handler(void *unused)
{
	struct sched_param param = { .sched_priority = 4 };

	sched_setscheduler(current, SCHED_RR, &param);

	GSL_LOGF();
	do {
		enable_irq(touch_irq);
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_interruptible(waiter, tpd_flag != 0);
		disable_irq(touch_irq);
		tpd_flag = 0;
		TPD_DEBUG_SET_TIME;
		set_current_state(TASK_RUNNING);
		GSL_LOGD("===touch_event_handler, task running===\n");

		report_data_handle();
	} while (!kthread_should_stop());

	return 0;
}

static irqreturn_t tpd_eint_interrupt_handler(void)
{
	tpd_flag = 1;
	wake_up_interruptible(&waiter);

	return IRQ_HANDLED;
}

static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, TPD_DEVICE);
	return 0;
}

#ifdef GREEN_MODE
static void green_mode(struct i2c_client *client, int mode)
{
	int i;
	u8 buf[4] = {0x00};

	if ((mode != MODE_ON) && (mode != MODE_OFF))
		return;

	for (i = 0; i < 5; i++) {
		buf[0] = 0x0a;
		gsl_i2c_write_bytes(client, 0xf0, &buf[0], 1);
		buf[0] = 0x00;
		buf[1] = 0x00;
		if (mode == MODE_ON) {
			GSL_LOGD("green_mode is on.");
			buf[2] = 0x01;
		} else if (mode == MODE_OFF) {
			GSL_LOGD("green_mode is off.");
			buf[2] = 0x00;
		}
		buf[3] = 0x5a;
		gsl_i2c_write_bytes(client, 0x08, buf, 4);
		msleep(20);
	}
}
#endif

#ifdef GSL_LATE_INIT_CHIP
static void gsl_late_init_worker(struct work_struct *work)
{
	int result = 0;
	int ret = 0;

	GSL_LOGD("---------gsl_late_init_worker-------\n");
	if (1) {
		result = check_mode(i2c_client,
			(power_status + interrupt_status + esd_scanning));
		if (result)
			init_chip(i2c_client);
		GSL_LOGD("---result num is[%d] ", result);
		GSL_LOGD("power_shutdowned[%d]", power_shutdowned);
		GSL_LOGD("interrupt_fail[%d] ", interrupt_fail);
		GSL_LOGD("esd_protected[%d]\n", esd_protected);
	}
	check_mem_data(i2c_client);
	if (ret < 0) {
		GSL_LOGE("Failed to init chip!\n");
		return;
	}
}
#endif

#ifdef TP_COMPATIBLE
static void cfg_adjust(struct i2c_client *client)
{
	u8 read_buf[4]  = {0};
	
	printk("=============gsl_load_cfg_adjust check start==============\n");
	msleep(500);
	gsl_i2c_read_bytes(client, 0xb8, read_buf, sizeof(read_buf));
	printk("fuc:cfg_adjust, b8: %x %x %x %x\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);

	match_gsl_tp_id(read_buf); // gslx_config.c
	clr_reg(client);
	reset_chip(client);

	gsl_load_fw(client);
	startup_chip(client);
	reset_chip(client);
	startup_chip(client);	
}
#endif

static int tpd_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct device_node *node = NULL;

	GSL_LOGF();

	tpd_gpio_output(GTP_RST_PORT, 0);
	msleep(100);

	ret = regulator_enable(tpd->reg);
	if (ret != 0) {
		GSL_LOGE("Failed to enable regulator: %d\n", ret);
		return -1;
	}

	msleep(100);
	tpd_gpio_output(GTP_RST_PORT, 1);
	tpd_gpio_as_int(GTP_INT_PORT);
	msleep(50);

	i2c_client = client;
    client->addr = 0x40;
        ret = test_i2c(client);
	if (ret < 0) {
		GSL_LOGE("------gslX680 test_i2c error------\n");
		return -1;
	}
#ifdef GSL_LATE_INIT_CHIP
	GSL_LOGD("tpd_i2c_probe () : queue gsl_late_init_workqueue\n");
	INIT_DELAYED_WORK(&gsl_late_init_work, gsl_late_init_worker);
	gsl_late_init_workqueue = create_singlethread_workqueue("gsl_late_init_workqueue");
	queue_delayed_work(gsl_late_init_workqueue, &gsl_late_init_work, LATE_INIT_CYCLE_BY_REG_CHECK);
#else
	ret = init_chip(i2c_client);
	check_mem_data(i2c_client);
	if (ret < 0) {
		GSL_LOGE("Failed to init chip!\n");
		return -1;
	}
#endif

#ifdef TP_COMPATIBLE
		cfg_adjust(i2c_client);
#endif


#ifdef GREEN_MODE
	green_mode(i2c_client, MODE_ON);
	reset_chip(i2c_client);
	startup_chip(i2c_client);
#endif
	node = of_find_matching_node(NULL, touch_of_match);
	if (node) {
		GSL_LOGD("node -> name = %s , touch_irq = %d\n", node->name, touch_irq);
		touch_irq = irq_of_parse_and_map(node, 0);
		GSL_LOGD("touch_irq = %d\n ", touch_irq);
		ret = request_irq(touch_irq,
			(irq_handler_t) tpd_eint_interrupt_handler,
			IRQF_TRIGGER_RISING, TPD_DEVICE, NULL);
		if (ret > 0) {
			ret = -1;
			GSL_LOGE(" gslX680 -- error : tpd request_irq IRQ LINE NOT AVAILABLE!.\n");
			return ret;
		}
	} else {
		ret = -1;
		GSL_LOGE("gslX680 -- error : no irq node!!\n");
		return ret;
	}
	disable_irq(touch_irq);

	tpd_load_status = 1;
	GSL_LOGD("tpd_load_status = 1");

	thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
	if (IS_ERR(thread)) {
		ret = PTR_ERR(thread);
		GSL_LOGE(TPD_DEVICE " failed to create kernel thread: %d\n", ret);
		return ret;
	}
#if 0/* def SUPPORT_TP_KERNEL_CHECK */
	tp_check_flag = ctp_factory_test();
	GSL_LOGD("\ntp_check_flag = %x\n", tp_check_flag);
	if (tp_check_flag == 0)
		tp_check_flag = 1;
	else
		tp_check_flag = 0;
	/* mdelay(500); */
	/* eboda_support_tp_check_put(tp_check_flag); */
	tpd_load_status = tp_check_flag;
#endif

#ifdef GSL_MONITOR
	GSL_LOGD("tpd_i2c_probe () : queue gsl_monitor_workqueue\n");

	INIT_DELAYED_WORK(&gsl_monitor_work, gsl_monitor_worker);
	gsl_monitor_workqueue = create_workqueue("gsl_monitor_workqueue");
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 100);
#endif

#ifdef TPD_PROC_DEBUG
	#if 0
		gsl_config_proc = create_proc_entry(GSL_CONFIG_PROC_FILE, 0666, NULL);
		if (gsl_config_proc == NULL) {
			GSL_LOGD("create_proc_entry %s failed\n", GSL_CONFIG_PROC_FILE);
		} else {
			gsl_config_proc->read_proc = gsl_config_read_proc;
			gsl_config_proc->write_proc = gsl_config_write_proc;
		}
	#else
	proc_create(GSL_CONFIG_PROC_FILE, 0660, NULL, &gsl_seq_fops);
	#endif
	gsl_proc_flag = 0;
#endif
	/* enable_irq(touch_irq); */
	GSL_LOGD("tpd_i2c_probe is ok -----------------");

	return 0;
}

static int tpd_i2c_remove(struct i2c_client *client)
{
	GSL_LOGD("==tpd_i2c_remove==\n");
#if !defined(CONFIG_MTK_I2C_EXTENSION) || defined(GSLTP_ENABLE_I2C_DMA)
	msg_dma_release();
#endif

	return 0;
}


static const struct i2c_device_id tpd_i2c_id[] = {{TPD_DEVICE, 0}, {} };
/* #ifdef ADD_I2C_DEVICE_ANDROID_4_0 */
/* static struct i2c_board_info __initdata gslX680_i2c_tpd={ I2C_BOARD_INFO(TPD_DEVICE, (GSLX680_ADDR))}; */
/* #else */
static unsigned short force[] = {0, (GSLX680_ADDR << 1), I2C_CLIENT_END, I2C_CLIENT_END};
static const unsigned short * const forces[] = { force, NULL };
/* static struct i2c_client_address_data addr_data = { .forces = forces,}; */
/* #endif */

static const struct of_device_id tpd_of_match[] = {
	{.compatible = "mediatek,cap_touch"},
	{},
};

struct i2c_driver tpd_i2c_driver = {
	.driver = {
		.name = TPD_DEVICE,
		.of_match_table = tpd_of_match,
	#ifndef ADD_I2C_DEVICE_ANDROID_4_0
		.owner = THIS_MODULE,
	#endif
	},
	.probe = tpd_i2c_probe,
	.remove = tpd_i2c_remove,
	.id_table = tpd_i2c_id,
	.detect = tpd_i2c_detect,
	#ifndef ADD_I2C_DEVICE_ANDROID_4_0
/* .address_data = &addr_data, */
	#endif
	.address_list = (const unsigned short *) forces,
};

int tpd_local_init(void)
{
	int retval;

	GSL_LOGF();
#ifdef TP_COMPATIBLE
	items_config_tp(); //gslx_config.c
	tp_list_config(); //gslx_config.c
#endif	
	
#if !defined(CONFIG_MTK_I2C_EXTENSION) || defined(GSLTP_ENABLE_I2C_DMA)
	retval = msg_dma_alloc();
	if (retval)
		return retval;
#endif

	tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
	retval = regulator_set_voltage(tpd->reg, 2800000, 2800000);
	if (retval != 0) {
		GSL_LOGE("Failed to set reg-vgp6 voltage: %d\n", retval);
		return -1;
	}
	tpd->vmch_reg = regulator_get(tpd->tpd_dev, "vmmc");
	retval = regulator_set_voltage(tpd->vmch_reg, 3300000, 3300000);
	if (retval != 0) {
	
		GSL_LOGE("Failed to set vmch_reg us960  voltage: %d\n", retval);
	
	}

	retval = regulator_enable(tpd->vmch_reg);
	if (retval != 0) {
		 
		GSL_LOGE("Failed to enable us960 regulator: %d\n", retval);
	
	}

	if (i2c_add_driver(&tpd_i2c_driver) != 0) {
		GSL_LOGE("unable to add i2c driver.\n");
		return -1;
	}

	if (tpd_load_status == 0) {
		GSL_LOGE("add error touch panel driver.\n");
		i2c_del_driver(&tpd_i2c_driver);
		return -1;
	}

	input_set_abs_params(tpd->dev, ABS_MT_TRACKING_ID, 0, (MAX_CONTACTS+1), 0, 0);
#ifdef TPD_HAVE_BUTTON
	tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);/* initialize tpd button data */
#endif

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
	TPD_DO_WARP = 1;
	memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT*4);
	memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT*4);
#endif

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
	memcpy(tpd_calmat, tpd_calmat_local, 8*4);
	memcpy(tpd_def_calmat, tpd_def_calmat_local, 8*4);
#endif
	tpd_type_cap = 1;

	GSL_LOGD("tpd_local_init is ok.");
	return 0;
}

/* Function to manage low power suspend */
static void tpd_suspend(struct device *h)
{
	//int retval = 0;
	
	if (tpd_halt == 1) {
		pr_info("gslX680 already in suspended status\n");
		return;
	}
	GSL_LOGF();
	
/* 	retval = regulator_disable(tpd->reg);
	if (retval != 0) {		 
		GSL_LOGE("Failed to disable vldo28 regulator: %d\n", retval);	
	} */

#ifdef GSL_MONITOR
	GSL_LOGD("gsl_ts_suspend () : cancel gsl_monitor_work\n");
	cancel_delayed_work_sync(&gsl_monitor_work);
#endif
	tpd_gpio_output(GTP_RST_PORT, 0);
	msleep(20);
/*
* #ifdef GREEN_MODE
*	tpd_gpio_output(GTP_RST_PORT, 0);
*	msleep(20);
*	tpd_gpio_output(GTP_RST_PORT, 1);
*	msleep(8);
*	green_mode(i2c_client, MODE_OFF);
*	check_mem_data(i2c_client);
* #endif
*	tpd_gpio_output(GTP_RST_PORT, 0);
 */
	disable_irq(touch_irq);
	tpd_halt = 1;

	GSL_LOGD("tpd_suspend is ok.");
}

/* Function to manage power-on resume */
static void tpd_resume(struct device *h)
{
	int retval = 0;
	if (tpd_halt == 0) {
		pr_info("gslX680 already in resumed status\n");
		return;
	}
	GSL_LOGF();

	retval = regulator_enable(tpd->reg);
	if (retval != 0) {		 
		GSL_LOGE("Failed to enable vldo28 regulator: %d\n", retval);	
	}
	
	tpd_gpio_output(GTP_RST_PORT, 1);
	msleep(20);

	reset_chip(i2c_client);
	startup_chip(i2c_client);
#ifndef TP_COMPATIBLE
	check_mem_data(i2c_client);
#endif
/*
* check_mem_data(i2c_client);
* #ifdef GREEN_MODE
* green_mode(i2c_client, MODE_ON);
* #endif
 */
#ifdef GSL_MONITOR
	GSL_LOGD("gsl_ts_resume () : queue gsl_monitor_work\n");
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 500);
#endif
	enable_irq(touch_irq);
	tpd_halt = 0;
	GSL_LOGD("tpd_resume is ok.");
}

static struct tpd_driver_t tpd_device_driver = {
	.tpd_device_name = GSLX680_NAME,
	.tpd_local_init = tpd_local_init,
	.suspend = tpd_suspend,
	.resume = tpd_resume,
#ifdef TPD_HAVE_BUTTON
	.tpd_have_button = 1,
#else
	.tpd_have_button = 0,
#endif
};
#if 0
static ssize_t db_value_store(struct class *class,
			struct class_attribute *attr,	const char *buf, size_t count)
{
	unsigned long rs_tmp;

	if (kstrtoul(buf, 10, &rs_tmp))
		return 0;

	rs_value1 = rs_tmp;

	return count;
}

static ssize_t db_value_show(struct class *class,
			struct class_attribute *attr,	char *buf)
{
	return sprintf(buf, "rs_value1 = %d \r\n", rs_value1);
}

static struct class_attribute db_class_attrs[] = {
	__ATTR(db, 0644, db_value_show, db_value_store),
	__ATTR_NULL
};
#endif

static struct class db_interface_class = {
	.name = "db_interface",
	/* .class_attrs = db_class_attrs, */
};

/* called when loaded into kernel */
static int __init tpd_driver_init(void)
{
	int ret = 0;

	GSL_LOGF();
	tpd_get_dts_info();
	/* register usr space */
	ret = class_register(&db_interface_class);
#ifdef ADD_I2C_DEVICE_ANDROID_4_0
	i2c_register_board_info(1, &gslX680_i2c_tpd, 1);
#endif
	if (tpd_driver_add(&tpd_device_driver) < 0)
		GSL_LOGE("add gslX680 driver failed\n");

	GSL_LOGD("gslX680 driver init ok");
	return 0;
}

/* should never be called */
static void __exit tpd_driver_exit(void)
{
	GSL_LOGD("Sileadinc gslX680 touch panel driver exit\n");
	/* input_unregister_device(tpd->dev); */
	class_unregister(&db_interface_class);
	tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);

MODULE_AUTHOR("leweihua");
MODULE_DESCRIPTION("GSLX680 TouchScreen  Driver");
MODULE_LICENSE("GPL v2");
