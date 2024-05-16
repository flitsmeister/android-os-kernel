/*
 * Touchright serial touchscreen driver
 *
 * Copyright (c) 2006 Rick Koch <n1gp@hotmail.com>
 *
 * Based on MicroTouch driver (drivers/input/touchscreen/mtouch.c)
 * Copyright (c) 2004 Vojtech Pavlik
 * and Dan Streetman <ddstreet@ieee.org>
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/serio.h>

#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/gpio.h>
#include <linux/device.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#endif
#include <linux/interrupt.h>


#define DRIVER_DESC	"MID serial uart dock driver"

#include <linux/miscdevice.h>
#include "mid_uart_dock.h"

#include <linux/fb.h>
#include <linux/notifier.h>


#define LOG_ERR (6)//(6)
#define LOG_DEBUG (7)//(7)
#define LOG_FULL (8)//(8)
#define Enable_LOG LOG_FULL
#define UART_DOCK_DRIVER_NAME "[kzhkzh_UART_DOCK]"
#define LOG_UART_DOCK(num, fmt, args...)   \
do {									\
	if (Enable_LOG >= (int)num) {		\
		printk(KERN_ERR UART_DOCK_DRIVER_NAME " <%s> <%d> "fmt"\n", __func__, __LINE__, ##args);	\
	}								   \
} while (0)

unsigned int dock_irqnr;
unsigned int GPIO_DOCKING_DET;
unsigned int GPIO_OTG_LDO_EN;//3.3V dock en
static struct delayed_work dock_check_work;
//static DEFINE_MUTEX(mid_kpd_mutex);
extern int get_docking_status(void);

static old_real_report_val = 0;
static bool MID_UART_DOCK_PLUG = false;
static struct wakeup_source *mid_uart_wakelock;
//static unsigned char old_mouse_data;
#if 1
struct mid_kpd {
	unsigned char old[8];
	int idx;
	unsigned char *new;
};
struct mid_kpd *kbd;


static int report_id = 0;
static int kplen = 0;

#endif

static DEFINE_MUTEX(mid_ctrl_mutex);


static const unsigned char mid_kbd_keycode[256] = {
	  0,  0,  0,  0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,
	 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,  2,  3,
	  4,  5,  6,  7,  8,  9, 10, 11, 28,  1, 14, 15, 57, 12, 13, 26,
	 27, 43, 43, 39, 40, 41, 51, 52, 53, 58, 59, 60, 61, 62, 63, 64,
	 65, 66, 67, 68, 87, 88, 99, 70,119,110,102,104,111,107,109,106,
	105,108,103, 69, 98, 55, 74, 78, 96, 79, 80, 81, 75, 76, 77, 71,
	 72, 73, 82, 83, 86,127,116,117,183,184,185,186,187,188,189,190,
	191,192,193,194,134,138,130,132,128,129,131,137,133,135,136,113,
	115,114,  0,  0,  0,121,  0, 89, 93,124, 92, 94, 95,  0,  0,  0,
	122,123, 90, 91, 85,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 29, 42, 56,125, 97, 54,100,126,164,166,165,163,161,115,114,113,
	150,158,159,128,136,177,178,176,142,152,173,140
};			   
#define MAX_KEY_CNT	(sizeof(mid_kbd_keycode)/sizeof(mid_kbd_keycode[0]))
#define DEVICE_NAME	"mid-input"
static struct input_dev *mid_input;
static struct input_dev *mid_tochpad;


/*
 * Definitions & global arrays.
 */

#define TR_FORMAT_TOUCH_BIT	0xFF //按键松开检测
//#define TR_FORMAT_STATUS_BYTE	0x40
//#define TR_FORMAT_STATUS_MASK	~TR_FORMAT_TOUCH_BIT

#define TR_LENGTH 9

#define TR_MIN_XC -127
#define TR_MAX_XC 127
#define TR_MIN_YC -127
#define TR_MAX_YC 127

#define MID_SERIO_CONTROL    1

/*
 * Per-touchscreen data.
 */

struct tr {
	struct input_dev *dev;
	struct serio *serio;
	int idx;
	signed char data[TR_LENGTH];
	char phys[32];
	
