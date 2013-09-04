/*
 *  Copyright (C) 2011-2012, LG Eletronics,Inc. All rights reserved.
 *      LGIT LCD device driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <linux/gpio.h>
#include <linux/syscore_ops.h>

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_lgit.h"
#include "mdp4.h"

static struct msm_panel_common_pdata *mipi_lgit_pdata;

static struct dsi_buf lgit_tx_buf;
static struct dsi_buf lgit_rx_buf;
static int skip_init;

#define DSV_ONBST 57

#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WXGA_PT)
/* for making one source of DSV feature. */
char lcd_mirror [2] = {0x36, 0x02};

// values of DSV setting START
char panel_setting_1 [6] = {0xB0, 0x43, 0x00, 0x00, 0x00, 0x00};
char panel_setting_2 [3] = {0xB3, 0x0A, 0x9F};

char display_mode1 [6] = {0xB5, 0x50, 0x20, 0x40, 0x00, 0x20};
char display_mode2 [8] = {0xB6, 0x00, 0x14, 0x0F, 0x16, 0x13, 0x05, 0x05};

char p_gamma_r_setting[10] = {0xD0, 0x40, 0x44, 0x76, 0x01, 0x00, 0x00, 0x30, 0x20, 0x01};
char n_gamma_r_setting[10] = {0xD1, 0x40, 0x44, 0x76, 0x01, 0x00, 0x00, 0x30, 0x20, 0x01};
char p_gamma_g_setting[10] = {0xD2, 0x40, 0x44, 0x76, 0x01, 0x00, 0x00, 0x30, 0x20, 0x01};
char n_gamma_g_setting[10] = {0xD3, 0x40, 0x44, 0x76, 0x01, 0x00, 0x00, 0x30, 0x20, 0x01};
char p_gamma_b_setting[10] = {0xD4, 0x20, 0x23, 0x74, 0x00, 0x1F, 0x10, 0x50, 0x33, 0x03};
char n_gamma_b_setting[10] = {0xD5, 0x20, 0x23, 0x74, 0x00, 0x1F, 0x10, 0x50, 0x33, 0x03};

char ief_on_set0[2] = {0xE0, 0x00};
char ief_on_set4[4] = {0xE4, 0x00, 0x00, 0x00};
char ief_on_set5[4] = {0xE5, 0x00, 0x00, 0x00};
char ief_on_set6[4] = {0xE6, 0x00, 0x00, 0x00};

