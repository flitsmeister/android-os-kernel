/**
******************************************************************************
* @file    ms7210_drv_hdmi_tx.h
* @author  
* @version V1.0.0
* @date    15-Nov-2016
* @brief   hdmi tx module driver declare
* @history 
*
* Copyright (c) 2009 - 2014, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#ifndef __MACROSILICON_MS7210_DRV_HDMI_TX_H__
#define __MACROSILICON_MS7210_DRV_HDMI_TX_H__


#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
*  Function name:   ms7210drv_hdmi_tx_set_channel
*  Description:     select which hdmi tx channel need to be configed
*  Entry:           [in]u8_chn: enum refer to HDMI_CHANNEL_E
*               
*  Returned value:  None
*  Remark:
***************************************************************/
MS7210_DRV_API VOID ms7210drv_hdmi_tx_set_channel(UINT8 u8_chn);


/***************************************************************
*  Function name:   ms7210drv_hdmi_tx_get_channel
*  Description:     get current configured channel id
*  Entry:           [in]None
*               
*  Returned value:  return UINT8 value enum refer to HDMI_CHANNEL_E
*  Remark:
***************************************************************/
MS7210_DRV_API UINT8 ms7210drv_hdmi_tx_get_channel(VOID);


/***************************************************************
*  Function name:   ms7210drv_hdmi_tx_phy_output_enable
*  Description:     hdmi tx module output timing on/off
*  Entry:           [in]b_enable: if true turn on 
*                                 else turn off
*               
*  Returned value:  None
*  Remark:
***************************************************************/
MS7210_DRV_API VOID ms7210drv_hdmi_tx_phy_output_enable(BOOL b_enable);


MS7210_DRV_API VOID ms7210drv_hdmi_tx_phy_data_R200_enable(BOOL b_enable);

//u8_main_pre: 0~7; u8_main_po:0~15
MS7210_DRV_API VOID ms7210drv_hdmi_tx_phy_clk_drive_config(UINT8 u8_main_pre, UINT8 u8_main_po);

//u8_main_pre: 0~7; u8_main_po,u8_post_po:0~15
MS7210_DRV_API VOID ms7210drv_hdmi_tx_phy_data_drive_config(UINT8 u8_main_pre, UINT8 u8_main_po, UINT8 u8_post_po);


MS7210_DRV_API VOID ms7210drv_hdmi_tx_phy_data_drive_enhance(BOOL b_enable);


MS7210_DRV_API VOID ms7210drv_hdmi_tx_phy_output_auto_ctrl(UINT8 u8_ctrl_mode, BOOL b_auto);

MS7210_DRV_API BOOL ms7210drv_hdmi_tx_pll_lock_status(VOID);


/***************************************************************
*  Function name:   ms7210drv_hdmi_tx_shell_video_mute_enable
*  Description:     hdmi tx shell module video mute
*  Entry:           [in]b_enable, if true mute screen else normal video
*
*  Returned value:  None
*  Remark:
***************************************************************/
MS7210_DRV_API VOID ms7210drv_hdmi_tx_shell_video_mute_enable(BOOL b_enable);


/***************************************************************
*  Function name:   ms7210drv_hdmi_tx_shell_audio_mute_enable
*  Description:     hdmi tx shell module audio mute
*  Entry:           [in]b_enable, if true mute audio else normal audio
*
*  Returned value:  None
*  Remark:
***************************************************************/
MS7210_DRV_API VOID ms7210drv_hdmi_tx_shell_audio_mute_enable(BOOL b_enable);


MS7210_DRV_API VOID ms7210drv_hdmi_tx_shell_set_hdmi_out(BOOL b_hdmi_out);

MS7210_DRV_API VOID ms7210drv_hdmi_tx_shell_set_gcp_packet_avmute(BOOL b_mute);

/***************************************************************
*  Function name:   ms7210drv_hdmi_tx_config
*  Description:     hdmi tx config
*  Entry:           [in]pt_hdmi_tx: struct refer to HDMI_CONFIG_T
*                      
*  Returned value:  None
*  Remark:
***************************************************************/
MS7210_DRV_API VOID ms7210drv_hdmi_tx_phy_config(UINT16 u16_video_clk);
MS7210_DRV_API VOID ms7210drv_hdmi_tx_shell_config(HDMI_CONFIG_T *pt_hdmi_tx);


