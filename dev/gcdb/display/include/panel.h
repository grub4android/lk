/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _PANEL_H_
#define _PANEL_H_

/*---------------------------------------------------------------------------*/
/* HEADER files                                                              */
/*---------------------------------------------------------------------------*/
#include <debug.h>
#include <smem.h>

#define TOTAL_RESET_GPIO_CTRL 5

/*---------------------------------------------------------------------------*/
/* Structure definition                                                      */
/*---------------------------------------------------------------------------*/

/*Panel Configuration */
typedef struct panel_config{

	char  *panel_node_id;
	char  *panel_controller;
	char  *panel_compatible;
	uint16_t panel_interface;
	uint16_t panel_type;
	char   *panel_destination;
	uint32_t panel_orientation;
	uint32_t panel_clockrate;
	uint16_t panel_framerate;
	uint16_t panel_channelid;
	uint16_t dsi_virtualchannel_id;
	uint16_t panel_broadcast_mode;
	uint16_t dsi_lp11_atinit;
	uint16_t dsi_initmaster_time;
	uint16_t dsi_stream;
	uint8_t  interleave_mode;
	uint32_t panel_bitclock_freq;
	uint32_t panel_operating_mode;
	uint32_t panel_with_enable_gpio;
};

typedef struct panel_resolution{

	uint16_t panel_width;
	uint16_t panel_height;
	uint16_t hfront_porch;
	uint16_t hback_porch;
	uint16_t hpulse_width;
	uint16_t hsync_skew;
	uint16_t vfront_porch;
	uint16_t vback_porch;
	uint16_t vpulse_width;
	uint16_t hleft_border;
	uint16_t hright_border;
	uint16_t vtop_border;
	uint16_t vbottom_border;
	uint16_t hactive_res;
	uint16_t vactive_res;
	uint16_t invert_data_polarity;
	uint16_t invert_vsync_polarity;
	uint16_t invert_hsync_polarity;
};

typedef struct color_info{
	uint8_t  color_format;
	uint8_t  color_order;
	uint8_t  underflow_color;
	uint8_t  border_color;
	uint8_t  pixel_packing;
	uint8_t  pixel_alignment;
};

typedef struct command_state {
	uint8_t oncommand_state;
	uint8_t offcommand_state;
};

typedef struct videopanel_info {
	uint8_t hsync_pulse;
	uint8_t hfp_power_mode;
	uint8_t hbp_power_mode;
	uint8_t hsa_power_mode;
	uint8_t bllp_eof_power_mode;
	uint8_t bllp_power_mode;
	uint8_t traffic_mode;
	uint8_t dma_delayafter_vsync;
	uint32_t  bllp_eof_power;
};

typedef struct commandpanel_info {
	uint8_t techeck_enable;
	uint8_t tepin_select;
	uint8_t teusing_tepin;
	uint8_t autorefresh_enable;
	uint32_t autorefresh_framenumdiv;
	uint32_t tevsync_rdptr_irqline;
	uint32_t tevsync_continue_lines;
	uint32_t tevsync_startline_divisor;
	uint32_t tepercent_variance;
	uint8_t tedcs_command;
	uint8_t disable_eotafter_hsxfer;
	uint32_t cmdmode_idletime;
};

typedef struct lane_configuration {
	uint8_t dsi_lanes;
	uint8_t dsi_lanemap;
	uint8_t lane0_state;
	uint8_t lane1_state;
	uint8_t lane2_state;
	uint8_t lane3_state;
};

typedef struct panel_timing {
	uint8_t dsi_mdp_trigger;
	uint8_t dsi_dma_trigger;
	uint8_t tclk_post;
	uint8_t tclk_pre;
};

enum {
	BL_PWM = 0,
	BL_WLED,
	BL_DCS,
	BL_LPG,
};

typedef struct panel_reset_sequence {
	uint8_t pin_state[TOTAL_RESET_GPIO_CTRL];
	uint32_t sleep[TOTAL_RESET_GPIO_CTRL];
	uint8_t pin_direction;
};

typedef struct backlight {
	uint16_t bl_interface_type;
	uint16_t bl_min_level;
	uint16_t bl_max_level;
	uint16_t bl_step;
	uint16_t bl_pmic_controltype;
	char     *bl_pmic_model;
};

#endif /*_PANEL_H_ */