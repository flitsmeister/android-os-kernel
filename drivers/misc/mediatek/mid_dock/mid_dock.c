#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
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
#endif
#include <linux/interrupt.h>

static struct delayed_work dock_check_work;

unsigned int GPIO_DOCKING_EN;
unsigned int GPIO_DOCKING_DET;
unsigned int GPIO_TYPE_C_DET;

unsigned int dock_irqnr;

#define TAG "MID_DOCKING"
#define LOG(fmt, args...)    pr_err(TAG": %s %d : "fmt, __FUNCTION__, __LINE__, ##args)

static void get_dts_info(void)
{
	struct device_node *node= NULL;
	int ret = 0;
	node = of_find_compatible_node(NULL, NULL, "mediatek, mid_docking");
	if(!node){
		LOG("of_find_compatible_node of mediatek, mid_docking\n");
		return ;
	}

	ret = of_get_named_gpio(node, "gpio_docking_en", 0);
    LOG("key=gpio_docking_en, ret=%d\n", ret);
    if(ret >= 0) {
    	GPIO_DOCKING_EN = (unsigned int )ret;
		gpio_request(GPIO_DOCKING_EN, "GPIO_DOCKING_EN");
    }
	ret = of_get_named_gpio(node, "gpio_docking_det", 0);
    LOG("key=gpio_docking_det, ret=%d\n", ret);
    if(ret >= 0) {
    	GPIO_DOCKING_DET = (unsigned int )ret;
		gpio_request(GPIO_DOCKING_DET, "GPIO_DOCKING_DET");
		gpio_direction_input(GPIO_DOCKING_DET);
    }
	ret = of_get_named_gpio(node, "gpio_type_c_det", 0);
    LOG("key=gpio_type_c_det, ret=%d\n", ret);
    if(ret >= 0) {
    	GPIO_TYPE_C_DET = (unsigned int )ret;
		gpio_request(GPIO_TYPE_C_DET, "GPIO_TYPE_C_DET");
		gpio_direction_input(GPIO_TYPE_C_DET);
    }

}

int get_docking_status(void){
	return gpio_get_value(GPIO_DOCKING_DET);
}
EXPORT_SYMBOL_GPL(get_docking_status);

int get_typec_status(void){
	return gpio_get_value(GPIO_TYPE_C_DET);
}
EXPORT_SYMBOL_GPL(get_typec_status);

static irqreturn_t dock_eint_interrupt_handler(void)
{
	if(get_docking_status() == 1){
		gpio_direction_output(GPIO_DOCKING_EN, 1);
	    gpio_set_value(GPIO_DOCKING_EN, 1);
		printk("MID_docking 1111 eint trigger!!!!\n");
	}else{
		gpio_direction_output(GPIO_DOCKING_EN, 0);
		gpio_set_value(GPIO_DOCKING_EN, 0);
		printk("MID_docking 000000 eint trigger!!!!\n");
	}
	 return IRQ_HANDLED;
}

static void dock_mode_switch(struct work_struct *work)
{
	printk("dock_mode_switch !!!!!!!!!");
	dock_eint_interrupt_handler();
}

static const struct of_device_id mid_match_table[] = {
	{.compatible = "mediatek, mid_docking",},
	{}
};
MODULE_DEVICE_TABLE(of, mid_match_table);

static int mid_docking_probe(struct platform_device *pdev)
{

	struct device_node *node1 = NULL;
	int ret = 0;

	node1 = of_find_matching_node(node1, mid_match_table);
	if (node1) {
		dock_irqnr = irq_of_parse_and_map(node1, 0);
		ret = request_irq(dock_irqnr, (irq_handler_t)dock_eint_interrupt_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "dock-eint", NULL);
		if (ret != 0) {
			printk("[MID DOCKING]EINT IRQ LINE NOT AVAILABLE\n");
		} else {
			printk("[MID DOCKING]set EINT finished %s, %d, dock_irqnr=%d\n",__func__, __LINE__, dock_irqnr);
			INIT_DELAYED_WORK(&dock_check_work, dock_mode_switch);
			schedule_delayed_work(&dock_check_work, msecs_to_jiffies(500));
		}
	} else {
		printk("[MID DOCKING]%s can't find compatible node1\n", __func__);
	}

	get_dts_info();
    printk("MID_DOCKING: get_dts_info\n");
	return 0;
}

static struct platform_driver mid_docking_driver = {
	.probe = mid_docking_probe,
	.driver = {
		   .name = "mid_docking",
		   .owner = THIS_MODULE,
		   .of_match_table = mid_match_table,
		   }
};

static int __init mid_docking_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&mid_docking_driver);
    printk("[MID_DOCKING] mid_docking_init  ret:%d !!!! \n");	
	if (ret < 0)
		pr_err("unable to register mid_docking driver.\n");

	return 0;
}

static void __exit mid_docking_exit(void)
{
	platform_driver_unregister(&mid_docking_driver);
	pr_debug("USB: Unregister this driver done\n");
}

late_initcall(mid_docking_init);
module_exit(mid_docking_exit);
MODULE_AUTHOR("huangxueke");
MODULE_DESCRIPTION("MediaTek MID_docking driver");
MODULE_LICENSE("GPL");
