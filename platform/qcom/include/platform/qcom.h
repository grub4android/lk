/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#pragma once

#include <stdint.h>

#ifdef QCOM_ADDITIONAL_INCLUDE
#include QCOM_ADDITIONAL_INCLUDE
#endif

void platform_qcom_init_pmm(void);
uint32_t platform_detect_panel(void);
uint32_t platform_get_smem_base_addr(void);

int boot_device_mask(int val);
void shutdown_device(void);
void reboot_device(unsigned reboot_reason);

void target_usb_init(void);
void target_usb_stop(void);
void target_fastboot_init(void);
const char* target_serialno(void);
const char* target_usb_controller(void);

#ifdef QCOM_ENABLE_SDHCI
void clock_config_cdc(uint32_t interface);
uint32_t target_ddr_cfg_val(void);
#endif
