/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __PLATFORM_MSM_SHARED_H
#define __PLATFORM_MSM_SHARED_H

#include <platform/msm_shared/dload_util.h>

#define REBOOT_MODE_NORMAL     0x77665501
#define REBOOT_MODE_RECOVERY   0x77665502
#define REBOOT_MODE_FASTBOOT   0x77665500

int platform_use_identity_mmu_mappings(void);

void display_init(void);
void display_shutdown(void);
void display_image_on_screen(void);

addr_t get_bs_info_addr(void);
uint32_t platform_get_sclk_count(void);

unsigned board_machtype(void);
unsigned board_platform_id(void);
unsigned check_reboot_mode(void);
void platform_uninit_timer(void);
void shutdown_device(void);
void reboot_device(unsigned);
int set_download_mode(enum dload_mode mode);
uint32_t platform_get_smem_base_addr(void);
void clock_config_cdc(uint8_t slot);
int get_target_boot_params(const char *cmdline, const char *part,
				  char *buf, int buflen);
#endif

