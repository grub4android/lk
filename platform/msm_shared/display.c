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
 */

#include <debug.h>
#include <err.h>
#include <string.h>
#include <dev/flash.h>
#include <platform/msm_shared/msm_panel.h>
#include <platform/msm_shared/mdp4.h>
#include <platform/msm_shared/mipi_dsi.h>
#include <platform/msm_shared/boot_stats.h>
#include <platform/msm_shared/mmc.h>
#include <platform/msm_shared/partition_parser.h>
#include <target/msm_shared.h>
#include <platform/msm_shared.h>
#include <platform/msm_shared/board.h>
#include <malloc.h>
#include "splash.h"

static struct msm_fb_panel_data *panel;

extern int lvds_on(struct msm_fb_panel_data *pdata);

static int msm_fb_alloc(struct fbcon_config *fb)
{
	if (fb == NULL)
		return ERR_INVALID_ARGS;

	if (fb->base == NULL)
		fb->base = memalign(4096, fb->width
							* fb->height
							* (fb->bpp / 8));

	if (fb->base == NULL)
		return ERR_INVALID_ARGS;

	return NO_ERROR;
}

int msm_display_config(void)
{
	int ret = NO_ERROR;
	int mdp_rev;
	struct msm_panel_info *pinfo;

	if (!panel)
		return ERR_INVALID_ARGS;

	pinfo = &(panel->panel_info);

	/* Set MDP revision */
	mdp_set_revision(panel->mdp_rev);

	switch (pinfo->type) {
	case LVDS_PANEL:
		dprintf(INFO, "Config LVDS_PANEL.\n");
		ret = mdp_lcdc_config(pinfo, &(panel->fb));
		if (ret)
			goto msm_display_config_out;
		break;
	case MIPI_VIDEO_PANEL:
		dprintf(INFO, "Config MIPI_VIDEO_PANEL.\n");

		mdp_rev = mdp_get_revision();
		if (mdp_rev == MDP_REV_50 || mdp_rev == MDP_REV_304)
			ret = mdss_dsi_config(panel);
		else
			ret = mipi_config(panel);

		if (ret)
			goto msm_display_config_out;

		if (pinfo->early_config)
			ret = pinfo->early_config((void *)pinfo);

		ret = mdp_dsi_video_config(pinfo, &(panel->fb));
		if (ret)
			goto msm_display_config_out;
		break;
	case MIPI_CMD_PANEL:
		dprintf(INFO, "Config MIPI_CMD_PANEL.\n");
		mdp_rev = mdp_get_revision();
		if (mdp_rev == MDP_REV_50 || mdp_rev == MDP_REV_304)
			ret = mdss_dsi_config(panel);
		else
			ret = mipi_config(panel);
		if (ret)
			goto msm_display_config_out;

		ret = mdp_dsi_cmd_config(pinfo, &(panel->fb));
		if (ret)
			goto msm_display_config_out;
		break;
	case LCDC_PANEL:
		dprintf(INFO, "Config LCDC PANEL.\n");
		ret = mdp_lcdc_config(pinfo, &(panel->fb));
		if (ret)
			goto msm_display_config_out;
		break;
	case HDMI_PANEL:
		dprintf(INFO, "Config HDMI PANEL.\n");
		ret = mdss_hdmi_config(pinfo, &(panel->fb));
		if (ret)
			goto msm_display_config_out;
		break;
	case EDP_PANEL:
		dprintf(INFO, "Config EDP PANEL.\n");
		ret = mdp_edp_config(pinfo, &(panel->fb));
		if (ret)
			goto msm_display_config_out;
		break;
	default:
		return ERR_INVALID_ARGS;
	};

	if (pinfo->config)
		ret = pinfo->config((void *)pinfo);

msm_display_config_out:
	return ret;
}

