

//#define ST_REPLACE_TEST_CMD_BY_DISPLAY_ID
#define ST_ADDRESS_MODE_WRITE_COMMAND_V2
#define WriteComm(cmd)		{(0x01), (cmd)}
#define WriteData(data)		{(0x02), (data)}
#define Delay_ms(time)		{(0x03), (time)}
#ifdef ST_REPLACE_TEST_CMD_BY_DISPLAY_ID
unsigned char test_id_1[3] = {0x80,0xA0,0xFB};
#endif /* ST_REPLACE_TEST_CMD_BY_DISPLAY_ID */


#define SITRONIX_TEST_CMD_NORMAL_MAX_LEN		2800
#define SITRONIX_TEST_CMD_OPEN_MAX_LEN			2800	//2800
#define SITRONIX_TEST_CMD_SHORT_ODD_MAX_LEN		2800	//1280
#define SITRONIX_TEST_CMD_SHORT_EVEN_MAX_LEN		2800	//1280
#define SITRONIX_TEST_CMD_UNIFORMITY_MAX_LEN		2800
#define SITRONIX_TEST_CMD_STD_MAX_LEN			2800

#define ST_SELFTEST_LOG_FILE
#define ST_SELFTEST_LOG_PATH    "/sdcard/ST_SELFTEST_LOG.txt"

#define ST_SELFTEST_INI_PATH    "st_selftest_criteria.ini"

//#define ST_SELFTEST_LOG_PATH "/data/vendor/misc/touch/ST_SELFTEST_LOG.txt"

#define ST_SELFTEST_IGNORE_FRAME		3
#define ST_SELFTEST_ADJUST_COUNT		0
#define ST_SELFTEST_SKIP_COLS			3
#define ST_SELFTEST_SHORT_MAX			650
#define ST_SELFTEST_OPEN_MIN			4090			//only for ST7123 Open test.
#define ST_SELFTEST_OPEN_MAX			4096			//only for ST7123 Open test.
#define ST_SELFTEST_OPEN_MUX_ON_OFF_MIN		500			//for ST7121P/ST7123P Mux On/Off Open test.
#define ST_SELFTEST_NORMAL_MIN			-300
#define ST_SELFTEST_NORMAL_MAX			3000


#define ST_SELFTEST_UNIFORMITY_SHIFT	0
#define ST_SELFTEST_UNIFORMITY_MIN		80
#define ST_SELFTEST_UNIFORMITY_MAX		125

#define ST_SELFTEST_STD_FRAME_CNT		100
#define ST_SELFTEST_STD_MAX				30	//3.0
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
AFE_CMD_T test_flash_afe_df[]	= {};	//No default value for ST7121P.
AFE_CMD_T test_cmd_open	[]	= { //for 7123 or 77921
    
WriteComm (0x00F000), 
WriteData (0x0000),
WriteComm (0x00F000), 
WriteData (0x0001),
WriteComm (0x537123), //Addr
WriteData (0xa53c),
WriteComm (0x537123), //AFE Unlock 
WriteData (0x1455),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x5555),
Delay_ms (100),
WriteComm (0x00F000), 
WriteData (0x0000),


/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver Start/////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//MCU Reset Keep L
WriteComm (0x00F302),
WriteData (0x0001),

//SW Reset
//WriteComm (0x0306FC),
//WriteData (0x5A9D),
//WriteData (0x0100),
//WriteComm (0x0306FE),
//WriteData (0x00A5),
/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////~Reset AFE & Driver End/////////////////////////
/////////////////////////////////////////////////////////////////////////////////
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0xF200),//CMD
WriteComm (0x0306FE),
WriteData (0x00A5), 

//---Sleep In--------------
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1000),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (120),

//---Sleep Out--------------
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1100),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (120),



