/**
******************************************************************************
* @file    ms7210_typedef.h
* @author  
* @version V1.0.0
* @date    24-Nov-2020
* @brief   Definitions for typedefs.
*
* Copyright (c) 2009-2014, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#ifndef __MACROSILICON_MS7210_COMMON_TYPEDEF_H__
#define __MACROSILICON_MS7210_COMMON_TYPEDEF_H__

/*
** Global typedefs.
*/

#ifndef MS_GLOBAL_DEFINE
#define MS_GLOBAL_DEFINE

#ifndef NULL
#define NULL ((void*)0)
#endif

// For ARM platform
#if defined (_PLATFORM_ARM_)
#define  __CODE const
#define  __XDATA
#define  __DATA
#define __IDATA
#define  __NEAR
#define  __IO volatile


typedef _Bool BOOL;

#elif defined (__STD_GCC__)
#define  __CODE const
#define  __XDATA
#define  __DATA
#define __IDATA
#define  __NEAR
#define  __IO volatile


typedef _Bool BOOL;

#elif defined (_PLATFORM_WINDOWS_)
#define  __CODE
#define  __XDATA
#define  __DATA
#define __IDATA
#define  __NEAR
#define  __IO

#elif defined (__KEIL_C__)
#define __CODE code
#define __XDATA xdata
#define __DATA data
#define __IDATA idata
#define __NEAR
#define __IO volatile

//bool bype
typedef bit BOOL;

#elif defined (__CSMC__)
#define __CODE const
#define __XDATA
#define __DATA 
#define __IDATA 
#define __NEAR @near
#define __IO volatile

//bool bype
typedef _Bool BOOL;
#elif defined (_IAR_)
#define __CODE const
#define __XDATA
#define __DATA 
#define __IDATA 
#define __NEAR @near
#define __IO volatile

//bool bype
typedef _Bool BOOL;
#endif // end of compiler platform define 


//unsigned integer type
typedef unsigned char UINT8;
typedef char          CHAR;
typedef unsigned short UINT16;

//signed integer type
typedef signed char INT8;
typedef signed short INT16;

//32bit type
#if defined (_PLATFORM_ARM_) || defined (_PLATFORM_WINDOWS_)
typedef unsigned int UINT32;
typedef signed int INT32;
#else
typedef unsigned long int UINT32;
typedef signed long int INT32;
#endif

#define VOID void

#define FALSE 0
#define TRUE  1

#define DISABLE 0
#define ENABLE  1

#define LOW     0
#define HIGH    1

#define OFF     0
#define ON      1


// Helper macros.
#define _UNUSED_(arg)     ((arg) = (arg))

#ifndef _countof
#define _countof(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))
#endif

#ifndef max
#define max(a, b)   (((a)>(b))?(a):(b)) 
#endif

#ifndef min
#define min(a, b)   (((a)<(b))?(a):(b))
#endif



/*
* generic mask macro definitions
*/
#define MSRT_BIT0                   (0x01)
#define MSRT_BIT1                   (0x02)
#define MSRT_BIT2                   (0x04)
#define MSRT_BIT3                   (0x08)
#define MSRT_BIT4                   (0x10)
#define MSRT_BIT5                   (0x20)
#define MSRT_BIT6                   (0x40)
#define MSRT_BIT7                   (0x80)
    
#define MSRT_MSB8BITS               MSRT_BIT7
#define MSRT_LSB                    MSRT_BIT0
    
// Bit7 ~ Bit0
#define MSRT_BITS7_6                (0xC0)
#define MSRT_BITS7_5                (0xE0)
#define MSRT_BITS7_4                (0xF0)
#define MSRT_BITS7_3                (0xF8)
#define MSRT_BITS7_2                (0xFC)
#define MSRT_BITS7_1                (0xFE)
#define MSRT_BITS7_0                (0xff)

#define MSRT_BITS6_5                (0x60)
#define MSRT_BITS6_4                (0x70)
#define MSRT_BITS6_2                (0x7c)
#define MSRT_BITS6_1                (0x7e)
#define MSRT_BITS6_0                (0x7f)

#define MSRT_BITS5_4                (0x30)
#define MSRT_BITS5_3                (0x38)
#define MSRT_BITS5_2                (0x3c)
#define MSRT_BITS5_0                (0x3f)

#define MSRT_BITS4_3                (0x18)
#define MSRT_BITS4_2                (0x1c)
#define MSRT_BITS4_1                (0x1e)
#define MSRT_BITS4_0                (0x1f)

