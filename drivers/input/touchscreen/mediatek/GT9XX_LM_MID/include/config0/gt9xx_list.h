#ifndef GT9XX_LIST_H
#define GT9XX_LIST_H

//typedef unsigned char u8;
#define CFG_GROUP_LEN(p_cfg_grp)  (ARRAY_SIZE(p_cfg_grp) / sizeof(p_cfg_grp[0]))

#include "gt9xx_mid.h"

#include "PF196_GT9271_SF_12316.h"
struct mid_cfg_data project_cfg_data[] = {
	{"PF196_GT9271_SF_12316", &PF196_GT9271_SF_12316_CFG},
};

#endif
