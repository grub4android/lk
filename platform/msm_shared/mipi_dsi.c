/* Copyright (c) 2010-2014, The Linux Foundation. All rights reserved.
 * Copyright (c) 2011-2014, Xiaomi Corporation. All rights reserved.
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

#include <reg.h>
#include <endian.h>
#include <mipi_dsi.h>
#include <dev/fbcon.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>
#include <target.h>
#include <target/display.h>
#include <platform/iomap.h>
#include <platform/clock.h>
#include <platform/timer.h>
#include <err.h>
#include <msm_panel.h>
#include <platform/msm_shared/timer.h>

extern void mdp_disable(void);
extern void mdp_start_dma(void);
extern int mipi_dsi_cmd_config(struct fbcon_config mipi_fb_cfg,
			unsigned short num_of_lanes);

#ifndef MIPI_DSI0_BASE
#define MIPI_DSI0_BASE MIPI_DSI_BASE
#endif

#ifndef MIPI_DSI1_BASE
#define MIPI_DSI1_BASE MIPI_DSI_BASE
#endif

static struct fbcon_config mipi_fb_cfg __UNUSED = {
	.height = 0,
	.width = 0,
	.stride = 0,
	.format = 0,
	.bpp = 0,
	.update_start = NULL,
	.update_done = NULL,
};

void secure_writel(uint32_t, uint32_t);
uint32_t secure_readl(uint32_t);

static uint32_t response_value = 0;

static char read_id_a1h_cmd[4] = { 0xA1, 0x00, 0x06, 0xA0 };	/* DTYPE_DCS_READ */
static struct mipi_dsi_cmd read_ddb_start_cmd __UNUSED =
	{sizeof(read_id_a1h_cmd), read_id_a1h_cmd, 0};

uint32_t mdss_dsi_read_panel_signature(uint32_t panel_signature)
{
	uint32_t rec_buf[1];
	uint32_t *lp __UNUSED = rec_buf, data __UNUSED;
	uint32_t ret = response_value;

#if (DISPLAY_TYPE_MDSS == 1)
	if (ret && ret != panel_signature)
		goto exit_read_signature;

	ret = mipi_dsi_cmds_tx(&read_ddb_start_cmd, 1);
	if (ret)
		goto exit_read_signature;
	if (!mdss_dsi_cmds_rx(&lp, 1, 1))
		goto exit_read_signature;

	data = ntohl(*lp);
	data = data >> 8;
	response_value = data;
	if (response_value != panel_signature)
		ret = response_value;

exit_read_signature:
	/* Keep the non detectable panel at the end and set panel signature 0xFFFF */
	if ((panel_signature == 0) || (panel_signature == 0xFFFF))
		ret = 0;
#endif
	return ret;
}

int mdss_dual_dsi_cmd_dma_trigger_for_panel(void)
{
	uint32_t ReadValue __UNUSED;
	uint32_t count __UNUSED = 0;
	int status = 0;

#if (DISPLAY_TYPE_MDSS == 1)
	writel(0x03030303, MIPI_DSI0_BASE + INT_CTRL);
	writel(0x1, MIPI_DSI0_BASE + CMD_MODE_DMA_SW_TRIGGER);
	dsb();

	writel(0x03030303, MIPI_DSI1_BASE + INT_CTRL);
	writel(0x1, MIPI_DSI1_BASE + CMD_MODE_DMA_SW_TRIGGER);
	dsb();

	ReadValue = readl(MIPI_DSI1_BASE + INT_CTRL) & 0x00000001;
	while (ReadValue != 0x00000001) {
		ReadValue = readl(MIPI_DSI1_BASE + INT_CTRL) & 0x00000001;
		count++;
		if (count > 0xffff) {
			status = FAIL;
			dprintf(CRITICAL,
				"Panel CMD: command mode dma test failed\n");
			return status;
		}
	}

	writel((readl(MIPI_DSI1_BASE + INT_CTRL) | 0x01000001),
			MIPI_DSI1_BASE + INT_CTRL);
	dprintf(SPEW, "Panel CMD: command mode dma tested successfully\n");
#endif
	return status;
}

int dsi_cmd_dma_trigger_for_panel(void)
{
	unsigned long ReadValue;
	unsigned long count = 0;
	int status = 0;

	writel(0x03030303, DSI_INT_CTRL);
	dsb();
	writel(0x1, DSI_CMD_MODE_DMA_SW_TRIGGER);
	dsb();
	ReadValue = readl(DSI_INT_CTRL) & 0x00000001;
	while (ReadValue != 0x00000001) {
		ReadValue = readl(DSI_INT_CTRL) & 0x00000001;
		count++;
		if (count > 0xffff) {
			status = FAIL;
			dprintf(CRITICAL,
				"Panel CMD: command mode dma test failed\n");
			return status;
		}
	}

	writel((readl(DSI_INT_CTRL) | 0x01000001), DSI_INT_CTRL);
	dsb();
	dprintf(SPEW, "Panel CMD: command mode dma tested successfully\n");
	return status;
}

