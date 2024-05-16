#
# Makefile for misc devices that really don't fit anywhere else.
#
ccflags-$(CONFIG_MTK_LCM) += -I$(srctree)/drivers/misc/mediatek/lcm/inc
LCM_SRC:= $(notdir $(wildcard $(srctree)/$(src)/*.c))
obj-y += $(patsubst %.c, %.o, ${LCM_SRC})