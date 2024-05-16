// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Joey Pan <joey.pan@mediatek.com>
 */

#include "mt65xx_lcm_list.h"
#include <lcm_drv.h>
#ifdef BUILD_LK
#include <platform/disp_drv_platform.h>
#else
#include <linux/delay.h>
/* #include <mach/mt_gpio.h> */
#endif
enum LCM_DSI_MODE_CON lcm_dsi_mode;

/* used to identify float ID PIN status */
#define LCD_HW_ID_STATUS_LOW      0
#define LCD_HW_ID_STATUS_HIGH     1
#define LCD_HW_ID_STATUS_FLOAT 0x02
#define LCD_HW_ID_STATUS_ERROR  0x03

#if defined(CONFIG_MACH_MT6739)//for MT8765 lcm config
struct LCM_DRIVER *lcm_driver_list[] = {
    &gs717I_xhd_jd9365da_boe_wxga_ips_8_lcm_drv,
    &gs717I_SAT080_JD9365DA_boe_wxga_ips_8_lcm_drv,
    &gs717I_xhd_zs080ny_er88577_boe_wxga_ips_8_lcm_drv,
	&gs716_hnh_jc08021701_31a_boe_wxga_ips_8_lcm_drv,
	&gs716_hnh_hc08021016_31a_boe_wxga_ips_8_lcm_drv,
	&gs716_wh_hx8394a_wxga_ips_8_lcm_drv,
	&gs716_wh_jd9365_cpt_wxga_ips_8_lcm_drv,
	&gs716_sat_sc7705_boe_wxga_ips_8_lcm_drv,
	&ek79007_lcm_drv,
	&gs716_bns_ili9881c_boe_wxga_ips_101_lcm_drv,
	&gs716_bns_gh8555bl_wxga_ips_101_lcm_drv,
	&gs716_fx_ili9881_im2byl806_wxga_ips_8_lcm_drv,
	&gs716_sq_jd9365_boe_wxga_ips_101_lcm_drv,
	&gs716_hnh_bc08028016_31b_wxga_ips_8_lcm_drv,
	&gs717_hnh_er88577_wxga_ips_8_lcm_drv,
	&gs717_sat_jd9365da_wxganl_ips_8_lcm_drv,
	&gs717_hnh_er88577_boe_wxga_ips_101_lcm_drv,
	&gs717_xdf_jd9365da_boe_wxga_ips_101_lcm_drv,
	&gs717_kytc_jd9365da_boe_wxga_ips_101_lcm_drv,	
	&gs716_ytgd_er88577_boe_wxganl_ips_101_lcm_drv,
	&gs716_lhyj_jd9365da_boe_wxganl_ips_101_lcm_drv,
	&gs716_hsd_ek79007_boe_wxga_ips_7_lcm_drv,
	&gs716_fx_ili9881c_boe_wxga_ips_101_lcm_drv,
	&gs868_fx_ili9881c_boe_wxga_ips_8_lcm_drv,
	&gs868_zjgd_ili9881_boe_wxga_ips_8_lcm_drv,
	&gs868_zjgd_jd9365da_boe_wxga_ips_8_lcm_drv,
	&gs716_hsd_ili9881_boe_wxganl_ips_101_lcm_drv,
	&gs708_xdf_ili9881_boe_wxganl_ips_101_lcm_drv,
	&gs708_sq_ili9881_boe_wxganl_ips_101_lcm_drv,
	&gs705_ek79007_lcm_drv,
	&gs705_fx_ili9881_hsd_wxga_ips_8_lcm_drv,
	&gs717_hnh_ili9881_boe_wxga_ips_8_lcm_drv,
	&gs716_jlt_jd9365_inx_wxga_ips_8_lcm_drv,
	&gs705_hz_jd9366_inx_wxga_ips_7_lcm_drv,
    &gs710_zs_ili9881_inx_wxga_ips_101_lcm_drv,
	&ek79007_ags768_lcm_drv,
    &gs716e_xdf_jd9365_boe_wxga_ips_101_lcm_drv,
    &gs716_zs_ni3006e_boe_wxga_ips_8_lcm_drv,
	&gs109_hjy_ili9881c_boe_wxga_ips_101_lcm_drv,
	&gs109_xy_jd9365_boe_wxga_ips_101_lcm_drv,
	&gs708_fx_jd9366_cpt_wxga_ips_8_lcm_drv,
	&gs708_cf_208110ah8027002_52e_wxga_ips_8_lcm_drv,
	&gs716_xy_er88577_wxganl_ips_8_lcm_drv,
	&gs716_lx_er88577_boe_wxga_ips_8_lcm_drv,
	&gs716_sq_jd9365da_boe_wxga_ips_8_lcm_drv,
	&gs716_fx_jd9366_boe_wxga_ips_8_lcm_drv,
	&gs716_cc_ili9881_zs101ni4043j4h8ii_i_boe_wxga_ips_8_lcm_drv,
	&gs716_ry_ili9881_zs101ni4042j4h8ii_b_cpt_wxga_ips_101_lcm_drv,
	&gs710_xp_ili9881_inx_wxga_ips_101_lcm_drv,
	&gs716_sat_jd9366_boe_wxga_ips_101_lcm_drv,
	&gs716_zxcy_jd9366_boe_wxga_ips_8_lcm_drv,
	&gs716_ztrh_ili9881c_boe_wxga_ips_101_lcm_drv,
	&gs716_wdq_wd101wxm_boe_wxga_ips_101_lcm_drv,
	&gs902i_hsd_ili9881_boe_hd720_ips_5_lcm_drv,
	&gs828i_mcsj_ili9881c_boe_wxganl_ips_101_lcm_drv,
	&gs716_xhs_jd9365da_h3_boe_wxga_ips_8_lcm_drv,
	&gs716_xhs_ili9881_boe_wxga_ips_8_lcm_drv,
	&gs716_sat_jd9365da_boe_wxga_ips_8_lcm_drv,
	&gs716_sat_sc7705_hsd080bww5_boe_wxga_ips_8_lcm_drv,
	&gs716_zg_ili6136s_cpt_wxga_ips_8_lcm_drv,
	&gs716_wdq_boe_wxga_ips_101_lcm_drv,
	&gs708_sq_jd9365_boe_wxganl_ips_101_lcm_drv,
	&gs710_fx_jd9366_boe_wxganl_ips_101_lcm_drv,
	&gs705_fx_jd9366_boe_wxganl_ips_8_lcm_drv,
	&gs710_cc_tbd_ips_wxganl_101_lcm_drv,
	&gs716_pb_jd9365aa_boe_wxga_ips_8_lcm_drv,
	&gs716_fx_ek79029ba2_boe_wxga_ips_8_lcm_drv,
	&gs716_cc_kd101na5_a008_boe_wxga_ips_101_lcm_drv,
	&gs716_ybc_jd9366_boe_wxga_ips_8_lcm_drv,
	&gs705_zs_ili9881_boe_wxga_ips_8_lcm_drv,
	&gs716e_xdf_jd9367_inx_wxga_ips_8_lcm_drv,
	&gs109_hjy_ili9881_cpt_wxganl_ips_101_lcm_drv,
	&gs716_sat_ili9881_boe_wxganl_ips_101_lcm_drv,
	&gs716_sq_ili9881_cpt_wxga_ips_8_lcm_drv,
	&gs716_fx_er88577_boe_wxga_ips_8_lcm_drv,
	&gs717_bns_jd9365da_boe_wxga_ips_8_lcm_drv,
	&gs717_zjgd_ili9881_boe_wxga_ips_101_lcm_drv,
	&gs717_xhs_ili9881_boe_wxga_ips_101_lcm_drv,
	&gs716_zs_ili9881c_boe_wxga_ips_8_lcm_drv,
	&gs716_xhs_ili9881_cpt_wxga_ips_8_lcm_drv,
	&gs716_sq_qc_jd9365_cpt_wxga_ips_8_lcm_drv,
	&gs716_fx_gh8555bl_k101_im2bgh05_wxga_ips_101_lcm_drv,
	&gs716_hz_jd9365_cpt_wxga_ips_8_lcm_drv,
	&gs716_gx_jd9365_boe_wxga_ips_8_lcm_drv,
	&gs708_zs_ili9881_boe_wxganl_ips_101_lcm_drv,
	&gs960_fx_ili9881c_im2byl02al_boe_wxga_ips_101_lcm_drv,
	&gs868_fx_ek79029ba2_boe_wxga_ips_8_lcm_drv,
	&gs717_sat_jd9365da_h3_boe_wxga_101_lcm_drv,
	&gs717_sq_jd9365d_h3_boe_wxga_ips_8_lcm_drv,
	&gs717_jhx_er88577_boe_wxga_ips_101_lcm_drv,
	&gs960_bns_ili9881c_boe_wxga_ips_101_lcm_drv,
	&gs717_jhx_ili9881_boe_wxga_ips_101_lcm_drv,
	&gs717_sat_er88577_wxga_ips_8_lcm_drv,
	&gs717_sat_jd9365da_h3_boe_wxga_8_lcm_drv,
	&gs717_bns_ili9881c_boe_wxga_ips_8_lcm_drv,
	&gs717_bns_jd9365da_boe_wxga_101_lcm_drv,
	&gs717_jlt_jd9365da_h3_hkc_wxga_101_lcm_drv,
	&gs717_sat_jd9365da_inx_wxga_ips_101_lcm_drv,
	&gs717_satgd_er88577_wxga_ips_8_lcm_drv,
};
#elif defined(CONFIG_MACH_MT6771)
struct LCM_DRIVER *lcm_driver_list[] = {
	&pf828_bns_ft8201_hsd_wvxga_ips_103_lcm_drv,
	&pf165_ld_nt36523_hsd_wvxga_ips_103_lcm_drv,
	&pf163_fx_ili9881c_boe_wxga_101_lcm_drv,
	&pf196_yds_nt36523_wuxga2000_ips_103_lcm_drv,
	&pf828_dj_ft8205_wuxga2000_inx_103_lcm_drv,
	&pf196_ol_hx83102_wuxga2000_ips_109_lcm_drv,
	&pf196_dj_ft8205_wuxga2000_ips_109_lcm_drv,
	&pf196_hlx_nt36523_wuxga_ips_105_lcm_drv,
	&pf717_sat_er88577_wxga_ips_101_lcm_drv,
	&pf193_ty_ili9881c_boe_wxga_ips_101_lcm_drv,
	&pf717_sq_jd9365da_h3_boe_wxga_ips_101_lcm_drv,
	&pf717_zjgd_hx8279d_boe_wuxga_ips_101_lcm_drv,
	&pf717_zjgd_hx8279d_boe_wuxga_ips_101_DL_lcm_drv,
	&pf717_jl_hx8279d_m101d002_mantix_wuxga_ips_101_lcm_drv,
	&pf204_boe_nt36523_tv120c9mn205940_wuxga_ips_12_lcm_drv,
	&pf196_hjr_hx83102_wuxga2000_ips_103_lcm_drv,
	&pf163_boe_tv104c9m_hx83102_wuxga2000_ips_101_lcm_drv,
	&pf196_boe_hx83102_tv106c9mll0_wuxga2000_ips_106_lcm_drv,
	&pf163_ol_ek79208ac_hjc_wxga_101_lcm_drv,
	&pf163_yds_nt36523_wuxga2000_ips_103_lcm_drv,
	&pf196_bns_hx8279_101c026fhd827d40e_wuxga_8_lcm_drv,
	&ipf460_hf_st7701s_32825_ips_qvga_2_lcm_drv,
	&pf829_nv_lt9711exb_boe_wuxgal_ips_120_lcm_drv,
};
#else//add for MT8768 - MT8766
	struct LCM_DRIVER *lcm_driver_list[] = {
	&us717_zy_ili9881c_hsd_wxganl_ips_101_lcm_drv,
	&us716u_fx_ili9881c_k080im2ayl815r_auo_wxga_8_lcm_drv,
	&us716_fx_hx8279d_mm2ja05a1_hjc_wuxga_101_lcm_drv,
	&us717_hx_ili9881c_inx_wxganl_ips_101_lcm_drv,
	&us717_jlt_ota7290_inx_wxganl_ips_101_lcm_drv,
	&us716_sq_ili9881c_sq0801q4ii31337r501_wxga_ips_8_lcm_drv,
	&us716_hsd_hx8279_m101031bi40a1_wuxganl_101_lcm_drv,
	&us828_fx_hx8279d_mm2qa01_inx_wuxga_101_lcm_drv,
	&us828_bns_ili9881c_h030hd981d31b_hsd_wxga_8_lcm_drv,
	&us828_bns_ft8201_hsd_wvxga_ips_103_lcm_drv,
	&us828_md_bh101uaa01_wuxga_ips_101_lcm_drv,
	&us716_fx_er88577_im2ayc805_auo_wxga_ips_8_lcm_drv,
	&us717_fx_ili9881_k101_im2byl02_l_wxga_ips_101_lcm_drv,
	&us716_fx_gh855bl_wxga_ips_101_lcm_drv,
	&us717_fx_ili9881c_im2byl02al_boe_wxga_ips_101_lcm_drv,
	&us717_fx_ili9881c_boe_wxga_ips_101_lcm_drv,
	&us717_fx_ota7290b_boe_wuxga_ips_101_lcm_drv,
	&us717_sq_jd9365d_sq101a4ei403_boe_wxga_101_lcm_drv,
	&us717_fx_er88577_im2ayc805_auo_wxga_ips_8_lcm_drv,
	&us717_fx_ota7290b_boe_wuxga_ips_8_lcm_drv,
	&us717_hst_hx8279d_m101031_boe_wuxga_101_lcm_drv,
	&us717_fx_8555_K080IM2CH806R_wxga_ips_8_lcm_drv,
	&us716_fx_jd9365_boe_wxga_ips_101_lcm_drv,
    &us716_ypd_ili9881c_boe_wxga_ips_101_lcm_drv,
	&us716_fx_ili9881c_boe_wxga_ips_8_lcm_drv,
    &uf281_tc358775_ltl106hl01_lcm_drv,
	&us717_zs_hx8279_zs101nh4051t4h9iix_inx_wuxga_ips_101_lcm_drv,
	&us717_bns_ili9881c_boe_wxga_8_lcm_drv,
	&us717_jhzn_ili9881c_boe_wxganl_101_lcm_drv,
	&us717_hjc_hx8279d_inx_wuxga_101_lcm_drv,
	&us717_qc_hx8279d_inx_wuxga_101_lcm_drv,
	&us717_bns_nt51021_boe_wuxga_8_lcm_drv,
	&us717_cc_gh8555al_boe_wuxga_ips_101_lcm_drv,
	&us717_txd_hx8279_boe_wuxga_ips_101_lcm_drv,
	&us145u_ag_wjwu101187a_boe_wuxga_ips_101_lcm_drv,
	&us145u_ag_lt8911exb_wuxga_ips_101_lcm_drv,
	&us828_fx_ota7290b_mm2bb05_boe_wuxga_101_lcm_drv,
	&us828_jlt_ili9881c_boe_wxga_101_lcm_drv,
	&us828_fx_ek79029_im2b8017r_boe_wxga_ips_8_lcm_drv,
	&us828_fx_ili9881c_im2hyl03l_hsd_wxga_101_lcm_drv,
	&us720_sat_hx8279_inx_wuxganl_ips_101_lcm_drv,
	&us960_sat_jd9365_inx_wxganl_ips_101_lcm_drv,
	&us960_sat_jlt28d02_inx_wxganl_ips_101_lcm_drv,
	&us960_sat_jd9365da_h3_inx_wxganl_ips_101_lcm_drv,
	&us720_zs_hx8279_zs101nh4051t4h9iix_inx_wuxga_ips_101_lcm_drv,
	&us828_hsx_hx8279d_101bf40a_boe_wuxga_101_lcm_drv,
	&us828_hnh_hx8279d_cg10132010_inx_wuxga_101_lcm_drv,
	&us717_jlt_ili9881c_hsd_wxga_ips_101_lcm_drv,
	&us716_fx_ili9881c_auo_wxga_ips_8_lcm_drv,
	&us716_fx_ek79029_im2b8017r_boe_wxga_ips_8_lcm_drv,
	&us828_cyx_hx8279d_101ml020bo32a_boe_wuxga_101_lcm_drv,
	&us828_hjr_nt36523_pand_wvxga_ips_103_lcm_drv,
	&us828_zs_hx8279_inx_wuxganl_ips_101_lcm_drv,
	&us828_fx_ili9881c_im2hyl03l_wxga_101_lcm_drv,
	&us828_hjr_ft8201ba_hsd_wvxga_ips_103_lcm_drv,
	&us717_mc_ili9881c_bh40ta10103_boe_wxga_101_lcm_drv,
	&us717_hjc_hx8279d_boe_wuxganl_101_lcm_drv,
	&us717_bns_ili9881_wxganl_ips_101_lcm_drv,
	&us717_xdf_ek79007_wvxga_ips_7_lcm_drv,
	&us717_fx_hx8279d_mm2qa01_inx_wuxga_101_lcm_drv,
	&us717_fx_ili9881_k101_im2byl02a_wxga_ips_101_lcm_drv,
	&us717_bns_ili9881c_boe_wxga_ips_8_lcm_drv,
	&us163_fx_ili9881c_im2byl03a_boe_wxga_101_lcm_drv,
	&us717u11d_fx_ili9881c_boe_wxga_ips_101_lcm_drv,
	&us717_lhm_ili9881c_inx_wxga_8_lcm_drv,
	&us717_jyx_jd9365da_h3_boe_wxga_101_lcm_drv,
	&us717_jhzn_er88577_wxga_ips_8_lcm_drv,
	&us868_bns_hx83102e_boe_wxga_ips_8_lcm_drv,
	&us868_jh_er88577_wjwx080099a_hkc_wxga_8_lcm_drv,
	&us868_blq_ili9881c_yn0800hd021b_01_ips_wxga_8_lcm_drv,
	&us720_sx_jd9365_wxga_ips_8_lcm_drv,
	&us720_cc_jd9365_wxga_ips_8_lcm_drv,
	&us720_ht_jd9365_wxga_ips_8_lcm_drv,
	&us828_fx_hx8279d_mm2ja05a1_hjc_wuxga_101_lcm_drv,
	&us163_dj_hx8279d_boe_wuxga_ips_101_lcm_drv,
	&us869_sat_jd9365da_h3_boe_wxga_101_lcm_drv,
	&us717u12d_sq_jd9365da_inx_wxga_ips_8_lcm_drv,
	&us717u12d_sq_q4ei317_jd9365da_inx_wxga_ips_8_lcm_drv,
	&us717u12d_hz_jd9366aa_boe_wxga_ips_7_lcm_drv,
	&us826_jd9365da_b5tv080wxqn88_boe_wxga_ips_8_lcm_drv,
	&us868_jyx_jd9365da_h3_boe_wxga_101_lcm_drv,
	&us717u12d_jlt_ota7290b_boe_wuxga_101_lcm_drv,
	&us717u12d_sat_er88577b_inx_wxga_ips_101_lcm_drv,
	&us868_bns_ili9881c_boe_wxga_ips_101_lcm_drv,
	&us717u12d_jyx_jd9365da_inx_wxga_ips_101_lcm_drv,
	&us717u12d_0x1129_boe_wuxga_101_lcm_drv,
	&us163_bns_hx8279d_boe_wuxga_ips_101_lcm_drv,
	&us717u12d_jl_jd9365da_h3_boe_wxga_ips_101_lcm_drv,
	&us717u12d_hnh_er88577b_inx_wxga_ips_101_lcm_drv,
	&us717u12d_jlt_jd9365da_inx_wxga_ips_8_lcm_drv,
	&us869_sat_er88577_inx_boe_wxga_8_lcm_drv,
	&us828_lh_ili9881c_tv080wxq_n88_boe_wxga_8_lcm_drv,
	&us868_tyzn_k101_im2kgh04_ips_wxga_101_lcm_drv,
	&us717u12d_tyzn_er88577b_inx_wxga_ips_8_lcm_drv,
	&us720_jlt_er88577_wxga_ips_8_lcm_drv,
	&us720_sat_sc7705_wxga_ips_8_lcm_drv,
	&us868_jl_jd9635da_jlm80b011p21wx_wxga_8_lcm_drv,
	&us720_jlt_jd9365da_wxga_ips_101_lcm_drv,
	&us163_blq_ili9881c_yn1010hd060n_inx_wxga_101_lcm_drv,
	&us717u12d_xty_ek79007_wsvganl_ips_7_lcm_drv,
	&us717u12d_wcl_ili9881d_w553246aaa_inx_wxga_546_lcm_drv,
	&iuf461_zy_jd9365_h3_qvga800_ips_3_lcm_drv,
	&us868_jyx_er88577b_jym101790328_ips_wxga_101_lcm_drv,
	&us720i20d_dyg_hx8279d_dyg08021039f_wuxganl_ips_8_lcm_drv,
};
#endif