int mdss_dsi_wait4_video_done(void)
{
	unsigned long read;
	unsigned long count = 0;
	int status = 0;

	/* If video mode is not enabled, return here */
	if ((readl(DSI_CTRL) & BIT(1)) == 0)
		return 0;

	read = readl(DSI_INT_CTRL);
	/* Enable VIDEO MODE DONE MASK and clear the interrupt */
	read = read | DSI_VIDEO_MODE_DONE_MASK | DSI_VIDEO_MODE_DONE_AK;
	writel(read, DSI_INT_CTRL);
	dsb();
	read = readl(DSI_INT_CTRL) & DSI_VIDEO_MODE_DONE_STAT;
	while (!read) {
		read = readl(DSI_INT_CTRL) & DSI_VIDEO_MODE_DONE_STAT;
		count++;
		if (count > 0xffff) {
			status = FAIL;
			dprintf(CRITICAL,
				"Panel CMD: Did not recieve video mode done interrupt\n");
			return status;
		}
	}

	writel((readl(DSI_INT_CTRL) | 0x01000001), DSI_INT_CTRL);
	dprintf(SPEW, "Panel wait_4_video_done: Recieved video mode done ack\n");

	/* Skip BLLP 4ms */
	mdelay(4);

	return status;

}

int mdss_dual_dsi_cmds_tx(struct mipi_dsi_cmd *cmds, int count)
{
	int ret = 0;
	struct mipi_dsi_cmd *cm __UNUSED;
	int i __UNUSED = 0;
	char pload[256] __UNUSED;
	uint32_t off __UNUSED;

#if (DISPLAY_TYPE_MDSS == 1)
	/* Align pload at 8 byte boundry */
	off = (uint32_t)pload;
	off &= 0x07;
	if (off)
		off = 8 - off;
	off += (uint32_t)pload;

	cm = cmds;
	for (i = 0; i < count; i++) {
		/* Wait for VIDEO_MODE_DONE */
		ret = mdss_dsi_wait4_video_done();
		if (ret)
			goto wait4video_error;

		memcpy((void *)off, (cm->payload), cm->size);
		writel(off, MIPI_DSI0_BASE + DMA_CMD_OFFSET);
		writel(cm->size, MIPI_DSI0_BASE + DMA_CMD_LENGTH);	// reg 0x48 for this build
		writel(off, MIPI_DSI1_BASE + DMA_CMD_OFFSET);
		writel(cm->size, MIPI_DSI1_BASE + DMA_CMD_LENGTH);	// reg 0x48 for this build
		dsb();
		ret += mdss_dual_dsi_cmd_dma_trigger_for_panel();
		if (cm->wait)
			mdelay(cm->wait);
		else
			udelay(80);
		cm++;
	}
wait4video_error:
#endif
	return ret;
}

int mdss_dsi_cmds_rx(uint32_t **rp, int rp_len, int rdbk_len)
{
	uint32_t *lp, data;
	char *dp;
	int i, off;
	int rlen, res;

	if (rdbk_len > rp_len) {
		return 0;
	}

	if (rdbk_len <= 2)
		rlen = 4;	/* short read */
	else
		rlen = MIPI_DSI_MRPS + 6;	/* 4 bytes header + 2 bytes crc */

	if (rlen > MIPI_DSI_REG_LEN) {
		return 0;
	}

	res = rlen & 0x03;

	rlen += res;		/* 4 byte align */
	lp = *rp;

	rlen += 3;
	rlen >>= 2;

	if (rlen > 4)
		rlen = 4;	/* 4 x 32 bits registers only */

	off = RDBK_DATA0;
	off += ((rlen - 1) * 4);

	for (i = 0; i < rlen; i++) {
		data = readl(MIPI_DSI_BASE + off);
		*lp = ntohl(data);	/* to network byte order */
		lp++;

		off -= 4;
	}

	if (rdbk_len > 2) {
		/*First 4 bytes + paded bytes will be header next len bytes would be payload */
		for (i = 0; i < rdbk_len; i++) {
			dp = (char*)*rp;
			dp[i] = dp[(res + i) >> 2];
		}
	}
	return rdbk_len;
}

