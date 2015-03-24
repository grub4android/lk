/* Copyright (c) 2011, 2014, The Linux Foundation. All rights reserved.
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
#ifndef _PLATFORM_MSM_SHARED_MDP_3_H_
#define _PLATFORM_MSM_SHARED_MDP_3_H_

#include <dev/fbcon.h>
#include <platform/msm_panel.h>

//TODO: Make a global PASS / FAIL define
#define PASS                        0
#define FAIL                        1

int mdp_dsi_video_config(struct msm_panel_info *pinfo, struct fbcon_config *fb);
int mdp_dsi_cmd_config(struct msm_panel_info *pinfo, struct fbcon_config *fb);
void mdp_disable(void);
int mdp_dsi_video_off(struct msm_panel_info *pinfo);
int mdp_dsi_cmd_off(void);
void mdp_set_revision(int rev);
int mdp_get_revision(void);
int mdp_dsi_video_on(struct msm_panel_info *pinfo);
int mdp_dma_on(struct msm_panel_info *pinfo);
int mdp_dma_off(void);
int mdp_edp_config(struct msm_panel_info *pinfo, struct fbcon_config *fb);
int mdp_edp_on(struct msm_panel_info *pinfo);
int mdp_edp_off(void);
int mdss_hdmi_config(struct msm_panel_info *pinfo, struct fbcon_config *fb);
int mdss_hdmi_on(struct msm_panel_info *pinfo);
int mdss_hdmi_off(void);

#endif
