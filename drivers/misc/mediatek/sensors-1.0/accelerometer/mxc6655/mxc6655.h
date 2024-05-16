/*
 * Copyright (C) 2010 MEMSIC, Inc.
 *
 * Initial Code:
 *	Aaron Peng
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/*
 * Definitions for MXC6655 accelorometer sensor chip.
 */
#ifndef __MXC6655_H__
#define __MXC6655_H__


#include	<linux/ioctl.h>	/* For IOCTL macros */



/************************************************/
/* 	Accelerometer defines section	 	*/
/************************************************/
#define MXC6655_DEV_NAME		"mxc6655x"

#define MXC6655_I2C_ADDR		0x15

#define MXC6655_REG_CHIP_ID          0x0E

/* MXC6655 register address */
#define MXC6655_REG_X			0x03
#define MXC6655_REG_Y			0x05
#define MXC6655_REG_Z			0x07
#define MXC6655_REG_TEMP		0x09
#define MXC6655_REG_CTRL		0x0D

#define MXC6655_ID_1			0x02
#define MXC6655_ID_2			0x03


/*para setting*/
#define MXC6655_AWAKE			0x40	/* power on */
#define MXC6655_SLEEP			0x01	/* power donw */

#define MXC6655_BW_50HZ			0x00
#define MXC6655_RANGE_2G		0x00
#define MXC6655_RANGE_4G		0x20
#define MXC6655_RANGE_8G		0x40


/*ERR code*/
#define MXC6655_SUCCESS			0
#define MXC6655_ERR_I2C			-1
#define MXC6655_ERR_STATUS		-3
#define MXC6655_ERR_SETUP_FAILURE	-4
#define MXC6655_ERR_GETGSENSORDATA	-5
#define MXC6655_ERR_IDENTIFICATION	-6

#define MXC6655_BUFSIZE			256
#define MXC6655_STABLE_DELAY	10


#endif /* __MXC6655_H__ */

