//verify by kochen use CTC4.96 20250513
//#define ST_REPLACE_TEST_CMD_BY_DISPLAY_ID
#define ST_ADDRESS_MODE_WRITE_COMMAND_V2
#define WriteComm(cmd)		{(0x01), (cmd)}
#define WriteData(data)		{(0x02), (data)}
#define Delay_ms(time)		{(0x03), (time)}
#ifdef ST_REPLACE_TEST_CMD_BY_DISPLAY_ID
unsigned char test_id_1[3] = {0x80,0xA0,0xFB};
#endif /* ST_REPLACE_TEST_CMD_BY_DISPLAY_ID */

//#define ST_REQUEST_SELF_TEST_INI
#define SITRONIX_TEST_CMD_OPEN_MAX_LEN			2900	//2800
#define SITRONIX_TEST_make_CMD_OPEN_MAX_LEN			2800
#define SITRONIX_TEST_CMD_SHORT_ODD_MAX_LEN		2800
#define SITRONIX_TEST_CMD_SHORT_EVEN_MAX_LEN	2800
#define SITRONIX_TEST_CMD_UNIFORMITY_MAX_LEN	2800
#define SITRONIX_TEST_CMD_STD_MAX_LEN		2800
#define SITRONIX_TEST_CMD_NORMAL_MAX_LEN		2800
#define SITRONIX_TEST_CMD_OPEN_MA		2800
#define ST_SELFTEST_OPEN_MUX_ON_OFF_MIN		500			//for ST7121P/ST7123P Mux On/Off Open test.
#define ST_SELFTEST_LOG_FILE    1
#define ST_SELFTEST_LOG_PATH    "/sdcard/ST_SELFTEST_LOG.txt"

#define ST_SELFTEST_INI_PATH    "st_selftest_criteria.ini"

//#define ST_SELFTEST_LOG_PATH "/data/vendor/misc/touch/ST_SELFTEST_LOG.txt"

#define ST_SELFTEST_IGNORE_FRAME	3
#define ST_SELFTEST_ADJUST_COUNT	0
#define ST_SELFTEST_SKIP_COLS		3
#define ST_SELFTEST_SHORT_MAX		650//450
#define ST_SELFTEST_OPEN_MIN		1000//100//for delta
#define ST_SELFTEST_OPEN_MAX		8000//1200 //for delta not use max
#define ST_SELFTEST_NORMAL_MIN		-2000//100//for delta
#define ST_SELFTEST_NORMAL_MAX		4000//1200 //for delta not use max

#define ST_SELFTEST_UNIFORMITY_SHIFT	0
#define ST_SELFTEST_UNIFORMITY_MIN		80
#define ST_SELFTEST_UNIFORMITY_MAX		125

#define ST_SELFTEST_STD_FRAME_CNT		10
#define ST_SELFTEST_STD_MAX				60  //30	//3.0
//#define ST_SELFTEST_STD_SQUARE100_MAX	ST_SELFTEST_STD_MAX * ST_SELFTEST_STD_MAX
#define ST_SELFTEST_STD_CALCULATE_LIMIT 1000 //100 * 10

typedef struct sitronix_afe_cmd {
	uint8_t		type;
	uint32_t	value;
} sitronix_afe_cmd_t;

#define AFE_CMD_T	sitronix_afe_cmd_t
int sitronix_request_test_criteria(const char *name);
char skipNodeArray[40][40]={{0xFF,0xFF}};
#ifdef ST_ADDRESS_MODE_WRITE_COMMAND_V2

unsigned char golden_buf[] = {};
unsigned char *test_flash_afe_df	= NULL;	//No default value for ST7121P.
unsigned char *test_cmd_open		= NULL;	//Reserved for ST7123 open test.
AFE_CMD_T EnterSleepOut[]= {    
WriteComm (0x537123), //Addr
WriteData (0xa53c),
WriteComm (0x537123), //AFE Unlock 
WriteData (0x1455),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x7555),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x5555),
Delay_ms (100),
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1100),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (100),
//MCU Reset Keep L
};
AFE_CMD_T EnterAFEMode[]= {
    WriteComm (0x537123), //Addr
WriteData (0xa53c),
WriteComm (0x537123), //AFE Unlock 
WriteData (0x1455),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x7555),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x5555),
Delay_ms (100),
};
unsigned char test_disable_sensor[]= {};
AFE_CMD_T test_cmd_open_mux_on[]= {
WriteComm (0x537123), //Addr
WriteData (0xa53c),
WriteComm (0x537123), //AFE Unlock 
WriteData (0x1455),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x7555),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x5555),
Delay_ms (100),






//---Sleep In--------------
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1000),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (200),


WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1100),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (200),

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver Start/////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//MCU Reset Keep L

WriteComm (0x00F300),
WriteData (0x5AA5),

WriteComm (0x00F302),
WriteData (0x0001),


Delay_ms (200),



WriteComm (0x0101DC), 
WriteData (0x0000),
Delay_ms (10),
WriteComm (0x0101DC), 
WriteData (0xF3F3),
Delay_ms (10),


