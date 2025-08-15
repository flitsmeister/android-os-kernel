/**
******************************************************************************
* @file    ms7210.c
* @author  
* @version V1.0.0
* @date    24-Nov-2020
* @brief   ms7210 SDK Library interfaces source file
* @history    
*
* Copyright (c) 2009-2020, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#include <linux/kernel.h>

#include "ms7210_comm.h"
#include "ms7210_drv_hdmi_tx.h"
#include "ms7210_drv_misc.h"
#include "ms7210_drv_dvin.h"
#include "ms7210_mpi.h"
#include "ms7210.h"
#include "debug_hdmi.h"


/****************************************/

/****************************************/
#if 0
#ifdef  MS_AUTO_BUILD_VERSION
#define MS7210_SDK_VERSION  MS_AUTO_BUILD_VERSION
#else
#define MS7210_SDK_VERSION __DATE__" "__TIME__
#endif
static __CODE CHAR g_sdk_version[] = {MS7210_SDK_VERSION};
#endif

static __CODE UINT8 g_u8_chip_addr[] = { 0xB2, 0x56 };
static __CODE UINT8 g_u8_chip_addr_num = sizeof(g_u8_chip_addr) / sizeof(UINT8);

/****************************************/
static BOOL g_b_input_valid = TRUE;
static VIDEOTIMING_T g_t_hdmirx_timing = {0x07,   1650,   750,   1280,   720,   7425,    6000,   260,    25,    40,   5};
static HDMI_CONFIG_T g_t_hdmirx_infoframe;

