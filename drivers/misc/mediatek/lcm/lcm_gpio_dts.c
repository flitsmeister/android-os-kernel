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

//8765-8768共用gpio解析
unsigned int GPIO_LCM_PWR_EN;
unsigned int GPIO_LCM_RST;
unsigned int GPIO_TP_RST;

#if 1//defined(CONFIG_MACH_MT6739)
unsigned int GPIO_LCM_STB;
unsigned int GPIO_LCM_VDD;//AD6=122  gpio_lcm_bias_en 
unsigned int GPIO_LCM_PWR;//8765 AE25=26  3.3
unsigned int GPIO_LCM_DSI_TE_EN;
unsigned int GPIO_LCM_1V8_EN;
unsigned int GPIO_LCM_3V3_EN;
#endif

unsigned int GPIO_LCM_BL_EN;
unsigned int GPIO_LCM_BIAS_EN;



unsigned int GPIO_LCM_3v3_EN;
unsigned int GPIO_LCM_1v8_EN;
unsigned int GPIO_OTG_EN;

unsigned int EDP_LCM_LED_EN1;
unsigned int GPIO_LCM_VLCM33_EN;
unsigned int GPIO_LCM_LT8911B_VDD18_EN;
unsigned int GPIO_LCM_GPIO5_IRQO2;

unsigned int LCM_DSI_TE;

unsigned int GPIO_CTP_TP_RST_EN; //tp reset
unsigned int GPIO_CTP_LDO; //TP_VDD1_8  TP_3_3V


#define TAG "MID"
#define LOG(fmt, args...)    pr_err(TAG": %s %d : "fmt, __FUNCTION__, __LINE__, ##args)

