#
# Makefile for the Novatek NT36523_SPI touchscreen driver.
#

# Each configuration option enables a list of files.

# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
obj-$(CONFIG_TOUCHSCREEN_MTK_NT36523_SPI) += nt36xxx.o nt36xxx_fw_update.o nt36xxx_ext_proc.o nt36xxx_mp_ctrlram.o