/***************************************************************
*  Function name:   ms7210drv_hdmi_tx_audio_config
*  Description:     hdmi tx audio config
*  Entry:           [in]pt_hdmi_tx: struct refer to HDMI_CONFIG_T
*                      
*  Returned value:  None
*  Remark:
***************************************************************/
MS7210_DRV_API VOID ms7210drv_hdmi_tx_audio_config(HDMI_CONFIG_T *pt_hdmi_tx);
MS7210_DRV_API VOID ms7210drv_hdmi_tx_shell_set_audio_mode(UINT8 u8_audio_mode);


MS7210_DRV_API VOID ms7210drv_hdmi_tx_clk_sel(UINT8 u8_sel);


MS7210_DRV_API VOID ms7210drv_hdmi_tx_phy_set_clk_ratio(UINT8 u8_ratio);


MS7210_DRV_API VOID ms7210drv_hdmi_tx_audio_fs_update(UINT8 u8_audio_rate);


MS7210_DRV_API VOID ms7210drv_hdmi_tx_shell_set_audio_cbyte_from_channel(UINT8 u8_tx_chn);
    

MS7210_DRV_API UINT8 ms7210drv_hdmi_tx_shell_get_audio_cbyte_status(VOID);


/***************************************************************
*  Function name:   ms7210drv_hdmi_tx_phy_power_down
*  Description:     hdmi tx module power down
*  Entry:           [in]None
*               
*  Returned value:  None
*  Remark:
***************************************************************/ 
MS7210_DRV_API VOID ms7210drv_hdmi_tx_phy_power_down(VOID);


/***************************************************************
*  Function name:   ms7210drv_hdmi_tx_shell_hpd
*  Description:     hdmi tx hot plug detection
*  Entry:           [in]
*               
*  Returned value:  if hdmi cable plug in return true, else return false
*  Remark:
***************************************************************/ 
MS7210_DRV_API BOOL ms7210drv_hdmi_tx_shell_hpd(VOID);


MS7210_DRV_API BOOL ms7210drv_hdmi_tx_shell_timing_stable(VOID);


MS7210_DRV_API BOOL ms7210drv_hdmi_tx_ddc_is_busy(VOID);


/***************************************************************
*  Function name:   ms7210drv_hdmi_tx_ddc_enable
*  Description:     hdmi tx module DDC enable
*  Entry:           [in]b_enable, if true enable ddc, else disable
*               
*  Returned value:  None
*  Remark:
***************************************************************/ 
MS7210_DRV_API VOID ms7210drv_hdmi_tx_ddc_enable(BOOL b_enable);


/***************************************************************
*  Function name:   ms7210drv_hdmi_tx_hdcp_enable
*  Description:     hdmi hdcp enable
*  Entry:           [in]b_enable, if true enable hdcp, else disable
*               
*  Returned value:  None
*  Remark:
***************************************************************/
MS7210_DRV_API VOID ms7210drv_hdmi_tx_hdcp_enable(BOOL b_enable);


/***************************************************************
*  Function name:   ms7210drv_hdmi_tx_hdcp_init
*  Description:     hdmi hdcp init
*  Entry:           [in]p_u8_key, 280 bytes hdmi tx key
*                       p_u8_ksv, 5 bytes hdmi tx ksv
*
*  Returned value:  if hdcp init success return true, else return false
*  Remark:
***************************************************************/
MS7210_DRV_API BOOL ms7210drv_hdmi_tx_hdcp_init(UINT8 *p_u8_key, UINT8 *p_u8_ksv);


/***************************************************************
*  Function name:   ms7210drv_hdmi_tx_hdcp_get_status
*  Description:     hdmi hdcp get tx/rx Ri verify result, 2s period polled
*  Entry:           [out]pt, refer to HDMI_HDCP_RI
*
*  Returned value:  if verify sucess return 0x01, else return 0x00
*  Remark:
***************************************************************/
MS7210_DRV_API UINT8 ms7210drv_hdmi_tx_hdcp_get_status(HDMI_HDCP_RI *pt);


/***************************************************************
*  Function name:   ms7210drv_hdmi_tx_parse_edid
*  Description:     parse hdmi sink edid
*  Entry:           [out]p_u8_edid_buf, buf for EDID, 256bytes
*                        pt_edid, refer to HDMI_EDID_FLAG_T
*
*  Returned value:  if parse sucess return 0x01, else return 0x00
*  Remark:
***************************************************************/
MS7210_DRV_API BOOL ms7210drv_hdmi_tx_parse_edid(UINT8 *p_u8_edid_buf, HDMI_EDID_FLAG_T *pt_edid);


#ifdef __cplusplus
}
#endif

#endif //__MACROSILICON_MS7210_DRV_HDMI_TX_H__
