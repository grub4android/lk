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

#define ABOOT_VERSION "0.5"

#if BOOT_2NDSTAGE
	#define ABOOT_PARTITION "boot"
#else
	#define ABOOT_PARTITION "aboot"
#endif

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

extern enum bootmode bootmode, bootmode_normal;
const char* strbootmode(enum bootmode bm);
void aboot_continue_boot(void);
#if WITH_XIAOMI_DUALBOOT
bool is_dualboot_supported(void);
enum bootmode get_dualboot_mode(void);
void set_dualboot_mode(enum bootmode mode);
#endif

#include <err.h>
#include <lib/sysparam.h>
#include <printf.h>
#define sysparam_define_type(fn, ret, internal) \
	static inline ret sysparam_read_##fn(const char* name) { \
		internal val = 0; \
		if(sysparam_read(name, &val, sizeof(val))!=sizeof(val)) \
			return 0; \
\
		return val; \
	} \
\
	static inline status_t sysparam_write_##fn(const char* name, ret val) { \
		internal newval = val; \
\
		if(!sysparam_get_ptr(name, NULL, NULL)) { \
			sysparam_remove(name); \
		} \
\
		return sysparam_add(name, &newval, sizeof(newval)); \
	}

sysparam_define_type(bool, bool, uint8_t);
sysparam_define_type(bootmode, enum bootmode, enum bootmode);

static const char* sysparm_read_str(const char* name) {
	const char* ptr = NULL;
	sysparam_get_ptr(name, (const void**)&ptr, NULL);
	return ptr;
}

static status_t sysparm_write_str(const char* name, const char* val) {
	if(!sysparam_get_ptr(name, NULL, NULL)) {
		sysparam_remove(name);
	}

	return sysparam_add(name, val, strlen(val));
}

#endif
