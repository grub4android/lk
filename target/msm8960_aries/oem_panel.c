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
#include <smem.h>
#include <msm_panel.h>
#include <board.h>
#include <mipi_dsi.h>
#include <endian.h>

#include "include/panel.h"
#include "panel_display.h"

/*---------------------------------------------------------------------------*/
/* GCDB Panel Database                                                       */
/*---------------------------------------------------------------------------*/
#include "include/panel_hitachi_720p_cmd.h"
#include "include/panel_lgd_720p_cmd.h"

/* Number of dectectable panels */
#define DISPLAY_MAX_PANEL_DETECTION 2

#define PM8921_GPIO_PANEL_ID            12

/*---------------------------------------------------------------------------*/
/* static panel selection variable                                           */
/*---------------------------------------------------------------------------*/
enum {
HITACHI_720P_CMD_PANEL,
SHARP_720P_CMD_PANEL,
LGD_720P_CMD_PANEL,
AUO_720P_CMD_PANEL,
JDI_720P_CMD_PANEL,
UNKNOWN_PANEL
};

/*
 * The list of panels that are supported on this target.
 * Any panel in this list can be selected using fastboot oem command.
 */
static struct panel_list supp_panels[] = {
	// mi2(s)
	{"hitachi_720p_cmd", HITACHI_720P_CMD_PANEL},
	{"sharp_720p_cmd", SHARP_720P_CMD_PANEL},
	// mi2a
	{"lgd_720p_cmd", LGD_720P_CMD_PANEL},
	{"auo_720p_cmd", AUO_720P_CMD_PANEL},
	{"jdi_720p_cmd", JDI_720P_CMD_PANEL},
};

static uint32_t panel_id;

static char novatek_panel_max_packet[4] = { 0x06, 0x00, 0x37, 0x80 };	/* DTYPE_SET_MAX_PACKET */
static char novatek_panel_manufacture_id[4] = { 0x04, 0x00, 0x06, 0xA0 };	/* DTYPE_DCS_READ */
static char novatek_panel_manufacture_id0[4] = { 0xBF, 0x00, 0x24, 0xA0 };	/* DTYPE_DCS_READ */

static struct mipi_dsi_cmd novatek_panel_max_packet_cmd =
    { sizeof(novatek_panel_max_packet), novatek_panel_max_packet };

static struct mipi_dsi_cmd novatek_panel_manufacture_id_cmd =
    { sizeof(novatek_panel_manufacture_id), novatek_panel_manufacture_id };

static struct mipi_dsi_cmd novatek_panel_manufacture_id0_cmd =
    { sizeof(novatek_panel_manufacture_id0), novatek_panel_manufacture_id0 };

static uint32_t mipi_novatek_manufacture_id(void)
{
	char rec_buf[24];
	char *rp = rec_buf;
	uint32_t *lp, data;

	mipi_dsi_cmds_tx(&novatek_panel_max_packet_cmd, 1);
	mipi_dsi_cmds_tx(&novatek_panel_manufacture_id_cmd, 1);
	mipi_dsi_cmds_rx(&rp, 2);

	lp = (uint32_t *) rp;
	data = (uint32_t) * lp;
	data = ntohl(data);
	data = data >> 8;
	return data;
}

static uint32_t mipi_novatek_manufacture_id0(void)
{
	char rec_buf[24];
	char *rp = rec_buf;
	uint32_t *lp, data;

	mipi_dsi_cmds_tx(&novatek_panel_manufacture_id0_cmd, 1);
	mipi_dsi_cmds_rx(&rp, 3);

	lp = (uint32_t *) rp;
	data = (uint32_t) * lp;
	data = ntohl(data);
	data = data >> 8;
	return data;
}

static int panel_id_detection()
{
	unsigned int lcd_id_det = 2;
	lcd_id_det = pmic8921_gpio_get(PM8921_GPIO_PANEL_ID);
	return lcd_id_det;
}

static void panel_manu_id_detection(uint32_t* manu_id, uint32_t* manu_id0)
{
	mipi_dsi_cmd_bta_sw_trigger();
	*manu_id = mipi_novatek_manufacture_id();
	*manu_id0 = mipi_novatek_manufacture_id0();
}

