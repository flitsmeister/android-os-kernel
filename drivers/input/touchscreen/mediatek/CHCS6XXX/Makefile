#semitouch touchpanel driver

ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/

obj-y	    += tlsc6x_driver.o

tlsc6x_driver-objs += tlsc6x_comp.o
tlsc6x_driver-objs += tlsc6x_main.o
