/* Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
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

#include <debug.h>
#include <err.h>
#include <string.h>
#include <smem.h>
#include <msm_panel.h>
#include <board.h>
#include <mipi_dsi.h>
#include <platform/gpio.h>
#include <target/display.h>
#include <platform/msm_shared/timer.h>

#include <panel.h>
#include <panel_display.h>

/*---------------------------------------------------------------------------*/
/* GCDB Panel Database                                                       */
/*---------------------------------------------------------------------------*/
#include <panel_auo_720p_video.h>
#include <panel_sharp_720p_video.h>

#define DISPLAY_MAX_PANEL_DETECTION 2

#define MAX_PANEL_ID_LEN 64

/*---------------------------------------------------------------------------*/
/* static panel selection variable                                           */
/*---------------------------------------------------------------------------*/
enum {
AUO_720P_VIDEO_PANEL,
SHARP_720P_VIDEO_PANEL,
UNKNOWN_PANEL
};

/*
 * The list of panels that are supported on this target.
 * Any panel in this list can be selected using fastboot oem command.
 */
static struct panel_list supp_panels[] = {
	{"auo_720p_video", AUO_720P_VIDEO_PANEL},
	{"sharp_720p_video", SHARP_720P_VIDEO_PANEL},
};

static uint32_t panel_id;

int oem_panel_rotation()
{
	int ret = NO_ERROR;
	return ret;
}

int oem_panel_on()
{
	/* OEM can keep there panel spefic on instructions in this
	function */
	return NO_ERROR;
}

int oem_panel_off()
{
	/* OEM can keep there panel spefic off instructions in this
	function */
	return NO_ERROR;
}

static int init_panel_data(struct panel_struct *panelstruct,
			struct msm_panel_info *pinfo,
			struct mdss_dsi_phy_ctrl *phy_db)
{
	int pan_type = PANEL_TYPE_DSI;

	switch (panel_id) {
	case AUO_720P_VIDEO_PANEL:
		panelstruct->paneldata    = &auo_720p_video_panel_data;
		panelstruct->panelres     = &auo_720p_video_panel_res;
		panelstruct->color        = &auo_720p_video_color;
		panelstruct->videopanel   = &auo_720p_video_video_panel;
		panelstruct->commandpanel = &auo_720p_video_command_panel;
		panelstruct->state        = &auo_720p_video_state;
		panelstruct->laneconfig   = &auo_720p_video_lane_config;
		panelstruct->paneltiminginfo
			= &auo_720p_video_timing_info;
		panelstruct->panelresetseq
			= &auo_720p_video_panel_reset_seq;
		panelstruct->backlightinfo = &auo_720p_video_backlight;
		pinfo->mipi.panel_cmds     = auo_720p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
			= AUO_720P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
			auo_720p_video_timings, TIMING_SIZE);
		break;
	case SHARP_720P_VIDEO_PANEL:
		panelstruct->paneldata    = &sharp_720p_video_panel_data;
		panelstruct->panelres     = &sharp_720p_video_panel_res;
		panelstruct->color        = &sharp_720p_video_color;
		panelstruct->videopanel   = &sharp_720p_video_video_panel;
		panelstruct->commandpanel = &sharp_720p_video_command_panel;
		panelstruct->state        = &sharp_720p_video_state;
		panelstruct->laneconfig   = &sharp_720p_video_lane_config;
		panelstruct->paneltiminginfo
			= &sharp_720p_video_timing_info;
		panelstruct->panelresetseq
			= &sharp_720p_video_panel_reset_seq;
		panelstruct->backlightinfo = &sharp_720p_video_backlight;
		pinfo->mipi.panel_cmds     = sharp_720p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
			= SHARP_720P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
			sharp_720p_video_timings, TIMING_SIZE);
		break;
        case UNKNOWN_PANEL:
                memset(panelstruct, 0, sizeof(struct panel_struct));
                memset(pinfo->mipi.panel_cmds, 0, sizeof(struct mipi_dsi_cmd));
                pinfo->mipi.num_of_panel_cmds = 0;
                memset(phy_db->timing, 0, TIMING_SIZE);
                pinfo->mipi.signature = 0;
		pan_type = PANEL_TYPE_UNKNOWN;
                break;
	}

	return pan_type;
}

uint32_t oem_panel_max_auto_detect_panels()
{
        return 0;
}

static bool is_sharp_panel(void)
{
	gpio_tlmm_config(34, 0, 0, 0, 3u, 0);
	gpio_set_dir(34, 0);
	mdelay(3);
	return gpio_status(34);
}

int oem_panel_select(const char *panel_name, struct panel_struct *panelstruct,
			struct msm_panel_info *pinfo,
			struct mdss_dsi_phy_ctrl *phy_db)
{
	int32_t panel_override_id;

	if (panel_name) {
		panel_override_id = panel_name_to_id(supp_panels,
				ARRAY_SIZE(supp_panels), panel_name);

		if (panel_override_id < 0) {
			dprintf(CRITICAL, "Not able to search the panel:%s\n",
					 panel_name + strspn(panel_name, " "));
		} else if (panel_override_id < UNKNOWN_PANEL) {
			/* panel override using fastboot oem command */
			panel_id = panel_override_id;

			dprintf(INFO, "OEM panel override:%s\n",
					panel_name + strspn(panel_name, " "));
			goto panel_init;
		}
	}

	if (is_sharp_panel())
		panel_id = SHARP_720P_VIDEO_PANEL;
	else
		panel_id = AUO_720P_VIDEO_PANEL;

panel_init:
	return init_panel_data(panelstruct, pinfo, phy_db);
}
