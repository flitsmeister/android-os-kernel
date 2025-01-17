/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
*/

#include <linux/types.h>
#ifndef _HL7005_SW_H_
#define _HL7005_SW_H_

#define HL7005_CON0		0x00
#define HL7005_CON1		0x01
#define HL7005_CON2		0x02
#define HL7005_CON3		0x03
#define HL7005_CON4		0x04
#define HL7005_CON5		0x05
#define HL7005_CON6		0x06
#define HL7005_REG_NUM		7

/* CON0 */
#define CON0_TMR_RST_MASK	0x01
#define CON0_TMR_RST_SHIFT	7
#define CON0_OTG_MASK		0x01
#define CON0_OTG_SHIFT		7
#define CON0_EN_STAT_MASK	0x01
#define CON0_EN_STAT_SHIFT	6
#define CON0_STAT_MASK		0x03
#define CON0_STAT_SHIFT		4
#define CON0_BOOST_MASK		0x01
#define CON0_BOOST_SHIFT	3
#define CON0_FAULT_MASK		0x07
#define CON0_FAULT_SHIFT	0

/* CON1 */
#define CON1_LIN_LIMIT_MASK	0x03
#define CON1_LIN_LIMIT_SHIFT	6
#define CON1_LOW_V_MASK		0x03
#define CON1_LOW_V_SHIFT	4
#define CON1_TE_MASK		0x01
#define CON1_TE_SHIFT		3
#define CON1_CE_MASK		0x01
#define CON1_CE_SHIFT		2
#define CON1_HZ_MODE_MASK	0x01
#define CON1_HZ_MODE_SHIFT	1
#define CON1_OPA_MODE_MASK	0x01
#define CON1_OPA_MODE_SHIFT	0

/* CON2 */
#define CON2_OREG_MASK		0x3F
#define CON2_OREG_SHIFT		2
#define CON2_OTG_PL_MASK	0x01
#define CON2_OTG_PL_SHIFT	1
#define CON2_OTG_EN_MASK	0x01
#define CON2_OTG_EN_SHIFT	0

/* CON3 */
#define CON3_VENDER_CODE_MASK	0xFF
#define CON3_VENDER_CODE_SHIFT	0
//#define CON3_PIN_MASK		0x03
//#define CON3_PIN_SHIFT		3
//#define CON3_REVISION_MASK	0x07
//#define CON3_REVISION_SHIFT	0

/* CON4 */
#define CON4_RESET_MASK		0x01
#define CON4_RESET_SHIFT	7
#define CON4_I_CHR_MASK		0x0F
#define CON4_I_CHR_SHIFT	3
#define CON4_I_TERM_MASK	0x07
#define CON4_I_TERM_SHIFT	0

/* CON5 */
//#define CON5_DIS_VREG_MASK	0x01
//#define CON5_DIS_VREG_SHIFT	6
#define CON5_ICHG4_MASK	    	0x01     //+1600MA
#define CON5_ICHG4_SHIFT		7
#define CON5_ICHG3_MASK	    	0x01	 //+800MA
#define CON5_ICHG3_SHIFT		6
#define CON5_IO_LEVEL_MASK	0x01
#define CON5_IO_LEVEL_SHIFT	5
//#define CON5_SP_STATUS_MASK	0x01
//#define CON5_SP_STATUS_SHIFT	4
//#define CON5_EN_LEVEL_MASK	0x01
//#define CON5_EN_LEVEL_SHIFT	3
//#define CON5_VSP_MASK		0x07
//#define CON5_VSP_SHIFT		0

/* CON6 */
#define CON6_ISAFE_MASK		0x07
#define CON6_ISAFE_SHIFT	4
#define CON6_VSAFE_MASK		0x0F
#define CON6_VSAFE_SHIFT	0

#endif
