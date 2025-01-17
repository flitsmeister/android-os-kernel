#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/slab.h>
#include <linux/types.h>
//#include <linux/xlog.h>
#include "kd_camera_typedef.h"

/*===FEATURE SWITH===*/
 // #define FPTPDAFSUPPORT   //for pdaf switch
 // #define FANPENGTAO   //for debug log
 #define LOG_INF LOG_INF_NEW
/*===FEATURE SWITH===*/

/****************************Modify Following Strings for Debug****************************/
#define PFX "S5K3L8PDAF"
#define LOG_INF_NEW(format, args...)    pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)
//#define LOG_INF_LOD(format, args...)    xlog_printk(ANDROID_LOG_INFO   , PFX, "[%s] " format, __FUNCTION__, ##args)
//#define LOG_1 LOG_INF("S5K3L8,MIPI 4LANE\n")
#define SENSORDB LOG_INF
/****************************   Modify end    *******************************************/

//#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern void kdSetI2CSpeed(u16 i2cSpeed);
//extern int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId, u16 transfer_length);
extern int iMultiReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId, u8 number);


#define USHORT             unsigned short
#define BYTE               unsigned char
#define Sleep(ms)          mdelay(ms)

/**************  CONFIG BY SENSOR >>> ************/
#define EEPROM_WRITE_ID   0xa0
#define I2C_SPEED        100
#define MAX_OFFSET       0xFFFF
#define DATA_SIZE         1404
#define START_ADDR        0X0763
BYTE S5K3L8_eeprom_data[DATA_SIZE]= {0};

/**************  CONFIG BY SENSOR <<< ************/

static kal_uint16 read_cmos_sensor_byte(kal_uint16 addr)
{
    kal_uint16 get_byte=0;
    char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };

	kdSetI2CSpeed(
        I2C_SPEED); /* Add this func to set i2c speed by each sensor */
    iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, EEPROM_WRITE_ID);
    return get_byte;
}

static bool _read_eeprom(kal_uint16 addr, kal_uint32 size)
{
	int i = 0;

	/* continue read reg by byte: */
	for (; i < size; i++) {
		S5K3L8_eeprom_data[i] = read_cmos_sensor_byte(addr + i);
		LOG_INF("add = 0x%x,\tvalue = 0x%x", i, S5K3L8_eeprom_data[i]);
	}
	return true;
}

bool S5K3L8_read_eeprom(kal_uint16 addr, BYTE *data, kal_uint32 size)
{
	addr = START_ADDR;
	size = DATA_SIZE;
	LOG_INF("Read EEPROM, addr = 0x%x, size = 0d%d\n", addr, size);
	if (!_read_eeprom(addr, size)) {
		LOG_INF("error:read_eeprom fail!\n");
		return false;
	}

	memcpy(data, S5K3L8_eeprom_data, size);
	return true;
}


