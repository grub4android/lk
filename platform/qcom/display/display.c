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

#include <err.h>
#include <pow2.h>
#include <debug.h>
#include <target.h>
#include <malloc.h>
#include <string.h>
#include <kernel/vm.h>
#include <platform.h>
#include <platform/mipi_dsi.h>
#include <platform/msm_panel.h>
#include <target/display.h>

#ifdef QCOM_DISPLAY_TYPE_MDP3
#include <platform/mdp3.h>
#endif

#ifdef QCOM_DISPLAY_TYPE_MDP4
#include <platform/mdp4.h>
#endif

#ifdef QCOM_DISPLAY_TYPE_MDP5
#include <platform/mdp5.h>
#endif

#ifdef QCOM_DISPLAY_TYPE_QPIC
#include <platform/qpic.h>
#endif

static struct msm_fb_panel_data *panel;

static int msm_fb_alloc(struct fbcon_config *fb)
{
	if (fb == NULL)
		return ERR_INVALID_ARGS;

	if (fb->base == NULL) {
		uint32_t size = fb->width
							* fb->height
							* (fb->bpp / 8);
		fb->base = pmm_alloc_map(size, ARCH_MMU_FLAG_UNCACHED);
	}

	if (fb->base == NULL)
		return ERR_INVALID_ARGS;

	return NO_ERROR;
}

int msm_display_config(void)
{
	int ret = NO_ERROR;
#if defined(QCOM_DISPLAY_TYPE_MDSS) || defined(QCOM_DISPLAY_TYPE_MIPI)
	int mdp_rev;
#endif
	struct msm_panel_info *pinfo;

	if (!panel)
		return ERR_INVALID_ARGS;

	pinfo = &(panel->panel_info);

	/* Set MDP revision */
	mdp_set_revision(panel->mdp_rev);

	switch (pinfo->type) {
#if defined(QCOM_DISPLAY_TYPE_MDSS) || defined(QCOM_DISPLAY_TYPE_MIPI)
	case LVDS_PANEL:
		dprintf(INFO, "Config LVDS_PANEL.\n");
		ret = mdp_lcdc_config(pinfo, &(panel->fb));
		if (ret)
			goto msm_display_config_out;
		break;
	case MIPI_VIDEO_PANEL:
		dprintf(INFO, "Config MIPI_VIDEO_PANEL.\n");

		mdp_rev = mdp_get_revision();
		if (mdp_rev == MDP_REV_50 || mdp_rev == MDP_REV_304 ||
						mdp_rev == MDP_REV_305)
			ret = mdss_dsi_config(panel);
		else
			ret = mipi_config(panel);

		if (ret)
			goto msm_display_config_out;

		if (pinfo->early_config)
			ret = pinfo->early_config(pinfo, &(panel->fb));

		ret = mdp_dsi_video_config(pinfo, &(panel->fb));
		if (ret)
			goto msm_display_config_out;
		break;
	case MIPI_CMD_PANEL:
		dprintf(INFO, "Config MIPI_CMD_PANEL.\n");
		mdp_rev = mdp_get_revision();
		if (mdp_rev == MDP_REV_50 || mdp_rev == MDP_REV_304 ||
						mdp_rev == MDP_REV_305)
			ret = mdss_dsi_config(panel);
		else
			ret = mipi_config(panel);
		if (ret)
			goto msm_display_config_out;

		if (pinfo->early_config)
			ret = pinfo->early_config(pinfo, &(panel->fb));

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
#endif
#if QCOM_DISPLAY_TYPE_QPIC
	case QPIC_PANEL:
		dprintf(INFO, "Config QPIC_PANEL.\n");
		qpic_init(pinfo, (int) panel->fb.base);
		break;
#endif
	default:
		return ERR_INVALID_ARGS;
	};

	if (pinfo->config)
		ret = pinfo->config((void *)pinfo);

#if defined(QCOM_DISPLAY_TYPE_MDSS) || defined(QCOM_DISPLAY_TYPE_MIPI)
msm_display_config_out:
#endif
	return ret;
}