	unsigned long last;
	unsigned char count;
};
#ifdef MID_SERIO_CONTROL
static int mid_report_uart_data(int recevice_data)
{
	int real_report_value = 0;
	LOG_UART_DOCK(LOG_DEBUG, " ###########  report case %x#########\n",recevice_data);
	switch(recevice_data)
	{
		case MID_KEY_NULL://无值
		real_report_value = 0;
		break;
		case UART_KEY_SEARCH:
		LOG_UART_DOCK(LOG_DEBUG, " ########### report KEY_SEARCH char ########## \n");
		real_report_value = KEY_SEARCH;
		break;
		
		case UART_KEY_BACK:
		LOG_UART_DOCK(LOG_DEBUG, " ########### report KEY_BACK char ########## \n");
		real_report_value = KEY_BACK;
		break;
		
		case UART_KEY_INTERNET_BROWSER:
		LOG_UART_DOCK(LOG_DEBUG, " ########### report UART_KEY_INTERNET_BROWSER char ########## \n");
		real_report_value = KEY_WWW;
		break;
		
		case UART_KEY_EMAIL:
		LOG_UART_DOCK(LOG_DEBUG, " ########### report KEY_EMAIL char ########## \n");
		real_report_value = KEY_EMAIL;
		break;
		
		case UART_KEY_MENU:
		LOG_UART_DOCK(LOG_DEBUG, " ########### report KEY_MENU char ########## \n");
		real_report_value = KEY_MENU;
		break;
		
		case UART_KEY_PLAYPAUSE:
		real_report_value = KEY_PLAYPAUSE;
		break;
		
		
		case UART_KEY_NEXTSONG:
		real_report_value = KEY_NEXTSONG;
		break;		
				
		case UART_KEY_PREVIOUSSONG:
		real_report_value = KEY_PREVIOUSSONG;
		break;
		
		
		
		case UART_KEY_MUTE:
		real_report_value = KEY_MUTE;
		break;
		
		case UART_KEY_VOLUMEDOWN:
		real_report_value = KEY_VOLUMEDOWN;
		break;
		
		case UART_KEY_VOLUMEUP:
		real_report_value = KEY_VOLUMEUP;
		break;
		

		
		case UART_KEY_POWER:
		real_report_value = KEY_POWER;
		break;
		
		case UART_KEY_HOME:
		real_report_value = KEY_HOMEPAGE;	/* AC Home */
		break;
		
		default:
			LOG_UART_DOCK(LOG_DEBUG, " ########### report ID index:%d. fatal error #########\n");
		break;
	}

	return real_report_value;
	
}

void mid_special_key_report(int kplen,struct serio *serio)
{
	struct tr *tr = serio_get_drvdata(serio);
	struct input_dev *dev = tr->dev;
	int real_report_val = 0;
	int mid_data;
	
	//42 & 62 report ID only with 2 byte key_data: (data[2]<<8)|data[1];
	mid_data = ((unsigned char)tr->data[2]<<8) | (unsigned char)tr->data[1];
	LOG_UART_DOCK(LOG_FULL, " mid_data:%d\n",mid_data);

	real_report_val = mid_report_uart_data(mid_data);
	LOG_UART_DOCK(LOG_FULL, " report new key real_report_val:%d,old_real_report_val:%d\n",real_report_val,old_real_report_val);
	if(real_report_val != old_real_report_val){//key有变化
		if(real_report_val != 0)//非空,当前是按下状态
		{
			LOG_UART_DOCK(LOG_FULL, " report new key press real_report_val:%d\n",real_report_val);
			input_report_key(dev, real_report_val, 1);
			input_sync(dev);

			
		}else{//松开,上报之前的key
			LOG_UART_DOCK(LOG_FULL, " report old key realease old_real_report_val:%d\n",old_real_report_val);
 			input_report_key(dev, old_real_report_val, 0);
			input_sync(dev);
			

		}
	}
	old_real_report_val = real_report_val;//record old key

	return ;
}