int mipi_dsi_cmds_tx(struct mipi_dsi_cmd *cmds, int count)
{
	int ret = 0;
	struct mipi_dsi_cmd *cm;
	int i = 0;
	char pload[256];
	uint32_t off;

	/* Align pload at 8 byte boundry */
	off = (uint32_t)pload;
	off &= 0x07;
	if (off)
		off = 8 - off;
	off += (uint32_t)pload;

	cm = cmds;
	for (i = 0; i < count; i++) {
		/* Wait for VIDEO_MODE_DONE */
		ret = mdss_dsi_wait4_video_done();
		if (ret)
			goto mipi_cmds_error;

		memcpy((void *)off, (cm->payload), cm->size);
		writel(off, DSI_DMA_CMD_OFFSET);
		writel(cm->size, DSI_DMA_CMD_LENGTH);	// reg 0x48 for this build
		dsb();
		ret += dsi_cmd_dma_trigger_for_panel();
		dsb();
		if (cm->wait)
			mdelay(cm->wait);
		else
			udelay(80);
		cm++;
	}
mipi_cmds_error:
	return ret;
}

/*
 * mipi_dsi_cmd_rx: can receive at most 16 bytes
 * per transaction since it only have 4 32bits reigsters
 * to hold data.
 * therefore Maximum Return Packet Size need to be set to 16.
 * any return data more than MRPS need to be break down
 * to multiple transactions.
 */
int mipi_dsi_cmds_rx(char **rp, int len)
{
	uint32_t *lp, data;
	char *dp;
	int i, off, cnt;
	int rlen, res;

	if (len <= 2)
		rlen = 4;	/* short read */
	else
		rlen = MIPI_DSI_MRPS + 6;	/* 4 bytes header + 2 bytes crc */

	if (rlen > MIPI_DSI_REG_LEN) {
		return 0;
	}

	res = rlen & 0x03;

	rlen += res;		/* 4 byte align */
	lp = (uint32_t *) (*rp);

	cnt = rlen;
	cnt += 3;
	cnt >>= 2;

	if (cnt > 4)
		cnt = 4;	/* 4 x 32 bits registers only */

	off = 0x068;		/* DSI_RDBK_DATA0 */
	off += ((cnt - 1) * 4);

	for (i = 0; i < cnt; i++) {
		data = (uint32_t) readl(MIPI_DSI_BASE + off);
		*lp++ = ntohl(data);	/* to network byte order */
		off -= 4;
	}

	if (len > 2) {
		/*First 4 bytes + paded bytes will be header next len bytes would be payload */
		for (i = 0; i < len; i++) {
			dp = *rp;
			dp[i] = dp[4 + res + i];
		}
	}

	return len;
}

int mipi_dsi_cmd_bta_sw_trigger(void)
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

