#ifndef BUILD_LK
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
//#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
//#include <mt-plat/mtk_thermal_typedefs.h>

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>

/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/
#define	lt8911b_MIPIRX_SLAVE_ADDR_WRITE 				(0x1f >> 1)

#define DEVICE_NAME "lt8911b"
#define lt8911b_DEBUG

static struct i2c_client *lt8911b_mipirx = NULL;
static const struct i2c_device_id lt8911b_i2c_id[] =
{
	{DEVICE_NAME,	0},
	{}
};

static const struct of_device_id lt8911b_of_match[] = {
	{.compatible = "lt8911b",},
	{},
};
static int lt8911b_i2c_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);

static struct i2c_driver lt8911b_i2c_driver = {
  .driver = {
      .name    = DEVICE_NAME,
#ifdef CONFIG_OF
		   .of_match_table = lt8911b_of_match,
#endif
  },

  .probe       = lt8911b_i2c_driver_probe,
  .id_table    = lt8911b_i2c_id,
};

/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
static DEFINE_MUTEX(lt8911b_i2c_access);
/**********************************************************
  *
  *   [I2C Function For Read/Write it8911b]
  *
  *********************************************************/
 
/*
static int lt8911b_i2c_read_byte(kal_uint8 dev_addr, u16 addr, u32 *returnData)
{
	u8     cmd_buf[8]={0};
	int      ret=0;
	u32 data;
	u8 lens;
#ifdef lt8911b_DEBUG

	printk("[KE/lt8911b_read_byte] dev_addr = 0x%x, read_data[0x%x] = 0x%x \n", dev_addr, addr, *returnData);
#endif

	mutex_lock(&lt8911b_i2c_access);

	if(dev_addr == lt8911b_MIPIRX_SLAVE_ADDR_WRITE)
	{
		//lt8911b_mipirx->ext_flag=((lt8911b_mipirx->ext_flag) & I2C_MASK_FLAG ) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;

	    cmd_buf[0] = ( addr >> 8 ) & 0xFF;
        cmd_buf[1] = addr & 0xFF; 
			
		lens = 1;
		
		printk(" %s dev_addr = 0x%x cmd_buf[0] = 0x%x cmd_buf[1] = 0x%x\r\n", __func__,dev_addr,cmd_buf[0],cmd_buf[1]);
		printk(" %s  lens = %d\r\n", __func__,lens);		
		if(1 == lens){
			ret = i2c_master_send(lt8911b_mipirx, &cmd_buf[0], (4<<8 | 1));
		}else{
			ret = i2c_master_send(lt8911b_mipirx, &cmd_buf[0], (4<<8 | 2));
		}
		
		if (ret < 0)
		{
			//lt8911b_mipirx->ext_flag=0;
			mutex_unlock(&lt8911b_i2c_access);
			printk("+++++++++++++++++++ i2c_master_send err!!!  ret = %d \r\n",ret);
			return 0;
		}

		data = (cmd_buf[3] << 24) | (cmd_buf[2] << 16) | (cmd_buf[1] << 8) | (cmd_buf[0]);
		
		//int readData_all = (readData[0] | (readData[1]<<8) | (readData[2]<<16) | (readData[3]<<24);
		
		printk(" %s   data = 0x%x \r\n", __func__,data);
		//lt8911b_mipirx->ext_flag=0;
	}
	else
	{
		printk("[lt8911b_i2c_read_byte]error:  no this dev_addr! \n");
	}

	mutex_unlock(&lt8911b_i2c_access);

	return 1;
}

static int lt8911b_i2c_write_byte(kal_uint8 dev_addr, kal_uint8 addr, kal_uint8 writeData)
{
	char    write_data[2] = {0};
	int     ret=0;

#ifdef lt8911b_DEBUG
	printk("[KE/lt8911b_i2c_write] dev_addr = 0x%x, write_data[0x%x] = 0x%x \n", dev_addr, addr, writeData);
#endif

	mutex_lock(&lt8911b_i2c_access);

	write_data[0] = addr;
	write_data[1] = writeData;

	if(dev_addr == lt8911b_MIPIRX_SLAVE_ADDR_WRITE)
	{
		lt8911b_mipirx->addr = lt8911b_MIPIRX_SLAVE_ADDR_WRITE;
		//lt8911b_mipirx->ext_flag=((lt8911b_mipirx->ext_flag) & I2C_MASK_FLAG) | I2C_DIRECTION_FLAG;

		ret = i2c_master_send(lt8911b_mipirx, write_data, 2);

		if(ret < 0)
		{
			//lt8911b_mipirx->ext_flag=0;
			mutex_unlock(&lt8911b_i2c_access);

			return 0;
		}

		//lt8911b_mipirx->ext_flag=0;
	}
	else
	{
		printk("[lt8911b_i2c_write_byte]error:  no this dev_addr! \n");
	}

	mutex_unlock(&lt8911b_i2c_access);

	return 1;
}
*/
static int tc358775_i2c_read(struct i2c_client *client, u16 addr, u8 *data, u8 len)
{
	//u8 beg = addr;
	int err;
	struct i2c_msg msgs[2] = { {0}, {0} };
	
	u8 reg_addr[2] = {0};
	u8 reg_len = 0;
	
    reg_addr[0] = ( addr >> 8 ) & 0xFF;
    reg_addr[1] = addr & 0xFF;     
    reg_len = 2;		

	//printk("====== 11 addr = 0x%x\r\n",addr);
	//printk("====== 11 reg_len = %d reg_addr[0]=0x%2x reg_addr[1]=0x%2x\r\n",reg_len,reg_addr[0],reg_addr[1]);

	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = reg_len;
	msgs[0].buf = reg_addr;
	//msgs[0].timing = 100;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = data;
	//msgs[1].timing = 100;

	err = i2c_transfer(client->adapter, msgs, sizeof(msgs) / sizeof(msgs[0]));
	if (err < 0) {
		printk("tc358775_i2c_read i2c_transfer error: addr = 0x%4x len = %x err = %d \r\n", addr, len, err);
		err = -EIO;
	} else {
		err = 0;
	}

	return err;

}

