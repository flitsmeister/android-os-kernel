/* SPDX-License-Identifier: GPL-2.0 */	
/*	
 * Copyright (c) 2019 MediaTek Inc.	
*/
u8 PF196_GT9271_SF_12316_GROUP0[] = {
0x5A,0xB0,0x04,0x80,0x07,0x0A,0x75,0x00,0x11,0x0A,0x28,0x0F,0x5F,
0x4B,0x03,0x00,0x00,0x00,0x00,0x00,0x11,0x00,0x00,0x18,0x1A,0x1E,
0x14,0x90,0x10,0xAA,0x32,0x34,0x12,0x0C,0x00,0x00,0x00,0x3C,0x03,
0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x1E,
0x46,0x94,0xD5,0x02,0x04,0x00,0x00,0x04,0xA1,0x20,0x00,0x92,0x26,
0x00,0x86,0x2D,0x00,0x7B,0x36,0x00,0x73,0x40,0x00,0x73,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x19,0x18,0x17,0x16,0x15,
0x14,0x11,0x10,0x0F,0x0E,0x0D,0x0C,0x09,0x08,0x07,0x06,0x05,0x04,
0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2A,
0x29,0x28,0x27,0x26,0x25,0x24,0x23,0x22,0x21,0x20,0x1F,0x1E,0x1C,
0x1B,0x19,0x14,0x13,0x12,0x11,0x10,0x0F,0x0E,0x0D,0x0C,0x0A,0x08,
0x07,0x06,0x04,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x7F,0x01};

/* TODO puts your group2 config info here,if need. */
u8 PF196_GT9271_SF_12316_GROUP1[] = {0};

/* TODO puts your group3 config info here,if need. */
u8 PF196_GT9271_SF_12316_GROUP2[] = {0};

/* TODO: define your config for Sensor_ID == 3 here, if needed */
u8 PF196_GT9271_SF_12316_GROUP3[] = {0};

/* TODO: define your config for Sensor_ID == 4 here, if needed */
u8 PF196_GT9271_SF_12316_GROUP4[] = {0};

/* TODO: define your config for Sensor_ID == 5 here, if needed */
u8 PF196_GT9271_SF_12316_GROUP5[] = {0};

struct ctp_cfg PF196_GT9271_SF_12316_CFG = {
	.lens = {CFG_GROUP_LEN(PF196_GT9271_SF_12316_GROUP0),
		 CFG_GROUP_LEN(PF196_GT9271_SF_12316_GROUP1),
		 CFG_GROUP_LEN(PF196_GT9271_SF_12316_GROUP2),
		 CFG_GROUP_LEN(PF196_GT9271_SF_12316_GROUP3),
		 CFG_GROUP_LEN(PF196_GT9271_SF_12316_GROUP4),
		 CFG_GROUP_LEN(PF196_GT9271_SF_12316_GROUP5)},
	.info = {PF196_GT9271_SF_12316_GROUP0, PF196_GT9271_SF_12316_GROUP1,
		 PF196_GT9271_SF_12316_GROUP2, PF196_GT9271_SF_12316_GROUP3,
		 PF196_GT9271_SF_12316_GROUP4, PF196_GT9271_SF_12316_GROUP5} };