/////////////////////////FRAM START//////////////////////////////
/////////////////////////FRAM START//////////////////////////////
/////////////////////////FRAM START//////////////////////////////
//-----FRAM Write(Coef Base0)---3.4
WriteComm (0x00E000),
WriteData (0x003F),
WriteData (0x004B),
WriteData (0x0057),
WriteData (0x0065),
WriteData (0x0072),
WriteData (0x0080),
WriteData (0x008F),
WriteData (0x009E),
WriteData (0x00AD),
WriteData (0x00BD),
WriteData (0x00CD),
WriteData (0x00DC),
WriteData (0x00EC),
WriteData (0x00FC),
WriteData (0x010B),
WriteData (0x011B),
WriteData (0x0129),
WriteData (0x0138),
WriteData (0x0146),
WriteData (0x0153),
WriteData (0x0160),
WriteData (0x016B),
WriteData (0x0176),
WriteData (0x0180),
WriteData (0x018a),
WriteData (0x0192),
WriteData (0x0199),
WriteData (0x019F),
WriteData (0x01A4),
WriteData (0x01A7),
WriteData (0x01AA),
WriteData (0x01AB),
//-----FRAM Write(Coef Base1)---
WriteComm (0x00E040),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
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
//---Freq[00]-----
WriteComm (0x00E140),//104.9K
WriteData (0x7E85),
WriteData (0x0005),
WriteData (0x0C00),
WriteData (0x000A),
WriteData (0x350D),
WriteData (0x313F),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[01]-----
WriteComm (0x00E150),//95.54k
WriteData (0x7E85),
WriteData (0x0013),
WriteData (0x0C00),
WriteData (0x0009),
WriteData (0x3B4B),
WriteData (0x3706),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[02]-----
WriteComm (0x00E160),//85.23k
WriteData (0x7E85),
WriteData (0x0026),
WriteData (0x0C00),
WriteData (0x0008),
WriteData (0x4333),
WriteData (0x3E4B),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[03]-----
WriteComm (0x00E170),//75k
WriteData (0x7E85),
WriteData (0x003E),
WriteData (0x0C00),
WriteData (0x0007),
WriteData (0x4D89),
WriteData (0x47B3),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[04]-----
WriteComm (0x00E180),//65.5k
WriteData (0x7E85),
WriteData (0x005B),
WriteData (0x0C00),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x549D),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[05]-----
WriteComm (0x00E190),//60k
WriteData (0x7E85),
WriteData (0x0070),
WriteData (0x0C00),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x549D),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[06]-----
WriteComm (0x00E1A0),//56.6k
WriteData (0x7E85),
WriteData (0x007C),
WriteData (0x0C00),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6720),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[07]-----
WriteComm (0x00E1B0),//50k
WriteData (0x7E85),
WriteData (0x00A2),
WriteData (0x0C00),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6720),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[08]-----
WriteComm (0x00E1C0),//104.9k
WriteData (0x7E85),
WriteData (0x0005),
WriteData (0x0C00),
WriteData (0x000A),
WriteData (0x350D),
WriteData (0x313F),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[09]-----
WriteComm (0x00E1D0),//95.54k
WriteData (0x7E85),
WriteData (0x0013),
WriteData (0x0C00),
WriteData (0x0009),
WriteData (0x3B4B),
WriteData (0x3706),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[10]-----
WriteComm (0x00E1E0),//85.23k
WriteData (0x7E85),
WriteData (0x0026),
WriteData (0x0C00),
WriteData (0x0008),
WriteData (0x4333),
WriteData (0x3E4B),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[11]-----
WriteComm (0x00E1F0),//75k
WriteData (0x7E85),
WriteData (0x003E),
WriteData (0x0C00),
WriteData (0x0007),
WriteData (0x4D89),
WriteData (0x47B3),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[12]-----
WriteComm (0x00E200),//65.5k
WriteData (0x7E85),
WriteData (0x005B),
WriteData (0x0C00),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x549D),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[13]-----
WriteComm (0x00E210),//60k
WriteData (0x7E85),
WriteData (0x0070),
WriteData (0x0C00),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x549D),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[14]-----
WriteComm (0x00E220),//56.6k
WriteData (0x7E85),
WriteData (0x007C),
WriteData (0x0C00),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6720),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[15]-----
WriteComm (0x00E230),//50k
WriteData (0x7E85),
WriteData (0x00A2),
WriteData (0x0C00),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6720),
WriteData (0x0000),
WriteData (0x4000),
//-----FRAM Write(OFTV Finger)---
WriteComm (0x00E240),//OFTV_Finger/Doze
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV Flush)---
WriteComm (0x00E24A),//OFTV_Flush0
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E254),//OFTV_Flush1
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E25E),//OFTV_Flush2
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV)---
WriteComm (0x00E268),//OFTV_unit0
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E272),//OFTV_unit1
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E27C),//OFTV_unit2
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E286),//OFTV_unit3
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E290),//OFTV_unit4
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E29A),//OFTV_unit5
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2A4),//OFTV_unit6
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2AE),//OFTV_unit7
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2B8),//OFTV_unit8
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2C2),//OFTV_unit9
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2CC),//OFTV_unit10
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2D6),//OFTV_unit11
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2E0),//OFTV_unit12
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2EA),//OFTV_unit13
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Near Far)---
WriteComm (0x00E2F4),//Near Far
WriteData (0x8000),//00
WriteData (0x8000),//01
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
//-----FRAM Write(Rx Active)---
WriteComm (0x00E3E4),//RX Active Table
WriteData (0x7FFF),//Unit0 14~0
WriteData (0x7FFF),//Unit1 14~0
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


/////////////////////////FRAM End//////////////////////////////
/////////////////////////FRAM End//////////////////////////////
/////////////////////////FRAM End//////////////////////////////



//-----AFE Register-------------
WriteComm (0x00F004),
WriteData (0x0400),
WriteComm (0x00F018),//OFTV EN
WriteData (0x0000),
WriteComm (0x00F01A),//OFTV_STEP
WriteData (0x4100),
WriteComm (0x00F01C),//INT_RUN
WriteData (0x3050),
WriteComm (0x00F024),//AA length
WriteData (0x0091),
WriteComm (0x00F026),//Flush length
WriteData (0x0091),
WriteComm (0x00F028),//Finger length
WriteData (0x0091),
WriteComm (0x00F02A),
WriteData (0x0091),
WriteComm (0x00F030),//Finger to Active(period)
WriteData (0x01E0),
WriteComm (0x00F036),
WriteData (0x0030),
WriteComm (0x00F03E),//IVDDASEL=01
WriteData (0x0010),
WriteComm (0x00F040),//CFB11
WriteData (0x3008),
WriteComm (0x00F042),//VAGDRV
WriteData (0x0070),
WriteComm (0x00F044),//RXNUM
WriteData (0x0F00),
WriteComm (0x00F046),//COL_TBL00~13
WriteData (0x1032),
WriteData (0x5476),
WriteData (0x98BA),
WriteData (0x00DC),
WriteComm (0x00F07C),//ICA_Merge
WriteData (0x0002),
WriteComm (0x00F082),//ESD_GOLDEN
WriteData (0x3664),
WriteComm (0x00F08A),//SIKP_TX
WriteData (0x1F0C),
WriteComm (0x00F094),//NOR_TRUNC
WriteData (0x3313),
//WriteComm (0x00F096),//AAMuxtable
//WriteData (0x0102),
//WriteData (0x0304),
//WriteData (0x0506),
//WriteData (0x0708),
//WriteData (0x090A),
//WriteData (0x0B0C),
//WriteData (0x0D0E),
WriteComm (0x00F096),//AAMuxtable
WriteData (0x0102),
WriteData (0x0405),
WriteData (0x0708),
WriteData (0x0A0B),
WriteData (0x0D0E),
WriteData (0x0B0C),
WriteData (0x0D0E),
WriteComm (0x00F0A4),//NoiseMuxtable
WriteData (0x3F3F),
WriteComm (0x00F0A6),
WriteData (0x3F3F),
WriteComm (0x00F0A8),//FingerMuxtable
WriteData (0x3F3F),
WriteData (0x3F3F),
WriteData (0x3F3F),
WriteData (0x3F3F),
WriteComm (0x00F0B6),//OFTV Trim noise unit -> finger unit
WriteData (0x544A),
WriteComm (0x00F500),//AA Freq Sel
WriteData (0x0021),
WriteComm (0x00F502),//Flush Freq Sel
WriteData (0xEEFF),
WriteComm (0x00F504),//Drive Mode 0
WriteData (0x1003),
WriteComm (0x00F506),//Unit 
WriteData (0x1A02),
WriteComm (0x00F508),// INT FR
WriteData (0x14D5),





