/* SPDX-License-Identifier: GPL-2.0 */	
/*	
 * Copyright (c) 2019 MediaTek Inc.	
*/
#ifndef GT9XX_LIST_H
#define GT9XX_LIST_H

#define CFG_GROUP_LEN(p_cfg_grp) (ARRAY_SIZE(p_cfg_grp) / sizeof(p_cfg_grp[0]))

#include "gt9xx_mid.h"
#include "US828_GT9110_MS1183.h"
#include "US828_GT911_MC070046.h"
#include "PF123UX_GT911_CJF0502.h"
struct mid_cfg_data project_cfg_data[] = {
	{"US828_GT9110_MS1183", &US828_GT9110_MS1183_CFG},
	{"US828_GT911_MC070046", &US828_GT911_MC070046_CFG},
	{"PF123UX_GT911_CJF0502", &PF123UX_GT911_CJF0502_CFG},
};

#endif