int msm_display_on(void)
{
	int ret = NO_ERROR;
#if defined(QCOM_DISPLAY_TYPE_MDSS) || defined(QCOM_DISPLAY_TYPE_MIPI)
	int mdp_rev;
#endif
	struct msm_panel_info *pinfo;

	if (!panel)
		return ERR_INVALID_ARGS;

	//bs_set_timestamp(BS_SPLASH_SCREEN_DISPLAY);

	pinfo = &(panel->panel_info);

	if (pinfo->pre_on) {
		ret = pinfo->pre_on();
		if (ret)
			goto msm_display_on_out;
	}

	switch (pinfo->type) {
#if defined(QCOM_DISPLAY_TYPE_MDSS) || defined(QCOM_DISPLAY_TYPE_MIPI)
	case LVDS_PANEL:
		dprintf(INFO, "Turn on LVDS PANEL.\n");
		ret = mdp_lcdc_on(panel);
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

		ret = mdss_dsi_post_on(panel);
		if (ret)
			goto msm_display_on_out;

		ret = mipi_dsi_on(pinfo);
		if (ret)
			goto msm_display_on_out;
		break;
	case MIPI_CMD_PANEL:
		dprintf(INFO, "Turn on MIPI_CMD_PANEL.\n");
		ret = mdp_dma_on(pinfo);
		if (ret)
			goto msm_display_on_out;
		mdp_rev = mdp_get_revision();
		if (mdp_rev != MDP_REV_50 && mdp_rev != MDP_REV_304 &&
						mdp_rev != MDP_REV_305) {
			ret = mipi_cmd_trigger();
			if (ret)
				goto msm_display_on_out;
		}

		ret = mdss_dsi_post_on(panel);
		if (ret)
			goto msm_display_on_out;

		break;
	case LCDC_PANEL:
		dprintf(INFO, "Turn on LCDC PANEL.\n");
		ret = mdp_lcdc_on(panel);
		if (ret)
			goto msm_display_on_out;
		break;
	case HDMI_PANEL:
		dprintf(INFO, "Turn on HDMI PANEL.\n");
		ret = mdss_hdmi_init();
		if (ret)
			goto msm_display_on_out;

		ret = mdss_hdmi_on(pinfo);
		if (ret)
			goto msm_display_on_out;
		break;
	case EDP_PANEL:
		dprintf(INFO, "Turn on EDP PANEL.\n");
		ret = mdp_edp_on(pinfo);
		if (ret)
			goto msm_display_on_out;
		break;
#endif
#ifdef QCOM_DISPLAY_TYPE_QPIC
	case QPIC_PANEL:
		dprintf(INFO, "Turn on QPIC_PANEL.\n");
		ret = qpic_on();
		if (ret) {
			dprintf(CRITICAL, "QPIC panel on failed\n");
			goto msm_display_on_out;
		}
		qpic_update();
		break;
#endif
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
		ret = pdata->clk_func(1, &(panel->panel_info));

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

int msm_display_off(void)
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
#if defined(QCOM_DISPLAY_TYPE_MDSS) || defined(QCOM_DISPLAY_TYPE_MIPI)
	case LVDS_PANEL:
		dprintf(INFO, "Turn off LVDS PANEL.\n");
		mdp_lcdc_off();
		break;
	case MIPI_VIDEO_PANEL:
		dprintf(INFO, "Turn off MIPI_VIDEO_PANEL.\n");
		ret = mdp_dsi_video_off(pinfo);
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
#endif
#ifdef QCOM_DISPLAY_TYPE_QPIC
	case QPIC_PANEL:
		dprintf(INFO, "Turn off QPIC_PANEL.\n");
		qpic_off();
		break;
#endif
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
		ret = panel->clk_func(0, pinfo);

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

#ifdef ENABLE_2NDSTAGE_BOOT
int msm_display_2ndstagefb_reserve(void) {
	struct fbcon_config config;
	mdp_dump_config(&config);

	// reserve framebuffer memory
	struct list_node list;
	list_initialize(&list);
	size_t fbsize = config.width*config.height*(config.bpp/3);
	pmm_alloc_range((paddr_t)config.base, fbsize/PAGE_SIZE, &list);

	return NO_ERROR;
}

int msm_display_2ndstagefb_setup(void) {
	// get memory
	struct fbcon_config* config = calloc(sizeof(struct fbcon_config), 1);
	mdp_dump_config(config);

	// map physical framebuffer
	size_t fbsize = config->width*config->height*(config->bpp/3);
	if(!arch_mmu_map((vaddr_t)config->base, (paddr_t)config->base, fbsize/PAGE_SIZE, ARCH_MMU_FLAG_UNCACHED))
		return ERR_NO_MEMORY;

	fbcon_setup(config);
	return NO_ERROR;
}
#endif