//------Serial_Buff_WR------ 
WriteComm (0x00F512), 
WriteData (0x0001),
WriteComm (0x00F512), 
WriteData (0x0000),
Delay_ms (1),

WriteComm (0x00F000),//SensOn
WriteData (0x0001),



};	//No default value for ST7121P.
AFE_CMD_T test_cmd_channel_mapping[] = {
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
};
AFE_CMD_T test_cmd_open_MuxOff[]= {
}; 
AFE_CMD_T test_cmd_short_odd[] = {
WriteComm (0x00F000), 
WriteData (0x0000),
WriteComm (0x00F000), 
WriteData (0x0001),
WriteComm (0x537123), //Addr
WriteData (0xa53c),
WriteComm (0x537123), //AFE Unlock 
WriteData (0x1455),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x5555),
Delay_ms (100),
WriteComm (0x00F000), 
WriteData (0x0000),

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver Start/////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//MCU Reset Keep L
WriteComm (0x00F300),
WriteData (0x5AA5),
WriteComm (0x00F302),
WriteData (0x0001),

Delay_ms (120),

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////~Reset AFE & Driver End/////////////////////////
/////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////FLASH OPT Low/////////////////////////
/////////////////////////////////////////////////////////////////////////////////

//WriteComm (0x0306FC),
//WriteData (0x5A9D),
//WriteData (0xF200),//CMD
//WriteComm (0x0306FE),
//WriteData (0x00A5), 
//WriteComm (0x0306FC),
//WriteData (0x5A9D),
//WriteData (0xE100),//CMD
//WriteData (0xCEA8),//CMD
//WriteComm (0x0306FE),
//WriteData (0x00A5), 

/////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////~FLASH OPT Low/////////////////////////
/////////////////////////////////////////////////////////////////////////////////

WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0xF000),//CMD
WriteComm (0x0306FE),
WriteData (0x00A5), 

//---Sleep In--------------
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1000),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (120),




/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver Start/////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//MCU Reset Keep L
WriteComm (0x00F300),
WriteData (0x5AA5),
WriteComm (0x00F302),
WriteData (0x0001),


/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////~Reset AFE & Driver End/////////////////////////
/////////////////////////////////////////////////////////////////////////////////


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
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0200),//20
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),//80
WriteData (0x0000),
//---Freq[00]-----
WriteComm (0x00E140),//104.9K
WriteData (0xFE05),
WriteData (0x00FE),
WriteData (0x0C00),
WriteData (0x0014),
WriteData (0xFF00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4000),
//-----FRAM Write(OFTV Finger)---
WriteComm (0x00E240),//OFTV_Finger/Doze
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV Flush)---
WriteComm (0x00E24A),//OFTV_Flush0
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E254),//OFTV_Flush1
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E25E),//OFTV_Flush2
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV)---
WriteComm (0x00E268),//OFTV_unit0
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E272),//OFTV_unit1
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E27C),//OFTV_unit2
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E286),//OFTV_unit3
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E290),//OFTV_unit4
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E29A),//OFTV_unit5
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2A4),//OFTV_unit6
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2AE),//OFTV_unit7
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2B8),//OFTV_unit8
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2C2),//OFTV_unit9
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2CC),//OFTV_unit10
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2D6),//OFTV_unit11
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2E0),//OFTV_unit12
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2EA),//OFTV_unit13
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Near Far)---
WriteComm (0x00E2F4),//Near Far
WriteData (0x8000),//00
WriteData (0x8000),//01
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
//-----FRAM Write(Rx Active)---
WriteComm (0x00E3E4),//RX Active Table
WriteData (0x7FFF),//Unit0 14~0
WriteData (0x7FFF),//Unit1 14~0
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


