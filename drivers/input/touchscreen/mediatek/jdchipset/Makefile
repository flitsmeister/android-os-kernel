# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -DCONFIG_TOUCHSCREEN_JADARD_DEBUG=y
ccflags-y += -DCONFIG_TOUCHSCREEN_JADARD_SORTING=y

# Makefile for the Jadard touchscreen drivers.
obj-$(CONFIG_TOUCHSCREEN_JDCHIPSET)	+= jadard_module.o
obj-$(CONFIG_TOUCHSCREEN_JDCHIPSET)	+= jadard_ic_JD9366T.o
obj-$(CONFIG_TOUCHSCREEN_JDCHIPSET)	+= jadard_common.o jadard_platform.o
obj-$(CONFIG_TOUCHSCREEN_JDCHIPSET)	+= jadard_debug.o
obj-$(CONFIG_TOUCHSCREEN_JDCHIPSET)	+= jadard_sorting.o