int mdss_dsi_host_init(struct mipi_dsi_panel_config *pinfo, uint32_t
		dual_dsi, uint32_t broadcast)
{
	uint8_t DMA_STREAM1 __UNUSED = 0;	// for mdp display processor path
	uint8_t EMBED_MODE1 __UNUSED = 1;	// from frame buffer
	uint8_t POWER_MODE2 __UNUSED = 1;	// from frame buffer
	uint8_t PACK_TYPE1 __UNUSED;		// long packet
	uint8_t VC1 __UNUSED = 0;
	uint8_t DT1 __UNUSED = 0;	// non embedded mode
	uint8_t WC1 __UNUSED = 0;	// for non embedded mode only
	uint8_t DLNx_EN __UNUSED;
	uint8_t lane_swap __UNUSED = 0;
	uint32_t timing_ctl __UNUSED = 0;
	uint32_t lane_swap_dsi1 __UNUSED = 0;
	uint32_t ctrl_mode __UNUSED = 0x105;	//Default is command mode to send cmds.

#if (DISPLAY_TYPE_MDSS == 1)
	switch (pinfo->num_of_lanes) {
	default:
	case 1:
		DLNx_EN = 1;	// 1 lane
		break;
	case 2:
		DLNx_EN = 3;	// 2 lane
		break;
	case 3:
		DLNx_EN = 7;	// 3 lane
		break;
	case 4:
		DLNx_EN = 0x0F;	/* 4 lanes */
		break;
	}

	PACK_TYPE1 = pinfo->pack;
	lane_swap = pinfo->lane_swap;
	timing_ctl = ((pinfo->t_clk_post << 8) | pinfo->t_clk_pre);

	if (pinfo->cmds_post_tg) {
		/*
		 * Need to send pixel data before sending the ON commands
		 * so need to configure controller to VIDEO MODE.
		 */
		ctrl_mode = 0x103;
	}

	if (dual_dsi) {
		writel(0x0001, MIPI_DSI1_BASE + SOFT_RESET);
		writel(0x0000, MIPI_DSI1_BASE + SOFT_RESET);

		writel((0 << 16) | 0x3f, MIPI_DSI1_BASE + CLK_CTRL);	/* Turn on all DSI Clks */
		writel(DMA_STREAM1 << 8 | 0x04, MIPI_DSI1_BASE + TRIG_CTRL);	// reg 0x80 dma trigger: sw
		// trigger 0x4; dma stream1

		writel(0 << 30 | DLNx_EN << 4 | ctrl_mode, MIPI_DSI1_BASE + CTRL);	// reg 0x00 for this
		// build
		writel(broadcast << 31 | EMBED_MODE1 << 28 | POWER_MODE2 << 26
				| PACK_TYPE1 << 24 | VC1 << 22 | DT1 << 16 | WC1,
				MIPI_DSI1_BASE + COMMAND_MODE_DMA_CTRL);

		if (readl(MIPI_DSI_BASE) == DSI_HW_REV_103_1) /*for 8939 hw dsi1 has Lane_map as 3210*/
			lane_swap_dsi1 = 0x7;
		else
			lane_swap_dsi1 = lane_swap;
		writel(lane_swap_dsi1, MIPI_DSI1_BASE + LANE_SWAP_CTL);
		writel(timing_ctl, MIPI_DSI1_BASE + TIMING_CTL);
	}

	writel(0x0001, MIPI_DSI0_BASE + SOFT_RESET);
	writel(0x0000, MIPI_DSI0_BASE + SOFT_RESET);

	writel((0 << 16) | 0x3f, MIPI_DSI0_BASE + CLK_CTRL);	/* Turn on all DSI Clks */
	writel(DMA_STREAM1 << 8 | 0x04, MIPI_DSI0_BASE + TRIG_CTRL);	// reg 0x80 dma trigger: sw
	// trigger 0x4; dma stream1

	writel(0 << 30 | DLNx_EN << 4 | ctrl_mode, MIPI_DSI0_BASE + CTRL);	// reg 0x00 for this
	// build
	writel(broadcast << 31 | EMBED_MODE1 << 28 | POWER_MODE2 << 26
	       | PACK_TYPE1 << 24 | VC1 << 22 | DT1 << 16 | WC1,
	       MIPI_DSI0_BASE + COMMAND_MODE_DMA_CTRL);

	writel(lane_swap, MIPI_DSI0_BASE + LANE_SWAP_CTL);
	writel(timing_ctl, MIPI_DSI0_BASE + TIMING_CTL);
#endif

	return 0;
}

void mdss_dsi_panel_shutdown(struct msm_panel_info *pinfo)
{
#if (DISPLAY_TYPE_MDSS == 1)
	unsigned long read_val = 0;
	if (pinfo->mipi.panel_off_cmds) {
		/*
		 * Once MDP TG is disabled, reset of DSI controller is
		 * needed before we send panel OFF commands.
		 */
		if (pinfo->type == MIPI_VIDEO_PANEL) {
			read_val = readl(MIPI_DSI0_BASE + CTRL);
			writel((read_val & ~BIT(0)), MIPI_DSI0_BASE + CTRL);
			writel(0x0001, MIPI_DSI0_BASE + SOFT_RESET);
			dsb();
			writel(0x0000, MIPI_DSI0_BASE + SOFT_RESET);
			dsb();
			/* Enable cmd mode only */
			writel(((read_val & ~BIT(1)) | BIT(2)),
						MIPI_DSI0_BASE + CTRL);
		}

		if (pinfo->mipi.broadcast) {
			if (pinfo->type == MIPI_VIDEO_PANEL) {
				read_val = readl(MIPI_DSI1_BASE + CTRL);
				writel((read_val & ~BIT(0)),
					MIPI_DSI1_BASE + CTRL);

				writel(0x0001, MIPI_DSI1_BASE + SOFT_RESET);
				dsb();
				writel(0x0000, MIPI_DSI1_BASE + SOFT_RESET);
				dsb();

				writel(((read_val & ~BIT(1)) | BIT(2)),
							MIPI_DSI1_BASE + CTRL);
			}
			mdss_dual_dsi_cmds_tx(pinfo->mipi.panel_off_cmds,
					pinfo->mipi.num_of_panel_off_cmds);
		} else {
			mipi_dsi_cmds_tx(pinfo->mipi.panel_off_cmds,
					pinfo->mipi.num_of_panel_off_cmds);
		}
	}
#endif
}

