/* Copyright (c) 2010-2011, The Linux Foundation. All rights reserved.
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
#ifndef _TARGET_MSM8960_ARIES_DISPLAY_H
#define _TARGET_MSM8960_ARIES_DISPLAY_H

/*---------------------------------------------------------------------------*/
/* HEADER files                                                              */
/*---------------------------------------------------------------------------*/
#if defined(QCOM_DISPLAY_TYPE_MDSS) || defined(QCOM_DISPLAY_TYPE_MIPI)
#include <dev/gcdb/display_resource.h>
#endif

/*---------------------------------------------------------------------------*/
/* GPIO configuration                                                        */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* LDO configuration                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Target Physical configuration                                             */
/*---------------------------------------------------------------------------*/

static const uint32_t panel_strength_ctrl[] = {
};

static const char panel_bist_ctrl[] = {
};

static const uint32_t panel_regulator_settings[] = {
};

static const char panel_lane_config[] = {
};

static const uint32_t panel_physical_ctrl[] = {
};

/*---------------------------------------------------------------------------*/
/* Other Configuration                                                       */
/*---------------------------------------------------------------------------*/
#define DISPLAY_CMDLINE_PREFIX "mdp.panel="

#define MIPI_HSYNC_PULSE_WIDTH       8
#define MIPI_HSYNC_BACK_PORCH_DCLK   16
#define MIPI_HSYNC_FRONT_PORCH_DCLK  23

#define MIPI_VSYNC_PULSE_WIDTH       2
#define MIPI_VSYNC_BACK_PORCH_LINES  2
#define MIPI_VSYNC_FRONT_PORCH_LINES 7

#define DSI_CAL_CTRL                          REG_DSI(0x0F4)
#define DSI_COMMAND_MODE_MDP_CTRL             REG_DSI(0x03C)
#define DSI_COMMAND_MODE_MDP_DCS_CMD_CTRL     REG_DSI(0x040)
#define DSI_COMMAND_MODE_MDP_STREAM0_CTRL     REG_DSI(0x054)
#define DSI_COMMAND_MODE_MDP_STREAM0_TOTAL    REG_DSI(0x058)
#define DSI_COMMAND_MODE_MDP_STREAM1_CTRL     REG_DSI(0x05C)
#define DSI_COMMAND_MODE_MDP_STREAM1_TOTAL    REG_DSI(0x060)
#define DSI_MISR_CMD_CTRL                     REG_DSI(0x09C)
#define DSI_EOT_PACKET_CTRL                   REG_DSI(0x0C8)
#define DSI_CAL_STRENGTH_CTRL                 REG_DSI(0x100)

#define PIXEL_NS_VAL          0x00002003
#define PIXEL_CC_VAL          0x00000005
#define DSI_NS_VAL            0x00000003
#define DSI_CC_VAL            0x00000005

uint32_t oem_panel_max_auto_detect_panels(void);

#endif
