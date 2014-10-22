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
#include <malloc.h>
#include <dev/fbcon.h>
#include <mipi_dsi.h>
#include <target/display.h>
#include <string.h>

static void* real_fb = NULL;
void sync_sw_buffer(void) {
	struct fbcon_config *config = fbcon_display();
	memcpy(real_fb, config->base, (config->width*config->height*config->bpp/8));
}

void target_display_init(const char *panel_name)
{
#ifdef DISPLAY_2NDSTAGE_FBADDR
	uint32_t fb_addr = DISPLAY_2NDSTAGE_FBADDR;
#else
	uint32_t fb_addr = MIPI_FB_ADDR;
#endif

	struct fbcon_config *config = NULL;
	config = (struct fbcon_config*)malloc(sizeof(struct fbcon_config));

	real_fb = (void*)fb_addr;
	config->width = DISPLAY_2NDSTAGE_WIDTH;
	config->height = DISPLAY_2NDSTAGE_HEIGHT;
	config->stride = config->width;
	config->bpp = DISPLAY_2NDSTAGE_BPP;
	config->format = DSI_VIDEO_DST_FORMAT_RGB888;
	config->update_start = NULL;
	config->update_done = NULL;

#if TARGET_MSM8960_ARIES
	config->base = real_fb;
	config->update_start = trigger_mdp_dsi;
#else
	uint8_t* fb = real_fb;
	int fb_size = (config->width*config->height*config->bpp/8);
	config->base = fb + fb_size;
	memset(config->base, 0, fb_size);
	config->update_start = sync_sw_buffer;
#endif

	fbcon_setup(config);
	display_image_on_screen();
}
