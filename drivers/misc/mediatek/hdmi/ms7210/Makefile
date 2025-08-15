#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hdmi/inc\
					-I$(srctree)/drivers/misc/mediatek/ext_disp/$(CONFIG_MTK_PLATFORM)\
					-I$(srctree)/drivers/misc/mediatek/lcm/inc\
					-I$(srctree)/drivers/misc/mediatek/video/include\
					-I$(srctree)/drivers/misc/mediatek/include/mt-plat\
					-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)/dispsys\
					-I$(srctree)/drivers/misc/mediatek/video/$(CONFIG_MTK_PLATFORM)\
					-I$(srctree)/drivers/misc/mediatek/dispsys/$(CONFIG_MTK_PLATFORM)

#ifeq ($(CONFIG_CUSTOM_KERNEL_HDMI), "MS7210")
ccflags-y += 	-DHDMI_MS7210_SUPPORT
ifeq ($(CONFIG_HDMI_SET_SPEAKER_OFF), "y")
ccflags-y += -DCONFIG_HDMI_SET_SPEAKER_OFF
endif
#endif

obj-y	+= hdmi_drv.o ms7210.o ms7210_drv_dvin.o ms7210_drv_hdmi_tx.o ms7210_drv_misc.o ms7210_mpi.o