unsigned char lcm_name_list[][128] = {
#if defined(HX8392A_DSI_CMD)
	"hx8392a_dsi_cmd",
#endif

#if defined(S6E3HA3_WQHD_2K_CMD)
	"s6e3ha3_wqhd_2k_cmd",
#endif

#if defined(HX8392A_DSI_VDO)
	"hx8392a_vdo_cmd",
#endif

#if defined(HX8392A_DSI_CMD_FWVGA)
	"hx8392a_dsi_cmd_fwvga",
#endif

#if defined(OTM9608_QHD_DSI_CMD)
	"otm9608a_qhd_dsi_cmd",
#endif

#if defined(OTM9608_QHD_DSI_VDO)
	"otm9608a_qhd_dsi_vdo",
#endif

#if defined(R63417_FHD_DSI_CMD_TRULY_NT50358)
	"r63417_fhd_dsi_cmd_truly_nt50358_drv",
#endif

#if defined(R63417_FHD_DSI_CMD_TRULY_NT50358_QHD)
	"r63417_fhd_dsi_cmd_truly_nt50358_qhd_drv",
#endif

#if defined(R63417_FHD_DSI_VDO_TRULY_NT50358)
	"r63417_fhd_dsi_vdo_truly_nt50358_drv",
#endif

#if defined(R63419_WQHD_TRULY_PHANTOM_2K_CMD_OK)
	"r63419_wqhd_truly_phantom_2k_cmd_ok",
#endif

#if defined(NT35695_FHD_DSI_CMD_TRULY_NT50358)
	"nt35695_fhd_dsi_cmd_truly_nt50358_drv",
#endif

#if defined(S6E3HA3_WQHD_2K_CMD_LANESWAP)
	"s6e3ha3_wqhd_2k_cmd_laneswap_drv",
#endif

#if defined(NT36380_WQHD_VDO_OK)
	"nt36380_wqhd_vdo_lcm_drv",
#endif
#if defined(NT35521_HD_DSI_VDO_TRULY_RT5081)
	"nt35521_hd_dsi_vdo_truly_rt5081_drv",
#endif

#if defined(ILI9881C_HDP_DSI_VDO_ILITEK_RT5081)
	"ili9881c_hdp_dsi_vdo_ilitek_rt5081_drv",
#endif

#if defined(NT35695B_FHD_DSI_VDO_AUO_RT5081_HDP)
	"nt35695B_fhd_dsi_vdo_auo_rt5081_hdp_drv",
#endif

#if defined(NT35695B_FHD_DSI_CMD_TRULY_RT5081_720P)
	"nt35695B_fhd_dsi_cmd_truly_rt5081_720p_lcm_drv",
#endif

#if defined(OPPO_TIANMA_TD4310_FHDP_DSI_VDO_RT5081)
	"oppo_tianma_td4310_fhdp_dsi_vdo_rt5081_drv",
#endif

#if defined(KD070FHFID015_DSI_1200X1920)
	"kd070fhfid015_dsi_1200x1920",
#endif
};