//-----AFE Register-------------
WriteComm (0x00F004),
WriteData (0x0400),
WriteComm (0x00F024),
WriteData (0x03FF),
WriteComm (0x00F026),
WriteData (0x0091),
WriteComm (0x00F028),
WriteData (0x0091),
WriteComm (0x00F02A),
WriteData (0x0091),
WriteComm (0x00F030),
WriteData (0x01E0),
WriteComm (0x00F03E),
WriteData (0x0000),
WriteComm (0x00F042),
WriteData (0x0070),
WriteComm (0x00F044),//RXNUM
WriteData (0x0F00),
WriteComm (0x00F046),//COL_TBL00~13
WriteData (0x1032),
WriteData (0x5476),
WriteData (0x98BA),
WriteData (0x00DC),
WriteComm (0x00F084),
WriteData (0x005A),
WriteComm (0x00F086),
WriteData (0x0258),
WriteComm (0x00F08A),
WriteData (0x1F0C),
WriteComm (0x00F094),//for short
WriteData (0x0010),
WriteComm (0x00F096),//AAMuxtable
WriteData (0x0102),
WriteData (0x0304),
WriteData (0x0506),
WriteData (0x0708),
WriteData (0x090A),
WriteData (0x0B0C),
WriteData (0x0D0E),
WriteComm (0x00F506),
WriteData (0x0E00),
WriteComm (0x00F508),
WriteData (0x14D5),
WriteComm (0x00F01C),
WriteData (0x7150),
//------Serial_Buff_WR------ 
WriteComm (0x00F512),
WriteData (0x0001),
WriteComm (0x00F512),
WriteData (0x0000),
Delay_ms (1),
WriteComm (0x00F038),//Short_EN=1
WriteData (0x0400),
WriteComm (0x00F040),//STOG_E=1 O=0
WriteData (0x3208),
//WriteComm (0x00F040),//STOG_E=0 O=1
//WriteData (0x3108),
WriteComm (0x00F000),
WriteData (0x0001),


Delay_ms (100),




};
AFE_CMD_T test_cmd_short_even[] = {
WriteComm (0x00F000), 
WriteData (0x0000),
WriteComm (0x00F000), 
WriteData (0x0001),
WriteComm (0x537123), //Addr
WriteData (0xa53c),
WriteComm (0x537123), //AFE Unlock 
WriteData (0x1455),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x5555),
Delay_ms (100),
WriteComm (0x00F000), 
WriteData (0x0000),

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver Start/////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//MCU Reset Keep L
WriteComm (0x00F300),
WriteData (0x5AA5),
WriteComm (0x00F302),
WriteData (0x0001),

Delay_ms (120),

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////~Reset AFE & Driver End/////////////////////////
/////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////FLASH OPT Low/////////////////////////
/////////////////////////////////////////////////////////////////////////////////

//WriteComm (0x0306FC),
//WriteData (0x5A9D),
//WriteData (0xF200),//CMD
//WriteComm (0x0306FE),
//WriteData (0x00A5), 
//WriteComm (0x0306FC),
//WriteData (0x5A9D),
//WriteData (0xE100),//CMD
//WriteData (0xCEA8),//CMD
//WriteComm (0x0306FE),
//WriteData (0x00A5), 

/////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////~FLASH OPT Low/////////////////////////
/////////////////////////////////////////////////////////////////////////////////

WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0xF000),//CMD
WriteComm (0x0306FE),
WriteData (0x00A5), 

//---Sleep In--------------
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1000),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (120),




/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver Start/////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//MCU Reset Keep L
WriteComm (0x00F300),
WriteData (0x5AA5),
WriteComm (0x00F302),
WriteData (0x0001),


/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////~Reset AFE & Driver End/////////////////////////
/////////////////////////////////////////////////////////////////////////////////


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
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0200),//20
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),//80
WriteData (0x0000),
//---Freq[00]-----
WriteComm (0x00E140),//104.9K
WriteData (0xFE05),
WriteData (0x00FE),
WriteData (0x0C00),
WriteData (0x0014),
WriteData (0xFF00),
WriteData (0x4000),
WriteData (0x0000),
WriteData (0x4000),
//-----FRAM Write(OFTV Finger)---
WriteComm (0x00E240),//OFTV_Finger/Doze
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV Flush)---
WriteComm (0x00E24A),//OFTV_Flush0
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E254),//OFTV_Flush1
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E25E),//OFTV_Flush2
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV)---
WriteComm (0x00E268),//OFTV_unit0
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E272),//OFTV_unit1
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E27C),//OFTV_unit2
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E286),//OFTV_unit3
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E290),//OFTV_unit4
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E29A),//OFTV_unit5
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2A4),//OFTV_unit6
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2AE),//OFTV_unit7
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2B8),//OFTV_unit8
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2C2),//OFTV_unit9
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2CC),//OFTV_unit10
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2D6),//OFTV_unit11
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2E0),//OFTV_unit12
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2EA),//OFTV_unit13
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Near Far)---
WriteComm (0x00E2F4),//Near Far
WriteData (0x8000),//00
WriteData (0x8000),//01
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
//-----FRAM Write(Rx Active)---
WriteComm (0x00E3E4),//RX Active Table
WriteData (0x7FFF),//Unit0 14~0
WriteData (0x7FFF),//Unit1 14~0
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


//-----AFE Register-------------
WriteComm (0x00F004),
WriteData (0x0400),
WriteComm (0x00F024),
WriteData (0x03FF),
WriteComm (0x00F026),
WriteData (0x0091),
WriteComm (0x00F028),
WriteData (0x0091),
WriteComm (0x00F02A),
WriteData (0x0091),
WriteComm (0x00F030),
WriteData (0x01E0),
WriteComm (0x00F03E),
WriteData (0x0000),
WriteComm (0x00F042),
WriteData (0x0070),
WriteComm (0x00F044),//RXNUM
WriteData (0x0F00),
WriteComm (0x00F046),//COL_TBL00~13
WriteData (0x1032),
WriteData (0x5476),
WriteData (0x98BA),
WriteData (0x00DC),
WriteComm (0x00F084),
WriteData (0x005A),
WriteComm (0x00F086),
WriteData (0x0258),
WriteComm (0x00F08A),
WriteData (0x1F0C),
WriteComm (0x00F094),//for short
WriteData (0x0010),
WriteComm (0x00F096),//AAMuxtable
WriteData (0x0102),
WriteData (0x0304),
WriteData (0x0506),
WriteData (0x0708),
WriteData (0x090A),
WriteData (0x0B0C),
WriteData (0x0D0E),
WriteComm (0x00F506),
WriteData (0x0E00),
WriteComm (0x00F508),
WriteData (0x14D5),
WriteComm (0x00F01C),
WriteData (0x7150),
//------Serial_Buff_WR------ 
WriteComm (0x00F512),
WriteData (0x0001),
WriteComm (0x00F512),
WriteData (0x0000),
Delay_ms (1),
WriteComm (0x00F038),//Short_EN=1
WriteData (0x0400),
//WriteComm (0x00F040),//STOG_E=1 O=0
//WriteData (0x3208),
WriteComm (0x00F040),//STOG_E=0 O=1
WriteData (0x3108),
WriteComm (0x00F000),
WriteData (0x0001),