int msm_display_on(void)
{
	int ret = NO_ERROR;
	int mdp_rev;
	struct msm_panel_info *pinfo;

	if (!panel)
		return ERR_INVALID_ARGS;

	bs_set_timestamp(BS_SPLASH_SCREEN_DISPLAY);

	pinfo = &(panel->panel_info);

	if (pinfo->pre_on) {
		ret = pinfo->pre_on();
		if (ret)
			goto msm_display_on_out;
	}

	switch (pinfo->type) {
	case LVDS_PANEL:
		dprintf(INFO, "Turn on LVDS PANEL.\n");
		ret = mdp_lcdc_on();
		if (ret)
			goto msm_display_on_out;
		ret = lvds_on(panel);
		if (ret)
			goto msm_display_on_out;
		break;
	case MIPI_VIDEO_PANEL:
		dprintf(INFO, "Turn on MIPI_VIDEO_PANEL.\n");
		ret = mdp_dsi_video_on(pinfo);
		if (ret)
			goto msm_display_on_out;
		ret = mipi_dsi_on();
		if (ret)
			goto msm_display_on_out;
		break;
	case MIPI_CMD_PANEL:
		dprintf(INFO, "Turn on MIPI_CMD_PANEL.\n");
		ret = mdp_dma_on(pinfo);
		if (ret)
			goto msm_display_on_out;
		mdp_rev = mdp_get_revision();
		if (mdp_rev != MDP_REV_50 && mdp_rev != MDP_REV_304) {
			ret = mipi_cmd_trigger();
			if (ret)
				goto msm_display_on_out;
		}
		break;
	case LCDC_PANEL:
		dprintf(INFO, "Turn on LCDC PANEL.\n");
		ret = mdp_lcdc_on();
		if (ret)
			goto msm_display_on_out;
		break;
#if DISPLAY_TYPE_MDSS
	case HDMI_PANEL:
		dprintf(INFO, "Turn on HDMI PANEL.\n");
		ret = mdss_hdmi_init();
		if (ret)
			goto msm_display_on_out;

		ret = mdss_hdmi_on();
		if (ret)
			goto msm_display_on_out;
		break;
#endif
	case EDP_PANEL:
		dprintf(INFO, "Turn on EDP PANEL.\n");
		ret = mdp_edp_on(pinfo);
		if (ret)
			goto msm_display_on_out;
		break;
	default:
		return ERR_INVALID_ARGS;
	};

	if (pinfo->on)
		ret = pinfo->on();

msm_display_on_out:
	return ret;
}

int msm_display_init(struct msm_fb_panel_data *pdata)
{
	int ret = NO_ERROR;

	panel = pdata;
	if (!panel) {
		ret = ERR_INVALID_ARGS;
		goto msm_display_init_out;
	}

	/* Turn on panel */
	if (pdata->power_func)
		ret = pdata->power_func(1, &(panel->panel_info));

	if (ret)
		goto msm_display_init_out;

	/* Enable clock */
	if (pdata->clk_func)
		ret = pdata->clk_func(1);

	/* Only enabled for auto PLL calculation */
	if (pdata->pll_clk_func)
		ret = pdata->pll_clk_func(1, &(panel->panel_info));

	if (ret)
		goto msm_display_init_out;

	/* pinfo prepare  */
	if (pdata->panel_info.prepare) {
		/* this is for edp which pinfo derived from edid */
		ret = pdata->panel_info.prepare();
		panel->fb.width =  panel->panel_info.xres;
		panel->fb.height =  panel->panel_info.yres;
		panel->fb.stride =  panel->panel_info.xres;
		panel->fb.bpp =  panel->panel_info.bpp;
	}

	if (ret)
		goto msm_display_init_out;

	ret = msm_fb_alloc(&(panel->fb));
	if (ret)
		goto msm_display_init_out;

	fbcon_setup(&(panel->fb));
	display_image_on_screen();
	ret = msm_display_config();
	if (ret)
		goto msm_display_init_out;

	ret = msm_display_on();
	if (ret)
		goto msm_display_init_out;

	if (pdata->post_power_func)
		ret = pdata->post_power_func(1);
	if (ret)
		goto msm_display_init_out;

	/* Turn on backlight */
	if (pdata->bl_func)
		ret = pdata->bl_func(1);

	if (ret)
		goto msm_display_init_out;

msm_display_init_out:
	return ret;
}

int msm_display_off()
{
	int ret = NO_ERROR;
	struct msm_panel_info *pinfo;

	if (!panel)
		return ERR_INVALID_ARGS;

	pinfo = &(panel->panel_info);

	if (pinfo->pre_off) {
		ret = pinfo->pre_off();
		if (ret)
			goto msm_display_off_out;
	}

	switch (pinfo->type) {
	case LVDS_PANEL:
		dprintf(INFO, "Turn off LVDS PANEL.\n");
		mdp_lcdc_off();
		break;
	case MIPI_VIDEO_PANEL:
		dprintf(INFO, "Turn off MIPI_VIDEO_PANEL.\n");
		ret = mdp_dsi_video_off();
		if (ret)
			goto msm_display_off_out;
		ret = mipi_dsi_off(pinfo);
		if (ret)
			goto msm_display_off_out;
		break;
	case MIPI_CMD_PANEL:
		dprintf(INFO, "Turn off MIPI_CMD_PANEL.\n");
		ret = mdp_dsi_cmd_off();
		if (ret)
			goto msm_display_off_out;
		ret = mipi_dsi_off(pinfo);
		if (ret)
			goto msm_display_off_out;
		break;
	case LCDC_PANEL:
		dprintf(INFO, "Turn off LCDC PANEL.\n");
		mdp_lcdc_off();
		break;
	case EDP_PANEL:
		dprintf(INFO, "Turn off EDP PANEL.\n");
		ret = mdp_edp_off();
		if (ret)
			goto msm_display_off_out;
		break;
	default:
		return ERR_INVALID_ARGS;
	};

	if (target_cont_splash_screen()) {
		dprintf(INFO, "Continuous splash enabled, keeping panel alive.\n");
		return NO_ERROR;
	}

	if (panel->post_power_func)
		ret = panel->post_power_func(0);
	if (ret)
		goto msm_display_off_out;

	/* Turn off backlight */
	if (panel->bl_func)
		ret = panel->bl_func(0);

	if (pinfo->off)
		ret = pinfo->off();

	/* Disable clock */
	if (panel->clk_func)
		ret = panel->clk_func(0);

	/* Only for AUTO PLL calculation */
	if (panel->pll_clk_func)
		ret = panel->pll_clk_func(0, pinfo);

	if (ret)
		goto msm_display_off_out;

	/* Disable panel */
	if (panel->power_func)
		ret = panel->power_func(0, pinfo);

msm_display_off_out:
	return ret;
}

