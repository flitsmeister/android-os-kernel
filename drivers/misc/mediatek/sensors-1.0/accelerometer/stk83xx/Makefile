ccflags-y += -DSTK_MTK
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/accelerometer/inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/hwmon/include
obj-y   +=  stk.o
stk-src += stk83xx.c stk83xx_drv_i2c.c stk83xx_mtk_i2c.c common_i2c.c common_timer.c common_gpio_mtk.c common_storage.c stk_axes_sim.c
stk-objs := $(stk-src:.c=.o)
CFLAGS_STK832X.o += -DDEBUG
