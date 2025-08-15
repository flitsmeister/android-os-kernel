# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/TPD_IIC/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/jdchipset/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox
ccflags-y += -I$(srctree)/drivers/tinno/dev_info/

ccflags-y += -DCONFIG_TOUCHSCREEN_JADARD_DEBUG=y
ccflags-y += -DCONFIG_TOUCHSCREEN_JADARD_SORTING=y

# Makefile for the Jadard touchscreen drivers.
obj-y += jadard_module.o
obj-y += jadard_ic_JD9366T.o
obj-y += jadard_common.o jadard_platform.o
obj-y += jadard_debug.o
obj-y += jadard_sorting.o