static struct fbimage logo_header = {{{0},0,0,0,{0}}, NULL};
struct fbimage* splash_screen_flash(void);

int splash_screen_check_header(struct fbimage *logo)
{
	if (memcmp(logo->header.magic, LOGO_IMG_MAGIC, 8))
		return -1;
	if (logo->header.width == 0 || logo->header.height == 0)
		return -1;
	return 0;
}

struct fbimage* splash_screen_flash(void)
{
	struct ptentry_msm *ptn;
	struct ptable_msm *ptable;
	struct fbcon_config *fb_display = NULL;
	struct fbimage *logo = &logo_header;


	ptable = flash_get_ptable();
	if (ptable == NULL) {
	dprintf(CRITICAL, "ERROR: Partition table not found\n");
	return NULL;
	}
	ptn = ptable_msm_find(ptable, "splash");
	if (ptn == NULL) {
		dprintf(CRITICAL, "ERROR: splash Partition not found\n");
		return NULL;
	}

	if (flash_read(ptn, 0,(unsigned int *) logo, sizeof(logo->header))) {
		dprintf(CRITICAL, "ERROR: Cannot read boot image header\n");
		return NULL;
	}

	if (splash_screen_check_header(logo)) {
		dprintf(CRITICAL, "ERROR: Boot image header invalid\n");
		return NULL;
	}

	fb_display = fbcon_display();
	if (fb_display) {
		uint8_t *base = (uint8_t *) fb_display->base;
		if (logo->header.width != fb_display->width || logo->header.height != fb_display->height) {
				base += LOGO_IMG_OFFSET;
		}

		if (flash_read(ptn + sizeof(logo->header), 0,
			base,
			((((logo->header.width * logo->header.height * fb_display->bpp/8) + 511) >> 9) << 9))) {
			fbcon_clear();
			dprintf(CRITICAL, "ERROR: Cannot read splash image\n");
			return NULL;
		}
		logo->image = base;
	}

	return logo;
}

struct fbimage* splash_screen_mmc(void)
{
	int index = INVALID_PTN;
	unsigned long long ptn = 0;
	struct fbcon_config *fb_display = NULL;
	struct fbimage *logo = &logo_header;

	index = partition_get_index("splash");
	if (index == 0) {
		dprintf(CRITICAL, "ERROR: splash Partition table not found\n");
		return NULL;
	}

	ptn = partition_get_offset(index);
	if (ptn == 0) {
		dprintf(CRITICAL, "ERROR: splash Partition invalid\n");
		return NULL;
	}

	if (mmc_read(ptn, (unsigned int *) logo, sizeof(logo->header))) {
		dprintf(CRITICAL, "ERROR: Cannot read splash image header\n");
		return NULL;
	}

	if (splash_screen_check_header(logo)) {
		dprintf(CRITICAL, "ERROR: Splash image header invalid\n");
		return NULL;
	}

	fb_display = fbcon_display();
	if (fb_display) {
		uint8_t *base = (uint8_t *) fb_display->base;
		if (logo->header.width != fb_display->width || logo->header.height != fb_display->height)
				base += LOGO_IMG_OFFSET;

		if (mmc_read(ptn + sizeof(logo->header),
			(unsigned int *)base,
			((((logo->header.width * logo->header.height * fb_display->bpp/8) + 511) >> 9) << 9))) {
			fbcon_clear();
			dprintf(CRITICAL, "ERROR: Cannot read splash image\n");
			return NULL;
		}

		logo->image = base;
	}

	return logo;
}


struct fbimage* fetch_image_from_partition(void)
{
	if (target_is_emmc_boot()) {
		return splash_screen_mmc();
	} else {
		return splash_screen_flash();
	}
}

int platform_get_splash_image(struct fbimage* fbimg, bool* flag) {
	struct fbimage *partimg;

	partimg = fetch_image_from_partition();
	if(!partimg) {
		*flag = false;
		fbimg->header.width = SPLASH_IMAGE_HEIGHT;
		fbimg->header.height = SPLASH_IMAGE_WIDTH;
#if DISPLAY_TYPE_MIPI
		fbimg->image = (unsigned char *)imageBuffer_rgb888;
#else
		fbimg->image = (unsigned char *)imageBuffer;
#endif
	}

	else {
		*flag = true;
		fbimg->header.width = partimg->header.width;
		fbimg->header.height = partimg->header.height;
		fbimg->image = partimg->image;
	}

	return NO_ERROR;
}