void *mid_scan(unsigned char *data, int c, size_t size)
{
	unsigned char *p = data;
	//LOG_UART_DOCK(LOG_DEBUG, " enter \n");
	while (size) {
		if (*p == c)
		return (void *)p;
		p++;
		size--;
	}
	return (unsigned char *)p;
} 
/* void mid_mouse_report(unsigned char mid_mouse_data,struct serio *serio)
{
	struct tr *tr = serio_get_drvdata(serio);
	//struct input_dev *dev = tr->dev;
	unsigned char new_mouse_data = mid_mouse_data;
	
	LOG_UART_DOCK(LOG_DEBUG, " enter new_mouse_data:%x,old_mouse_data:%x\n",new_mouse_data,old_mouse_data);
	if(old_mouse_data != new_mouse_data)
	{
	
		input_event(mid_tochpad,EV_MSC,MSC_SCAN,1);

		input_event(mid_tochpad,EV_KEY,BTN_MOUSE,new_mouse_data);		
		input_report_rel(mid_tochpad, BTN_LEFT,   new_mouse_data & 0x01);
		input_report_rel(mid_tochpad, BTN_RIGHT,  new_mouse_data & 0x02);
		input_report_rel(mid_tochpad, BTN_MIDDLE, new_mouse_data & 0x04);
	}
	
	old_mouse_data = new_mouse_data;
	
} */

static irqreturn_t tr_interrupt(struct serio *serio,
		unsigned char data, unsigned int flags)
{
	struct tr *tr = serio_get_drvdata(serio);
	struct input_dev *dev = tr->dev;
	int i;

	
#if 0 //test interrupts rate
	LOG_UART_DOCK(LOG_DEBUG, "enter  data:0x%x\n",data);
	return IRQ_HANDLED;
#endif	

	if(tr->idx == 0)
	{
		report_id = (data & 0xe0) >> 5;
		kplen = data & 0x1F;
	}
	
	//LOG_UART_DOCK(LOG_DEBUG, "enter  data:0x%x\n",data);
	tr->data[tr->idx++] = (signed char)data;
	
	if (tr->idx == (kplen+1))
		goto event_handle;
	
	return IRQ_HANDLED;
 	//if (report_id != 0) {

event_handle :						
			switch(report_id)
			{
				case 1://data[3-8]

#if 1
						for (i = 0; i < 8; i++) {
							kbd->new[i] = (unsigned char)tr->data[i+1];//将data数组每次缓存到 kbd->new 数组去比较判断
							//LOG_UART_DOCK(LOG_DEBUG, "current_real_data[0-7] report kbd->new[%d]:%x\n",i,kbd->new[i]);
							
							input_report_key(dev, mid_kbd_keycode[i + 224], (kbd->new[0] >> i) & 1);//检测第i位是否为1，如果为1，则上报按键mid_kbd_keycode[i + 224]
							//LOG_UART_DOCK(LOG_DEBUG, "mid_kbd_keycode[%d]%x\n",i + 224,mid_kbd_keycode[i + 224]);
						}
						for (i = 2; i < 8; i++) {							
					
							if (kbd->old[i] > 3 && mid_scan(kbd->new + 2, kbd->old[i], 6) == kbd->new + 8) {
								if (mid_kbd_keycode[kbd->old[i]]){
									input_report_key(dev, mid_kbd_keycode[kbd->old[i]], 0);//新出现的按键值没有old[i],释放old[i]
									LOG_UART_DOCK(LOG_DEBUG, 
										"New key released.\n",
										kbd->new[i]);
								}else{
									LOG_UART_DOCK(LOG_DEBUG, 
										"Unknown key (scancode %#x) released.\n",
										kbd->old[i]);
								}
							}
						
							if (kbd->new[i] > 3 && mid_scan(kbd->old + 2, kbd->new[i], 6) == kbd->old + 8) {
								if (mid_kbd_keycode[kbd->new[i]])//old[i]对应的input码有意义
								{
									input_report_key(dev, mid_kbd_keycode[kbd->new[i]], 1);//上报新键按下事件
									LOG_UART_DOCK(LOG_DEBUG, 
										"New key (scancode %#x) pressed.\n",
										kbd->new[i]);
								}else{
									LOG_UART_DOCK(LOG_DEBUG, 
										"Unknown key (scancode %#x) pressed.\n",
										kbd->new[i]);
								}
							}
						}
						input_sync(dev);
						memcpy(kbd->old, kbd->new, 8);//记录上一次report数组
#endif						

					break;
				case 2:
					LOG_UART_DOCK(LOG_FULL, " ########### report ID 42.report system control key value #########\n");
					mid_special_key_report(kplen,serio);
					break;
				case 3:
					mid_special_key_report(kplen,serio);
					LOG_UART_DOCK(LOG_FULL, " ########### report ID 62.report consumer key value #########\\n");
					break;
				case 5:

						input_report_key(mid_tochpad, BTN_LEFT,   tr->data[1] & 0x01);
						input_report_key(mid_tochpad, BTN_RIGHT,  tr->data[1] & 0x02);
						input_report_key(mid_tochpad, BTN_MIDDLE, tr->data[1] & 0x04);
						
						input_report_rel(mid_tochpad, REL_WHEEL, tr->data[4]);//上下滚轮
						input_report_rel(mid_tochpad, REL_HWHEEL, tr->data[5]);//水平滚轮
						input_report_rel(mid_tochpad, REL_X, (signed char)tr->data[2]);
						input_report_rel(mid_tochpad, REL_Y, (signed char)tr->data[3]);
						input_sync(mid_tochpad);

					break;	
				default:
					LOG_UART_DOCK(LOG_DEBUG, " ########### report ID index:%d. fatal error #########\n");
					break;
				
			}

			tr->idx = 0;

	return IRQ_HANDLED;
}