//Single Noise 7AA+2Noise
//TP80us NormalizeTrunc=10
//-----FRAM Write(Coef Base0)---3.4
WriteComm (0x00E000),
WriteData (0x0043),
WriteData (0x0050),
WriteData (0x005E),
WriteData (0x006C),
WriteData (0x007A),
WriteData (0x0089),
WriteData (0x0099),
WriteData (0x00A9),
WriteData (0x00BA),
WriteData (0x00CA),
WriteData (0x00DB),
WriteData (0x00EC),
WriteData (0x00FD),
WriteData (0x010E),
WriteData (0x011E),
WriteData (0x012E),
WriteData (0x013E),
WriteData (0x014E),
WriteData (0x015D),
WriteData (0x016B),
WriteData (0x0178),
WriteData (0x0185),
WriteData (0x0191),
WriteData (0x019B),
WriteData (0x01A5),
WriteData (0x01AE),
WriteData (0x01B6),
WriteData (0x01BC),
WriteData (0x01C1),
WriteData (0x01C5),
WriteData (0x01C8),
WriteData (0x01C9),
//-----FRAM Write(Coef Base1)---
WriteComm (0x00E040),//Beta=1.000
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Coef Base2)---
WriteComm (0x00E080),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Coef Base3)---
WriteComm (0x00E0C0),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Coef Base4)---
WriteComm (0x00E100),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
/*FRAM[0x0000], (NsUnit[00]: Touch, TxFreq=104.1K, Base=0) */
WriteComm (0x00E140),
WriteData (0x5488),
WriteData (0x6004),
WriteData (0x700C),
WriteData (0x2BD3),
WriteData (0x2618),
WriteData (0x0007),
WriteData (0x4D89),
WriteData (0x430B),
/*FRAM[0x0020], (NsUnit[01]: Touch, TxFreq=95.24K, Base=0) */
WriteComm (0x00E150),
WriteData (0x5488),
WriteData (0x690D),
WriteData (0x700B),
WriteData (0x3000),
WriteData (0x29B0),
WriteData (0x0007),
WriteData (0x4D89),
WriteData (0x430B),
/*FRAM[0x0040], (NsUnit[02]: Touch, TxFreq=84.75K, Base=0) */
WriteComm (0x00E160),
WriteData (0x5488),
WriteData (0x761A),
WriteData (0x7009),
WriteData (0x3B4B),
WriteData (0x3366),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x4F01),
/*FRAM[0x0060], (NsUnit[03]: Touch, TxFreq=74.07K, Base=0) */
WriteComm (0x00E170),
WriteData (0x5488),
WriteData (0x872B),
WriteData (0x7008),
WriteData (0x4333),
WriteData (0x3A2D),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6060),
/*FRAM[0x0080], (NsUnit[04]: Touch, TxFreq=66.7K, Base=0) */
WriteComm (0x00E180),
WriteData (0x5488),
WriteData (0x963A),
WriteData (0x7007),
WriteData (0x4D89),
WriteData (0x430B),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6060),
/*FRAM[0x00A0], (NsUnit[05]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E190),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x00C0], (NsUnit[06]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1A0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x00E0], (NsUnit[07]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1B0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0100], (NsUnit[08]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1C0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0120], (NsUnit[09]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1D0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0140], (NsUnit[10]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1E0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0160], (NsUnit[11]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1F0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0180], (NsUnit[12]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E200),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x01A0], (NsUnit[13]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E210),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
//-----FRAM Write(OFTV_MER)---
WriteComm (0x00E220),//Trim11111
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
//-----FRAM Write(OFTV_MUX0)---
WriteComm (0x00E24C),//Trim00000
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX1)---
WriteComm (0x00E278),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX2)---
WriteComm (0x00E2A4),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX3)---
WriteComm (0x00E2D0),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX4)---
WriteComm (0x00E2FC),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX5)---
WriteComm (0x00E328),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX6)---
WriteComm (0x00E354),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX7)---
WriteComm (0x00E380),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(RX_GAIN_TP)---
WriteComm (0x00E3AC),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
//-----FRAM Write(RX_GAIN_SELF)---
WriteComm (0x00E3EC),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
//----AFE Reg
WriteComm (0x00F004), //IRQ_SW
WriteData (0x0000),
WriteComm (0x00F016), //OFTV 
WriteData (0x0109),
WriteComm (0x00F024),//7AA+2N 
WriteData (0x72A0),
WriteComm (0x00F042), //ANA_CTRL
WriteData (0x4100),
WriteComm (0x00F044), //ANA_CTRL
WriteData (0x8011),
WriteComm (0x00F046), //ANA_CTRL
WriteData (0x0000),
WriteComm (0x00F04A), //ROW_CNT
WriteData (0x2000),
WriteComm (0x00F05C), //RAMAtable
WriteData (0x0001),
WriteData (0x0203),
WriteData (0x0405),
WriteData (0x060E), //N1
WriteData (0x1000), //N2
WriteData (0x0000), 
WriteComm (0x00F068), //RAMBtable
WriteData (0x0D0C),
WriteData (0x0B0A),
WriteData (0x0908),
WriteData (0x070F), //N1
WriteData (0x1100), //N2
WriteData (0x0000), 
WriteComm (0x00F122), //DR_FR_CNT 
WriteData (0x0001),
WriteComm (0x00F128), //TRGT_Set   
WriteData (0x0001),
WriteComm (0x00F156), //Active_LENGTH     
WriteData (0x5F5F),
WriteComm (0x00F158), //IDLE_SELF_LENGTH     
WriteData (0x5F5F),
WriteComm (0x00F174), //OFTV_STEP     
WriteData (0x007F),
//------TP_Pump_SEL----------
WriteComm (0x00F146), 
WriteData (0x0081),
WriteComm (0x00F148),                        
WriteData (0x013B),
WriteComm (0x00F160), 
WriteData (0x0060),
//--------SubSPI------------------
WriteComm (0x00F200),                        
WriteData (0x818C),
WriteComm (0x00F202),                        
WriteData (0x9000),
WriteComm (0x00F20C), //VAG_HZ_MODE=1
WriteData (0x6100),
WriteComm (0x00F20E), //ADC_CLK_RATIO
WriteData (0x2343),
WriteComm (0x00F212), //RX_CH_A/B
WriteData (0x00FF), //A31:16
WriteData (0xFFFF), //A15:0
WriteData (0x00FF), //B31:16
WriteData (0xFFFF), //B15:0
WriteComm (0x00F21A), 
WriteData (0x0300),
WriteComm (0x00F21C), //ADC_OFFSET
WriteData (0x1000),
WriteComm (0x00F230), //NOR_TRUNC_TP
WriteData (0x0A0A),
WriteComm (0x00F232), //NOR_TRUNC_IDL
WriteData (0x0A0A),
WriteComm (0x00F234), //Skip_Tx=1
WriteData (0xE80A),
WriteComm (0x00F236), //CFB=00
WriteData (0x4700),
WriteComm (0x00F23C), //MUXA
WriteData (0x0123),
WriteData (0x456F),
WriteData (0xFF0F),
WriteData (0xF0FF),
WriteComm (0x00F248), //MUXB
WriteData (0x0123),
WriteData (0x456F),
WriteData (0xFFF0),
WriteData (0x0FFF),
WriteComm (0x00F254), //MUX_DLY
WriteData (0x008C),
WriteComm (0x00F25C), 
WriteData (0x875C), 
WriteComm (0x00F25E), //RX_SENSING_DONE
WriteData (0x0108), 
WriteComm (0x00F262), //AFEON
WriteData (0x003F),
WriteComm (0x00F264), //COFTV_Merge=10
WriteData (0x0002),
WriteComm (0x00F266), //DB_OFTV_DLY
WriteData (0x0000),
WriteComm (0x00F26A), //RST_DUR
WriteData (0x0707),
//------Side Region need to set SUB_SPI------ 
WriteComm (0x00F312), //Write to Side Addr
WriteData (0x0200),
WriteComm (0x00F314), //Write Length 54word
WriteData (0x0138),
Delay_ms (10),
WriteComm (0x00F506), //OFTV_TRIM_START
WriteData (0x0000),
WriteComm (0x00F50A), //Normal freq
WriteData (0x01DD),
WriteComm (0x00F50C), //AA/Merge Freq
WriteData (0x0000),
//------Serial_Buff_WR------ 
WriteComm (0x00F51A), 
WriteData (0x0001),
WriteComm (0x00F51A), 
WriteData (0x0000),
Delay_ms (10),
//----TB_Sel MUXA_TB1,MUXB_TB2-----
WriteComm (0x00F090), 
WriteData (0x0000), // A A
WriteData (0x0055), // A B
WriteData (0x5555), // B B
//----TB1 Set MUA-----
WriteComm (0x00F096), 
WriteData (0x0001), //RX00 01
WriteData (0x0203), //RX02 03
WriteData (0x0405), //RX04 05
WriteData (0x0607), //RX06 07
WriteData (0x0809), //RX08 09
WriteData (0x0A0B), //RX10 11
WriteData (0x0C0D), //RX12 13
WriteData (0x0E0F), //RX14 15
WriteData (0x1011), //RX16 17
WriteData (0x1213), //RX18 19
WriteData (0x1415), //RX20 21
WriteData (0x1617), //RX22 23
WriteData (0x1819), //RX24 25
WriteData (0x1A1B), //RX26 27
WriteData (0x1C1D), //RX28 29
WriteData (0x1E1F), //RX30 31
//----TB2 Set MUXB------
WriteComm (0x00F0B6), 
WriteData (0x0001), //RX00 01
WriteData (0x0203), //RX02 03
WriteData (0x0405), //RX04 05
WriteData (0x0607), //RX06 07
WriteData (0x0809), //RX08 09
WriteData (0x0A0B), //RX10 11
WriteData (0x0C0D), //RX12 13
WriteData (0x0E0F), //RX14 15
WriteData (0x1011), //RX16 17
WriteData (0x1213), //RX18 19
WriteData (0x1415), //RX20 21
WriteData (0x1617), //RX22 23
WriteData (0x1819), //RX24 25
WriteData (0x1A1B), //RX26 27
WriteData (0x1C1D), //RX28 29
WriteData (0x1E1F), //RX30 31
WriteComm (0x00F000), 
WriteData (0x0002),
//-------Trim OFTV Flow-----------------
WriteComm (0x00F024), //7AA+1Self+0Noise
WriteData (0x74A0),
WriteComm (0x00F506), //OFTV Trim on 5frame
WriteData (0x0001),
//------Serial_Buff_WR------ 
WriteComm (0x00F51A), 
WriteData (0x0001),
WriteComm (0x00F51A), 
WriteData (0x0000),
Delay_ms (200),
WriteComm (0x00F024), //7AA+0Self+2Noise
WriteData (0x72A0),
//--------END Trim---------------------------------

//TRGT_ON_Flow
//WriteComm (0x00F000),//Sensing on 
//WriteData (0x0002),
//WriteComm (0x00F162), //TRGT Mode 
//WriteData (0x000C),
//WriteComm (0x00F020), //HD_DLY=FF
//WriteData (0x5410),
//------------Open_Test---------------
WriteComm (0x00F402),//OpenTest=1
WriteData (0x5100),
//------Serial_Buff_WR------ 
WriteComm (0x00F51A), 
WriteData (0x0001),
WriteComm (0x00F51A), 
WriteData (0x0000),
Delay_ms (10),
WriteComm (0x00F000), 
WriteData (0x0002),
Delay_ms (100),

};
AFE_CMD_T test_cmd_open_MuxOff[]= {
WriteComm (0x537123), //Addr
WriteData (0xa53c),
WriteComm (0x537123), //AFE Unlock 
WriteData (0x1455),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x7555),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x5555),
Delay_ms (100),






//---Sleep In--------------
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1000),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (200),


WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1100),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (200),

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver Start/////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//MCU Reset Keep L

WriteComm (0x00F300),
WriteData (0x5AA5),

WriteComm (0x00F302),
WriteData (0x0001),


Delay_ms (200),



WriteComm (0x0101DC), 
WriteData (0x0000),
Delay_ms (10),
WriteComm (0x0101DC), 
WriteData (0xF3F3),
Delay_ms (10),


//Single Noise 7AA+2Noise
//TP80us NormalizeTrunc=10
//-----FRAM Write(Coef Base0)---3.4
WriteComm (0x00E000),
WriteData (0x0043),
WriteData (0x0050),
WriteData (0x005E),
WriteData (0x006C),
WriteData (0x007A),
WriteData (0x0089),
WriteData (0x0099),
WriteData (0x00A9),
WriteData (0x00BA),
WriteData (0x00CA),
WriteData (0x00DB),
WriteData (0x00EC),
WriteData (0x00FD),
WriteData (0x010E),
WriteData (0x011E),
WriteData (0x012E),
WriteData (0x013E),
WriteData (0x014E),
WriteData (0x015D),
WriteData (0x016B),
WriteData (0x0178),
WriteData (0x0185),
WriteData (0x0191),
WriteData (0x019B),
WriteData (0x01A5),
WriteData (0x01AE),
WriteData (0x01B6),
WriteData (0x01BC),
WriteData (0x01C1),
WriteData (0x01C5),
WriteData (0x01C8),
WriteData (0x01C9),
//-----FRAM Write(Coef Base1)---
WriteComm (0x00E040),//Beta=1.000
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Coef Base2)---
WriteComm (0x00E080),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Coef Base3)---
WriteComm (0x00E0C0),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Coef Base4)---
WriteComm (0x00E100),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
/*FRAM[0x0000], (NsUnit[00]: Touch, TxFreq=104.1K, Base=0) */
WriteComm (0x00E140),
WriteData (0x5488),
WriteData (0x6004),
WriteData (0x700C),
WriteData (0x2BD3),
WriteData (0x2618),
WriteData (0x0007),
WriteData (0x4D89),
WriteData (0x430B),
/*FRAM[0x0020], (NsUnit[01]: Touch, TxFreq=95.24K, Base=0) */
WriteComm (0x00E150),
WriteData (0x5488),
WriteData (0x690D),
WriteData (0x700B),
WriteData (0x3000),
WriteData (0x29B0),
WriteData (0x0007),
WriteData (0x4D89),
WriteData (0x430B),
/*FRAM[0x0040], (NsUnit[02]: Touch, TxFreq=84.75K, Base=0) */
WriteComm (0x00E160),
WriteData (0x5488),
WriteData (0x761A),
WriteData (0x7009),
WriteData (0x3B4B),
WriteData (0x3366),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x4F01),
/*FRAM[0x0060], (NsUnit[03]: Touch, TxFreq=74.07K, Base=0) */
WriteComm (0x00E170),
WriteData (0x5488),
WriteData (0x872B),
WriteData (0x7008),
WriteData (0x4333),
WriteData (0x3A2D),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6060),
/*FRAM[0x0080], (NsUnit[04]: Touch, TxFreq=66.7K, Base=0) */
WriteComm (0x00E180),
WriteData (0x5488),
WriteData (0x963A),
WriteData (0x7007),
WriteData (0x4D89),
WriteData (0x430B),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6060),
/*FRAM[0x00A0], (NsUnit[05]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E190),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x00C0], (NsUnit[06]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1A0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x00E0], (NsUnit[07]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1B0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0100], (NsUnit[08]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1C0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0120], (NsUnit[09]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1D0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0140], (NsUnit[10]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1E0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0160], (NsUnit[11]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1F0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0180], (NsUnit[12]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E200),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x01A0], (NsUnit[13]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E210),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
//-----FRAM Write(OFTV_MER)---
WriteComm (0x00E220),//Trim11111
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
//-----FRAM Write(OFTV_MUX0)---
WriteComm (0x00E24C),//Trim00000
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX1)---
WriteComm (0x00E278),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX2)---
WriteComm (0x00E2A4),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX3)---
WriteComm (0x00E2D0),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX4)---
WriteComm (0x00E2FC),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX5)---
WriteComm (0x00E328),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX6)---
WriteComm (0x00E354),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX7)---
WriteComm (0x00E380),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(RX_GAIN_TP)---
WriteComm (0x00E3AC),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
//-----FRAM Write(RX_GAIN_SELF)---
WriteComm (0x00E3EC),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
//----AFE Reg
WriteComm (0x00F004), //IRQ_SW
WriteData (0x0000),
WriteComm (0x00F016), //OFTV 
WriteData (0x0109),
WriteComm (0x00F024),//7AA+2N 
WriteData (0x72A0),
WriteComm (0x00F042), //ANA_CTRL
WriteData (0x4100),
WriteComm (0x00F044), //ANA_CTRL
WriteData (0x8011),
WriteComm (0x00F046), //ANA_CTRL
WriteData (0x0000),
WriteComm (0x00F04A), //ROW_CNT
WriteData (0x2000),
WriteComm (0x00F05C), //RAMAtable
WriteData (0x0001),
WriteData (0x0203),
WriteData (0x0405),
WriteData (0x060E), //N1
WriteData (0x1000), //N2
WriteData (0x0000), 
WriteComm (0x00F068), //RAMBtable
WriteData (0x0D0C),
WriteData (0x0B0A),
WriteData (0x0908),
WriteData (0x070F), //N1
WriteData (0x1100), //N2
WriteData (0x0000), 
WriteComm (0x00F122), //DR_FR_CNT 
WriteData (0x0001),
WriteComm (0x00F128), //TRGT_Set   
WriteData (0x0001),
WriteComm (0x00F156), //Active_LENGTH     
WriteData (0x5F5F),
WriteComm (0x00F158), //IDLE_SELF_LENGTH     
WriteData (0x5F5F),
WriteComm (0x00F174), //OFTV_STEP     
WriteData (0x007F),
//------TP_Pump_SEL----------
WriteComm (0x00F146), 
WriteData (0x0081),
WriteComm (0x00F148),                        
WriteData (0x013B),
WriteComm (0x00F160), 
WriteData (0x0060),
//--------SubSPI------------------
WriteComm (0x00F200),                        
WriteData (0x818C),
WriteComm (0x00F202),                        
WriteData (0x9000),
WriteComm (0x00F20C), //VAG_HZ_MODE=1
WriteData (0x6100),
WriteComm (0x00F20E), //ADC_CLK_RATIO
WriteData (0x2343),
WriteComm (0x00F212), //RX_CH_A/B
WriteData (0x00FF), //A31:16
WriteData (0xFFFF), //A15:0
WriteData (0x00FF), //B31:16
WriteData (0xFFFF), //B15:0
WriteComm (0x00F21A), 
WriteData (0x0300),
WriteComm (0x00F21C), //ADC_OFFSET
WriteData (0x1000),
WriteComm (0x00F230), //NOR_TRUNC_TP
WriteData (0x0A0A),
WriteComm (0x00F232), //NOR_TRUNC_IDL
WriteData (0x0A0A),
WriteComm (0x00F234), //Skip_Tx=1
WriteData (0xE80A),
WriteComm (0x00F236), //CFB=00
WriteData (0x4700),
WriteComm (0x00F23C), //MUXA
WriteData (0x0123),
WriteData (0x456F),
WriteData (0xFF0F),
WriteData (0xF0FF),
WriteComm (0x00F248), //MUXB
WriteData (0x0123),
WriteData (0x456F),
WriteData (0xFFF0),
WriteData (0x0FFF),
WriteComm (0x00F254), //MUX_DLY
WriteData (0x008C),
WriteComm (0x00F25C), 
WriteData (0x875C), 
WriteComm (0x00F25E), //RX_SENSING_DONE
WriteData (0x0108), 
WriteComm (0x00F262), //AFEON
WriteData (0x003F),
WriteComm (0x00F264), //COFTV_Merge=10
WriteData (0x0002),
WriteComm (0x00F266), //DB_OFTV_DLY
WriteData (0x0000),
WriteComm (0x00F26A), //RST_DUR
WriteData (0x0707),
//------Side Region need to set SUB_SPI------ 
WriteComm (0x00F312), //Write to Side Addr
WriteData (0x0200),
WriteComm (0x00F314), //Write Length 54word
WriteData (0x0138),
Delay_ms (10),
WriteComm (0x00F506), //OFTV_TRIM_START
WriteData (0x0000),
WriteComm (0x00F50A), //Normal freq
WriteData (0x01DD),
WriteComm (0x00F50C), //AA/Merge Freq
WriteData (0x0000),
//------Serial_Buff_WR------ 
WriteComm (0x00F51A), 
WriteData (0x0001),
WriteComm (0x00F51A), 
WriteData (0x0000),
Delay_ms (10),
//----TB_Sel MUXA_TB1,MUXB_TB2-----
WriteComm (0x00F090), 
WriteData (0x0000), // A A
WriteData (0x0055), // A B
WriteData (0x5555), // B B
//----TB1 Set MUA-----
WriteComm (0x00F096), 
WriteData (0x0001), //RX00 01
WriteData (0x0203), //RX02 03
WriteData (0x0405), //RX04 05
WriteData (0x0607), //RX06 07
WriteData (0x0809), //RX08 09
WriteData (0x0A0B), //RX10 11
WriteData (0x0C0D), //RX12 13
WriteData (0x0E0F), //RX14 15
WriteData (0x1011), //RX16 17
WriteData (0x1213), //RX18 19
WriteData (0x1415), //RX20 21
WriteData (0x1617), //RX22 23
WriteData (0x1819), //RX24 25
WriteData (0x1A1B), //RX26 27
WriteData (0x1C1D), //RX28 29
WriteData (0x1E1F), //RX30 31
//----TB2 Set MUXB------
WriteComm (0x00F0B6), 
WriteData (0x0001), //RX00 01
WriteData (0x0203), //RX02 03
WriteData (0x0405), //RX04 05
WriteData (0x0607), //RX06 07
WriteData (0x0809), //RX08 09
WriteData (0x0A0B), //RX10 11
WriteData (0x0C0D), //RX12 13
WriteData (0x0E0F), //RX14 15
WriteData (0x1011), //RX16 17
WriteData (0x1213), //RX18 19
WriteData (0x1415), //RX20 21
WriteData (0x1617), //RX22 23
WriteData (0x1819), //RX24 25
WriteData (0x1A1B), //RX26 27
WriteData (0x1C1D), //RX28 29
WriteData (0x1E1F), //RX30 31
WriteComm (0x00F000), 
WriteData (0x0002),
//-------Trim OFTV Flow-----------------
WriteComm (0x00F024), //7AA+1Self+0Noise
WriteData (0x74A0),
WriteComm (0x00F506), //OFTV Trim on 5frame
WriteData (0x0001),
//------Serial_Buff_WR------ 
WriteComm (0x00F51A), 
WriteData (0x0001),
WriteComm (0x00F51A), 
WriteData (0x0000),
Delay_ms (200),
WriteComm (0x00F024), //7AA+0Self+2Noise
WriteData (0x72A0),
//--------END Trim---------------------------------

//TRGT_ON_Flow
//WriteComm (0x00F000),//Sensing on 
//WriteData (0x0002),
//WriteComm (0x00F162), //TRGT Mode 
//WriteData (0x000C),
//WriteComm (0x00F020), //HD_DLY=FF
//WriteData (0x5410),
//------------Open_Test---------------
WriteComm (0x00F402),//OpenTest=1
WriteData (0x5100),

//------Write Mux Off------ 
WriteComm (0x00F23C),//Sensing on 
WriteData (0xFFFF),
WriteComm (0x00F23E),//Sensing on 
WriteData (0xFFFF),
WriteComm (0x00F248),//Sensing on 
WriteData (0xFFFF),
WriteComm (0x00F24A),//Sensing on 
WriteData (0xFFFF),
//------Serial_Buff_WR------ 
WriteComm (0x00F51A), 
WriteData (0x0001),
WriteComm (0x00F51A), 
WriteData (0x0000),
Delay_ms (10),
WriteComm (0x00F000), 
WriteData (0x0002),
Delay_ms (100),
}; 
AFE_CMD_T test_cmd_short_odd[] = {
WriteComm (0x537123), //Addr
WriteData (0xa53c),
WriteComm (0x537123), //AFE Unlock 
WriteData (0x1455),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x7555),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x5555),
Delay_ms (100),



//---Sleep In--------------
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1000),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (100),


/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver Start/////////////////////////
/////////////////////////////////////////////////////////////////////////////////

WriteComm (0x00F300),
WriteData (0x5AA5),


//MCU Reset Keep L
WriteComm (0x00F302),
WriteData (0x0001),

Delay_ms (100),
WriteComm (0x0101DC), 
WriteData (0x0000),
Delay_ms (100),
WriteComm (0x0101DC), 
WriteData (0xF3F3),
Delay_ms (100),
/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver End/////////////////////////
/////////////////////////////////////////////////////////////////////////////////


//FRAM[0x0000], (NsUnit[00]: Touch, TxFreq=104.1K, Base=0) //
WriteComm (0x00E140),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0020], (NsUnit[01]: Touch, TxFreq=95.24K, Base=0) //
WriteComm (0x00E150),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0040], (NsUnit[02]: Touch, TxFreq=84.75K, Base=0) //
WriteComm (0x00E160),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0060], (NsUnit[03]: Touch, TxFreq=74.07K, Base=0) //
WriteComm (0x00E170),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0080], (NsUnit[04]: Touch, TxFreq=66.7K, Base=0) //
WriteComm (0x00E180),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x00A0], (NsUnit[05]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E190),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x00C0], (NsUnit[06]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E1A0),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x00E0], (NsUnit[07]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E1B0),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0100], (NsUnit[08]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E1C0),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0120], (NsUnit[09]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E1D0),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0140], (NsUnit[10]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E1E0),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0160], (NsUnit[11]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E1F0),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0180], (NsUnit[12]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E200),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x01A0], (NsUnit[13]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E210),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//-----FRAM Write(Coef Base0)---
WriteComm (0x00E000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0004),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(RX_GAIN_TP)---
WriteComm (0x00E3AC),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),

//----AFE Reg
WriteComm (0x00F004), //IRQ_SW
WriteData (0x0000),
WriteComm (0x00F024), //Glob_Hopping_Unit=9
WriteData (0x8280),
WriteComm (0x00F042), //ANA_CTRL
WriteData (0x4160),
WriteComm (0x00F044), //ANA_CTRL
WriteData (0x8011),
WriteComm (0x00F046), //ANA_CTRL
WriteData (0x0046),
WriteComm (0x00F04A),//ROW_CNT
WriteData (0x2000),
WriteComm (0x00F156), //Active_LENGTH     
WriteData (0xFFFF),
WriteComm (0x00F158), //IDLE_SELF_LENGTH     
WriteData (0xFFFF),
//------TP_Pump_SEL----------
WriteComm (0x00F146), 
WriteData (0x0081),
WriteComm (0x00F148),                        
WriteData (0x031A),
WriteComm (0x00F160), 
WriteData (0x0060),
//--------SubSPI------------------
WriteComm (0x00F200),                        
WriteData (0x9190),
WriteComm (0x00F202),                        
WriteData (0x9000),
WriteComm (0x00F206),//Mulit-Noise
WriteData (0x0108),
WriteComm (0x00F20C), //VAG_HZ_MODE=1
WriteData (0x6100),
WriteComm (0x00F21C), //ADC_OFFSET
WriteData (0x1000),
WriteComm (0x00F230),//NOR_TRUNC_TP
WriteData (0x0000),
WriteComm (0x00F234),
WriteData (0x2800),
WriteComm (0x00F236),
WriteData (0x8000),
WriteComm (0x00F254), //MUX_DLY
WriteData (0x0190),
WriteComm (0x00F25C), 
WriteData (0x875C), 
WriteComm (0x00F402),
WriteData (0x0000),
WriteComm (0x00F162), //TRGT Mode 
WriteData (0x0004),
WriteComm (0x00F020), //INT_HD_DLY
WriteData (0x5410),
//--------ShortTest_Col_table-----------
WriteComm (0x00F074), //MuxAtable
WriteData (0x0001),
WriteData (0x0203),
WriteData (0x0405),
WriteData (0x0607),
WriteData (0x0809),
WriteData (0x0A16), 
WriteComm (0x00F080), //MuxBtable
WriteData (0x0B0C),
WriteData (0x0D0E),
WriteData (0x0F10),
WriteData (0x1112),
WriteData (0x1314),
WriteData (0x1516),
//------------Short_Test---------------
WriteComm (0x00F20E),//ShortTest=1
WriteData (0x0323),
WriteComm (0x00F25E),//STOG_E=0,_O=1
WriteData (0x0118),
//WriteComm (0x00F25E),//STOG_E=1,_O=0
//WriteData (0x0128),
//------------Short_TestEnd------------
//------Serial_Buff_WR------ 
WriteComm (0x00F51A), 
WriteData (0x0001),
WriteComm (0x00F51A), 
WriteData (0x0000),
Delay_ms (100),
WriteComm (0x00F000), 
WriteData (0x0002),
Delay_ms (100),

};
AFE_CMD_T test_cmd_short_even[] = {
WriteComm (0x537123), //Addr
WriteData (0xa53c),
WriteComm (0x537123), //AFE Unlock 
WriteData (0x1455),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x7555),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x5555),
Delay_ms (100),



//---Sleep In--------------
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1000),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (100),


/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver Start/////////////////////////
/////////////////////////////////////////////////////////////////////////////////

WriteComm (0x00F300),
WriteData (0x5AA5),


//MCU Reset Keep L
WriteComm (0x00F302),
WriteData (0x0001),

Delay_ms (100),
WriteComm (0x0101DC), 
WriteData (0x0000),
Delay_ms (100),
WriteComm (0x0101DC), 
WriteData (0xF3F3),
Delay_ms (100),
/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver End/////////////////////////
/////////////////////////////////////////////////////////////////////////////////


//FRAM[0x0000], (NsUnit[00]: Touch, TxFreq=104.1K, Base=0) //
WriteComm (0x00E140),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0020], (NsUnit[01]: Touch, TxFreq=95.24K, Base=0) //
WriteComm (0x00E150),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0040], (NsUnit[02]: Touch, TxFreq=84.75K, Base=0) //
WriteComm (0x00E160),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0060], (NsUnit[03]: Touch, TxFreq=74.07K, Base=0) //
WriteComm (0x00E170),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0080], (NsUnit[04]: Touch, TxFreq=66.7K, Base=0) //
WriteComm (0x00E180),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x00A0], (NsUnit[05]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E190),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x00C0], (NsUnit[06]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E1A0),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x00E0], (NsUnit[07]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E1B0),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0100], (NsUnit[08]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E1C0),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0120], (NsUnit[09]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E1D0),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0140], (NsUnit[10]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E1E0),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0160], (NsUnit[11]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E1F0),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x0180], (NsUnit[12]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E200),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//FRAM[0x01A0], (NsUnit[13]: Touch, TxFreq=56.2K, Base=0) //
WriteComm (0x00E210),
WriteData (0x5408),
WriteData (0x0020),
WriteData (0x7007),
WriteData (0x0E00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4D89),
WriteData (0x430B),
//-----FRAM Write(Coef Base0)---
WriteComm (0x00E000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0004),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(RX_GAIN_TP)---
WriteComm (0x00E3AC),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),

//----AFE Reg
WriteComm (0x00F004), //IRQ_SW
WriteData (0x0000),
WriteComm (0x00F024), //Glob_Hopping_Unit=9
WriteData (0x8280),
WriteComm (0x00F042), //ANA_CTRL
WriteData (0x4160),
WriteComm (0x00F044), //ANA_CTRL
WriteData (0x8011),
WriteComm (0x00F046), //ANA_CTRL
WriteData (0x0046),
WriteComm (0x00F04A),//ROW_CNT
WriteData (0x2000),
WriteComm (0x00F156), //Active_LENGTH     
WriteData (0xFFFF),
WriteComm (0x00F158), //IDLE_SELF_LENGTH     
WriteData (0xFFFF),
//------TP_Pump_SEL----------
WriteComm (0x00F146), 
WriteData (0x0081),
WriteComm (0x00F148),                        
WriteData (0x031A),
WriteComm (0x00F160), 
WriteData (0x0060),
//--------SubSPI------------------
WriteComm (0x00F200),                        
WriteData (0x9190),
WriteComm (0x00F202),                        
WriteData (0x9000),
WriteComm (0x00F206),//Mulit-Noise
WriteData (0x0108),
WriteComm (0x00F20C), //VAG_HZ_MODE=1
WriteData (0x6100),
WriteComm (0x00F21C), //ADC_OFFSET
WriteData (0x1000),
WriteComm (0x00F230),//NOR_TRUNC_TP
WriteData (0x0000),
WriteComm (0x00F234),
WriteData (0x2800),
WriteComm (0x00F236),
WriteData (0x8000),
WriteComm (0x00F254), //MUX_DLY
WriteData (0x0190),
WriteComm (0x00F25C), 
WriteData (0x875C), 
WriteComm (0x00F402),
WriteData (0x0000),
WriteComm (0x00F162), //TRGT Mode 
WriteData (0x0004),
WriteComm (0x00F020), //INT_HD_DLY
WriteData (0x5410),
//--------ShortTest_Col_table-----------
WriteComm (0x00F074), //MuxAtable
WriteData (0x0001),
WriteData (0x0203),
WriteData (0x0405),
WriteData (0x0607),
WriteData (0x0809),
WriteData (0x0A16), 
WriteComm (0x00F080), //MuxBtable
WriteData (0x0B0C),
WriteData (0x0D0E),
WriteData (0x0F10),
WriteData (0x1112),
WriteData (0x1314),
WriteData (0x1516),
//------------Short_Test---------------
WriteComm (0x00F20E),//ShortTest=1
WriteData (0x0323),
//WriteComm (0x00F25E),//STOG_E=0,_O=1
//WriteData (0x0118),
WriteComm (0x00F25E),//STOG_E=1,_O=0
WriteData (0x0128),
//------------Short_TestEnd------------
//------Serial_Buff_WR------ 
WriteComm (0x00F51A), 
WriteData (0x0001),
WriteComm (0x00F51A), 
WriteData (0x0000),
Delay_ms (100),
WriteComm (0x00F000), 
WriteData (0x0002),
Delay_ms (100),

};    
AFE_CMD_T test_cmd_std[] = {
WriteComm (0x537123), //Addr
WriteData (0xa53c),
WriteComm (0x537123), //AFE Unlock 
WriteData (0x1455),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x7555),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x5555),
Delay_ms (100),





//---Sleep Out--------------
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1100),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (100),

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver Start/////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//MCU Reset Keep L

WriteComm (0x00F300),
WriteData (0x5AA5),

WriteComm (0x00F302),
WriteData (0x0001),


Delay_ms (100),



WriteComm (0x0101DC), 
WriteData (0x0000),
Delay_ms (10),
WriteComm (0x0101DC), 
WriteData (0xF3F3),
Delay_ms (10),



//Single Noise 7AA+2Noise
//TP80us NormalizeTrunc=10
//-----FRAM Write(Coef Base0)---3.4
WriteComm (0x00E000),
WriteData (0x0043),
WriteData (0x0050),
WriteData (0x005E),
WriteData (0x006C),
WriteData (0x007A),
WriteData (0x0089),
WriteData (0x0099),
WriteData (0x00A9),
WriteData (0x00BA),
WriteData (0x00CA),
WriteData (0x00DB),
WriteData (0x00EC),
WriteData (0x00FD),
WriteData (0x010E),
WriteData (0x011E),
WriteData (0x012E),
WriteData (0x013E),
WriteData (0x014E),
WriteData (0x015D),
WriteData (0x016B),
WriteData (0x0178),
WriteData (0x0185),
WriteData (0x0191),
WriteData (0x019B),
WriteData (0x01A5),
WriteData (0x01AE),
WriteData (0x01B6),
WriteData (0x01BC),
WriteData (0x01C1),
WriteData (0x01C5),
WriteData (0x01C8),
WriteData (0x01C9),
//-----FRAM Write(Coef Base1)---
WriteComm (0x00E040),//Beta=1.000
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Coef Base2)---
WriteComm (0x00E080),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Coef Base3)---
WriteComm (0x00E0C0),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Coef Base4)---
WriteComm (0x00E100),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
/*FRAM[0x0000], (NsUnit[00]: Touch, TxFreq=104.1K, Base=0) */
WriteComm (0x00E140),
WriteData (0x5488),
WriteData (0x6004),
WriteData (0x700C),
WriteData (0x2BD3),
WriteData (0x2618),
WriteData (0x0007),
WriteData (0x4D89),
WriteData (0x430B),
/*FRAM[0x0020], (NsUnit[01]: Touch, TxFreq=95.24K, Base=0) */
WriteComm (0x00E150),
WriteData (0x5488),
WriteData (0x690D),
WriteData (0x700B),
WriteData (0x3000),
WriteData (0x29B0),
WriteData (0x0007),
WriteData (0x4D89),
WriteData (0x430B),
/*FRAM[0x0040], (NsUnit[02]: Touch, TxFreq=84.75K, Base=0) */
WriteComm (0x00E160),
WriteData (0x5488),
WriteData (0x761A),
WriteData (0x7009),
WriteData (0x3B4B),
WriteData (0x3366),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x4F01),
/*FRAM[0x0060], (NsUnit[03]: Touch, TxFreq=74.07K, Base=0) */
WriteComm (0x00E170),
WriteData (0x5488),
WriteData (0x872B),
WriteData (0x7008),
WriteData (0x4333),
WriteData (0x3A2D),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6060),
/*FRAM[0x0080], (NsUnit[04]: Touch, TxFreq=66.7K, Base=0) */
WriteComm (0x00E180),
WriteData (0x5488),
WriteData (0x963A),
WriteData (0x7007),
WriteData (0x4D89),
WriteData (0x430B),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6060),
/*FRAM[0x00A0], (NsUnit[05]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E190),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x00C0], (NsUnit[06]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1A0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x00E0], (NsUnit[07]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1B0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0100], (NsUnit[08]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1C0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0120], (NsUnit[09]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1D0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0140], (NsUnit[10]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1E0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0160], (NsUnit[11]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1F0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0180], (NsUnit[12]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E200),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x01A0], (NsUnit[13]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E210),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
//-----FRAM Write(OFTV_MER)---
WriteComm (0x00E220),//Trim11111
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
//-----FRAM Write(OFTV_MUX0)---
WriteComm (0x00E24C),//Trim00000
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX1)---
WriteComm (0x00E278),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX2)---
WriteComm (0x00E2A4),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX3)---
WriteComm (0x00E2D0),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX4)---
WriteComm (0x00E2FC),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX5)---
WriteComm (0x00E328),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX6)---
WriteComm (0x00E354),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX7)---
WriteComm (0x00E380),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(RX_GAIN_TP)---
WriteComm (0x00E3AC),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
//-----FRAM Write(RX_GAIN_SELF)---
WriteComm (0x00E3EC),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
//----AFE Reg
WriteComm (0x00F004), //IRQ_SW
WriteData (0x0000),
WriteComm (0x00F016), //OFTV 
WriteData (0x0109),
WriteComm (0x00F024),//7AA+2N 
WriteData (0x72A0),
WriteComm (0x00F042), //ANA_CTRL
WriteData (0x4100),
WriteComm (0x00F044), //ANA_CTRL
WriteData (0x8011),
WriteComm (0x00F046), //ANA_CTRL
WriteData (0x0000),
WriteComm (0x00F04A), //ROW_CNT
WriteData (0x2000),
WriteComm (0x00F05C), //RAMAtable
WriteData (0x0001),
WriteData (0x0203),
WriteData (0x0405),
WriteData (0x060E), //N1
WriteData (0x1000), //N2
WriteData (0x0000), 
WriteComm (0x00F068), //RAMBtable
WriteData (0x0D0C),
WriteData (0x0B0A),
WriteData (0x0908),
WriteData (0x070F), //N1
WriteData (0x1100), //N2
WriteData (0x0000), 
WriteComm (0x00F122), //DR_FR_CNT 
WriteData (0x0001),
WriteComm (0x00F128), //TRGT_Set   
WriteData (0x0001),
WriteComm (0x00F156), //Active_LENGTH     
WriteData (0x5F5F),
WriteComm (0x00F158), //IDLE_SELF_LENGTH     
WriteData (0x5F5F),
WriteComm (0x00F174), //OFTV_STEP     
WriteData (0x007F),
//------TP_Pump_SEL----------
WriteComm (0x00F146), 
WriteData (0x0081),
WriteComm (0x00F148),                        
WriteData (0x013B),
WriteComm (0x00F160), 
WriteData (0x0060),
//--------SubSPI------------------
WriteComm (0x00F200),                        
WriteData (0x818C),
WriteComm (0x00F202),                        
WriteData (0x9000),
WriteComm (0x00F20C), //VAG_HZ_MODE=1
WriteData (0x6100),
WriteComm (0x00F20E), //ADC_CLK_RATIO
WriteData (0x2343),
WriteComm (0x00F212), //RX_CH_A/B
WriteData (0x00FF), //A31:16
WriteData (0xFFFF), //A15:0
WriteData (0x00FF), //B31:16
WriteData (0xFFFF), //B15:0
WriteComm (0x00F21A), 
WriteData (0x0300),
WriteComm (0x00F21C), //ADC_OFFSET
WriteData (0x1000),
WriteComm (0x00F230), //NOR_TRUNC_TP
WriteData (0x0A0A),
WriteComm (0x00F232), //NOR_TRUNC_IDL
WriteData (0x0A0A),
WriteComm (0x00F234), //Skip_Tx=1
WriteData (0xE80A),
WriteComm (0x00F236), //CFB=00
WriteData (0x4700),
WriteComm (0x00F23C), //MUXA
WriteData (0x0123),
WriteData (0x456F),
WriteData (0xFF0F),
WriteData (0xF0FF),
WriteComm (0x00F248), //MUXB
WriteData (0x0123),
WriteData (0x456F),
WriteData (0xFFF0),
WriteData (0x0FFF),
WriteComm (0x00F254), //MUX_DLY
WriteData (0x008C),
WriteComm (0x00F25C), 
WriteData (0x875C), 
WriteComm (0x00F25E), //RX_SENSING_DONE
WriteData (0x0108), 
WriteComm (0x00F262), //AFEON
WriteData (0x003F),
WriteComm (0x00F264), //COFTV_Merge=10
WriteData (0x0002),
WriteComm (0x00F266), //DB_OFTV_DLY
WriteData (0x0000),
WriteComm (0x00F26A), //RST_DUR
WriteData (0x0707),
//------Side Region need to set SUB_SPI------ 
WriteComm (0x00F312), //Write to Side Addr
WriteData (0x0200),
WriteComm (0x00F314), //Write Length 54word
WriteData (0x0138),
Delay_ms (10),
WriteComm (0x00F506), //OFTV_TRIM_START
WriteData (0x0000),
WriteComm (0x00F50A), //Normal freq
WriteData (0x01DD),
WriteComm (0x00F50C), //AA/Merge Freq
WriteData (0x0000),
//------Serial_Buff_WR------ 
WriteComm (0x00F51A), 
WriteData (0x0001),
WriteComm (0x00F51A), 
WriteData (0x0000),
Delay_ms (10),
//----TB_Sel MUXA_TB1,MUXB_TB2-----
WriteComm (0x00F090), 
WriteData (0x0000), // A A
WriteData (0x0055), // A B
WriteData (0x5555), // B B
//----TB1 Set MUA-----
WriteComm (0x00F096), 
WriteData (0x0001), //RX00 01
WriteData (0x0203), //RX02 03
WriteData (0x0405), //RX04 05
WriteData (0x0607), //RX06 07
WriteData (0x0809), //RX08 09
WriteData (0x0A0B), //RX10 11
WriteData (0x0C0D), //RX12 13
WriteData (0x0E0F), //RX14 15
WriteData (0x1011), //RX16 17
WriteData (0x1213), //RX18 19
WriteData (0x1415), //RX20 21
WriteData (0x1617), //RX22 23
WriteData (0x1819), //RX24 25
WriteData (0x1A1B), //RX26 27
WriteData (0x1C1D), //RX28 29
WriteData (0x1E1F), //RX30 31
//----TB2 Set MUXB------
WriteComm (0x00F0B6), 
WriteData (0x0001), //RX00 01
WriteData (0x0203), //RX02 03
WriteData (0x0405), //RX04 05
WriteData (0x0607), //RX06 07
WriteData (0x0809), //RX08 09
WriteData (0x0A0B), //RX10 11
WriteData (0x0C0D), //RX12 13
WriteData (0x0E0F), //RX14 15
WriteData (0x1011), //RX16 17
WriteData (0x1213), //RX18 19
WriteData (0x1415), //RX20 21
WriteData (0x1617), //RX22 23
WriteData (0x1819), //RX24 25
WriteData (0x1A1B), //RX26 27
WriteData (0x1C1D), //RX28 29
WriteData (0x1E1F), //RX30 31
WriteComm (0x00F000), 
WriteData (0x0002),
//-------Trim OFTV Flow-----------------
WriteComm (0x00F024), //7AA+1Self+0Noise
WriteData (0x74A0),
WriteComm (0x00F506), //OFTV Trim on 5frame
WriteData (0x0001),
//------Serial_Buff_WR------ 
WriteComm (0x00F51A), 
WriteData (0x0001),
WriteComm (0x00F51A), 
WriteData (0x0000),
Delay_ms (100),
WriteComm (0x00F024), //7AA+0Self+2Noise
WriteData (0x72A0),
//--------END Trim---------------------------------
WriteComm (0x00F000), 
WriteData (0x0002),


};
AFE_CMD_T test_cmd_normal_rawdata[] = {
WriteComm (0x537123), //Addr
WriteData (0xa53c),
WriteComm (0x537123), //AFE Unlock 
WriteData (0x1455),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x7555),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x5555),
Delay_ms (100),





//---Sleep Out--------------
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1100),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (100),

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver Start/////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//MCU Reset Keep L

WriteComm (0x00F300),
WriteData (0x5AA5),

WriteComm (0x00F302),
WriteData (0x0001),


Delay_ms (100),



WriteComm (0x0101DC), 
WriteData (0x0000),
Delay_ms (10),
WriteComm (0x0101DC), 
WriteData (0xF3F3),
Delay_ms (10),



//Single Noise 7AA+2Noise
//TP80us NormalizeTrunc=10
//-----FRAM Write(Coef Base0)---3.4
WriteComm (0x00E000),
WriteData (0x0043),
WriteData (0x0050),
WriteData (0x005E),
WriteData (0x006C),
WriteData (0x007A),
WriteData (0x0089),
WriteData (0x0099),
WriteData (0x00A9),
WriteData (0x00BA),
WriteData (0x00CA),
WriteData (0x00DB),
WriteData (0x00EC),
WriteData (0x00FD),
WriteData (0x010E),
WriteData (0x011E),
WriteData (0x012E),
WriteData (0x013E),
WriteData (0x014E),
WriteData (0x015D),
WriteData (0x016B),
WriteData (0x0178),
WriteData (0x0185),
WriteData (0x0191),
WriteData (0x019B),
WriteData (0x01A5),
WriteData (0x01AE),
WriteData (0x01B6),
WriteData (0x01BC),
WriteData (0x01C1),
WriteData (0x01C5),
WriteData (0x01C8),
WriteData (0x01C9),
//-----FRAM Write(Coef Base1)---
WriteComm (0x00E040),//Beta=1.000
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Coef Base2)---
WriteComm (0x00E080),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Coef Base3)---
WriteComm (0x00E0C0),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Coef Base4)---
WriteComm (0x00E100),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
/*FRAM[0x0000], (NsUnit[00]: Touch, TxFreq=104.1K, Base=0) */
WriteComm (0x00E140),
WriteData (0x5488),
WriteData (0x6004),
WriteData (0x700C),
WriteData (0x2BD3),
WriteData (0x2618),
WriteData (0x0007),
WriteData (0x4D89),
WriteData (0x430B),
/*FRAM[0x0020], (NsUnit[01]: Touch, TxFreq=95.24K, Base=0) */
WriteComm (0x00E150),
WriteData (0x5488),
WriteData (0x690D),
WriteData (0x700B),
WriteData (0x3000),
WriteData (0x29B0),
WriteData (0x0007),
WriteData (0x4D89),
WriteData (0x430B),
/*FRAM[0x0040], (NsUnit[02]: Touch, TxFreq=84.75K, Base=0) */
WriteComm (0x00E160),
WriteData (0x5488),
WriteData (0x761A),
WriteData (0x7009),
WriteData (0x3B4B),
WriteData (0x3366),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x4F01),
/*FRAM[0x0060], (NsUnit[03]: Touch, TxFreq=74.07K, Base=0) */
WriteComm (0x00E170),
WriteData (0x5488),
WriteData (0x872B),
WriteData (0x7008),
WriteData (0x4333),
WriteData (0x3A2D),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6060),
/*FRAM[0x0080], (NsUnit[04]: Touch, TxFreq=66.7K, Base=0) */
WriteComm (0x00E180),
WriteData (0x5488),
WriteData (0x963A),
WriteData (0x7007),
WriteData (0x4D89),
WriteData (0x430B),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6060),
/*FRAM[0x00A0], (NsUnit[05]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E190),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x00C0], (NsUnit[06]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1A0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x00E0], (NsUnit[07]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1B0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0100], (NsUnit[08]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1C0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0120], (NsUnit[09]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1D0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0140], (NsUnit[10]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1E0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0160], (NsUnit[11]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E1F0),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x0180], (NsUnit[12]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E200),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
/*FRAM[0x01A0], (NsUnit[13]: Touch, TxFreq=56.2K, Base=0) */
WriteComm (0x00E210),
WriteData (0x5488),
WriteData (0xB256),
WriteData (0x7006),
WriteData (0x5BA2),
WriteData (0x4F01),
WriteData (0x0004),
WriteData (0x9000),
WriteData (0x7B6B),
//-----FRAM Write(OFTV_MER)---
WriteComm (0x00E220),//Trim11111
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
WriteData (0x7FFF),
//-----FRAM Write(OFTV_MUX0)---
WriteComm (0x00E24C),//Trim00000
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX1)---
WriteComm (0x00E278),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX2)---
WriteComm (0x00E2A4),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX3)---
WriteComm (0x00E2D0),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX4)---
WriteComm (0x00E2FC),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX5)---
WriteComm (0x00E328),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX6)---
WriteComm (0x00E354),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV_MUX7)---
WriteComm (0x00E380),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(RX_GAIN_TP)---
WriteComm (0x00E3AC),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
//-----FRAM Write(RX_GAIN_SELF)---
WriteComm (0x00E3EC),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
WriteData (0x8080),
//----AFE Reg
WriteComm (0x00F004), //IRQ_SW
WriteData (0x0000),
WriteComm (0x00F016), //OFTV 
WriteData (0x0109),
WriteComm (0x00F024),//7AA+2N 
WriteData (0x72A0),
WriteComm (0x00F042), //ANA_CTRL
WriteData (0x4100),
WriteComm (0x00F044), //ANA_CTRL
WriteData (0x8011),
WriteComm (0x00F046), //ANA_CTRL
WriteData (0x0000),
WriteComm (0x00F04A), //ROW_CNT
WriteData (0x2000),
WriteComm (0x00F05C), //RAMAtable
WriteData (0x0001),
WriteData (0x0203),
WriteData (0x0405),
WriteData (0x060E), //N1
WriteData (0x1000), //N2
WriteData (0x0000), 
WriteComm (0x00F068), //RAMBtable
WriteData (0x0D0C),
WriteData (0x0B0A),
WriteData (0x0908),
WriteData (0x070F), //N1
WriteData (0x1100), //N2
WriteData (0x0000), 
WriteComm (0x00F122), //DR_FR_CNT 
WriteData (0x0001),
WriteComm (0x00F128), //TRGT_Set   
WriteData (0x0001),
WriteComm (0x00F156), //Active_LENGTH     
WriteData (0x5F5F),
WriteComm (0x00F158), //IDLE_SELF_LENGTH     
WriteData (0x5F5F),
WriteComm (0x00F174), //OFTV_STEP     
WriteData (0x007F),
//------TP_Pump_SEL----------
WriteComm (0x00F146), 
WriteData (0x0081),
WriteComm (0x00F148),                        
WriteData (0x013B),
WriteComm (0x00F160), 
WriteData (0x0060),
//--------SubSPI------------------
WriteComm (0x00F200),                        
WriteData (0x818C),
WriteComm (0x00F202),                        
WriteData (0x9000),
WriteComm (0x00F20C), //VAG_HZ_MODE=1
WriteData (0x6100),
WriteComm (0x00F20E), //ADC_CLK_RATIO
WriteData (0x2343),
WriteComm (0x00F212), //RX_CH_A/B
WriteData (0x00FF), //A31:16
WriteData (0xFFFF), //A15:0
WriteData (0x00FF), //B31:16
WriteData (0xFFFF), //B15:0
WriteComm (0x00F21A), 
WriteData (0x0300),
WriteComm (0x00F21C), //ADC_OFFSET
WriteData (0x1000),
WriteComm (0x00F230), //NOR_TRUNC_TP
WriteData (0x0A0A),
WriteComm (0x00F232), //NOR_TRUNC_IDL
WriteData (0x0A0A),
WriteComm (0x00F234), //Skip_Tx=1
WriteData (0xE80A),
WriteComm (0x00F236), //CFB=00
WriteData (0x4700),
WriteComm (0x00F23C), //MUXA
WriteData (0x0123),
WriteData (0x456F),
WriteData (0xFF0F),
WriteData (0xF0FF),
WriteComm (0x00F248), //MUXB
WriteData (0x0123),
WriteData (0x456F),
WriteData (0xFFF0),
WriteData (0x0FFF),
WriteComm (0x00F254), //MUX_DLY
WriteData (0x008C),
WriteComm (0x00F25C), 
WriteData (0x875C), 
WriteComm (0x00F25E), //RX_SENSING_DONE
WriteData (0x0108), 
WriteComm (0x00F262), //AFEON
WriteData (0x003F),
WriteComm (0x00F264), //COFTV_Merge=10
WriteData (0x0002),
WriteComm (0x00F266), //DB_OFTV_DLY
WriteData (0x0000),
WriteComm (0x00F26A), //RST_DUR
WriteData (0x0707),
//------Side Region need to set SUB_SPI------ 
WriteComm (0x00F312), //Write to Side Addr
WriteData (0x0200),
WriteComm (0x00F314), //Write Length 54word
WriteData (0x0138),
Delay_ms (10),
WriteComm (0x00F506), //OFTV_TRIM_START
WriteData (0x0000),
WriteComm (0x00F50A), //Normal freq
WriteData (0x01DD),
WriteComm (0x00F50C), //AA/Merge Freq
WriteData (0x0000),
//------Serial_Buff_WR------ 
WriteComm (0x00F51A), 
WriteData (0x0001),
WriteComm (0x00F51A), 
WriteData (0x0000),
Delay_ms (10),
//----TB_Sel MUXA_TB1,MUXB_TB2-----
WriteComm (0x00F090), 
WriteData (0x0000), // A A
WriteData (0x0055), // A B
WriteData (0x5555), // B B
//----TB1 Set MUA-----
WriteComm (0x00F096), 
WriteData (0x0001), //RX00 01
WriteData (0x0203), //RX02 03
WriteData (0x0405), //RX04 05
WriteData (0x0607), //RX06 07
WriteData (0x0809), //RX08 09
WriteData (0x0A0B), //RX10 11
WriteData (0x0C0D), //RX12 13
WriteData (0x0E0F), //RX14 15
WriteData (0x1011), //RX16 17
WriteData (0x1213), //RX18 19
WriteData (0x1415), //RX20 21
WriteData (0x1617), //RX22 23
WriteData (0x1819), //RX24 25
WriteData (0x1A1B), //RX26 27
WriteData (0x1C1D), //RX28 29
WriteData (0x1E1F), //RX30 31
//----TB2 Set MUXB------
WriteComm (0x00F0B6), 
WriteData (0x0001), //RX00 01
WriteData (0x0203), //RX02 03
WriteData (0x0405), //RX04 05
WriteData (0x0607), //RX06 07
WriteData (0x0809), //RX08 09
WriteData (0x0A0B), //RX10 11
WriteData (0x0C0D), //RX12 13
WriteData (0x0E0F), //RX14 15
WriteData (0x1011), //RX16 17
WriteData (0x1213), //RX18 19
WriteData (0x1415), //RX20 21
WriteData (0x1617), //RX22 23
WriteData (0x1819), //RX24 25
WriteData (0x1A1B), //RX26 27
WriteData (0x1C1D), //RX28 29
WriteData (0x1E1F), //RX30 31
WriteComm (0x00F000), 
WriteData (0x0002),
//-------Trim OFTV Flow-----------------
//WriteComm (0x00F024), //7AA+1Self+0Noise
//WriteData (0x74A0),
//WriteComm (0x00F506), //OFTV Trim on 5frame
//WriteData (0x0001),
//------Serial_Buff_WR------ 
WriteComm (0x00F51A), 
WriteData (0x0001),
WriteComm (0x00F51A), 
WriteData (0x0000),
Delay_ms (100),
WriteComm (0x00F024), //7AA+0Self+2Noise
WriteData (0x72A0),
//--------END Trim---------------------------------
WriteComm (0x00F000), 
WriteData (0x0002),
};