int tc358775_i2c_write(struct i2c_client *client, u16 addr, u32 write_data, u8 len)
{
	//u8 beg = addr;
	int err;
	struct i2c_msg msgs[1] = { {0} };
	u8 buffer[6] = {0};
	u8 lens = 0;
	//printk("tc358775_i2c_write  start\r\n");
	if(4 == len){		
		buffer[0] = (addr >> 8) & 0xFF;
		buffer[1] = addr & 0xFF;
		buffer[2] = write_data & 0xFF;
		buffer[3] = (write_data >> 8) & 0xFF;
		lens = 4;       
	}else{
		buffer[0] = (addr >> 8) & 0xFF;
		buffer[1] = addr & 0xFF;
		buffer[2] = write_data & 0xFF;     
		buffer[3] = (write_data >> 8) & 0xFF;
		buffer[4] = (write_data >> 16) & 0xFF;	
		buffer[5] = (write_data >> 24) & 0xFF;		
		lens = 6;	
	}

	printk("tc358775_i2c_write addr = 0x%4x , write_data = 0x%8x \r\n",addr,write_data);
	//printk("======= reg_len = %d buf[0] = 0x%x buf[1] = 0x%x buf[2] = 0x%x  buf[3] = 0x%x\r\n",reg_len,buf[0],buf[1],buf[2],buf[3]);
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = lens;
	msgs[0].buf = buffer;
	//msgs[0].timing = 100;

	err = i2c_transfer(client->adapter, msgs, sizeof(msgs) / sizeof(msgs[0]));
	//printk("====== i2c_transfer  err = %d \r\n",err);
	if (err <0) {
		printk("tc358775_i2c_write i2c_transfer error: addr = 0x%4x len = %x err = %d \r\n", addr, len, err);
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
}

int  lt8911_reg_i2c_read(u16 addr,u8 len)
{
	//kal_uint8 dev_addr = lt8911b_MIPIRX_SLAVE_ADDR_WRITE;
	u32 data2;
	u8 databuf[4] = { 0 };
	int res = 0;
	int try_read = 0;

	//mdelay(20);
	res = tc358775_i2c_read(lt8911b_mipirx, addr, databuf, len);
	if (res < 0){

		for(try_read = 1;try_read<=10;try_read++){
			//mdelay(20);
			res = tc358775_i2c_read(lt8911b_mipirx, addr, databuf, len);
			if(res == 0){
				printk("%s ok!!!! addr = 0x%4x  len = 0x%8x try_write = %d \r\n", __func__,addr,len,try_read);
				break;
			}
		}
		if(try_read ==11){
			printk("%s error!!!! addr = 0x%4x  len = 0x%8x try_read = %d \r\n", __func__,addr,len,try_read);
		}

		//printk("lt8911_reg_i2c_read tc358775_i2c_read error!!! \r\n");
	}else{
		//printk("====== databuf[0]:%2x databuf[1]:%2x databuf[2]:%2x databuf[3]:%2x done! \r\n ", databuf[0],databuf[1],databuf[2],databuf[3]);
	}
	data2 = (databuf[3] << 24) | (databuf[2] << 16) | (databuf[1] << 8) | (databuf[0]);
	//printk("%s addr = 0x%4x data = 0x%8x \r\n", __func__,addr,data2);
	
	return 0;
}

int lt8911_reg_i2c_write(u16 addr, u32 val,u8 len)
{
	int err = 0;
	int try_write = 0;

	if(tc358775_i2c_write(lt8911b_mipirx, addr, val, len) != 0){

		for(try_write = 1;try_write<=10;try_write++){
			//mdelay(20);
			err = tc358775_i2c_write(lt8911b_mipirx, addr, val, len);
			if(err == 0){
				printk("%s ok!!!! addr = 0x%4x  val = 0x%8x try_write = %d \r\n", __func__,addr,val,try_write);
				break;
			}
		}
		if(try_write ==11){
			printk("%s error!!!! addr = 0x%4x  val = 0x%8x try_write = %d \r\n", __func__,addr,val,try_write);
		}

	}
	return 0;
}

static int match_id(const struct i2c_client *client, const struct i2c_device_id *id)
{
	if (strcmp(client->name, id->name) == 0)
		return true;
	else
		return false;
}

static int lt8911b_i2c_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err=0;

	printk("[lt8911b_i2c_driver_probe] start!\n");

	if(match_id(client, &lt8911b_i2c_id[0]))
	{
	  if (!(lt8911b_mipirx = kmalloc(sizeof(struct i2c_client), GFP_KERNEL)))
		{
	    err = -ENOMEM;
	    goto exit;
	  }

	  memset(lt8911b_mipirx, 0, sizeof(struct i2c_client));

	  lt8911b_mipirx = client;
	}
	else
	{
		printk("[lt8911b_i2c_driver_probe] error!\n");

		err = -EIO;
		goto exit;
	}

	printk("[lt8911b_i2c_driver_probe] %s i2c sucess!\n", client->name);

	return 0;

	exit:
	return err;

}

#define lt8911b_BUSNUM 4
#if 0
static struct i2c_board_info __initdata lt8911b_I2C[] =
{
	{I2C_BOARD_INFO(DEVICE_NAME, 	lt8911b_MIPIRX_SLAVE_ADDR_WRITE)},
};
#endif
static int __init lt8911b_init(void)
{
	printk("[lt8911b_init] init start\n");

//	i2c_register_board_info(lt8911b_BUSNUM, lt8911b_I2C, 1);

	if(i2c_add_driver(&lt8911b_i2c_driver)!=0)
	{
		printk("[lt8911b_init] Failed to register lt8911b i2c driver.\n");
	}
	else
	{
		printk("[lt8911b_init] Success to register lt8911b i2c driver.\n");
	}

	return 0;
}

static void __exit lt8911b_exit(void)
{
	i2c_del_driver(&lt8911b_i2c_driver);
}

module_init(lt8911b_init);
module_exit(lt8911b_exit);

MODULE_DESCRIPTION("I2C lt8911b Driver");
MODULE_AUTHOR("James Lo<james.lo@mediatek.com>");
#endif