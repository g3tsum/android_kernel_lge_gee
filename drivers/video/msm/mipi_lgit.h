/*
 *  Copyright (C) 2011-2012, LG Eletronics,Inc. All rights reserved.
 *      LGIT LCD device driver
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef MIPI_LGIT_H
#define MIPI_LGIT_H

int mipi_lgit_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel);

#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WXGA_PT)
/* for making one source of DSV feature. */
extern char lcd_mirror [2];

// values of DSV setting START
extern char panel_setting_1 [6];
extern char panel_setting_2 [3];

extern char display_mode1 [6];
extern char display_mode2 [8];

extern char p_gamma_r_setting[10];
extern char n_gamma_r_setting[10];
extern char p_gamma_g_setting[10];
extern char n_gamma_g_setting[10];
extern char p_gamma_b_setting[10];
extern char n_gamma_b_setting[10];

extern char ief_on_set0[2];
extern char ief_on_set4[4];
extern char ief_on_set5[4];
extern char ief_on_set6[4];

extern char ief_set1[5];
extern char ief_set2[3];
extern char ief_set3[6];
extern char ief_set7[9];
extern char ief_set8[9];
extern char ief_set9[9];
extern char ief_setA[9];
extern char ief_setB[9];
extern char ief_setC[9];

extern char osc_setting[4];
extern char power_setting3[13];
extern char power_setting4[6];
extern char power_setting5[4];

#ifdef CONFIG_LGIT_VIDEO_WXGA_CABC
extern char cabc_set0[2];
extern char cabc_set1[2];
extern char cabc_set2[2];
extern char cabc_set3[2];
extern char cabc_set4[6];
#endif

extern char exit_sleep_power_control_2[2];
extern char exit_sleep_power_control_3[2];
extern char otp_protection[3];
extern char sleep_out_for_cabc[2];
extern char gate_output_enabled_by_manual[2];

extern char display_on[2];

extern char display_off[2];

extern char enter_sleep[2];

extern char analog_boosting_power_control[2];
extern char enter_sleep_power_control_3[2];
extern char enter_sleep_power_control_2[2];

extern char deep_standby[2];

extern struct dsi_cmd_desc lgit_power_on_set_1[33];

extern struct dsi_cmd_desc lgit_power_on_set_2[2];

extern struct dsi_cmd_desc lgit_power_on_set_3[4];

extern struct dsi_cmd_desc lgit_power_off_set_1[2];

extern struct dsi_cmd_desc lgit_power_off_set_2[4];
#endif
#endif  /* MIPI_LGIT_H */