static DVIN_CONFIG_T g_t_dvin_config = { DVIN_CS_MODE_RGB, DVIN_BW_MODE_8_10_12BIT, DVIN_SQ_MODE_NONSEQ, DVIN_DR_MODE_DDR, DVIN_SY_MODE_HSVSDE };
//static DVIN_CONFIG_T g_t_dvin_config = { DVIN_CS_MODE_RGB,DVIN_BW_MODE_16_20_24BIT,DVIN_SQ_MODE_NONSEQ,DVIN_DR_MODE_SDR,DVIN_SY_MODE_HSVSDE};
static BOOL g_b_output_valid = FALSE;
static VIDEOTIMING_T g_t_hdmitx_timing;
static HDMI_CONFIG_T g_t_hdmitx_infoframe;
static UINT8 g_u8_tx_stable_timer_count = 0;
#define TX_STABLE_TIMEOUT (3)
/****************************************/
#if 0
CHAR* ms7210_sdk_version(VOID)
{
    return (CHAR*)g_sdk_version;
}
#endif
BOOL ms7210_chip_connect_detect(UINT8 u8_chip_addr)
{
    UINT8 i;
    for (i = 0; i < (u8_chip_addr ? 1 : g_u8_chip_addr_num); i++)
    {
        HAL_SetChipAddr(u8_chip_addr ? u8_chip_addr : g_u8_chip_addr[i]);
        if (ms7210drv_misc_package_sel_get() == 0x02)
        {
            if (ms7210drv_misc_chipisvalid())
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

VOID ms7210_dvin_init(DVIN_CONFIG_T *t_dvin_config, UINT8 u8_spdif_in)
{
    ms7210drv_misc_rc_freq_set();
    ms7210drv_csc_config_input((DVIN_CS_MODE_E)t_dvin_config->u8_cs_mode);
    if (ms7210drv_dvin_mode_config(t_dvin_config))
    {
        ms7210drv_hdmi_tx_phy_set_clk_ratio(1);
    }
    ms7210drv_misc_dig_pads_pull_set(2);
    ms7210drv_misc_audio_pad_in_spdif(u8_spdif_in);
    ms7210drv_hdmi_tx_shell_set_audio_mode(u8_spdif_in ? 1 : 0);
    ms7210drv_misc_freqm_pclk_enable();
}

VOID ms7210_dvin_data_swap(UINT8 u8_swap_mode)
{
    switch (u8_swap_mode)
    {
    case 0:
        ms7210drv_dvin_data_swap_all();
        break;

    case 1:
        ms7210drv_dvin_data_swap_rb_channel();
        break;

    case 2:
        ms7210drv_dvin_data_swap_yc_channel();
        break;
    }
}

VOID ms7210_dvin_phase_adjust(BOOL b_invert, UINT8 u8_delay)
{
    ms7210drv_dvin_pa_adjust(b_invert, u8_delay);
}

BOOL ms7210_dvin_timing_get(DVIN_TIMING_DET_T *t_dvin_det)
{
    memset(t_dvin_det, 0, sizeof(DVIN_TIMING_DET_T));
    t_dvin_det->u16_pixclk = ms7210drv_misc_freqm_pclk_get();
    if (t_dvin_det->u16_pixclk > 500)
    {
        if (ms7210drv_hdmi_tx_pll_lock_status())
        {
            ms7210drv_dvin_timing_detect(t_dvin_det);
        }
        else
        {
            ms7210drv_hdmi_tx_phy_config(t_dvin_det->u16_pixclk);
        }
    }
    if ((t_dvin_det->u16_hactive < 50) || (t_dvin_det->u16_vtotal < 50) || (t_dvin_det->u16_htotal < t_dvin_det->u16_hactive))
    {
        return FALSE;
    }
    return TRUE;
}

VOID ms7210_dvin_timing_config(DVIN_CONFIG_T *t_dvin_config, VIDEOTIMING_T *ptTiming, HDMI_CONFIG_T *pt_hdmi_tx)
{
    ms7210drv_hdmi_tx_phy_set_clk_ratio((UINT8)ms7210drv_dvin_timing_config(t_dvin_config, ptTiming, &pt_hdmi_tx->u8_clk_rpt));
    pt_hdmi_tx->u16_video_clk = ptTiming->u16_pixclk;
}

VOID ms7210_dvin_video_config(BOOL b_config)
{
    ms7210drv_dvin_clk_reset_release(b_config);
}

BOOL ms7210_hdmitx_hpd_detect(VOID)
{
    return ms7210drv_hdmi_tx_shell_hpd();
}

BOOL ms7210_hdmitx_edid_get(UINT8 *u8_edid)
{
    BOOL b_succ = FALSE;
    HDMI_EDID_FLAG_T pt_edid;
    
    if (u8_edid != NULL)
    {
        b_succ = ms7210drv_hdmi_tx_parse_edid(u8_edid, &pt_edid);
    }

    return b_succ;
}

BOOL ms7210_hdmitx_input_timing_stable_get(VOID)
{
    return ms7210drv_hdmi_tx_shell_timing_stable();
}

VOID ms7210_hdmitx_output_config(HDMI_CONFIG_T *pt_hdmi_tx)
{
    ms7210drv_hdmi_tx_phy_output_enable(FALSE);
    ms7210drv_hdmi_tx_hdcp_enable(FALSE);
    ms7210drv_hdmi_tx_shell_set_gcp_packet_avmute(FALSE);

    ms7210drv_csc_config_output((HDMI_CS_E)pt_hdmi_tx->u8_color_space);

    ms7210drv_hdmi_tx_phy_config(pt_hdmi_tx->u16_video_clk);
    ms7210drv_hdmi_tx_shell_config(pt_hdmi_tx);

    ms7210drv_hdmi_tx_shell_video_mute_enable(FALSE);
    ms7210drv_hdmi_tx_shell_audio_mute_enable(FALSE);
    ms7210drv_hdmi_tx_phy_output_enable(TRUE);
}

VOID ms7210_hdmitx_shutdown_output(VOID)
{
    ms7210drv_hdmi_tx_phy_output_enable(FALSE);
    ms7210drv_hdmi_tx_phy_power_down();
    //
    ms7210drv_hdmi_tx_hdcp_enable(FALSE);
    //_hdmi_tx_hdcp_param_default(u8_output_chn);
}

VOID ms7210_init_test(VOID)
{
	HDMITX_DEBUG_PRINTF("ms7210 chip connect = %d\n", ms7210_chip_connect_detect(0x56));
    ms7210_dvin_init(&g_t_dvin_config, 0);
	g_b_output_valid = FALSE;
    //ms7210_dvin_phase_adjust(FALSE, 2);
    //ms7210_dvin_data_swap(2);
}

VOID ms7210_media_service(VOID)
{
    BOOL b_outpit_valid = g_b_output_valid;
    if (g_b_input_valid && b_outpit_valid)
    {
        if (g_u8_tx_stable_timer_count < TX_STABLE_TIMEOUT)
        {
            g_u8_tx_stable_timer_count++;
            return;
        }
        if (!ms7210_hdmitx_input_timing_stable_get())
        {
            b_outpit_valid = FALSE;
            g_u8_tx_stable_timer_count = 0;
            HDMITX_DEBUG_PRINTF("output unstable\n");
        }
    }
    if (g_b_input_valid != b_outpit_valid)
    {
        if (g_b_input_valid)
        {
            if (!g_b_output_valid)
            {
                g_t_hdmitx_timing = g_t_hdmirx_timing;
                g_t_hdmitx_infoframe = g_t_hdmirx_infoframe;
                ms7210_dvin_timing_config(&g_t_dvin_config, &g_t_hdmitx_timing, &g_t_hdmitx_infoframe);
                ms7210_dvin_video_config(TRUE);
                g_t_hdmitx_infoframe.u8_color_space = MS7210_HDMI_RGB;
                g_t_hdmitx_infoframe.u8_color_depth = 0;
				g_t_hdmitx_infoframe.u8_hdmi_flag = TRUE;
            }
            ms7210_hdmitx_output_config(&g_t_hdmitx_infoframe);
            g_u8_tx_stable_timer_count = 0;
            HDMITX_DEBUG_PRINTF("config output\n");
        }
        else
        {
            ms7210_hdmitx_shutdown_output();
            ms7210_dvin_video_config(FALSE);
            HDMITX_DEBUG_PRINTF("shutdown output\n");
        }
        g_b_output_valid = g_b_input_valid;
    }
}