int mdss_dsi_panel_initialize(struct mipi_dsi_panel_config *pinfo, uint32_t
		broadcast)
{
	int status = 0;
	uint32_t ctrl_mode __UNUSED = 0;

#if (DISPLAY_TYPE_MDSS == 1)
	if (pinfo->panel_on_cmds) {

		ctrl_mode = readl(MIPI_DSI0_BASE + CTRL);
		if (broadcast) {
			/* Enable command mode before sending the commands. */
			writel(ctrl_mode | 0x04, MIPI_DSI0_BASE + CTRL);
			writel(ctrl_mode | 0x04, MIPI_DSI1_BASE + CTRL);
			status = mdss_dual_dsi_cmds_tx(pinfo->panel_on_cmds,
					pinfo->num_of_panel_on_cmds);
			writel(ctrl_mode, MIPI_DSI0_BASE + CTRL);
			writel(ctrl_mode, MIPI_DSI1_BASE + CTRL);

		} else {
			/* Enable command mode before sending the commands. */
			writel(ctrl_mode | 0x04, MIPI_DSI0_BASE + CTRL);
			status = mipi_dsi_cmds_tx(pinfo->panel_on_cmds,
					pinfo->num_of_panel_on_cmds);
			writel(ctrl_mode, MIPI_DSI0_BASE + CTRL);
			if (!status && target_panel_auto_detect_enabled())
				status =
					mdss_dsi_read_panel_signature(pinfo->signature);
			dprintf(SPEW, "Read panel signature status = 0x%x \n", status);
		}
	}
#endif
	return status;
}

int mipi_dsi_panel_initialize(struct mipi_dsi_panel_config *pinfo)
{
	uint8_t DMA_STREAM1 = 0;	// for mdp display processor path
	uint8_t EMBED_MODE1 = 1;	// from frame buffer
	uint8_t POWER_MODE2 = 1;	// from frame buffer
	uint8_t PACK_TYPE1;		// long packet
	uint8_t VC1 = 0;
	uint8_t DT1 = 0;	// non embedded mode
	uint8_t WC1 = 0;	// for non embedded mode only
	int status = 0;
	uint8_t DLNx_EN;

	switch (pinfo->num_of_lanes) {
	default:
	case 1:
		DLNx_EN = 1;	// 1 lane
		break;
	case 2:
		DLNx_EN = 3;	// 2 lane
		break;
	case 3:
		DLNx_EN = 7;	// 3 lane
		break;
	case 4:
		DLNx_EN = 0x0F;	/* 4 lanes */
		break;
	}

	PACK_TYPE1 = pinfo->pack;

	writel(0x0001, DSI_SOFT_RESET);
	writel(0x0000, DSI_SOFT_RESET);

	writel((0 << 16) | 0x3f, DSI_CLK_CTRL);	/* Turn on all DSI Clks */
	writel(DMA_STREAM1 << 8 | 0x04, DSI_TRIG_CTRL);	// reg 0x80 dma trigger: sw
	// trigger 0x4; dma stream1

	writel(0 << 30 | DLNx_EN << 4 | 0x105, DSI_CTRL);	// reg 0x00 for this
	// build
	writel(EMBED_MODE1 << 28 | POWER_MODE2 << 26
	       | PACK_TYPE1 << 24 | VC1 << 22 | DT1 << 16 | WC1,
	       DSI_COMMAND_MODE_DMA_CTRL);

	if (pinfo->panel_on_cmds)
		status = mipi_dsi_cmds_tx(pinfo->panel_on_cmds,
					  pinfo->num_of_panel_on_cmds);

	return status;
}

#if TARGET_MSM8960_ARIES
static int aries_display_initialized = 0;
void trigger_mdp_dsi(void)
{
#if !BOOT_2NDSTAGE
	if(!aries_display_initialized)
		return;
#endif

	dsb();
	mdp_start_dma();
	mdelay(10);
	dsb();
	writel(0x1, DSI_CMD_MODE_MDP_SW_TRIGGER);
	mdelay(10);
}

int
config_dsi_cmd_mode(unsigned short disp_width, unsigned short disp_height,
		    unsigned short img_width, unsigned short img_height,
		    unsigned short dst_format,
		    unsigned short traffic_mode, unsigned short datalane_num, int rgb_swap)
{
	unsigned char DST_FORMAT;
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

	aries_display_initialized = 1;
	trigger_mdp_dsi();

	status = 1;
	return status;
}

void mipi_dsi_cmd_trigger(struct msm_fb_panel_data *pdata)
{
	struct msm_panel_info *pinfo = NULL;
	struct fbcon_config mipi_fb_cfg;

	if (pdata)
		pinfo = &(pdata->panel_info);
	else
		return;

	unsigned short display_wd = pinfo->xres;
	unsigned short display_ht = pinfo->yres;
	unsigned short image_wd = pinfo->xres;
	unsigned short image_ht = pinfo->yres;
	unsigned short dst_format = 0;
	unsigned short traffic_mode = 0;
	unsigned short num_of_lanes = pinfo->mipi.num_of_lanes;

	mipi_fb_cfg.width = display_wd;
	mipi_fb_cfg.height = display_ht;

	mipi_dsi_cmd_config(mipi_fb_cfg, num_of_lanes);
	mdelay(50);
	config_dsi_cmd_mode(display_wd, display_ht, image_wd, image_ht,
			dst_format, traffic_mode,
			num_of_lanes, /* num_of_lanes */
			pinfo->mipi.rgb_swap);

}
#endif

