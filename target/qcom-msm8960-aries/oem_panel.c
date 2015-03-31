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

#include <err.h>
#include <reg.h>
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <target/display.h>
#include <platform/mdp4.h>
#include <platform/board.h>
#include <platform/iomap.h>
#include <platform/msm8960.h>
#include <platform/mipi_dsi.h>
#include <platform/msm_panel.h>
#include <platform/qcom_timer.h>
#include <dev/pmic/pm8921.h>
#include <dev/gcdb/gcdb_display.h>

/*---------------------------------------------------------------------------*/
/* GCDB Panel Database                                                       */
/*---------------------------------------------------------------------------*/
#include <dev/gcdb/panels/panel_hitachi_720p_cmd.h>
#include <dev/gcdb/panels/panel_lgd_720p_cmd.h>

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
    { sizeof(novatek_panel_max_packet), novatek_panel_max_packet, 0 };

static struct mipi_dsi_cmd novatek_panel_manufacture_id_cmd =
    { sizeof(novatek_panel_manufacture_id), novatek_panel_manufacture_id, 0 };

static struct mipi_dsi_cmd novatek_panel_manufacture_id0_cmd =
    { sizeof(novatek_panel_manufacture_id0), novatek_panel_manufacture_id0, 0 };

static int mipi_dsi_cmd_bta_sw_trigger(void)
{
	uint32_t data;
	int cnt = 0;
	int err = 0;

	writel(0x01, MIPI_DSI_BASE + 0x094);	/* trigger */
	while (cnt < 10000) {
		data = readl(MIPI_DSI_BASE + 0x0004);	/*DSI_STATUS */
		if ((data & 0x0010) == 0)
			break;
		cnt++;
	}
	if (cnt == 10000)
		err = 1;
	return err;
}

static void trigger_mdp_dsi(void)
{
	DSB;
	mdp_start_dma();
	mdelay(10);
	DSB;
	writel(0x1, DSI_CMD_MODE_MDP_SW_TRIGGER);
}

static int
config_dsi_cmd_mode(unsigned short disp_width, unsigned short disp_height,
		    unsigned short img_width, unsigned short img_height,
		    unsigned short dst_format,
		    unsigned short traffic_mode, unsigned short datalane_num, int rgb_swap)
{
	unsigned char DST_FORMAT;
	//unsigned char TRAFIC_MODE;
	unsigned char DLNx_EN;
	// video mode data ctrl
	int status = 0;
	unsigned char interleav = 0;
	unsigned char ystride = 0x03;
	// disable mdp first

	writel(0x00000000, DSI_CLK_CTRL);
	writel(0x00000000, DSI_CLK_CTRL);
	writel(0x00000000, DSI_CLK_CTRL);
	writel(0x00000000, DSI_CLK_CTRL);
	writel(0x00000002, DSI_CLK_CTRL);
	writel(0x00000006, DSI_CLK_CTRL);
	writel(0x0000000e, DSI_CLK_CTRL);
	writel(0x0000001e, DSI_CLK_CTRL);
	writel(0x0000003e, DSI_CLK_CTRL);

	writel(0x13ff3fe0, DSI_ERR_INT_MASK0);

	// writel(0, DSI_CTRL);

	// writel(0, DSI_ERR_INT_MASK0);

	DST_FORMAT = 8;		// RGB888
	dprintf(SPEW, "DSI_Cmd_Mode - Dst Format: RGB888\n");

	switch (datalane_num) {
	default:
	case 1:
		DLNx_EN = 1;
		break;
	case 2:
		DLNx_EN = 3;
		break;
	case 3:
		DLNx_EN = 7;
		break;
	case 4:
		DLNx_EN = 0xF;
		break;
	}

	//TRAFIC_MODE = 0;	// non burst mode with sync pulses
	dprintf(SPEW, "Traffic mode: non burst mode with sync pulses\n");

	writel(0x02020202, DSI_INT_CTRL);

	int data = 0x00100000;
	data |= ((rgb_swap & 0x07) << 16);
	writel(data | DST_FORMAT, DSI_COMMAND_MODE_MDP_CTRL);

	writel((img_width * ystride + 1) << 16 | 0x0039,
	       DSI_COMMAND_MODE_MDP_STREAM0_CTRL);
	writel((img_width * ystride + 1) << 16 | 0x0039,
	       DSI_COMMAND_MODE_MDP_STREAM1_CTRL);
	writel(img_height << 16 | img_width,
	       DSI_COMMAND_MODE_MDP_STREAM0_TOTAL);
	writel(img_height << 16 | img_width,
	       DSI_COMMAND_MODE_MDP_STREAM1_TOTAL);
	writel(0xEE, DSI_CAL_STRENGTH_CTRL);
	writel(0x80000000, DSI_CAL_CTRL);
	writel(0x40, DSI_TRIG_CTRL);
	writel(0x13c2c, DSI_COMMAND_MODE_MDP_DCS_CMD_CTRL);
	writel(interleav << 30 | 0 << 24 | 0 << 20 | DLNx_EN << 4 | 0x105,
	       DSI_CTRL);
	mdelay(10);
	writel(0x14000000, DSI_COMMAND_MODE_DMA_CTRL);
	writel(0x10000000, DSI_MISR_CMD_CTRL);
	writel(0x13ff3fe0, DSI_ERR_INT_MASK0);
	writel(0x1, DSI_EOT_PACKET_CTRL);
	// writel(0x0, MDP_OVERLAYPROC0_START);

	trigger_mdp_dsi();

	status = 1;
	return status;
}