#define MSRT_BITS3_2                (0x0C)
#define MSRT_BITS3_1                (0x0E)
#define MSRT_BITS3_0                (0x0F)

#define MSRT_BITS2_1                (0x06)
#define MSRT_BITS2_0                (0x07)

#define MSRT_BITS1_0                (0x03)

#endif


// 20121207, for video data type 
#ifndef MS_TIMING_DEFINE
#define MS_TIMING_DEFINE

typedef struct _T_MS_VIDEO_SIZE_
{
    UINT16 u16_h;
    UINT16 u16_v;
} VIDEOSIZE_T;

typedef struct _T_MS7210_VIDEO_TIMING_
{
    UINT8           u8_polarity;
    UINT16          u16_htotal;
    UINT16          u16_vtotal;
    UINT16          u16_hactive;
    UINT16          u16_vactive;
    UINT16          u16_pixclk;     /*10000hz*/
    UINT16          u16_vfreq;      /*0.01hz*/
    UINT16          u16_hoffset;    /* h sync start to h active*/
    UINT16          u16_voffset;    /* v sync start to v active*/
    UINT16          u16_hsyncwidth;
    UINT16          u16_vsyncwidth;
} VIDEOTIMING_T;

typedef enum _E_SYNC_POLARITY_
{
    ProgrVNegHNeg = 0x01,
    ProgrVNegHPos = 0x03,
    ProgrVPosHNeg = 0x05,
    ProgrVPosHPos = 0x07,

    InterVNegHNeg = 0x00,
    InterVNegHPos = 0x02,
    InterVPosHNeg = 0x04,
    InterVPosHPos = 0x06
}SYNCPOLARITY_E;

typedef struct _T_WIN_BORDER_
{
    INT16 top;
    INT16 bottom;
    INT16 left;
    INT16 right;
} WINBORDER_T;

#endif


//
//HDMI video
#ifndef MS_HDMI_DEFINE
#define MS_HDMI_DEFINE

/* For MTK drvice*/
//UINT8 MS7210_HPD_Status = FALSE;

typedef struct _T_MISC_TIMING_
{
    UINT8           u8_vic;
    VIDEOTIMING_T   st_timing;
}MISCTIMING_T;

typedef enum _E_MS7210_VIDEO_FORMAT_
{
    VFMT_CEA_NULL                        = 0,      
    VFMT_CEA_02_720x480P_60HZ            ,  //has been include in timing table 
    VFMT_CEA_04_1280x720P_60HZ           ,  //include 
    VFMT_CEA_05_1920x1080I_60HZ          ,  //include 
    VFMT_CEA_06_720x480I_60HZ            ,  //include   
    VFMT_CEA_16_1920x1080P_60HZ          ,  //include
    VFMT_CEA_17_720x576P_50HZ            ,  //include 
    VFMT_CEA_19_1280x720P_50HZ           ,  //include
    VFMT_CEA_20_1920x1080I_50HZ          ,  //include
    VFMT_CEA_21_720x576I_50HZ            ,  //include  
    VFMT_CEA_31_1920x1080P_50HZ          , //include 
    VFMT_CEA_32_1920x1080P_24HZ          , //include 
    VFMT_CEA_33_1920x1080P_25HZ          , //include 
    VFMT_CEA_34_1920x1080P_30HZ          , //include  
    VFMT_CEA_39_1920x1080I_50HZ          , //include
    VFMT_CEA_60_1280x720P_24HZ           ,  
    VFMT_CEA_61_1280x720P_25HZ           ,  
    VFMT_CEA_62_1280x720P_30HZ           ,
                               
    VFMT_INVALID                         = 0xFF
}MS7210_VIDEOFORMAT_E;