Delay_ms (100),




};    
AFE_CMD_T test_cmd_std[] = {
WriteComm (0x00F000), 
WriteData (0x0000),
WriteComm (0x00F000), 
WriteData (0x0001),
WriteComm (0x537123), //Addr
WriteData (0xa53c),
WriteComm (0x537123), //AFE Unlock 
WriteData (0x1455),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x5555),
Delay_ms (100),
WriteComm (0x00F000), 
WriteData (0x0000),


/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver Start/////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//MCU Reset Keep L
WriteComm (0x00F300),
WriteData (0x5AA5),
WriteComm (0x00F302),
WriteData (0x0001),
//////////////////////////
Delay_ms (120),
///////////////////////////////////////////////////////
////////////////////////////////~Reset AFE & Driver End/////////////////////////
/////////////////////////////////////////////////////////////////////////////////
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0xF000),//CMD
WriteComm (0x0306FE),
WriteData (0x00A5), 

//---Sleep In--------------
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1000),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (120),

//---Sleep Out--------------
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1100),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (120),



/////////////////////////FRAM START//////////////////////////////
/////////////////////////FRAM START//////////////////////////////
/////////////////////////FRAM START//////////////////////////////
//-----FRAM Write(Coef Base0)---3.4
WriteComm (0x00E000),
WriteData (0x003F),
WriteData (0x004B),
WriteData (0x0057),
WriteData (0x0065),
WriteData (0x0072),
WriteData (0x0080),
WriteData (0x008F),
WriteData (0x009E),
WriteData (0x00AD),
WriteData (0x00BD),
WriteData (0x00CD),
WriteData (0x00DC),
WriteData (0x00EC),
WriteData (0x00FC),
WriteData (0x010B),
WriteData (0x011B),
WriteData (0x0129),
WriteData (0x0138),
WriteData (0x0146),
WriteData (0x0153),
WriteData (0x0160),
WriteData (0x016B),
WriteData (0x0176),
WriteData (0x0180),
WriteData (0x018a),
WriteData (0x0192),
WriteData (0x0199),
WriteData (0x019F),
WriteData (0x01A4),
WriteData (0x01A7),
WriteData (0x01AA),
WriteData (0x01AB),
//-----FRAM Write(Coef Base1)---
WriteComm (0x00E040),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
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
//---Freq[00]-----
WriteComm (0x00E140),//104.9K
WriteData (0x7E85),
WriteData (0x0005),
WriteData (0x0C00),
WriteData (0x000A),
WriteData (0x350D),
WriteData (0x313F),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[01]-----
WriteComm (0x00E150),//95.54k
WriteData (0x7E85),
WriteData (0x0013),
WriteData (0x0C00),
WriteData (0x0009),
WriteData (0x3B4B),
WriteData (0x3706),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[02]-----
WriteComm (0x00E160),//85.23k
WriteData (0x7E85),
WriteData (0x0026),
WriteData (0x0C00),
WriteData (0x0008),
WriteData (0x4333),
WriteData (0x3E4B),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[03]-----
WriteComm (0x00E170),//75k
WriteData (0x7E85),
WriteData (0x003E),
WriteData (0x0C00),
WriteData (0x0007),
WriteData (0x4D89),
WriteData (0x47B3),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[04]-----
WriteComm (0x00E180),//65.5k
WriteData (0x7E85),
WriteData (0x005B),
WriteData (0x0C00),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x549D),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[05]-----
WriteComm (0x00E190),//60k
WriteData (0x7E85),
WriteData (0x0070),
WriteData (0x0C00),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x549D),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[06]-----
WriteComm (0x00E1A0),//56.6k
WriteData (0x7E85),
WriteData (0x007C),
WriteData (0x0C00),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6720),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[07]-----
WriteComm (0x00E1B0),//50k
WriteData (0x7E85),
WriteData (0x00A2),
WriteData (0x0C00),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6720),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[08]-----
WriteComm (0x00E1C0),//104.9k
WriteData (0x7E85),
WriteData (0x0005),
WriteData (0x0C00),
WriteData (0x000A),
WriteData (0x350D),
WriteData (0x313F),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[09]-----
WriteComm (0x00E1D0),//95.54k
WriteData (0x7E85),
WriteData (0x0013),
WriteData (0x0C00),
WriteData (0x0009),
WriteData (0x3B4B),
WriteData (0x3706),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[10]-----
WriteComm (0x00E1E0),//85.23k
WriteData (0x7E85),
WriteData (0x0026),
WriteData (0x0C00),
WriteData (0x0008),
WriteData (0x4333),
WriteData (0x3E4B),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[11]-----
WriteComm (0x00E1F0),//75k
WriteData (0x7E85),
WriteData (0x003E),
WriteData (0x0C00),
WriteData (0x0007),
WriteData (0x4D89),
WriteData (0x47B3),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[12]-----
WriteComm (0x00E200),//65.5k
WriteData (0x7E85),
WriteData (0x005B),
WriteData (0x0C00),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x549D),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[13]-----
WriteComm (0x00E210),//60k
WriteData (0x7E85),
WriteData (0x0070),
WriteData (0x0C00),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x549D),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[14]-----
WriteComm (0x00E220),//56.6k
WriteData (0x7E85),
WriteData (0x007C),
WriteData (0x0C00),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6720),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[15]-----
WriteComm (0x00E230),//50k
WriteData (0x7E85),
WriteData (0x00A2),
WriteData (0x0C00),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6720),
WriteData (0x0000),
WriteData (0x4000),
//-----FRAM Write(OFTV Finger)---
WriteComm (0x00E240),//OFTV_Finger/Doze
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV Flush)---
WriteComm (0x00E24A),//OFTV_Flush0
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E254),//OFTV_Flush1
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E25E),//OFTV_Flush2
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV)---
WriteComm (0x00E268),//OFTV_unit0
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E272),//OFTV_unit1
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E27C),//OFTV_unit2
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E286),//OFTV_unit3
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E290),//OFTV_unit4
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E29A),//OFTV_unit5
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2A4),//OFTV_unit6
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2AE),//OFTV_unit7
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2B8),//OFTV_unit8
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2C2),//OFTV_unit9
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2CC),//OFTV_unit10
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2D6),//OFTV_unit11
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2E0),//OFTV_unit12
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2EA),//OFTV_unit13
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Near Far)---
WriteComm (0x00E2F4),//Near Far
WriteData (0x8000),//00
WriteData (0x8000),//01
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
//-----FRAM Write(Rx Active)---
WriteComm (0x00E3E4),//RX Active Table
WriteData (0x7FFF),//Unit0 14~0
WriteData (0x7FFF),//Unit1 14~0
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