#define LCM_COMPILE_ASSERT(condition) \
	LCM_COMPILE_ASSERT_X(condition, __LINE__)
#define LCM_COMPILE_ASSERT_X(condition, line) \
	LCM_COMPILE_ASSERT_XX(condition, line)
#define LCM_COMPILE_ASSERT_XX(condition, line) \
	char assertion_failed_at_line_##line[(condition) ? 1 : -1]

unsigned int lcm_count =
	sizeof(lcm_driver_list) / sizeof(struct LCM_DRIVER *);
LCM_COMPILE_ASSERT(sizeof(lcm_driver_list) / sizeof(struct LCM_DRIVER *) != 0);
#if defined(NT35520_HD720_DSI_CMD_TM) | \
	defined(NT35520_HD720_DSI_CMD_BOE) | \
	defined(NT35521_HD720_DSI_VDO_BOE) | \
	defined(NT35521_HD720_DSI_VIDEO_TM)
static unsigned char lcd_id_pins_value = 0xFF;

/*
 * Function:    which_lcd_module_triple
 * Description: read LCD ID PIN status,could identify three status:highlowfloat
 * Input:       none
 * Output:      none
 * Return:      LCD ID1|ID0 value
 * Others:
 */
unsigned char which_lcd_module_triple(void)
{
	unsigned char  high_read0 = 0;
	unsigned char  low_read0 = 0;
	unsigned char  high_read1 = 0;
	unsigned char  low_read1 = 0;
	unsigned char  lcd_id0 = 0;
	unsigned char  lcd_id1 = 0;
	unsigned char  lcd_id = 0;
	/*Solve Coverity scan warning : check return value*/
	unsigned int ret = 0;

	/*only recognise once*/
	if (lcd_id_pins_value != 0xFF)
		return lcd_id_pins_value;

	/*Solve Coverity scan warning : check return value*/
	ret = mt_set_gpio_mode(GPIO_DISP_ID0_PIN, GPIO_MODE_00);
	if (ret != 0)
		pr_debug("[LCM]ID0 mt_set_gpio_mode fail\n");

	ret = mt_set_gpio_dir(GPIO_DISP_ID0_PIN, GPIO_DIR_IN);
	if (ret != 0)
		pr_debug("[LCM]ID0 mt_set_gpio_dir fail\n");

	ret = mt_set_gpio_pull_enable(GPIO_DISP_ID0_PIN, GPIO_PULL_ENABLE);
	if (ret != 0)
		pr_debug("[LCM]ID0 mt_set_gpio_pull_enable fail\n");

	ret = mt_set_gpio_mode(GPIO_DISP_ID1_PIN, GPIO_MODE_00);
	if (ret != 0)
		pr_debug("[LCM]ID1 mt_set_gpio_mode fail\n");

	ret = mt_set_gpio_dir(GPIO_DISP_ID1_PIN, GPIO_DIR_IN);
	if (ret != 0)
		pr_debug("[LCM]ID1 mt_set_gpio_dir fail\n");

	ret = mt_set_gpio_pull_enable(GPIO_DISP_ID1_PIN, GPIO_PULL_ENABLE);
	if (ret != 0)
		pr_debug("[LCM]ID1 mt_set_gpio_pull_enable fail\n");

	/*pull down ID0 ID1 PIN*/
	ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN, GPIO_PULL_DOWN);
	if (ret != 0)
		pr_debug("[LCM]ID0 mt_set_gpio_pull_select->Down fail\n");

	ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN, GPIO_PULL_DOWN);
	if (ret != 0)
		pr_debug("[LCM]ID1 mt_set_gpio_pull_select->Down fail\n");

	/* delay 100ms , for discharging capacitance*/
	mdelay(100);
	/* get ID0 ID1 status*/
	low_read0 = mt_get_gpio_in(GPIO_DISP_ID0_PIN);
	low_read1 = mt_get_gpio_in(GPIO_DISP_ID1_PIN);
	/* pull up ID0 ID1 PIN */
	ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN, GPIO_PULL_UP);
	if (ret != 0)
		pr_debug("[LCM]ID0 mt_set_gpio_pull_select->UP fail\n");

	ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN, GPIO_PULL_UP);
	if (ret != 0)
		pr_debug("[LCM]ID1 mt_set_gpio_pull_select->UP fail\n");

	/* delay 100ms , for charging capacitance */
	mdelay(100);
	/* get ID0 ID1 status */
	high_read0 = mt_get_gpio_in(GPIO_DISP_ID0_PIN);
	high_read1 = mt_get_gpio_in(GPIO_DISP_ID1_PIN);

	if (low_read0 != high_read0) {
		/*float status , pull down ID0 ,to prevent electric leakage*/
		ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,
			GPIO_PULL_DOWN);
		if (ret != 0)
			pr_debug("[LCM]ID0 mt_set_gpio_pull_select->Down fail\n");

		lcd_id0 = LCD_HW_ID_STATUS_FLOAT;
	} else if ((low_read0 == LCD_HW_ID_STATUS_LOW) &&
		(high_read0 == LCD_HW_ID_STATUS_LOW)) {
		/*low status , pull down ID0 ,to prevent electric leakage*/
		ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,
			GPIO_PULL_DOWN);
		if (ret != 0)
			pr_debug("[LCM]ID0 mt_set_gpio_pull_select->Down fail\n");

		lcd_id0 = LCD_HW_ID_STATUS_LOW;
	} else if ((low_read0 == LCD_HW_ID_STATUS_HIGH) &&
		(high_read0 == LCD_HW_ID_STATUS_HIGH)) {
		/*high status , pull up ID0 ,to prevent electric leakage*/
		ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN, GPIO_PULL_UP);
		if (ret != 0)
			pr_debug("[LCM]ID0 mt_set_gpio_pull_select->UP fail\n");

		lcd_id0 = LCD_HW_ID_STATUS_HIGH;
	} else {
		pr_debug("[LCM] Read LCD_id0 error\n");
		ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,
			GPIO_PULL_DISABLE);
		if (ret != 0)
			pr_debug("[KERNEL/LCM]ID0 mt_set_gpio_pull_select->Disable fail\n");

		lcd_id0 = LCD_HW_ID_STATUS_ERROR;
	}


	if (low_read1 != high_read1) {
		/*float status , pull down ID1 ,to prevent electric leakage*/
		ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,
			GPIO_PULL_DOWN);
		if (ret != 0)
			pr_debug("[LCM]ID1 mt_set_gpio_pull_select->Down fail\n");

		lcd_id1 = LCD_HW_ID_STATUS_FLOAT;
	} else if ((low_read1 == LCD_HW_ID_STATUS_LOW) &&
		(high_read1 == LCD_HW_ID_STATUS_LOW)) {
		/*low status , pull down ID1 ,to prevent electric leakage*/
		ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,
			GPIO_PULL_DOWN);
		if (ret != 0)
			pr_debug("[LCM]ID1 mt_set_gpio_pull_select->Down fail\n");

		lcd_id1 = LCD_HW_ID_STATUS_LOW;
	} else if ((low_read1 == LCD_HW_ID_STATUS_HIGH) &&
		(high_read1 == LCD_HW_ID_STATUS_HIGH)) {
		/*high status , pull up ID1 ,to prevent electric leakage*/
		ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN, GPIO_PULL_UP);
		if (ret != 0)
			pr_debug("[LCM]ID1 mt_set_gpio_pull_select->UP fail\n");

		lcd_id1 = LCD_HW_ID_STATUS_HIGH;
	} else {

		pr_debug("[LCM] Read LCD_id1 error\n");
		ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,
			GPIO_PULL_DISABLE);
		if (ret != 0)
			pr_debug("[KERNEL/LCM]ID1 mt_set_gpio_pull_select->Disable fail\n");

		lcd_id1 = LCD_HW_ID_STATUS_ERROR;
	}
#ifdef BUILD_LK
	dprintf(CRITICAL, "%s,lcd_id0:%d\n", __func__, lcd_id0);
	dprintf(CRITICAL, "%s,lcd_id1:%d\n", __func__, lcd_id1);
#else
	pr_debug("[LCM]%s,lcd_id0:%d\n", __func__, lcd_id0);
	pr_debug("[LCM]%s,lcd_id1:%d\n", __func__, lcd_id1);
#endif
	lcd_id =  lcd_id0 | (lcd_id1 << 2);

#ifdef BUILD_LK
	dprintf(CRITICAL, "%s,lcd_id:%d\n", __func__, lcd_id);
#else
	pr_debug("[LCM]%s,lcd_id:%d\n", __func__, lcd_id);
#endif

	lcd_id_pins_value = lcd_id;
	return lcd_id;
}
#endif