static int mipi_dsi_cmd_trigger(struct msm_panel_info *pinfo, struct fbcon_config *fb)
{
	unsigned short display_wd = pinfo->xres;
	unsigned short display_ht = pinfo->yres;
	unsigned short image_wd = pinfo->xres;
	unsigned short image_ht = pinfo->yres;
	unsigned short dst_format = 0;
	unsigned short traffic_mode = 0;
	unsigned short num_of_lanes = pinfo->mipi.num_of_lanes;

	mdp_dsi_cmd_config(pinfo, fb);
	mdelay(50);
	config_dsi_cmd_mode(display_wd, display_ht, image_wd, image_ht,
			dst_format, traffic_mode,
			num_of_lanes, /* num_of_lanes */
			pinfo->mipi.rgb_swap);

	mdelay(34);

	// set fbcon update callback
	fb->update_start = trigger_mdp_dsi;

	return 0;
}

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

static int panel_id_detection(void)
{
	unsigned int lcd_id_det = 2;
	lcd_id_det = pmic8921_gpio_get(PM8921_GPIO_PANEL_ID);
	return lcd_id_det;
}

static void panel_manu_id_detection(uint32_t* manu_id, uint32_t* manu_id0)
{
	mdss_dsi_cmd_bta_sw_trigger(MIPI_DSI_BASE);
	*manu_id = mipi_novatek_manufacture_id();
	*manu_id0 = mipi_novatek_manufacture_id0();
}

int oem_panel_rotation(void)
{
	/* OEM can keep there panel spefic on instructions in this
	function */
	return NO_ERROR;
}


int oem_panel_on(void)
{
	/* OEM can keep there panel spefic on instructions in this
	function */
	return NO_ERROR;
}

int oem_panel_off(void)
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
			pinfo->mipi.panel_on_cmds
						= hitachi_720p_cmd_on_command;
			pinfo->mipi.num_of_panel_on_cmds
						= HITACHI_720P_CMD_ON_COMMAND;
			pinfo->mipi.panel_off_cmds
						= hitachi_720p_cmd_off_command;
			pinfo->mipi.num_of_panel_off_cmds
						= HITACHI_720P_CMD_OFF_COMMAND;
		}
		else {
			pinfo->mipi.panel_on_cmds
					= sharp_720p_cmd_on_command;
			pinfo->mipi.num_of_panel_on_cmds
					= SHARP_720P_CMD_ON_COMMAND;
			pinfo->mipi.panel_off_cmds
					= sharp_720p_cmd_off_command;
			pinfo->mipi.num_of_panel_off_cmds
					= SHARP_720P_CMD_OFF_COMMAND;
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
			pinfo->mipi.panel_on_cmds
						= lgd_720p_cmd_on_command;
			pinfo->mipi.num_of_panel_on_cmds
						= LGD_720P_CMD_ON_COMMAND;
			pinfo->mipi.panel_off_cmds
						= lgd_720p_cmd_off_command;
			pinfo->mipi.num_of_panel_off_cmds
						= LGD_720P_CMD_OFF_COMMAND;
		}
		else if(panel_id==AUO_720P_CMD_PANEL) {
			pinfo->mipi.panel_on_cmds
					= auo_720p_cmd_on_command;
			pinfo->mipi.num_of_panel_on_cmds
					= AUO_720P_CMD_ON_COMMAND;
			pinfo->mipi.panel_off_cmds
					= auo_720p_cmd_off_command;
			pinfo->mipi.num_of_panel_off_cmds
					= AUO_720P_CMD_OFF_COMMAND;
		}
		else {
			pinfo->mipi.panel_on_cmds
					= jdi_720p_cmd_on_command;
			pinfo->mipi.num_of_panel_on_cmds
					= JDI_720P_CMD_ON_COMMAND;
			pinfo->mipi.panel_off_cmds
					= jdi_720p_cmd_off_command;
			pinfo->mipi.num_of_panel_off_cmds
					= JDI_720P_CMD_OFF_COMMAND;
		}
		memcpy(phy_db->timing,
				lgd_720p_cmd_timings, TIMING_SIZE);
		break;
	case UNKNOWN_PANEL:
		memset(panelstruct, 0, sizeof(struct panel_struct));
		memset(pinfo->mipi.panel_on_cmds, 0, sizeof(struct mipi_dsi_cmd));
		pinfo->mipi.num_of_panel_on_cmds = 0;
		memset(pinfo->mipi.panel_off_cmds, 0, sizeof(struct mipi_dsi_cmd));
		pinfo->mipi.num_of_panel_off_cmds = 0;
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

uint32_t oem_panel_max_auto_detect_panels(void)
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
	uint32_t manu_id, manu_id0;

	if (panel_name && panel_name[0]) {
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

	pinfo->early_config = mipi_dsi_cmd_trigger;

panel_init:
	return init_panel_data(panelstruct, pinfo, phy_db);
}
