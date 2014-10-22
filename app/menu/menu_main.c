#include <app.h>
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <arch/ops.h>
#include <dev/keys.h>
#include <app/aboot.h>
#include <platform.h>
#include <printf.h>

#include "menu_private.h"
#include "../aboot/grub.h"

static void menu_exec_normal(void) {
	boot_linux_from_storage(BOOTMODE_NORMAL);
}
#if WITH_XIAOMI_DUALBOOT
static void menu_format_normal(char** buf) {
	*buf = calloc(100, 1);
	snprintf(*buf, 100, "    Normal Powerup [%s]", dual_boot_sign==DUALBOOT_BOOT_SECOND?"System2":"System1");
}

static void menu_exec_other(void) {
	boot_linux_from_storage(dual_boot_sign==DUALBOOT_BOOT_SECOND?DUALBOOT_BOOT_FIRST:DUALBOOT_BOOT_SECOND);
}
static void menu_format_other(char** buf) {
	*buf = calloc(100, 1);
	snprintf(*buf, 100, "    Alternative [%s]", dual_boot_sign!=DUALBOOT_BOOT_SECOND?"System2":"System1");
}
#endif

static void menu_exec_grub(void) {
	grub_boot();
}

static void menu_exec_recovery(void) {
	boot_linux_from_storage(BOOTMODE_RECOVERY);
}

static void menu_dload_mode(void) {
	if (set_download_mode(EMERGENCY_DLOAD))
	{
		dprintf(CRITICAL,"dload mode not supported by target\n");
	}
	else
	{
		reboot_device(DLOAD);
		dprintf(CRITICAL,"Failed to reboot into dload mode\n");
	}
}

static void menu_settings(void) {
	menu_enter(entries_settings);
}

static void menu_reboot(void) {
	reboot_device(DLOAD);
	dprintf(CRITICAL,"Failed to reboot\n");
}

struct menu_entry entries_main[] = {
#if WITH_XIAOMI_DUALBOOT
	{"", &menu_exec_normal, &menu_format_normal},
	{"", &menu_exec_other, &menu_format_other},
#else
	{"    Normal Powerup", &menu_exec_normal, NULL},
#endif
	{"    Recovery", &menu_exec_recovery, NULL},
	{"    GRUB", &menu_exec_grub, NULL},
	{"    Download Mode", &menu_dload_mode, NULL},
	{"    Settings", &menu_settings, NULL},
	{"    Reboot", &menu_reboot, NULL},
	{NULL,NULL,NULL},
};