char ief_set1[5] = {0xE1, 0x00, 0x00, 0x01, 0x01};
char ief_set2[3] = {0xE2, 0x01, 0x00};
char ief_set3[6] = {0xE3, 0x00, 0x00, 0x42, 0x35, 0x00};
char ief_set7[9] = {0xE7, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};
char ief_set8[9] = {0xE8, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D};
char ief_set9[9] = {0xE9, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B};
char ief_setA[9] = {0xEA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char ief_setB[9] = {0xEB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char ief_setC[9] = {0xEC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

char osc_setting[4] =     {0xC0, 0x00, 0x0A, 0x10};
char power_setting3[13] = {0xC3, 0x00, 0x88, 0x03, 0x20, 0x01, 0x57, 0x4F, 0x33,0x02,0x38,0x38,0x00};
char power_setting4[6] =  {0xC4, 0x31, 0x24, 0x11, 0x11, 0x3D};
char power_setting5[4] =  {0xC5, 0x3B, 0x3B, 0x03};

#ifdef CONFIG_LGIT_VIDEO_WXGA_CABC
char cabc_set0[2] = {0x51, 0xFF};
char cabc_set1[2] = {0x5E, 0x00}; // CABC MIN
char cabc_set2[2] = {0x53, 0x2C};
char cabc_set3[2] = {0x55, 0x02};
char cabc_set4[6] = {0xC8, 0x22, 0x22, 0x22, 0x33, 0x80};//A-CABC applied
#endif

char exit_sleep_power_control_2[2] =  {0xC2,0x06};
char exit_sleep_power_control_3[2] =  {0xC2,0x0E};
char otp_protection[3] =  {0xF1,0x10,0x00};
char sleep_out_for_cabc[2] = {0x11,0x00};
char gate_output_enabled_by_manual[2] = {0xC1,0x08};

char display_on[2] =  {0x29,0x00};

char display_off[2] = {0x28,0x00};

char enter_sleep[2] = {0x10,0x00};

char analog_boosting_power_control[2] = {0xC2,0x00};
char enter_sleep_power_control_3[2] = {0xC2,0x01};
char enter_sleep_power_control_2[2] = {0xC2,0x00};

char deep_standby[2] = {0xC1,0x02};

struct dsi_cmd_desc lgit_power_on_set_1[33] = {
	// Display Initial Set
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lcd_mirror), lcd_mirror},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(panel_setting_1), panel_setting_1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(panel_setting_2), panel_setting_2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(display_mode1), display_mode1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(display_mode2), display_mode2},

	// Gamma Set
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(p_gamma_r_setting), p_gamma_r_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(n_gamma_r_setting), n_gamma_r_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(p_gamma_g_setting), p_gamma_g_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(n_gamma_g_setting), n_gamma_g_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(p_gamma_b_setting), p_gamma_b_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(n_gamma_b_setting), n_gamma_b_setting},

	// IEF set
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set0), ief_on_set0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_set1), ief_set1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_set2), ief_set2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_set3), ief_set3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set4), ief_on_set4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set5), ief_on_set5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set6), ief_on_set6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_set7), ief_set7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_set8), ief_set8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_set9), ief_set9},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_setA), ief_setA},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_setB), ief_setB},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_setC), ief_setC},

	// Power Supply Set
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(osc_setting), osc_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(power_setting3), power_setting3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(power_setting4), power_setting4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(power_setting5), power_setting5},

#ifdef CONFIG_LGIT_VIDEO_WXGA_CABC
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cabc_set0), cabc_set0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cabc_set1), cabc_set1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cabc_set2), cabc_set2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cabc_set3), cabc_set3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cabc_set4), cabc_set4},
#endif
};

struct dsi_cmd_desc lgit_power_on_set_2[2] = {
	{DTYPE_GEN_LWRITE,  1, 0, 0, 10, sizeof(exit_sleep_power_control_2), exit_sleep_power_control_2},
	{DTYPE_GEN_LWRITE,  1, 0, 0, 1, sizeof(exit_sleep_power_control_3), exit_sleep_power_control_3},
};

struct dsi_cmd_desc lgit_power_on_set_3[4] = {
	// Power Supply Set
	{DTYPE_GEN_LWRITE,  1, 0, 0, 0, sizeof(otp_protection), otp_protection},
	{DTYPE_GEN_LWRITE,  1, 0, 0, 0, sizeof(sleep_out_for_cabc), sleep_out_for_cabc},
	{DTYPE_GEN_LWRITE,  1, 0, 0, 0, sizeof(gate_output_enabled_by_manual), gate_output_enabled_by_manual},
	{DTYPE_DCS_WRITE,  1, 0, 0, 0, sizeof(display_on), display_on},
};

struct dsi_cmd_desc lgit_power_off_set_1[2] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 5, sizeof(enter_sleep), enter_sleep},
};

struct dsi_cmd_desc lgit_power_off_set_2[4] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(analog_boosting_power_control), analog_boosting_power_control},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(enter_sleep_power_control_3), enter_sleep_power_control_3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(enter_sleep_power_control_2), enter_sleep_power_control_2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(deep_standby), deep_standby}
};
#endif

static int lgit_external_dsv_onoff(uint8_t on_off)
{
	int ret =0;
	static int init_done=0;

	if (!init_done) {
		ret = gpio_request(DSV_ONBST,"DSV_ONBST_en");
		if (ret) {
			pr_err("%s: failed to request DSV_ONBST gpio \n", __func__);
			goto out;
		}
		ret = gpio_direction_output(DSV_ONBST, 1);
		if (ret) {
			pr_err("%s: failed to set DSV_ONBST direction\n", __func__);
			goto err_gpio;
		}
		init_done = 1;
	}

	gpio_set_value(DSV_ONBST, on_off);
	mdelay(20);
	goto out;

err_gpio:
	gpio_free(DSV_ONBST);
out:
	return ret;
}