static VIDEOTIMING_T __CODE g_arrTimingTable[] =
	{
    //===============================================================================================================================================
    //                    Total           Active             Freq          Offset        Sync
    //  Polarity        H       V        H      V      pixclk     V       H      V      H     V
    //===============================================================================================================================================
	{ProgrVNegHNeg,    858,    525,    720,    480,    2700,    6000,   122,    36,    62,   6}, // 2 - 720  x 480p@60                   
    {ProgrVPosHPos,   1650,    750,   1280,    720,    7425,    6000,   260,    25,    40,   5}, // 4 - 1280 x 720p@60Hz            
    {InterVPosHPos,   2200,   1125,   1920,   1080,    7425,    6000,   192,    20,    44,   5}, // 5 - 1920 x 1080i@60             
    {InterVNegHNeg,   1716,    525,   1440,    480,    2700,    6000,   238,    18,   124,   3}, // 6 - 1440 x 480i@60              
    {ProgrVPosHPos,   2200,   1125,   1920,   1080,   14835,    6000,   192,    41,    44,   5}, // 16 - 1920 x 1080p@60            
    {ProgrVNegHNeg,    864,    625,    720,    576,    2700,    5000,   132,    44,    64,   5}, // 17 - 720  x 576p@50             
    {ProgrVPosHPos,   1980,    750,   1280,    720,    7425,    5000,   260,    25,    40,   5}, // 19 - 1280 x 720p@50Hz           
    {InterVPosHPos,   2640,   1125,   1920,   1080,    7425,    5000,   192,    20,    44,   5}, // 20 - 1920 x 1080i@50Hz          
    {InterVNegHNeg,   1728,    625,   1440,    576,    2700,    5000,   264,    22,   126,   3}, // 21 - 1440 x 576i@50             
    {ProgrVPosHPos,   2640,   1125,   1920,   1080,   14850,    5000,   192,    41,    44,   5}, // 31 - 1920 x 1080p@50            
    {ProgrVPosHPos,   2750,   1125,   1920,   1080,    7417,    2400,   192,    41,    44,   5}, // 32 - 1920 x 1080p@24            
    {ProgrVPosHPos,   2640,   1125,   1920,   1080,    7425,    2500,   192,    41,    44,   5}, // 33 - 1920 x 1080p@25            
    {ProgrVPosHPos,   2200,   1125,   1920,   1080,    7417,    3000,   192,    41,    44,   5}, // 34 - 1920 x 1080p@30            
    {InterVNegHNeg,   2304,   1250,   1920,   1080,    7200,    5000,   352,    62,    168,  5}, // 39 - 1920 x 1080i@50
};

//
typedef enum _E_HDMI_VIDEO_CLK_REPEAT_
{
    HDMI_X1CLK      = 0x00,
    HDMI_X2CLK      = 0x01,
    HDMI_X3CLK      = 0x02,
    HDMI_X4CLK      = 0x03,
    HDMI_X5CLK      = 0x04,
    HDMI_X6CLK      = 0x05,
    HDMI_X7CLK      = 0x06,
    HDMI_X8CLK      = 0x07,
    HDMI_X9CLK      = 0x08,
    HDMI_X10CLK     = 0x09
}HDMI_CLK_RPT_E;

typedef enum _E_HDMI_VIDEO_ASPECT_RATIO_
{
    HDMI_4X3     = 0x01,
    HDMI_16X9    = 0x02
}HDMI_ASPECT_RATIO_E;


typedef enum _E_HDMI_VIDEO_SCAN_INFO_
{
    HDMI_OVERSCAN     = 0x01,    //television type
    HDMI_UNDERSCAN    = 0x02     //computer type
}HDMI_SCAN_INFO_E;

typedef enum _E_HDMI_COLOR_SPACE_
{
    MS7210_HDMI_RGB        = 0x00,
    HDMI_YCBCR422   = 0x01,
    HDMI_YCBCR444   = 0x02,
    HDMI_YUV420     = 0x03
}HDMI_CS_E;

typedef enum _E_HDMI_COLOR_DEPTH_
{
    HDMI_COLOR_DEPTH_8BIT    = 0x00,
    HDMI_COLOR_DEPTH_10BIT   = 0x01,
    HDMI_COLOR_DEPTH_12BIT   = 0x02,
    HDMI_COLOR_DEPTH_16BIT   = 0x03
}HDMI_COLOR_DEPTH_E;

typedef enum _E_HDMI_COLORIMETRY_
{
    HDMI_COLORIMETRY_601    = 0x00,
    HDMI_COLORIMETRY_709    = 0x01,
    HDMI_COLORIMETRY_656    = 0x02,
    HDMI_COLORIMETRY_1120   = 0x03,
    HDMI_COLORIMETRY_SMPTE  = 0x04,
    HDMI_COLORIMETRY_XVYCC601 = 0x05,
    HDMI_COLORIMETRY_XVYCC709 = 0x06
}HDMI_COLORIMETRY_E;

//HDMI vendor specific
typedef enum _E_HDMI_VIDEO_FORMAT_
{
    HDMI_NO_ADD_FORMAT,
    HDMI_4Kx2K_FORMAT,
    HDMI_3D_FORMAT
}HDMI_VIDEO_FORMAT_E;