/*
 * tr_disconnect() is the opposite of tr_connect()
 */

static void tr_disconnect(struct serio *serio)
{
	struct tr *tr = serio_get_drvdata(serio);
	LOG_UART_DOCK(LOG_DEBUG," enter\n");
	input_get_device(tr->dev);
	input_unregister_device(tr->dev);
	serio_close(serio);
	serio_set_drvdata(serio, NULL);
	input_put_device(tr->dev);
	kfree(tr);
}

/*
 * tr_connect() is the routine that is called when someone adds a
 * new serio device that supports the Touchright protocol and registers it as
 * an input device.
 */

static int tr_connect(struct serio *serio, struct serio_driver *drv)
{
	struct tr *tr;
	
	//kzh add for report temp buf start
	struct mid_kpd *mid_kbd;
	unsigned char *mid_new;
	//kzh add for report temp buf end
	
	struct input_dev *input_dev;
	int err,i;
	
	
	LOG_UART_DOCK(LOG_DEBUG," enter\n");
	tr = kzalloc(sizeof(struct tr), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!tr || !input_dev) {
		err = -ENOMEM;
		LOG_UART_DOCK(LOG_ERR,"unable to register input_dev.\n");
		goto fail1;
	}
	
	//kzh add for report temp buf start
	mid_kbd = kzalloc(sizeof(struct mid_kpd), GFP_KERNEL);
	if (!mid_kbd ) {
		err = -ENOMEM;
		LOG_UART_DOCK(LOG_ERR,"unable to kzalloc mid_kbd.\n");
		return err;
	}
	mid_new = kzalloc(sizeof(unsigned char), GFP_KERNEL);
	if (!mid_new ) {
		err = -ENOMEM;
		LOG_UART_DOCK(LOG_ERR,"unable to kzalloc mid_new.\n");
		return err;
	}
	mid_kbd->new = mid_new;
	kbd = mid_kbd;
	//kzh add for report temp buf end

	tr->serio = serio;
	tr->dev = input_dev;
	snprintf(tr->phys, sizeof(tr->phys), "%s/input0", serio->phys);

	input_dev->name = "uart_dock";
	input_dev->phys = tr->phys;
	input_dev->id.bustype = SERIO_RS232;
	input_dev->id.vendor = 0x3F;
	input_dev->id.product = 1;
	input_dev->id.version = 0x0110;
	input_dev->dev.parent = &serio->dev;
	
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(EV_SYN, input_dev->evbit);


	for(i = 0; i < 256; i++){
		input_set_capability(input_dev, EV_KEY, mid_kbd_keycode[i]);
	}

	//kzhkzh add for extra media & system control key 	
	input_set_capability(input_dev, EV_KEY, KEY_SEARCH);
	input_set_capability(input_dev, EV_KEY, KEY_WWW);//UART_KEY_INTERNET_BROWSER
	input_set_capability(input_dev, EV_KEY, KEY_EMAIL);
	input_set_capability(input_dev, EV_KEY, KEY_MENU);
	input_set_capability(input_dev, EV_KEY, KEY_HOMEPAGE);
	

	




	serio_set_drvdata(serio, tr);

	err = serio_open(serio, drv);
	LOG_UART_DOCK(LOG_DEBUG," err:%d\n",err);
	if (err){
		LOG_UART_DOCK(LOG_ERR," faile to open serio \n");
		goto fail2;
	}

	err = input_register_device(tr->dev);
	LOG_UART_DOCK(LOG_DEBUG," err:%d\n",err);
	if (err){
		LOG_UART_DOCK(LOG_ERR," faile to register  input dev \n");
		goto fail3;
	}
	LOG_UART_DOCK(LOG_DEBUG," enter\n");
	return 0;

 fail3:	serio_close(serio);
 fail2:	serio_set_drvdata(serio, NULL);
 fail1:	input_free_device(input_dev);
	kfree(tr);
	return err;
}


