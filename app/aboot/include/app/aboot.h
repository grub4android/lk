/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
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

#ifndef _APP_ABOOT_H_
#define _APP_ABOOT_H_

#include "../../devinfo.h"

#define ABOOT_VERSION "0.5"

enum bootmode {
	BOOTMODE_AUTO = 0,
	BOOTMODE_NORMAL,
#if WITH_XIAOMI_DUALBOOT
	BOOTMODE_SECOND,
#endif
	BOOTMODE_RECOVERY,
	BOOTMODE_FASTBOOT,
	BOOTMODE_DLOAD,
	BOOTMODE_GRUB,

	BOOTMODE_MAX,
};

extern device_info device;
extern enum bootmode bootmode, bootmode_normal;
const char* strbootmode(enum bootmode bm);
void aboot_continue_boot(void);
#if WITH_XIAOMI_DUALBOOT
bool is_dualboot_supported(void);
enum bootmode get_dualboot_mode(void);
void set_dualboot_mode(enum bootmode mode);
#endif

void write_device_info(device_info *dev);

#endif