typedef enum _E_HDMI_4Kx2K_VIC_
{
    HDMI_4Kx2K_30HZ = 0x01,
    HDMI_4Kx2K_25HZ,
    HDMI_4Kx2K_24HZ,
    HDMI_4Kx2K_24HZ_SMPTE
}HDMI_4Kx2K_VIC_E;

typedef enum _E_HDMI_3D_STRUCTURE_
{
    HDMI_FRAME_PACKING,
    HDMI_FIELD_ALTERNATIVE,
    HDMI_LINE_ALTERNATIVE,
    HDMI_SIDE_BY_SIDE_FULL,
    L_DEPTH,
    L_DEPTH_GRAPHICS,
    SIDE_BY_SIDE_HALF = 8
}HDMI_3D_STRUCTURE_E;

//HDMI audio
typedef enum _E_HDMI_AUDIO_MODE_
{
    HDMI_AUD_MODE_AUDIO_SAMPLE  = 0x00,
    HDMI_AUD_MODE_HBR           = 0x01,
    HDMI_AUD_MODE_DSD           = 0x02,
    HDMI_AUD_MODE_DST           = 0x03
}HDMI_AUDIO_MODE_E;

typedef enum _E_HDMI_AUDIO_I2S_RATE_
{
    HDMI_AUD_RATE_44K1  = 0x00,
    HDMI_AUD_RATE_48K   = 0x02,
    HDMI_AUD_RATE_32K   = 0x03,
    HDMI_AUD_RATE_88K2  = 0x08,
    HDMI_AUD_RATE_96K   = 0x0A,
    HDMI_AUD_RATE_176K4 = 0x0C,
    HDMI_AUD_RATE_192K  = 0x0E
}HDMI_AUDIO_RATE_E;

typedef enum _E_HDMI_AUDIO_LENGTH_
{
    HDMI_AUD_LENGTH_16BITS    = 0x00,
    HDMI_AUD_LENGTH_20BITS    = 0x01,
    HDMI_AUD_LENGTH_24BITS    = 0x02
}HDMI_AUDIO_LENGTH_E;

typedef enum _E_HDMI_AUDIO_CHANNEL_
{
    HDMI_AUD_2CH    = 0x01,
    HDMI_AUD_3CH    = 0x02,
    HDMI_AUD_4CH    = 0x03,
    HDMI_AUD_5CH    = 0x04,
    HDMI_AUD_6CH    = 0x05,
    HDMI_AUD_7CH    = 0x06,
    HDMI_AUD_8CH    = 0x07
}HDMI_AUDIO_CHN_E;


typedef struct _T_HDMI_CONFIG_PARA_
{   
    UINT8  u8_hdmi_flag;          // FALSE = dvi out;  TRUE = hdmi out
    UINT8  u8_vic;                // reference to CEA-861 VIC
    UINT16 u16_video_clk;         // TMDS video clk, uint 10000Hz
    UINT8  u8_clk_rpt;            // enum refer to HDMI_CLK_RPT_E. X2CLK = 480i/576i, others = X1CLK
    UINT8  u8_scan_info;          // enum refer to HDMI_SCAN_INFO_E
    UINT8  u8_aspect_ratio;       // enum refer to HDMI_ASPECT_RATIO_E
    UINT8  u8_color_space;        // enum refer to HDMI_CS_E
    UINT8  u8_color_depth;        // enum refer to HDMI_COLOR_DEPTH_E
    UINT8  u8_colorimetry;        // enum refer to HDMI_COLORIMETRY_E. IT601 = 480i/576i/480p/576p, ohters = IT709
    //
    UINT8  u8_video_format;       // enum refer to HDMI_VIDEO_FORMAT_E
    UINT8  u8_4Kx2K_vic;          // enum refer to HDMI1.4 extented resolution transmission
    UINT8  u8_3D_structure;       // enum refer to HDMI_3D_STRUCTURE_E
    //
    UINT8  u8_audio_mode;         // enum refer to HDMI_AUDIO_MODE_E
    UINT8  u8_audio_rate;         // enum refer to HDMI_AUDIO_RATE_E
    UINT8  u8_audio_bits;         // enum refer to HDMI_AUDIO_LENGTH_E
    UINT8  u8_audio_channels;     // enum refer to HDMI_AUDIO_CHN_E
    UINT8  u8_audio_speaker_locations;  // 0~255, refer to CEA-861 audio infoframe, BYTE4
}HDMI_CONFIG_T;
#endif