int oem_panel_rotation()
{
	/* OEM can keep there panel spefic on instructions in this
	function */
	return NO_ERROR;
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
	case HITACHI_720P_CMD_PANEL:
	case SHARP_720P_CMD_PANEL:
		panelstruct->paneldata    = &hitachi_720p_cmd_panel_data;
		panelstruct->panelres     = &hitachi_720p_cmd_panel_res;
		panelstruct->color        = &hitachi_720p_cmd_color;
		panelstruct->videopanel   = &hitachi_720p_cmd_video_panel;
		panelstruct->commandpanel = &hitachi_720p_cmd_command_panel;
		panelstruct->state        = &hitachi_720p_cmd_state;
		panelstruct->laneconfig   = &hitachi_720p_cmd_lane_config;
		panelstruct->paneltiminginfo
					 = &hitachi_720p_cmd_timing_info;
		panelstruct->panelresetseq
					 = &hitachi_720p_cmd_panel_reset_seq;
		panelstruct->backlightinfo = &hitachi_720p_cmd_backlight;
		if(panel_id==HITACHI_720P_CMD_PANEL) {
			pinfo->mipi.panel_cmds
						= hitachi_720p_cmd_on_command;
			pinfo->mipi.num_of_panel_cmds
						= HITACHI_720P_CMD_ON_COMMAND;
		}
		else {
			pinfo->mipi.panel_cmds
					= sharp_720p_cmd_on_command;
			pinfo->mipi.num_of_panel_cmds
					= SHARP_720P_CMD_ON_COMMAND;
		}
		memcpy(phy_db->timing,
				hitachi_720p_cmd_timings, TIMING_SIZE);
		break;
	case LGD_720P_CMD_PANEL:
	case AUO_720P_CMD_PANEL:
	case JDI_720P_CMD_PANEL:
		panelstruct->paneldata    = &lgd_720p_cmd_panel_data;
		panelstruct->panelres     = &lgd_720p_cmd_panel_res;
		panelstruct->color        = &lgd_720p_cmd_color;
		panelstruct->videopanel   = &lgd_720p_cmd_video_panel;
		panelstruct->commandpanel = &lgd_720p_cmd_command_panel;
		panelstruct->state        = &lgd_720p_cmd_state;
		panelstruct->laneconfig   = &lgd_720p_cmd_lane_config;
		panelstruct->paneltiminginfo
					 = &lgd_720p_cmd_timing_info;
		panelstruct->panelresetseq
					 = &lgd_720p_cmd_panel_reset_seq;
		panelstruct->backlightinfo = &lgd_720p_cmd_backlight;
		if(panel_id==LGD_720P_CMD_PANEL) {
			pinfo->mipi.panel_cmds
						= lgd_720p_cmd_on_command;
			pinfo->mipi.num_of_panel_cmds
						= LGD_720P_CMD_ON_COMMAND;
		}
		else if(panel_id==AUO_720P_CMD_PANEL) {
			pinfo->mipi.panel_cmds
					= auo_720p_cmd_on_command;
			pinfo->mipi.num_of_panel_cmds
					= AUO_720P_CMD_ON_COMMAND;
		}
		else {
			pinfo->mipi.panel_cmds
					= jdi_720p_cmd_on_command;
			pinfo->mipi.num_of_panel_cmds
					= JDI_720P_CMD_ON_COMMAND;
		}
		memcpy(phy_db->timing,
				lgd_720p_cmd_timings, TIMING_SIZE);
		break;
	case UNKNOWN_PANEL:
		memset(panelstruct, 0, sizeof(struct panel_struct));
		memset(pinfo->mipi.panel_cmds, 0, sizeof(struct mipi_dsi_cmd));
		pinfo->mipi.num_of_panel_cmds = 0;
		memset(phy_db->timing, 0, TIMING_SIZE);
		pinfo->mipi.signature = 0;
		dprintf(CRITICAL, "Unknown Panel");
		return PANEL_TYPE_UNKNOWN;
	default:
		dprintf(CRITICAL, "Panel ID not detected %d\n", panel_id);
		return PANEL_TYPE_UNKNOWN;
	}
	return pan_type;
}

uint32_t oem_panel_max_auto_detect_panels()
{
	return target_panel_auto_detect_enabled() ?
				DISPLAY_MAX_PANEL_DETECTION : 0;
}

int oem_panel_select(const char *panel_name, struct panel_struct *panelstruct,
			struct msm_panel_info *pinfo,
			struct mdss_dsi_phy_ctrl *phy_db)
{
	uint32_t hw_id = board_hardware_id();
	uint32_t target_id = board_target_id();
	int32_t panel_override_id;
	uint32_t *manu_id, *manu_id0;

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

	switch (target_id) {
	case LINUX_MACHTYPE_8064_MTP:
	case LINUX_MACHTYPE_8064_MITWO:
		if (panel_id_detection())
			panel_id = HITACHI_720P_CMD_PANEL;
		else
			panel_id = SHARP_720P_CMD_PANEL;
		break;
	case LINUX_MACHTYPE_8960_CDP:
	case LINUX_MACHTYPE_8960_MITWOA:
		panel_manu_id_detection(&manu_id, &manu_id0);
		if ((manu_id) && (manu_id0))
			panel_id = LGD_720P_CMD_PANEL;
		if ((manu_id)&& !manu_id0)
			panel_id = AUO_720P_CMD_PANEL;
		if (manu_id == 0x60 && manu_id0 == 0x141304)
			panel_id = JDI_720P_CMD_PANEL;
		else {
			dprintf(CRITICAL, "Unknown panel\n");
			return PANEL_TYPE_UNKNOWN;
		}
		break;
	default:
		dprintf(CRITICAL, "Display not enabled for %d HW type\n"
								, hw_id);
		return PANEL_TYPE_UNKNOWN;
	}

panel_init:
	return init_panel_data(panelstruct, pinfo, phy_db);
}