/////////////////////////FRAM End//////////////////////////////
/////////////////////////FRAM End//////////////////////////////
/////////////////////////FRAM End//////////////////////////////



//-----AFE Register-------------
WriteComm (0x00F004),
WriteData (0x0400),
WriteComm (0x00F018),//OFTV EN
WriteData (0x0000),
WriteComm (0x00F01A),//OFTV_STEP
WriteData (0x4100),
WriteComm (0x00F01C),//INT_RUN
WriteData (0x3050),
WriteComm (0x00F024),//AA length
WriteData (0x0091),
WriteComm (0x00F026),//Flush length
WriteData (0x0091),
WriteComm (0x00F028),//Finger length
WriteData (0x0091),
WriteComm (0x00F02A),
WriteData (0x0091),
WriteComm (0x00F030),//Finger to Active(period)
WriteData (0x01E0),
WriteComm (0x00F036),
WriteData (0x0030),
WriteComm (0x00F03E),//IVDDASEL=01
WriteData (0x0010),
WriteComm (0x00F040),//CFB11
WriteData (0x3008),
WriteComm (0x00F042),//VAGDRV
WriteData (0x0070),
WriteComm (0x00F044),//RXNUM
WriteData (0x0F00),
WriteComm (0x00F046),//COL_TBL00~13
WriteData (0x1032),
WriteData (0x5476),
WriteData (0x98BA),
WriteData (0x00DC),
WriteComm (0x00F07C),//ICA_Merge
WriteData (0x0002),
WriteComm (0x00F082),//ESD_GOLDEN
WriteData (0x3664),
WriteComm (0x00F08A),//SIKP_TX
WriteData (0x1F0C),
WriteComm (0x00F094),//NOR_TRUNC
WriteData (0x3313),
//WriteComm (0x00F096),//AAMuxtable
//WriteData (0x0102),
//WriteData (0x0304),
//WriteData (0x0506),
//WriteData (0x0708),
//WriteData (0x090A),
//WriteData (0x0B0C),
//WriteData (0x0D0E),
WriteComm (0x00F096),//AAMuxtable
WriteData (0x0102),
WriteData (0x0405),
WriteData (0x0708),
WriteData (0x0A0B),
WriteData (0x0D0E),
WriteData (0x0B0C),
WriteData (0x0D0E),
WriteComm (0x00F0A4),//NoiseMuxtable
WriteData (0x3F3F),
WriteComm (0x00F0A6),
WriteData (0x3F3F),
WriteComm (0x00F0A8),//FingerMuxtable
WriteData (0x3F3F),
WriteData (0x3F3F),
WriteData (0x3F3F),
WriteData (0x3F3F),
WriteComm (0x00F0B6),//OFTV Trim noise unit -> finger unit
WriteData (0x544A),
WriteComm (0x00F500),//AA Freq Sel
WriteData (0x0021),
WriteComm (0x00F502),//Flush Freq Sel
WriteData (0xEEFF),
WriteComm (0x00F504),//Drive Mode 0
WriteData (0x1003),
WriteComm (0x00F508),// INT FR
WriteData (0x14D5),


/////////////////////////////////////////////////////
//////////Mux Modify 0x0102~0x0E02///////////////////
/////////////////////////////////////////////////////
WriteComm (0x00F506),//Unit 
WriteData (0x0A02), //0A: ACTIVE_UNIT_NUM[3:0]
/////////////////////////////////////////////////////
//////////Mux Modify 0x0102~0x0E02///////////////////
/////////////////////////////////////////////////////






