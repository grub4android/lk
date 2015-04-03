/* Copyright (c) 2012-2015, The Linux Foundation. All rights reserved.
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

#ifndef _PLATFORM_MSM_SHARED_MSM_PANEL_H_
#define _PLATFORM_MSM_SHARED_MSM_PANEL_H_

#include <stdint.h>
#include <stdbool.h>
#include <dev/fbcon.h>

#define TRUE	1
#define FALSE	0

/* panel type list */
#define NO_PANEL		0xffff	/* No Panel */
#define MDDI_PANEL		1	/* MDDI */
#define EBI2_PANEL		2	/* EBI2 */
#define LCDC_PANEL		3	/* internal LCDC type */
#define EXT_MDDI_PANEL		4	/* Ext.MDDI */
#define TV_PANEL		5	/* TV */
#define HDMI_PANEL		6	/* HDMI TV */
#define DTV_PANEL		7	/* DTV */
#define MIPI_VIDEO_PANEL	8	/* MIPI */
#define MIPI_CMD_PANEL		9	/* MIPI */
#define WRITEBACK_PANEL		10	/* Wifi display */
#define LVDS_PANEL		11	/* LVDS */
#define EDP_PANEL		12	/* EDP */
#define QPIC_PANEL		13	/* QPIC */

#define DISPLAY_UNKNOWN		0
#define DISPLAY_1		1
#define DISPLAY_2		2

enum mdss_mdp_pipe_type {
	MDSS_MDP_PIPE_TYPE_VIG,
	MDSS_MDP_PIPE_TYPE_RGB,
	MDSS_MDP_PIPE_TYPE_DMA,
};

enum msm_mdp_hw_revision {
	MDP_REV_20 = 1,
	MDP_REV_22,
	MDP_REV_30,
	MDP_REV_303,
	MDP_REV_304,
	MDP_REV_305,
	MDP_REV_31,
	MDP_REV_40,
	MDP_REV_41,
	MDP_REV_42,
	MDP_REV_43,
	MDP_REV_44,
	MDP_REV_50,
};

/* panel info type */
struct lcd_panel_info {
	uint32_t vsync_enable;
	uint32_t refx100;
	uint32_t v_back_porch;
	uint32_t v_front_porch;
	uint32_t v_pulse_width;
	uint32_t hw_vsync_mode;
	uint32_t vsync_notifier_period;
	uint32_t rev;
};

struct hdmi_panel_info {
	uint32_t h_back_porch;
	uint32_t h_front_porch;
	uint32_t h_pulse_width;
	uint32_t v_back_porch;
	uint32_t v_front_porch;
	uint32_t v_pulse_width;
};

struct lcdc_panel_info {
	uint32_t h_back_porch;
	uint32_t h_front_porch;
	uint32_t h_pulse_width;
	uint32_t v_back_porch;
	uint32_t v_front_porch;
	uint32_t v_pulse_width;
	uint32_t border_clr;
	uint32_t underflow_clr;
	uint32_t hsync_skew;
	/* Pad width */
	uint32_t xres_pad;
	/* Pad height */
	uint32_t yres_pad;
	uint8_t dual_pipe;
	uint8_t split_display;
	uint8_t pipe_swap;
	uint8_t dst_split;
};

struct fbc_panel_info {
	uint32_t enabled;
	uint32_t comp_ratio;
	uint32_t comp_mode;
	uint32_t qerr_enable;
	uint32_t cd_bias;
	uint32_t pat_enable;
	uint32_t vlc_enable;
	uint32_t bflc_enable;

	uint32_t line_x_budget;
	uint32_t block_x_budget;
	uint32_t block_budget;

	uint32_t lossless_mode_thd;
	uint32_t lossy_mode_thd;
	uint32_t lossy_rgb_thd;
	uint32_t lossy_mode_idx;

	uint32_t slice_height;
	uint32_t pred_mode;
	uint32_t max_pred_err;
};

/* intf timing settings */
struct intf_timing_params {
	uint32_t width;
	uint32_t height;
	uint32_t xres;
	uint32_t yres;

	uint32_t h_back_porch;
	uint32_t h_front_porch;
	uint32_t v_back_porch;
	uint32_t v_front_porch;
	uint32_t hsync_pulse_width;
	uint32_t vsync_pulse_width;

	uint32_t border_clr;
	uint32_t underflow_clr;
	uint32_t hsync_skew;
};