/*
 * The serio driver structure.
 */

static const struct serio_device_id tr_serio_ids[] = {
	{
		.type	= SERIO_RS232,
		.proto	= 0x3F,
		.id	= SERIO_ANY,
		.extra	= SERIO_ANY,
	},
	{ 0 }
};

MODULE_DEVICE_TABLE(serio, tr_serio_ids);



static struct serio_driver mid_serio_drv = {
	.driver		= {
		.name	= "uart_dock",
	},
	.description	= DRIVER_DESC,
	.id_table	= tr_serio_ids,
	.interrupt	= tr_interrupt,
	.connect	= tr_connect,
	.disconnect	= tr_disconnect,
};
#endif

//kzhkzh add for platform driver register
static void mid_get_dts_info(void)
{
	struct device_node *node= NULL;
	int ret = 0;
	node = of_find_compatible_node(NULL, NULL, "mediatek, mid_docking");
	if(!node){
		LOG_UART_DOCK(LOG_ERR,"of_find_compatible_node of mediatek, mid_docking\n");
		return ;
	}
	
	ret = of_get_named_gpio(node, "gpio_dock_3v3_en", 0);
	LOG_UART_DOCK(LOG_DEBUG,"gpio_dock_3v3_en:%d\n",ret);
    if(ret >= 0) {
    	GPIO_OTG_LDO_EN = (unsigned int )ret;
		gpio_request(GPIO_OTG_LDO_EN, "gpio_dock_3v3_en");
    }
	gpio_direction_output(GPIO_OTG_LDO_EN, 0);//only for test
	ret = of_get_named_gpio(node, "gpio_docking_det", 0);
    LOG_UART_DOCK(LOG_DEBUG,"gpio_docking_det:%d\n",ret);
    if(ret >= 0) {
    	GPIO_DOCKING_DET = (unsigned int )ret;
		gpio_request(GPIO_DOCKING_DET, "GPIO_DOCKING_DET");
		gpio_direction_input(GPIO_DOCKING_DET);
    }

}

int get_docking_status(void){
	return gpio_get_value(GPIO_DOCKING_DET);
}
EXPORT_SYMBOL_GPL(get_docking_status);

static irqreturn_t dock_eint_interrupt_handler(void)
{
	disable_irq_nosync(dock_irqnr);
	schedule_delayed_work(&dock_check_work, msecs_to_jiffies(200));

	 return IRQ_HANDLED;
}

static void dock_mode_switch(struct work_struct *work)
{
	//LOG_UART_DOCK(LOG_DEBUG,"dock_mode_switch\n");
	if((get_docking_status() == 0)&&(MID_UART_DOCK_PLUG == false)){
		gpio_direction_output(GPIO_OTG_LDO_EN, 1);
		
		//kzh add for report new sw event for uart dock 
 		input_report_switch(mid_input, SW_MID_UART_DOCK,1);
		input_sync(mid_input);
		MID_UART_DOCK_PLUG = true;

		irq_set_irq_type(dock_irqnr, IRQF_TRIGGER_HIGH);
		LOG_UART_DOCK(LOG_DEBUG,"MID_docking 1111 eint trigger,MID_UART_DOCK_PLUG:%d\n",MID_UART_DOCK_PLUG);

		__pm_stay_awake(mid_uart_wakelock);
	}else if ((get_docking_status() == 1)&&(MID_UART_DOCK_PLUG == true)){
		gpio_direction_output(GPIO_OTG_LDO_EN, 0);
		
		//kzh add for report new sw event for uart dock 
		input_report_switch(mid_input, SW_MID_UART_DOCK,0);
		input_sync(mid_input);
		MID_UART_DOCK_PLUG = false;

		irq_set_irq_type(dock_irqnr, IRQF_TRIGGER_LOW);
		LOG_UART_DOCK(LOG_DEBUG,"MID_docking 0000 eint trigger,MID_UART_DOCK_PLUG:%d\n",MID_UART_DOCK_PLUG);

		__pm_relax(mid_uart_wakelock);
	}
	enable_irq(dock_irqnr);
}
static const struct of_device_id mid_match_table[] = {
	{.compatible = "mediatek, mid_docking",},
	{}
};
MODULE_DEVICE_TABLE(of, mid_match_table);


