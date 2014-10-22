#include <app.h>
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <arch/ops.h>
#include <dev/keys.h>
#include <app/aboot.h>
#include <platform.h>

#include "menu_private.h"

static void menu_exec_normal(void) {
	boot_linux_from_storage(BOOTMODE_NORMAL);
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
	{"    Normal Powerup", &menu_exec_normal, NULL},
	{"    Recovery", &menu_exec_recovery, NULL},
	{"    Download Mode", &menu_dload_mode, NULL},
	{"    Settings", &menu_settings, NULL},
	{"    Reboot", &menu_reboot, NULL},
	{NULL,NULL,NULL},
};