int mipi_config(struct msm_fb_panel_data *panel)
{
	int ret = NO_ERROR;
	struct msm_panel_info *pinfo;
	struct mipi_dsi_panel_config mipi_pinfo;

	if (!panel)
		return ERR_INVALID_ARGS;

	pinfo = &(panel->panel_info);
	mipi_pinfo.mode = pinfo->mipi.mode;
	mipi_pinfo.num_of_lanes = pinfo->mipi.num_of_lanes;
	mipi_pinfo.dsi_phy_config = pinfo->mipi.dsi_phy_db;
	mipi_pinfo.panel_on_cmds = pinfo->mipi.panel_on_cmds;
	mipi_pinfo.num_of_panel_on_cmds = pinfo->mipi.num_of_panel_on_cmds;
	mipi_pinfo.lane_swap = pinfo->mipi.lane_swap;
	mipi_pinfo.pack = 1;

	/* Enable MMSS_AHB_ARB_MATER_PORT_E for
	   arbiter master0 and master 1 request */
#if (!DISPLAY_MIPI_PANEL_RENESAS && !DISPLAY_TYPE_DSI6G && !DISPLAY_TYPE_8610)
	writel(0x00001800, MMSS_SFPB_GPREG);
#endif

	mipi_dsi_phy_init(&mipi_pinfo);

	ret += mipi_dsi_panel_initialize(&mipi_pinfo);

	if (pinfo->rotate && panel->rotate)
		pinfo->rotate();

#if TARGET_MSM8960_ARIES
	mipi_dsi_cmd_trigger(panel);
	mdelay(34);
#endif

	return ret;
}

int mdss_dsi_video_mode_config(uint16_t disp_width,
	uint16_t disp_height,
	uint16_t img_width,
	uint16_t img_height,
	uint16_t hsync_porch0_fp,
	uint16_t hsync_porch0_bp,
	uint16_t vsync_porch0_fp,
	uint16_t vsync_porch0_bp,
	uint16_t hsync_width,
	uint16_t vsync_width,
	uint16_t dst_format,
	uint16_t traffic_mode,
	uint8_t lane_en,
	uint16_t low_pwr_stop_mode,
	uint8_t eof_bllp_pwr,
	uint8_t interleav,
	uint32_t ctl_base)
{
	int status = 0;

#if (DISPLAY_TYPE_MDSS == 1)
	/* disable mdp first */
	mdp_disable();

	writel(0x00000000, ctl_base + CLK_CTRL);
	writel(0x00000002, ctl_base + CLK_CTRL);
	writel(0x00000006, ctl_base + CLK_CTRL);
	writel(0x0000000e, ctl_base + CLK_CTRL);
	writel(0x0000001e, ctl_base + CLK_CTRL);
	writel(0x0000023f, ctl_base + CLK_CTRL);

	writel(0, ctl_base + CTRL);

	writel(0x03f03fe0, ctl_base + ERR_INT_MASK0);

	writel(0x02020202, ctl_base + INT_CTRL);

	/* For 8916/8939, enable DSI timing double buffering */
	if (readl(ctl_base) >= DSI_HW_REV_103 &&
				mdp_get_revision() != MDP_REV_305)
		writel(0x1, ctl_base + TIMING_DB_MODE);

	writel(((disp_width + hsync_porch0_bp) << 16) | hsync_porch0_bp,
			ctl_base + VIDEO_MODE_ACTIVE_H);

	writel(((disp_height + vsync_porch0_bp) << 16) | (vsync_porch0_bp),
			ctl_base + VIDEO_MODE_ACTIVE_V);

	if (mdp_get_revision() >= MDP_REV_41 ||
				mdp_get_revision() == MDP_REV_304 ||
				mdp_get_revision() == MDP_REV_305) {
		writel(((disp_height + vsync_porch0_fp
			+ vsync_porch0_bp - 1) << 16)
			| (disp_width + hsync_porch0_fp
			+ hsync_porch0_bp - 1),
			ctl_base + VIDEO_MODE_TOTAL);
	} else {
		writel(((disp_height + vsync_porch0_fp
			+ vsync_porch0_bp) << 16)
			| (disp_width + hsync_porch0_fp
			+ hsync_porch0_bp),
			ctl_base + VIDEO_MODE_TOTAL);
	}

	writel((hsync_width << 16) | 0, ctl_base + VIDEO_MODE_HSYNC);

	writel(0 << 16 | 0, ctl_base + VIDEO_MODE_VSYNC);

	writel(vsync_width << 16 | 0, ctl_base + VIDEO_MODE_VSYNC_VPOS);

	/* For 8916/8939, flush the DSI timing registers */
	if (readl(ctl_base) >= DSI_HW_REV_103 &&
				mdp_get_revision() != MDP_REV_305)
		writel(0x1, ctl_base + TIMING_FLUSH);

	writel(0x0, ctl_base + EOT_PACKET_CTRL);

	writel(0x00000100, ctl_base + MISR_VIDEO_CTRL);

	if (mdp_get_revision() >= MDP_REV_41 || mdp_get_revision() == MDP_REV_305) {
		writel(low_pwr_stop_mode << 16 |
				eof_bllp_pwr << 12 | traffic_mode << 8
				| dst_format << 4 | 0x0, ctl_base + VIDEO_MODE_CTRL);
	} else {
		writel(1 << 28 | 1 << 24 | 1 << 20 | low_pwr_stop_mode << 16 |
				eof_bllp_pwr << 12 | traffic_mode << 8
				| dst_format << 4 | 0x0, ctl_base + VIDEO_MODE_CTRL);
	}

	writel(0x3fd08, ctl_base + HS_TIMER_CTRL);
	writel(0x00010100, ctl_base + MISR_VIDEO_CTRL);

	writel(0x00010100, ctl_base + INT_CTRL);
	writel(0x02010202, ctl_base + INT_CTRL);
	writel(0x02030303, ctl_base + INT_CTRL);

	writel(interleav << 30 | 0 << 24 | 0 << 20 | lane_en << 4
			| 0x103, ctl_base + CTRL);
#endif

	return status;
}

