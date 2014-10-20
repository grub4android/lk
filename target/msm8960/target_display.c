/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
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
#include <debug.h>
#include <err.h>
#include <msm_panel.h>
#include <mipi_dsi.h>
#include <dev/pm8921.h>
#include <board.h>
#include <mdp4.h>
#include <target/display.h>
#include <target/board.h>

#include "include/panel.h"
#include "include/display_resource.h"

int target_backlight_ctrl(struct backlight *bl, uint8_t enable)
{
	return 0;
}

int target_panel_clock(uint8_t enable, struct msm_panel_info *pinfo)
{
	if (enable) {
		mdp_clock_init();
		mmss_clock_init();
	} else if(!target_cont_splash_screen()) {
		mmss_clock_disable();
	}

	return 0;
}

int target_panel_reset(uint8_t enable, struct panel_reset_sequence *resetseq,
						struct msm_panel_info *pinfo)
{
	int ret = NO_ERROR;
	if (enable) {

	} else if(!target_cont_splash_screen()) {
	}

	return ret;
}

int target_ldo_ctrl(uint8_t enable, struct msm_panel_info *pinfo)
{
	if (enable) {
	} else {
		if (!target_cont_splash_screen()) {
		}
	}

	return 0;
}

bool target_display_panel_node(char *panel_name, char *pbuf, uint16_t buf_size)
{
	return gcdb_display_cmdline_arg(panel_name, pbuf, buf_size);
}

void target_display_init(const char *panel_name)
{
	uint32_t panel_loop = 0;
	uint32_t ret = 0;

	if (!strcmp(panel_name, NO_PANEL_CONFIG)) {
		dprintf(INFO, "Skip panel configuration\n");
		return;
	}

	do {
		ret = gcdb_display_init(panel_name, MDP_REV_44, MIPI_FB_ADDR);
		if (ret) {
			/*Panel signature did not match, turn off the display*/
			target_force_cont_splash_disable(true);
			msm_display_off();
			target_force_cont_splash_disable(false);
		} else {
			break;
		}
	} while (++panel_loop <= oem_panel_max_auto_detect_panels());
}

void target_display_shutdown(void)
{
	gcdb_display_shutdown();
}