static ssize_t mid_input_write(struct file *file, const char __user *buf, size_t count_want, loff_t *ppos)
{
	char kbuf[64] = {0};
	int rc = 0;
	unsigned int ret_value = 0;//获取转换后的数据
	memset(kbuf,0,64);

	rc = copy_from_user(kbuf, buf, count_want);
	if (rc)
	{
		LOG_UART_DOCK(LOG_DEBUG,"copy_from_user fail!\n");
		return -EINVAL;
	}
	LOG_UART_DOCK(LOG_DEBUG,"kernel parse kbuf:%s\n", kbuf);

	mutex_lock(&mid_ctrl_mutex);
	ret_value = simple_strtol(kbuf, NULL, 16);
	mutex_unlock(&mid_ctrl_mutex);
	LOG_UART_DOCK(LOG_DEBUG,"Last ret_value:0x%x\n",ret_value);

	if(1 == ret_value)
	{
		schedule_delayed_work(&dock_check_work, msecs_to_jiffies(5000));//kzhkzh add for System first boot on,need exec
	}

	return count_want;
}


static struct file_operations mid_dev_fops = {
	.owner	=THIS_MODULE,
	.write	=mid_input_write,
	//.read = mid_input_read,
};

static struct miscdevice mid_misc_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "mid_uart",
	.fops	= &mid_dev_fops,
};
//kzhkzh add for boot_on

static int mid_input_init(void)
{
	int ret = 0;

	mid_input = input_allocate_device();
	if(!mid_input) 
		return -ENOMEM;
 
	__set_bit(EV_SW, mid_input->evbit);
	__set_bit(EV_SYN, mid_input->evbit);
	__set_bit(SW_MID_UART_DOCK, mid_input->swbit);
	__set_bit(EV_KEY, mid_input->evbit);
	
	input_set_capability(mid_input, EV_KEY, KEY_F10);
	
 
	mid_input->name = "mid_input";
	//mid_input->phys = "mid-input/input0";
 
	mid_input->id.bustype = BUS_HOST;
 
	if(input_register_device(mid_input) != 0)
	{
		LOG_UART_DOCK(LOG_DEBUG," mid-input mid_input register device fail!!\n");
		input_free_device(mid_input);
		return -ENODEV;
	}
	
	ret = misc_register(&mid_misc_dev);
	
	LOG_UART_DOCK(LOG_DEBUG,"enter probe success End,ret:%d!!!!\n",ret);
	return ret;
}