//------Serial_Buff_WR------ 
WriteComm (0x00F512), 
WriteData (0x0001),
WriteComm (0x00F512), 
WriteData (0x0000),
Delay_ms (1),

WriteComm (0x00F000),//SensOn
WriteData (0x0001),

Delay_ms (120),





};
AFE_CMD_T test_cmd_normal_rawdata[] = {
WriteComm (0x00F000), 
WriteData (0x0000),
WriteComm (0x00F000), 
WriteData (0x0001),
WriteComm (0x537123), //Addr
WriteData (0xa53c),
WriteComm (0x537123), //AFE Unlock 
WriteData (0x1455),
WriteComm (0x537123), //HWRAM Unlock
WriteData (0x5555),
Delay_ms (100),
WriteComm (0x00F000), 
WriteData (0x0000),


/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////Reset AFE & Driver Start/////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//MCU Reset Keep L
WriteComm (0x00F300),
WriteData (0x5AA5),
WriteComm (0x00F302),
WriteData (0x0001),
//////////////////////////
Delay_ms (120),
///////////////////////////////////////////////////////
////////////////////////////////~Reset AFE & Driver End/////////////////////////
/////////////////////////////////////////////////////////////////////////////////
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0xF000),//CMD
WriteComm (0x0306FE),
WriteData (0x00A5), 

//---Sleep In--------------
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1000),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (120),

//---Sleep Out--------------
WriteComm (0x0306FC),
WriteData (0x5A9D),
WriteData (0x1100),
WriteComm (0x0306FE),
WriteData (0x00A5),
Delay_ms (120),



/////////////////////////FRAM START//////////////////////////////
/////////////////////////FRAM START//////////////////////////////
/////////////////////////FRAM START//////////////////////////////
//-----FRAM Write(Coef Base0)---3.4
WriteComm (0x00E000),
WriteData (0x003F),
WriteData (0x004B),
WriteData (0x0057),
WriteData (0x0065),
WriteData (0x0072),
WriteData (0x0080),
WriteData (0x008F),
WriteData (0x009E),
WriteData (0x00AD),
WriteData (0x00BD),
WriteData (0x00CD),
WriteData (0x00DC),
WriteData (0x00EC),
WriteData (0x00FC),
WriteData (0x010B),
WriteData (0x011B),
WriteData (0x0129),
WriteData (0x0138),
WriteData (0x0146),
WriteData (0x0153),
WriteData (0x0160),
WriteData (0x016B),
WriteData (0x0176),
WriteData (0x0180),
WriteData (0x018a),
WriteData (0x0192),
WriteData (0x0199),
WriteData (0x019F),
WriteData (0x01A4),
WriteData (0x01A7),
WriteData (0x01AA),
WriteData (0x01AB),
//-----FRAM Write(Coef Base1)---
WriteComm (0x00E040),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
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
//---Freq[00]-----
WriteComm (0x00E140),//104.9K
WriteData (0x7E85),
WriteData (0x0005),
WriteData (0x0C00),
WriteData (0x000A),
WriteData (0x350D),
WriteData (0x313F),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[01]-----
WriteComm (0x00E150),//95.54k
WriteData (0x7E85),
WriteData (0x0013),
WriteData (0x0C00),
WriteData (0x0009),
WriteData (0x3B4B),
WriteData (0x3706),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[02]-----
WriteComm (0x00E160),//85.23k
WriteData (0x7E85),
WriteData (0x0026),
WriteData (0x0C00),
WriteData (0x0008),
WriteData (0x4333),
WriteData (0x3E4B),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[03]-----
WriteComm (0x00E170),//75k
WriteData (0x7E85),
WriteData (0x003E),
WriteData (0x0C00),
WriteData (0x0007),
WriteData (0x4D89),
WriteData (0x47B3),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[04]-----
WriteComm (0x00E180),//65.5k
WriteData (0x7E85),
WriteData (0x005B),
WriteData (0x0C00),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x549D),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[05]-----
WriteComm (0x00E190),//60k
WriteData (0x7E85),
WriteData (0x0070),
WriteData (0x0C00),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x549D),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[06]-----
WriteComm (0x00E1A0),//56.6k
WriteData (0x7E85),
WriteData (0x007C),
WriteData (0x0C00),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6720),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[07]-----
WriteComm (0x00E1B0),//50k
WriteData (0x7E85),
WriteData (0x00A2),
WriteData (0x0C00),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6720),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[08]-----
WriteComm (0x00E1C0),//104.9k
WriteData (0x7E85),
WriteData (0x0005),
WriteData (0x0C00),
WriteData (0x000A),
WriteData (0x350D),
WriteData (0x313F),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[09]-----
WriteComm (0x00E1D0),//95.54k
WriteData (0x7E85),
WriteData (0x0013),
WriteData (0x0C00),
WriteData (0x0009),
WriteData (0x3B4B),
WriteData (0x3706),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[10]-----
WriteComm (0x00E1E0),//85.23k
WriteData (0x7E85),
WriteData (0x0026),
WriteData (0x0C00),
WriteData (0x0008),
WriteData (0x4333),
WriteData (0x3E4B),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[11]-----
WriteComm (0x00E1F0),//75k
WriteData (0x7E85),
WriteData (0x003E),
WriteData (0x0C00),
WriteData (0x0007),
WriteData (0x4D89),
WriteData (0x47B3),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[12]-----
WriteComm (0x00E200),//65.5k
WriteData (0x7E85),
WriteData (0x005B),
WriteData (0x0C00),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x549D),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[13]-----
WriteComm (0x00E210),//60k
WriteData (0x7E85),
WriteData (0x0070),
WriteData (0x0C00),
WriteData (0x0006),
WriteData (0x5BA2),
WriteData (0x549D),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[14]-----
WriteComm (0x00E220),//56.6k
WriteData (0x7E85),
WriteData (0x007C),
WriteData (0x0C00),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6720),
WriteData (0x0000),
WriteData (0x4000),
//---Freq[15]-----
WriteComm (0x00E230),//50k
WriteData (0x7E85),
WriteData (0x00A2),
WriteData (0x0C00),
WriteData (0x0005),
WriteData (0x7000),
WriteData (0x6720),
WriteData (0x0000),
WriteData (0x4000),
//-----FRAM Write(OFTV Finger)---
WriteComm (0x00E240),//OFTV_Finger/Doze
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV Flush)---
WriteComm (0x00E24A),//OFTV_Flush0
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E254),//OFTV_Flush1
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E25E),//OFTV_Flush2
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(OFTV)---
WriteComm (0x00E268),//OFTV_unit0
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E272),//OFTV_unit1
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E27C),//OFTV_unit2
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E286),//OFTV_unit3
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E290),//OFTV_unit4
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E29A),//OFTV_unit5
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2A4),//OFTV_unit6
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2AE),//OFTV_unit7
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2B8),//OFTV_unit8
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2C2),//OFTV_unit9
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2CC),//OFTV_unit10
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2D6),//OFTV_unit11
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2E0),//OFTV_unit12
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteComm (0x00E2EA),//OFTV_unit13
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
WriteData (0x0000),
//-----FRAM Write(Near Far)---
WriteComm (0x00E2F4),//Near Far
WriteData (0x8000),//00
WriteData (0x8000),//01
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
WriteData (0x8000),
//-----FRAM Write(Rx Active)---
WriteComm (0x00E3E4),//RX Active Table
WriteData (0x7FFF),//Unit0 14~0
WriteData (0x7FFF),//Unit1 14~0
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


