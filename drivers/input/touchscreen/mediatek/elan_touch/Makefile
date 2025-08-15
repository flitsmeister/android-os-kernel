#
# Makefile for the Novatek NT36523_SPI touchscreen driver.
#

# Each configuration option enables a list of files.

# Linux driver folder

ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/

obj-$(CONFIG_TOUCHSCREEN_ELAN) += elan_ts.o elan_update.o elan_tool.o