AFE_CMD_T test_cmd_channel_mapping[] = {
//----TB_Sel MUXA_TB1,MUXB_TB2-----
WriteComm (0x00F090), 
WriteData (0x0000), // A A
WriteData (0x0055), // A B
WriteData (0x5555), // B B
//----TB1 Set MUA-----
WriteComm (0x00F096), 
WriteData (0x0001), //RX00 01
WriteData (0x0203), //RX02 03
WriteData (0x0405), //RX04 05
WriteData (0x0607), //RX06 07
WriteData (0x0809), //RX08 09
WriteData (0x0A0B), //RX10 11
WriteData (0x0C0D), //RX12 13
WriteData (0x0E0F), //RX14 15
WriteData (0x1011), //RX16 17
WriteData (0x1213), //RX18 19
WriteData (0x1415), //RX20 21
WriteData (0x1617), //RX22 23
WriteData (0x1819), //RX24 25
WriteData (0x1A1B), //RX26 27
WriteData (0x1C1D), //RX28 29
WriteData (0x1E1F), //RX30 31
//----TB2 Set MUXB------
WriteComm (0x00F0B6), 
WriteData (0x0001), //RX00 01
WriteData (0x0203), //RX02 03
WriteData (0x0405), //RX04 05
WriteData (0x0607), //RX06 07
WriteData (0x0809), //RX08 09
WriteData (0x0A0B), //RX10 11
WriteData (0x0C0D), //RX12 13
WriteData (0x0E0F), //RX14 15
WriteData (0x1011), //RX16 17
WriteData (0x1213), //RX18 19
WriteData (0x1415), //RX20 21
WriteData (0x1617), //RX22 23
WriteData (0x1819), //RX24 25
WriteData (0x1A1B), //RX26 27
WriteData (0x1C1D), //RX28 29
WriteData (0x1E1F), //RX30 31
WriteComm (0x00F000), 
WriteData (0x0002),
//------Serial_Buff_WR------ 
WriteComm (0x00F51A), 
WriteData (0x0001),
WriteComm (0x00F51A), 
WriteData (0x0000),
Delay_ms (200),
//--------END Trim---------------------------------
WriteComm (0x00F000), 
WriteData (0x0002),
Delay_ms (100),
};
	

#else

#endif    