#define COMPATILE_NAME "mediatek,lcm"
static void get_dts_info(void)
{
	struct device_node *node1 = NULL;
	int ret = 0;
	node1 = of_find_compatible_node(NULL, NULL, COMPATILE_NAME);
	if(!node1){
		LOG("of_find_compatible_node of : %s\n", COMPATILE_NAME);
		return ;
	}
    ret = of_get_named_gpio(node1, "gpio_lcm_pwr_en", 0);
    LOG("key=gpio_lcm_pwr_en, err=%d\n", ret);
    if(ret >= 0) {
    	GPIO_LCM_PWR_EN = (unsigned int )ret;
		gpio_request(GPIO_LCM_PWR_EN, "GPIO_LCM_PWR_EN");
		GPIO_LCM_PWR = GPIO_LCM_PWR_EN;//add for 8765 compile success!
    }
	
	
	#if 1// defined(CONFIG_MACH_MT6739)//8765
	ret = of_get_named_gpio(node1, "lcm_vdd_gpio", 0);
    LOG("key=lcm_vdd_gpio, err=%d\n", ret);
    if(ret >= 0) {
    	GPIO_LCM_VDD = (unsigned int )ret;
		gpio_request(GPIO_LCM_VDD, "GPIO_LCM_VDD");
    }
	
	ret = of_get_named_gpio(node1, "gpio_lcm_1v8_en", 0);
    LOG("key=gpio_lcm_1v8_en, err=%d\n", ret);
    if(ret >= 0) {
    	GPIO_LCM_1V8_EN = (unsigned int )ret;
		gpio_request(GPIO_LCM_1V8_EN, "GPIO_LCM_1V8_EN");
    }
	
	ret = of_get_named_gpio(node1, "gpio_lcm_3v3_en", 0);
    LOG("key=gpio_lcm_3v3_en, err=%d\n", ret);
    if(ret >= 0) {
    	GPIO_LCM_3V3_EN = (unsigned int )ret;
		gpio_request(GPIO_LCM_3V3_EN, "GPIO_LCM_3V3_EN");
    }
	
	ret = of_get_named_gpio(node1, "gpio_lcm_dsi_te_en", 0);
    LOG("key=gpio_lcm_dsi_te_en, err=%d\n", ret);
    if(ret >= 0) {
    	GPIO_LCM_DSI_TE_EN = (unsigned int )ret;
		gpio_request(GPIO_LCM_DSI_TE_EN, "GPIO_LCM_DSI_TE_EN");
    }
	#endif

    ret = of_get_named_gpio(node1, "gpio_lcm_rst", 0);
    LOG("key=gpio_lcm_rst, err=%d\n", ret);
    if(ret >= 0) {
    	GPIO_LCM_RST = (unsigned int )ret;
		gpio_request(GPIO_LCM_RST, "GPIO_LCM_RST");
    }

	ret = of_get_named_gpio(node1, "gpio_tp_rst", 0);
    LOG("key=gpio_tp_rst, err=%d\n", ret);
    if(ret >= 0) {
		GPIO_TP_RST = (unsigned int )ret;
		gpio_request(GPIO_TP_RST, "GPIO_TP_RST");
    }

    ret = of_get_named_gpio(node1, "gpio_lcm_bl_en", 0);
    LOG("key=gpio_lcm_bl_en, err=%d\n", ret);
    if(ret >= 0) {
    	GPIO_LCM_BL_EN = (unsigned int )ret;
		gpio_request(GPIO_LCM_BL_EN, "GPIO_LCM_BL_EN");
    } 

        ret = of_get_named_gpio(node1, "gpio_lcm_bias_en", 0);
    LOG("key=gpio_lcm_bias_en, err=%d\n", ret);
    if(ret >= 0) {
        GPIO_LCM_BIAS_EN = (unsigned int )ret;
        gpio_request(GPIO_LCM_BIAS_EN, "GPIO_LCM_BIAS_EN");
    }

    ret = of_get_named_gpio(node1, "gpio_lcm_stb", 0);
    LOG("key=gpio_lcm_stb, err=%d\n", ret);
    if(ret >= 0) {
    	GPIO_LCM_STB = (unsigned int )ret;
		gpio_request(GPIO_LCM_STB, "GPIO_LCM_STB");
    }   


     ret = of_get_named_gpio(node1, "gpio_lcm_3v3_en", 0);
    LOG("LCM:key=gpio_lcm_3v3_en, err=%d\n", ret);
    if(ret >= 0) {
        GPIO_LCM_3v3_EN = (unsigned int )ret;
        gpio_request(GPIO_LCM_3v3_EN, "GPIO_LCM_3v3_EN");
    }

     ret = of_get_named_gpio(node1, "gpio_lcm_vlcm33_en", 0);
    LOG("LCM:key=gpio_lcm_1v8_en, ret=%d\n", ret);
    if(ret >= 0) {
        GPIO_LCM_1v8_EN = (unsigned int )ret;
        gpio_request(GPIO_LCM_1v8_EN, "GPIO_LCM_1v8_EN");
    } 
    ret = of_get_named_gpio(node1, "gpio_otg_en", 0);
    LOG("LCM:key=gpio_otg_en, ret=%d\n", ret);
    if(ret >= 0) {
        GPIO_OTG_EN = (unsigned int )ret;
        gpio_request(GPIO_OTG_EN, "GPIO_OTG_EN");
    }       


    ret = of_get_named_gpio(node1, "edp_lcm_led_en", 0);
    LOG("LCM:key=edp_lcm_led_en, ret=%d\n", ret);
    if(ret >= 0) {
        EDP_LCM_LED_EN1 = (unsigned int )ret;
        gpio_request(EDP_LCM_LED_EN1, "EDP_LCM_LED_EN1");
    } 
    ret = of_get_named_gpio(node1, "gpio_lcm_vlcm33_en", 0);
    LOG("LCM:key=gpio_lcm_vlcm33_en, ret=%d\n", ret);
    if(ret >= 0) {
        GPIO_LCM_VLCM33_EN = (unsigned int )ret;
        gpio_request(GPIO_LCM_VLCM33_EN, "GPIO_LCM_VLCM33_EN");
    } 

    ret = of_get_named_gpio(node1, "gpio_lcm_lt8911b_vdd18_en", 0);
    LOG("LCM:key=gpio_lcm_lt8911b_vdd18_en, ret=%d\n", ret);
    if(ret >= 0) {
        GPIO_LCM_LT8911B_VDD18_EN = (unsigned int )ret;
        gpio_request(GPIO_LCM_LT8911B_VDD18_EN, "GPIO_LCM_LT8911B_VDD18_EN");
    } 
    ret = of_get_named_gpio(node1, "gpio_lcm_gpio5_irqo2", 0);
    LOG("LCM:key=gpio_lcm_gpio5_irqo2, ret=%d\n", ret);
    if(ret >= 0) {
        GPIO_LCM_GPIO5_IRQO2 = (unsigned int )ret;
        gpio_request(GPIO_LCM_GPIO5_IRQO2, "GPIO_LCM_GPIO5_IRQO2");
    } 	

     ret = of_get_named_gpio(node1, "gpio_ctp_tp_rst_en", 0);
    LOG("LCM:key=gpio_ctp_tp_rst_en, ret=%d\n", ret);
    if(ret >= 0) {
        GPIO_CTP_TP_RST_EN = (unsigned int )ret;
        gpio_request(GPIO_CTP_TP_RST_EN, "GPIO_CTP_TP_RST_EN");
    } 
    ret = of_get_named_gpio(node1, "gpio_ctp_tp_power_en", 0);
    LOG("LCM:key=gpio_ctp_tp_power_en, ret=%d\n", ret);
    if(ret >= 0) {
        GPIO_CTP_LDO = (unsigned int )ret;
        gpio_request(GPIO_CTP_LDO, "GPIO_CTP_LDO");
    }

    ret = of_get_named_gpio(node1, "gpio_lcm_dsi_te", 0);
    LOG("LCM:key=gpio_lcm_dsi_te, ret=%d\n", ret);
    if(ret >= 0) {
        LCM_DSI_TE = (unsigned int )ret;
        gpio_request(LCM_DSI_TE, "LCM_DSI_TE");
    }  
}
static int __init lcm_gpio_dts_init(void)
{
	LOG("LCM: enter\n");
	get_dts_info();
	return 0;
}

static void __exit lcm_gpio_dts_exit(void)
{

	pr_debug("LCM: Unregister this driver done\n");
}

late_initcall(lcm_gpio_dts_init);
module_exit(lcm_gpio_dts_exit);
MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("LCM display subsystem driver");
MODULE_LICENSE("GPL");
