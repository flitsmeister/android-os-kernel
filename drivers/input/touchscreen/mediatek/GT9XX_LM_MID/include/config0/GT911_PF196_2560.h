/* ***************************PART2:TODO define********************************** */
/* STEP_1(REQUIRED):Change config table. */
u8 GT911_PF196_2560_GROUP0[] = {
0x5F,0x00,0x0A,0x40,0x06,0x0A,0x4D,0x00,0x01,0x08,0x28,0x05,0x50,
0x32,0x03,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x8C,0x2E,0x0E,0x17,0x15,0x31,0x0D,0x00,0x00,0x02,0x9A,0x04,
0x1C,0x00,0x00,0x00,0x00,0x00,0x03,0x64,0x32,0x00,0x00,0x00,0x0F,
0x41,0x94,0xC5,0x02,0x07,0x00,0x00,0x04,0x9C,0x11,0x00,0x76,0x17,
0x00,0x5A,0x1F,0x00,0x45,0x2A,0x00,0x37,0x38,0x00,0x37,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1C,0x1A,0x18,0x16,0x14,
0x12,0x10,0x0E,0x0C,0x0A,0x08,0x06,0x04,0x02,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x02,0x04,0x06,0x08,0x0A,0x0C,0x0F,0x10,0x12,0x13,0x14,0x16,0x18,
0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x24,0x26,0x28,0x29,0x2A,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x0B,0x01};

/* TODO puts your group2 config info here,if need. */
u8 GT911_PF196_2560_GROUP1[] = { 0 };

/* TODO puts your group3 config info here,if need. */
u8 GT911_PF196_2560_GROUP2[] = { 0 };

/* TODO: define your config for Sensor_ID == 3 here, if needed */
u8 GT911_PF196_2560_GROUP3[] = { 0 };

/* TODO: define your config for Sensor_ID == 4 here, if needed */
u8 GT911_PF196_2560_GROUP4[] = { 0 };

/* TODO: define your config for Sensor_ID == 5 here, if needed */
u8 GT911_PF196_2560_GROUP5[] = { 0 };

struct ctp_cfg GT911_PF196_2560_CFG = {
	.lens = {CFG_GROUP_LEN(GT911_PF196_2560_GROUP0),
		 CFG_GROUP_LEN(GT911_PF196_2560_GROUP1),
		 CFG_GROUP_LEN(GT911_PF196_2560_GROUP2),
		 CFG_GROUP_LEN(GT911_PF196_2560_GROUP3),
		 CFG_GROUP_LEN(GT911_PF196_2560_GROUP4),
		 CFG_GROUP_LEN(GT911_PF196_2560_GROUP5)},
	.info = {GT911_PF196_2560_GROUP0,
		 GT911_PF196_2560_GROUP1,
		 GT911_PF196_2560_GROUP2,
		 GT911_PF196_2560_GROUP3,
		 GT911_PF196_2560_GROUP4,
		 GT911_PF196_2560_GROUP5}
};
