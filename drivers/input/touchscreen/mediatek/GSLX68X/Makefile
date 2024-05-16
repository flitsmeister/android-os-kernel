
# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/

ifeq ($(CONFIG_MID_ITEMS_SUPPORT),y)
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/GSLX68X/include/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/GSLX68X/mid/
endif

obj-y +=  gsl_point_id_point_active_pen_no_gesture.o
obj-y	+=  mtk_gslX680.o

obj-$(CONFIG_MID_ITEMS_SUPPORT) 	+= 	gslx680_config.o