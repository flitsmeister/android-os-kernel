// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 */
 
#ifndef FSA4480_I2C_H
#define FSA4480_I2C_H

#include <linux/of.h>
#include <linux/notifier.h>

enum fsa_function{
	FSA_MIC_GND_SWAP,
	FSA_USBC_ORIENTATION_CC1,
	FSA_USBC_ORIENTATION_CC2,
	FSA_USBC_DISPLAYPORT_DISCONNECTED,
	FSA_EVENT_MAX
};


#endif /* FSA4480_I2C_H */