int mdss_dsi_config(struct msm_fb_panel_data *panel)
{
	int ret = NO_ERROR;
	struct msm_panel_info *pinfo __UNUSED;
	struct mipi_dsi_panel_config mipi_pinfo __UNUSED;

#if (DISPLAY_TYPE_MDSS == 1)
	if (!panel)
		return ERR_INVALID_ARGS;

	pinfo = &(panel->panel_info);
	mipi_pinfo.mode = pinfo->mipi.mode;
	mipi_pinfo.num_of_lanes = pinfo->mipi.num_of_lanes;
	mipi_pinfo.mdss_dsi_phy_config = pinfo->mipi.mdss_dsi_phy_db;
	mipi_pinfo.panel_on_cmds = pinfo->mipi.panel_on_cmds;
	mipi_pinfo.num_of_panel_on_cmds = pinfo->mipi.num_of_panel_on_cmds;
	mipi_pinfo.lane_swap = pinfo->mipi.lane_swap;
	mipi_pinfo.pack = 0;
	mipi_pinfo.t_clk_pre = pinfo->mipi.t_clk_pre;
	mipi_pinfo.t_clk_post = pinfo->mipi.t_clk_post;
	mipi_pinfo.signature = pinfo->mipi.signature;
	mipi_pinfo.cmds_post_tg = pinfo->mipi.cmds_post_tg;

	mdss_dsi_phy_init(&mipi_pinfo, MIPI_DSI0_BASE, DSI0_PHY_BASE);
	if (pinfo->mipi.dual_dsi)
		mdss_dsi_phy_init(&mipi_pinfo, MIPI_DSI1_BASE, DSI1_PHY_BASE);

	ret = mdss_dsi_host_init(&mipi_pinfo, pinfo->mipi.dual_dsi,
						pinfo->mipi.broadcast);
	if (ret) {
		dprintf(CRITICAL, "dsi host init error\n");
		goto error;
	}

	mdss_dsi_phy_contention_detection(&mipi_pinfo, DSI0_PHY_BASE);

	if (panel->pre_init_func) {
		ret = panel->pre_init_func();
		if (ret) {
			dprintf(CRITICAL, "pre_init_func error\n");
			goto error;
		}
	}

	if (!mipi_pinfo.cmds_post_tg) {
		ret = mdss_dsi_panel_initialize(&mipi_pinfo, pinfo->mipi.broadcast);
		if (ret) {
			dprintf(CRITICAL, "dsi panel init error\n");
			goto error;
		}
	}

	if (pinfo->rotate && panel->rotate)
		pinfo->rotate();
#endif

goto error;
error:
	return ret;
}

int mdss_dsi_post_on(struct msm_fb_panel_data *panel)
{
	int ret = 0;
	struct msm_panel_info *pinfo = &(panel->panel_info);
	struct mipi_dsi_panel_config mipi_pinfo;

	if (pinfo->mipi.cmds_post_tg) {
		mipi_pinfo.panel_on_cmds = pinfo->mipi.panel_on_cmds;
		mipi_pinfo.num_of_panel_on_cmds =
				pinfo->mipi.num_of_panel_on_cmds;
		mipi_pinfo.signature = pinfo->mipi.signature;

		ret = mdss_dsi_panel_initialize(&mipi_pinfo, pinfo->mipi.broadcast);
		if (ret) {
			dprintf(CRITICAL, "dsi panel init error\n");
		}
	}
	return ret;
}