static int mipi_lgit_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct dcs_cmd_req cmdreq;
	int ret = 0;

	pr_info("%s started\n", __func__);

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds =	lgit_power_on_set_1;
	cmdreq.cmds_cnt = ARRAY_SIZE(lgit_power_on_set_1);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);

	if(!skip_init){
		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
		memset(&cmdreq, 0, sizeof(cmdreq));
		cmdreq.cmds =	lgit_power_on_set_2;
		cmdreq.cmds_cnt = ARRAY_SIZE(lgit_power_on_set_2);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);
		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);
	}
	skip_init = false;

	ret = lgit_external_dsv_onoff(1);
	if (ret < 0) {
		pr_err("%s: failed to turn on external dsv\n", __func__);
		return ret;
	}

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds =	lgit_power_on_set_3;
	cmdreq.cmds_cnt = ARRAY_SIZE(lgit_power_on_set_3);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);

	pr_info("%s finished\n", __func__);
	return 0;
}

static int mipi_lgit_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct dcs_cmd_req cmdreq;
	int ret = 0;

	pr_info("%s started\n", __func__);

	if (mipi_lgit_pdata->bl_pwm_disable)
		mipi_lgit_pdata->bl_pwm_disable();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds =	lgit_power_off_set_1;
	cmdreq.cmds_cnt = ARRAY_SIZE(lgit_power_off_set_1);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);

	ret = lgit_external_dsv_onoff(0);
	if (ret < 0) {
		pr_err("%s: failed to turn off external dsv\n", __func__);
		return ret;
	}

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds =	lgit_power_off_set_2;
	cmdreq.cmds_cnt = ARRAY_SIZE(lgit_power_off_set_2);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);

	pr_info("%s finished\n", __func__);
	return 0;
}

static void mipi_lgit_lcd_shutdown(void)
{
	struct dcs_cmd_req cmdreq;
	int ret = 0;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds =	lgit_power_off_set_1;
	cmdreq.cmds_cnt = ARRAY_SIZE(lgit_power_off_set_1);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);

	ret = lgit_external_dsv_onoff(0);
	if (ret < 0) {
		pr_err("%s: failed to turn off external dsv\n", __func__);
	}
	mdelay(20);

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds =	lgit_power_off_set_2;
	cmdreq.cmds_cnt = ARRAY_SIZE(lgit_power_off_set_2);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);

	pr_info("%s finished\n", __func__);
}

static int mipi_lgit_backlight_on_status(void)
{
	return (mipi_lgit_pdata->bl_on_status());
}

static void mipi_lgit_set_backlight_board(struct msm_fb_data_type *mfd)
{
	int level;

	level = (int)mfd->bl_level;
	mipi_lgit_pdata->backlight_level(level, 0, 0);
}

struct syscore_ops panel_syscore_ops = {
	.shutdown = mipi_lgit_lcd_shutdown,
};

static int mipi_lgit_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_lgit_pdata = pdev->dev.platform_data;
		return 0;
	}

	pr_info("%s start\n", __func__);

	skip_init = true;
	msm_fb_add_device(pdev);

	register_syscore_ops(&panel_syscore_ops);

	return 0;
}

static struct platform_driver this_driver = {
	.probe = mipi_lgit_lcd_probe,
	.driver = {
		.name = "mipi_lgit",
	},
};

static struct msm_fb_panel_data lgit_panel_data = {
	.on = mipi_lgit_lcd_on,
	.off = mipi_lgit_lcd_off,
	.set_backlight = mipi_lgit_set_backlight_board,
	.get_backlight_on_status = mipi_lgit_backlight_on_status,
};

static int ch_used[3];

int mipi_lgit_device_register(struct msm_panel_info *pinfo,
		u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_lgit", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	lgit_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &lgit_panel_data,
			sizeof(lgit_panel_data));
	if (ret) {
		pr_err("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_err("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}
	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_lgit_lcd_init(void)
{
	mipi_dsi_buf_alloc(&lgit_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&lgit_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_lgit_lcd_init);