#ifndef MS_HDMITX_DEFINE
#define MS_HDMITX_DEFINE

//HDMI TX module define

//HDMI TX channel
typedef enum _E_HDMI_TX_CHANNEL_
{
    HDMI_TX_CHN0      = 0x00,
    HDMI_TX_CHN1      = 0x01,
    HDMI_TX_CHN2      = 0x02,
    HDMI_TX_CHN3      = 0x03
}HDMI_CHANNEL_E;

typedef struct _T_HDMI_HDCP_RI_
{   
    UINT8 TX_Ri0;
    UINT8 TX_Ri1;
    UINT8 RX_Ri0;
    UINT8 RX_Ri1;
}HDMI_HDCP_RI;

//HDMI EDID
typedef struct _T_HDMI_EDID_FLAG_
{   
    UINT8    u8_hdmi_sink;              //1 = HDMI sink, 0 = dvi
    UINT8    u8_color_space;            //color space support flag, flag 1 valid. BIT5: YCBCR444 flag; BIT4: YCBCR422 flag.(RGB must be support)
    //
    UINT8    u8_edid_total_blocks;      //block numbers, 128bytes in one block
    UINT16   u16_preferred_pixel_clk;   //EDID Preferred pixel clock rate, u16_preferred_pixel_clk * 10000Hz, ERROR code is 0xFFFF
    UINT32   u32_preferred_timing;      //EDID Preferred Timing (Hact*Vact)
    UINT8    u8_max_tmds_clk;           //HDMI VSDB max tmds clock, u8_max_tmds_clk * 5 Mhz
    UINT32   u32_max_video_block_timing;//EDID max video block timing (Hact*Vact)
    UINT8    u8_hdmi_2_0_flag;          //1 = HDMI 2.0
}HDMI_EDID_FLAG_T;
#endif


//dvin
#ifndef MS_DVIN_DEFINE
#define MS_DVIN_DEFINE

typedef struct T_DVIN_CONFIG
{
    UINT8 u8_cs_mode;  //refer to DVIN_CS_MODE_E
    UINT8 u8_bw_mode;  //refer to DVIN_BW_MODE_E
    UINT8 u8_sq_mode;  //refer to DVIN_SQ_MODE_E
    UINT8 u8_dr_mode;  //refer to DVIN_DR_MODE_E
    UINT8 u8_sy_mode;  //refer to DVIN_SY_MODE_E
}DVIN_CONFIG_T;

typedef enum _E_DVIN_CS_MODE_
{
    DVIN_CS_MODE_RGB,
    DVIN_CS_MODE_YUV444,
    DVIN_CS_MODE_YUV422
}DVIN_CS_MODE_E;

typedef enum _E_DVIN_BW_MODE_
{
    DVIN_BW_MODE_16_20_24BIT,
    DVIN_BW_MODE_8_10_12BIT
}DVIN_BW_MODE_E;

typedef enum _E_DVIN_SQ_MODE_
{
    DVIN_SQ_MODE_NONSEQ,
    DVIN_SQ_MODE_SEQ
}DVIN_SQ_MODE_E;

typedef enum _E_DVIN_DR_MODE_
{
    DVIN_DR_MODE_SDR,
    DVIN_DR_MODE_DDR
}DVIN_DR_MODE_E;

typedef enum _E_DVIN_SY_MODE_
{
    DVIN_SY_MODE_HSVSDE,      // 8/16/24-bit BT601
    DVIN_SY_MODE_HSVS,
    DVIN_SY_MODE_VSDE,        // non suport interlace mode
    DVIN_SY_MODE_DEONLY,
    DVIN_SY_MODE_EMBEDDED,    // 16-bit BT1120 or 8bit BT656
    DVIN_SY_MODE_2XEMBEDDED,  // 8-bit BT1120
    DVIN_SY_MODE_BTAT1004     // 16-bit BTA-T1004
}DVIN_SY_MODE_E;

typedef struct T_DVIN_TIMING_DET
{
    UINT16 u16_htotal;
    UINT16 u16_vtotal;
    UINT16 u16_hactive;
    UINT16 u16_pixclk;     /*10000hz*/
}DVIN_TIMING_DET_T;

#endif

#endif  // __MACROSILICON_MS7210_COMMON_TYPEDEF_H__