int mdss_dsi_cmd_mode_config(uint16_t disp_width,
	uint16_t disp_height,
	uint16_t img_width,
	uint16_t img_height,
	uint16_t dst_format,
	uint8_t ystride,
	uint8_t lane_en,
	uint8_t interleav,
	uint32_t ctl_base)
{
	uint16_t dst_fmt __UNUSED = 0;

	switch (dst_format) {
	case DSI_VIDEO_DST_FORMAT_RGB565:
		dst_fmt = DSI_CMD_DST_FORMAT_RGB565;
		break;
	case DSI_VIDEO_DST_FORMAT_RGB666:
	case DSI_VIDEO_DST_FORMAT_RGB666_LOOSE:
		dst_fmt = DSI_CMD_DST_FORMAT_RGB666;
		break;
	case DSI_VIDEO_DST_FORMAT_RGB888:
		dst_fmt = DSI_CMD_DST_FORMAT_RGB888;
		break;
	default:
		dprintf(CRITICAL, "unsupported dst format\n");
		return ERR_NOT_SUPPORTED;
	}

#if (DISPLAY_TYPE_MDSS == 1)
	writel(0x00000000, ctl_base + CLK_CTRL);
	writel(0x00000000, ctl_base + CLK_CTRL);
	writel(0x00000000, ctl_base + CLK_CTRL);
	writel(0x00000000, ctl_base + CLK_CTRL);
	writel(0x00000002, ctl_base + CLK_CTRL);
	writel(0x00000006, ctl_base + CLK_CTRL);
	writel(0x0000000e, ctl_base + CLK_CTRL);
	writel(0x0000001e, ctl_base + CLK_CTRL);
	writel(0x0000023f, ctl_base + CLK_CTRL);

	writel(0, ctl_base + CTRL);

	writel(0x03f03fe0, ctl_base + ERR_INT_MASK0);

	writel(0x02020202, ctl_base + INT_CTRL);

	writel(dst_fmt, ctl_base + COMMAND_MODE_MDP_CTRL);
	writel((img_width * ystride + 1) << 16 | 0x0039,
	       ctl_base + COMMAND_MODE_MDP_STREAM0_CTRL);
	writel((img_width * ystride + 1) << 16 | 0x0039,
	       ctl_base + COMMAND_MODE_MDP_STREAM1_CTRL);
	writel(img_height << 16 | img_width,
	       ctl_base + COMMAND_MODE_MDP_STREAM0_TOTAL);
	writel(img_height << 16 | img_width,
	       ctl_base + COMMAND_MODE_MDP_STREAM1_TOTAL);
	writel(0x13c2c, ctl_base + COMMAND_MODE_MDP_DCS_CMD_CTRL);
	writel(interleav << 30 | 0 << 24 | 0 << 20 | lane_en << 4 | 0x105,
	       ctl_base + CTRL);
	writel(0x14000000, ctl_base + COMMAND_MODE_DMA_CTRL);
	writel(0x10000000, ctl_base + MISR_CMD_CTRL);
	writel(0x1, ctl_base + EOT_PACKET_CTRL);
#endif
	return 0;
}

int mipi_dsi_on()
{
	int ret = NO_ERROR;
	unsigned long ReadValue;
	unsigned long count = 0;

	ReadValue = readl(DSI_INT_CTRL) & 0x00010000;

	mdelay(10);

	while (ReadValue != 0x00010000) {
		ReadValue = readl(DSI_INT_CTRL) & 0x00010000;
		count++;
		if (count > 0xffff) {
			dprintf(CRITICAL, "Video lane test failed\n");
			return ERR_IO;
		}
	}

	dprintf(INFO, "Video lane tested successfully\n");
	return ret;
}

int mipi_dsi_off(struct msm_panel_info *pinfo)
{
	if(!target_cont_splash_screen())
	{
		mdss_dsi_panel_shutdown(pinfo);
		writel(0, DSI_CLK_CTRL);
		writel(0, DSI_CTRL);
	}

	writel(0x1115501, MIPI_DSI0_BASE + INT_CTRL);
	if (pinfo->mipi.broadcast)
		writel(0x1115501, MIPI_DSI1_BASE + INT_CTRL);

	return NO_ERROR;
}

int mipi_cmd_trigger()
{
	writel(0x1, DSI_CMD_MODE_MDP_SW_TRIGGER);

	return NO_ERROR;
}