struct mipi_panel_info {
	char cmds_post_tg;	/* send on commands after tg on */
	char mode;		/* video/cmd */
	char interleave_mode;
	int eof_bllp_power;
	uint32_t bitclock;
	char crc_check;
	char ecc_check;
	char dst_format;	/* shared by video and command */
	char num_of_lanes;
	char data_lane0;
	char data_lane1;
	char data_lane2;
	char data_lane3;
	char dlane_swap;	/* data lane swap */
	char rgb_swap;
	char b_sel;
	char g_sel;
	char r_sel;
	char rx_eot_ignore;
	char tx_eot_append;
	char t_clk_post; /* 0xc0, DSI_CLKOUT_TIMING_CTRL */
	char t_clk_pre;  /* 0xc0, DSI_CLKOUT_TIMING_CTRL */
	char vc;	/* virtual channel */
	struct mipi_dsi_phy_ctrl *dsi_phy_db;
	struct mdss_dsi_phy_ctrl *mdss_dsi_phy_db;
	struct mdss_dsi_pll_config *dsi_pll_config;
	struct mipi_dsi_cmd *panel_on_cmds;
	int num_of_panel_on_cmds;
	struct mipi_dsi_cmd *panel_off_cmds;
	int num_of_panel_off_cmds;
	/* video mode */
	char pulse_mode_hsa_he;
	char hfp_power_stop;
	char hbp_power_stop;
	char hsa_power_stop;
	char eof_bllp_power_stop;
	char bllp_power_stop;
	char traffic_mode;
	char frame_rate;
	/* command mode */
	char interleave_max;
	char insert_dcs_cmd;
	char wr_mem_continue;
	char wr_mem_start;
	char te_sel;
	char stream;	/* 0 or 1 */
	char mdp_trigger;
	char dma_trigger;
	uint32_t dsi_pclk_rate;
	/* The packet-size should not bet changed */
	char no_max_pkt_size;
	/* Clock required during LP commands */
	char force_clk_lane_hs;
	char lane_swap;
	uint8_t dual_dsi;
	uint8_t broadcast;
	uint8_t mode_gpio_state;
	uint32_t signature;
	uint32_t use_enable_gpio;
	uint32_t ctl_base;
	uint32_t phy_base;
	uint32_t sctl_base;
	uint32_t sphy_base;
	uint32_t reg_base;
	uint32_t sreg_base;
	uint32_t pll_0_base;
	uint32_t pll_1_base;
};

struct edp_panel_info {
	int max_lane_count;
	unsigned long max_link_clk;
};

enum lvds_mode {
	LVDS_SINGLE_CHANNEL_MODE,
	LVDS_DUAL_CHANNEL_MODE,
};

struct lvds_panel_info {
	enum lvds_mode channel_mode;
	/* Channel swap in dual mode */
	char channel_swap;
};

struct labibb_desc {
	char amoled_panel; /* lcd = 0, amoled = 1*/
	char force_config; /* 0 to use default value */
	uint32_t ibb_min_volt;
	uint32_t ibb_max_volt;
	uint32_t lab_min_volt;
	uint32_t lab_max_volt;
	char pwr_up_delay; /* ndx to => 1250, 2500, 5000 and 10000 us */
	char pwr_down_delay; /* ndx to => 1250, 2500, 5000 and 10000 us */
	char ibb_discharge_en;
};

struct msm_panel_info {
	uint32_t xres;
	uint32_t yres;
	uint32_t bpp;
	uint32_t type;
	uint32_t wait_cycle;
	uint32_t clk_rate;
	uint32_t orientation;
	uint32_t dest;
	/*  Select pipe type for handoff */
	uint32_t pipe_type;
	char     lowpowerstop;
	char     lcd_reg_en;
	uint32_t border_top;
	uint32_t border_bottom;
	uint32_t border_left;
	uint32_t border_right;

	struct lcd_panel_info lcd;
	struct lcdc_panel_info lcdc;
	struct fbc_panel_info fbc;
	struct mipi_panel_info mipi;
	struct lvds_panel_info lvds;
	struct hdmi_panel_info hdmi;
	struct edp_panel_info edp;

	struct labibb_desc *labibb;

	int (*on) (void);
	int (*off) (void);
	int (*pre_on) (void);
	int (*pre_off) (void);
	int (*prepare) (void);
	int (*early_config) (struct msm_panel_info *pinfo, struct fbcon_config *fb);
	int (*config) (void *pdata);
	int (*rotate) (void);
};

struct msm_fb_panel_data {
	struct msm_panel_info panel_info;
	struct fbcon_config fb;
	int mdp_rev;
	int rotate;

	/* function entry chain */
	int (*power_func) (uint8_t enable, struct msm_panel_info *);
	uint32_t (*clk_func) (uint8_t enable, struct msm_panel_info *pinfo);
	int (*bl_func) (uint8_t enable);
	uint32_t (*pll_clk_func) (uint8_t enable, struct msm_panel_info *);
	int (*post_power_func)(int enable);
	int (*pre_init_func)(void);
};

int mdp_lcdc_config(struct msm_panel_info *pinfo, struct fbcon_config *fb);
int lvds_on(struct msm_fb_panel_data *pdata);
int mdp_lcdc_on(struct msm_fb_panel_data *pdata);
int mdp_lcdc_off(void);
int target_display_pre_on(void);
int target_display_post_on(void);
int target_display_pre_off(void);
int target_display_post_off(void);
int target_display_get_base_offset(uint32_t base);
int target_ldo_ctrl(uint8_t enable, struct msm_panel_info *pinfo);
void target_edp_panel_init(struct msm_panel_info *pinfo);
int target_edp_panel_clock(uint8_t enable, struct msm_panel_info *pinfo);
int target_edp_panel_enable(void);
int target_edp_panel_disable(void);
int target_edp_bl_ctrl(int enable);
int target_hdmi_panel_clock(uint8_t enable, struct msm_panel_info *pinfo);
int target_hdmi_regulator_ctrl(bool enable);
int mdss_hdmi_init(void);
int target_hdmi_gpio_ctrl(bool enable);
int target_cont_splash_screen(void);
void target_force_cont_splash_disable(uint8_t override);
bool target_display_panel_node(const char *panel_name, char *pbuf, uint16_t buf_size);
void target_display_init(const char *panel_name);
void target_display_shutdown(void);
uint8_t target_panel_auto_detect_enabled(void);
int msm_display_off(void);
int msm_display_init(struct msm_fb_panel_data *pdata);
uint8_t target_is_edp(void);

#ifdef QCOM_ENABLE_2NDSTAGE_BOOT
int msm_display_2ndstagefb_reserve(void);
int msm_display_2ndstagefb_setup(void);
#endif

#endif