/////////////////////////FRAM End//////////////////////////////
/////////////////////////FRAM End//////////////////////////////
/////////////////////////FRAM End//////////////////////////////



//-----AFE Register-------------
WriteComm (0x00F004),
WriteData (0x0400),
WriteComm (0x00F018),//OFTV EN
WriteData (0x0000),
WriteComm (0x00F01A),//OFTV_STEP
WriteData (0x4100),
WriteComm (0x00F01C),//INT_RUN
WriteData (0x3050),
WriteComm (0x00F024),//AA length
WriteData (0x0091),
WriteComm (0x00F026),//Flush length
WriteData (0x0091),
WriteComm (0x00F028),//Finger length
WriteData (0x0091),
WriteComm (0x00F02A),
WriteData (0x0091),
WriteComm (0x00F030),//Finger to Active(period)
WriteData (0x01E0),
WriteComm (0x00F036),
WriteData (0x0030),
WriteComm (0x00F03E),//IVDDASEL=01
WriteData (0x0010),
WriteComm (0x00F040),//CFB11
WriteData (0x3008),
WriteComm (0x00F042),//VAGDRV
WriteData (0x0070),
WriteComm (0x00F044),//RXNUM
WriteData (0x0F00),
WriteComm (0x00F046),//COL_TBL00~13
WriteData (0x1032),
WriteData (0x5476),
WriteData (0x98BA),
WriteData (0x00DC),
WriteComm (0x00F07C),//ICA_Merge
WriteData (0x0002),
WriteComm (0x00F082),//ESD_GOLDEN
WriteData (0x3664),
WriteComm (0x00F08A),//SIKP_TX
WriteData (0x1F0C),
WriteComm (0x00F094),//NOR_TRUNC
WriteData (0x3313),
//WriteComm (0x00F096),//AAMuxtable
//WriteData (0x0102),
//WriteData (0x0304),
//WriteData (0x0506),
//WriteData (0x0708),
//WriteData (0x090A),
//WriteData (0x0B0C),
//WriteData (0x0D0E),
WriteComm (0x00F096),//AAMuxtable
WriteData (0x0102),
WriteData (0x0405),
WriteData (0x0708),
WriteData (0x0A0B),
WriteData (0x0D0E),
WriteData (0x0B0C),
WriteData (0x0D0E),
WriteComm (0x00F0A4),//NoiseMuxtable
WriteData (0x3F3F),
WriteComm (0x00F0A6),
WriteData (0x3F3F),
WriteComm (0x00F0A8),//FingerMuxtable
WriteData (0x3F3F),
WriteData (0x3F3F),
WriteData (0x3F3F),
WriteData (0x3F3F),
WriteComm (0x00F0B6),//OFTV Trim noise unit -> finger unit
WriteData (0x544A),
WriteComm (0x00F500),//AA Freq Sel
WriteData (0x0021),
WriteComm (0x00F502),//Flush Freq Sel
WriteData (0xEEFF),
WriteComm (0x00F504),//Drive Mode 0
WriteData (0x1003),
WriteComm (0x00F508),// INT FR
WriteData (0x14D5),


/////////////////////////////////////////////////////
//////////Mux Modify 0x0102~0x0E02///////////////////
/////////////////////////////////////////////////////
WriteComm (0x00F506),//Unit 
WriteData (0x0A02), //0A: ACTIVE_UNIT_NUM[3:0]
/////////////////////////////////////////////////////
//////////Mux Modify 0x0102~0x0E02///////////////////
/////////////////////////////////////////////////////






//------Serial_Buff_WR------ 
WriteComm (0x00F512), 
WriteData (0x0001),
WriteComm (0x00F512), 
WriteData (0x0000),
Delay_ms (1),

WriteComm (0x00F000),//SensOn
WriteData (0x0001),

Delay_ms (120),


};

#else

#endif    