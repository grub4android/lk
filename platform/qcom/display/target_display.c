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
#include <err.h>
#include <debug.h>
#include <platform.h>
#include <platform/smem.h>
#include <platform/board.h>
#include <platform/msm_panel.h>

__WEAK int mdp_lcdc_config(struct msm_panel_info *pinfo, struct fbcon_config *fb)
{
	return 0;
}
__WEAK int lvds_on(struct msm_fb_panel_data *pdata)
{
	return 0;
}
__WEAK int mdp_lcdc_on(struct msm_fb_panel_data *pdata)
{
	return 0;
}
__WEAK int mdp_lcdc_off(void)
{
	return 0;
}
__WEAK int target_display_pre_on(void)
{
	return 0;
}
__WEAK int target_display_post_on(void)
{
	return 0;
}
__WEAK int target_display_pre_off(void)
{
	return 0;
}
__WEAK int target_display_post_off(void)
{
	return 0;
}
__WEAK int target_display_get_base_offset(uint32_t base)
{
	return 0;
}
__WEAK int target_ldo_ctrl(uint8_t enable, struct msm_panel_info *pinfo)
{
    return 0;
}

__WEAK void target_edp_panel_init(struct msm_panel_info *pinfo)
{
	return;
}

__WEAK int target_edp_panel_clock(uint8_t enable, struct msm_panel_info *pinfo)
{
	return 0;
}

__WEAK int target_edp_panel_enable(void)
{
	return 0;
}

__WEAK int target_edp_panel_disable(void)
{
	return 0;
}

__WEAK int target_edp_bl_ctrl(int enable)
{
	return 0;
}

__WEAK int target_hdmi_panel_clock(uint8_t enable, struct msm_panel_info *pinfo)
{
	return 0;
}

__WEAK int target_hdmi_regulator_ctrl(bool enable)
{
	return 0;
}
__WEAK int mdss_hdmi_init(void)
{
	return 0;
}

__WEAK int target_hdmi_gpio_ctrl(bool enable)
{
	return 0;
}

static uint8_t splash_override;
/* Returns 1 if target supports continuous splash screen. */
__WEAK int target_cont_splash_screen(void)
{
	uint8_t splash_screen = 0;
	if(!splash_override) {
		switch(board_hardware_id())
		{
			case HW_PLATFORM_SURF:
			case HW_PLATFORM_MTP:
			case HW_PLATFORM_FLUID:
			case HW_PLATFORM_LIQUID:
				dprintf(SPEW, "Target_cont_splash=1\n");
				splash_screen = 1;
				break;
			default:
				dprintf(SPEW, "Target_cont_splash=0\n");
				splash_screen = 0;
		}
	}
	return splash_screen;
}

__WEAK void target_force_cont_splash_disable(uint8_t override)
{
        splash_override = override;
}

__WEAK bool target_display_panel_node(char *panel_name, char *pbuf,
	uint16_t buf_size)
{
	return false;
}

__WEAK void target_display_init(const char *panel_name)
{
}

__WEAK void target_display_shutdown(void)
{
}

__WEAK uint8_t target_panel_auto_detect_enabled(void)
{
	return 0;
}

__WEAK uint8_t target_is_edp(void)
{
	return 0;
}
