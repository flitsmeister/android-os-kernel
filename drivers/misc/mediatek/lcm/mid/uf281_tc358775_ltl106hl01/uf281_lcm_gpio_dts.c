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

unsigned int EDP_RESET;
unsigned int LCM_POWER_EN;
unsigned int EDP_POWER_EN;
unsigned int EDP_ENPSR;
unsigned int EDP_STANDBY;
unsigned int EDP_INT;
unsigned int VLED_EN;

#define TAG "MID"
#define LOG(fmt, args...)    pr_err(TAG": %s %d : "fmt, __FUNCTION__, __LINE__, ##args)

#define COMPATILE_NAME "mediatek,uf281_lcm"
static void uf281_get_dts_info(void)
{
	struct device_node *node1 = NULL;
	int ret = 0;
	node1 = of_find_compatible_node(NULL, NULL, COMPATILE_NAME);
	if(!node1){
		LOG("of_find_compatible_node of : %s\n", COMPATILE_NAME);
		return ;
	}
    
    ret = of_get_named_gpio(node1, "edp_reset", 0);
    LOG("key=edp_reset, err=%d\n", ret);
    if(ret >= 0) {
        EDP_RESET = (unsigned int )ret;
        gpio_request(EDP_RESET, "EDP_RESET");
    }

    ret = of_get_named_gpio(node1, "lcm_power_en", 0);
    LOG("key=lcm_power_en, err=%d\n", ret);
    if(ret >= 0) {
        LCM_POWER_EN = (unsigned int )ret;
        gpio_request(LCM_POWER_EN, "LCM_POWER_EN");
    } 

    ret = of_get_named_gpio(node1, "edp_power_en", 0);
    LOG("key=edp_power_en, err=%d\n", ret);
    if(ret >= 0) {
        EDP_POWER_EN = (unsigned int )ret;
        gpio_request(EDP_POWER_EN, "EDP_POWER_EN");
    } 

    ret = of_get_named_gpio(node1, "edp_enpsr", 0);
    LOG("key=edp_enpsr, err=%d\n", ret);
    if(ret >= 0) {
        EDP_ENPSR = (unsigned int )ret;
        gpio_request(EDP_ENPSR, "EDP_ENPSR");
    } 

    ret = of_get_named_gpio(node1, "edp_standby", 0);
    LOG("key=edp_standby, err=%d\n", ret);
    if(ret >= 0) {
        EDP_STANDBY = (unsigned int )ret;
        gpio_request(EDP_STANDBY, "EDP_STANDBY");
    } 

    //ret = of_get_named_gpio(node1, "EDP_INT", 0);
    //LOG("key=EDP_INT, err=%d\n", ret);
    //if(ret >= 0) {
    //    EDP_INT = (unsigned int )ret;
    //    gpio_request(EDP_INT, "EDP_INT");
    //} 
    
    ret = of_get_named_gpio(node1, "vled_en", 0);
    LOG("key=vled_en, err=%d\n", ret);
    if(ret >= 0) {
        VLED_EN = (unsigned int )ret;
        gpio_request(VLED_EN, "VLED_EN");
    }    	
}
static int __init uf281_lcm_gpio_dts_init(void)
{
	LOG("LCM: enter\n");
	uf281_get_dts_info();
	return 0;
}

static void __exit uf281_lcm_gpio_dts_exit(void)
{

	pr_debug("LCM: Unregister this driver done\n");
}

late_initcall(uf281_lcm_gpio_dts_init);
module_exit(uf281_lcm_gpio_dts_exit);
MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("LCM display subsystem driver");
MODULE_LICENSE("GPL");
