#ifndef __APP_ABOOT_H
#define __APP_ABOOT_H

#include <platform.h>

typedef enum {
	BOOTMODE_AUTO,
	BOOTMODE_ANDROID,
	BOOTMODE_RECOVERY,
	BOOTMODE_FASTBOOT,
	BOOTMODE_GRUB,
	BOOTMODE_DOWNLOAD,
	BOOTMODE_PANIC,

	BOOTMODE_UNKNOWN,
	BOOTMODE_MAX
} bootmode_t;

void aboot_boot(bootmode_t reason);

#endif
