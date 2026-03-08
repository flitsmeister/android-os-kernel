ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/sitronix_ts/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/

sitronix-ts-objs	= sitronix_ts.o sitronix_ts_i2c.o sitronix_ts_spi.o sitronix_ts_utility.o sitronix_ts_mt.o sitronix_ts_nodes.o sitronix_ts_upgrade.o sitronix_ts_test.o sitronix_ts_proximity.o
obj-$(CONFIG_TOUCHSCREEN_SITRONIX_TOUCH)+= sitronix-ts.o