static int mid_mouse_init(void)
{
	mid_tochpad = input_allocate_device();
	if(!mid_tochpad) 
		return -ENOMEM;
 
	__set_bit(EV_SW, mid_tochpad->evbit);
	__set_bit(EV_SYN, mid_tochpad->evbit);
	__set_bit(SW_MID_UART_DOCK, mid_tochpad->swbit);
	__set_bit(EV_KEY, mid_tochpad->evbit);
	
	mid_tochpad->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
	mid_tochpad->keybit[BIT_WORD(BTN_MOUSE)] = BIT_MASK(BTN_LEFT) |
		BIT_MASK(BTN_RIGHT);
	mid_tochpad->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y); 

	//set_bit(mid_tochpad, EV_MSC, MSC_SCAN);
	mid_tochpad->name = "mid_mouse";
	mid_input->phys = "mid_mouse/input0";
	set_bit(BTN_MIDDLE, mid_tochpad->keybit);
	set_bit(BTN_SIDE, mid_tochpad->keybit);
	set_bit(BTN_EXTRA, mid_tochpad->keybit);	
	set_bit(REL_WHEEL, mid_tochpad->relbit);	
	set_bit(REL_HWHEEL, mid_tochpad->relbit);
	
	input_set_capability(mid_tochpad, EV_KEY, KEY_F10);

 	mid_tochpad->id.bustype = BUS_RS232;
	mid_tochpad->id.vendor = 0x3F;
	mid_tochpad->id.product = 0x001e;
	mid_tochpad->id.version = 0x0110;
	if(input_register_device(mid_tochpad) != 0)
	{
		LOG_UART_DOCK(LOG_DEBUG," mid-input mid_tochpad register device fail!!\n");
		input_free_device(mid_tochpad);
		return -ENODEV;
	}
 
	LOG_UART_DOCK(LOG_DEBUG," mid_tochpad register tinitialized %d\n");
	return 0;
}

static int mid_docking_probe(struct platform_device *pdev)
{
	
	struct device_node *node1 = NULL;
	int ret = 0;
	
	mid_get_dts_info();
	
	node1 = of_find_matching_node(node1, mid_match_table);
	if (node1) {
		dock_irqnr = irq_of_parse_and_map(node1, 0);
		ret = request_irq(dock_irqnr, (irq_handler_t)dock_eint_interrupt_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "dock-eint", NULL);
		if (ret != 0) {
			LOG_UART_DOCK(LOG_ERR,"EINT IRQ LINE NOT AVAILABLE\n");
		} else {
			LOG_UART_DOCK(LOG_DEBUG,"[MID DOCKING]set EINT finished %s, %d, dock_irqnr=%d\n",__func__, __LINE__, dock_irqnr);
			INIT_DELAYED_WORK(&dock_check_work, dock_mode_switch);
			//schedule_delayed_work(&dock_check_work, msecs_to_jiffies(25000));
		}
	} else {
		LOG_UART_DOCK(LOG_DEBUG,"[MID DOCKING] can't find compatible node1\n");
	}
	
	if (enable_irq_wake(dock_irqnr) < 0)
		LOG_UART_DOCK(LOG_DEBUG,"irq %d enable irq wake fail\n", dock_irqnr);

	
	#ifdef MID_SERIO_CONTROL
	ret = serio_register_driver(&mid_serio_drv);
	LOG_UART_DOCK(LOG_DEBUG,"serio serio_register_driver register-ret:%d\n",ret);
	#endif

	mid_uart_wakelock = wakeup_source_register(&pdev->dev,"mid_uart_wakelock");//mid add for suspend ,need have a wakelock to make sure resume func ok.

	ret = mid_input_init();
    LOG_UART_DOCK(LOG_DEBUG,"mid_input mid_input_init register-ret:%d\n",ret);
	ret = mid_mouse_init();
    LOG_UART_DOCK(LOG_DEBUG,"mid_input mid_mouse_init register-ret:%d\n",ret);

	return 0;
}

static int mid_uart_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}
static int mid_uart_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver mid_docking_driver = {
	.probe = mid_docking_probe,
	.driver = {
		   .name = "mid_docking",
		   .owner = THIS_MODULE,
		   .of_match_table = mid_match_table,
		   },
		   
	.suspend = mid_uart_suspend,
	.resume = mid_uart_resume,	   
};
static int __init mid_init(void)

{
	int ret = 0;

	ret = platform_driver_register(&mid_docking_driver);
    LOG_UART_DOCK(LOG_DEBUG,"[MID_DOCKING] mid_docking_init  ret:%d !!!! \n");	
	if (ret < 0)
		LOG_UART_DOCK(LOG_ERR,"unable to register mid_docking driver.\n");
	LOG_UART_DOCK(LOG_DEBUG," enter\n");

	return 0;

}

static void __exit  mid_exit(void)

{

#ifdef MID_SERIO_CONTROL
	serio_unregister_driver(&mid_serio_drv);
#endif	
	return;

}

module_init(mid_init);

module_exit(mid_exit);
//module_serio_driver(mid_serio_drv);

MODULE_AUTHOR("kuangzenghui@szroco.com");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
