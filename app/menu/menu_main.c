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
	bootmode=BOOTMODE_AUTO;
	aboot_continue_boot();
}
static void menu_format_normal(char** buf) {
	*buf = calloc(100, 1);

#if WITH_XIAOMI_DUALBOOT
	if(device.bootmode==BOOTMODE_AUTO) {
		snprintf(*buf, 100, "    Normal Powerup [%s=%s]", strbootmode(device.bootmode), get_dualboot_mode()==BOOTMODE_SECOND?"System2":"System1");
		return;
	}
#endif

	snprintf(*buf, 100, "    Normal Powerup [%s]", strbootmode(device.bootmode));
}

static void menu_exec_grub(void) {
	bootmode=BOOTMODE_GRUB;
	aboot_continue_boot();
}

static void menu_exec_recovery(void) {
	bootmode=BOOTMODE_RECOVERY;
	aboot_continue_boot();
}

static void menu_dload_mode(void) {
	bootmode=BOOTMODE_DLOAD;
	aboot_continue_boot();
}

static void menu_settings(void) {
	menu_enter(entries_settings);
}

static void menu_reboot(void) {
	reboot_device(DLOAD);
	dprintf(CRITICAL,"Failed to reboot\n");
}

static void menu_shutdown(void) {
	shutdown_device();
	dprintf(CRITICAL,"Failed to shutdown\n");
}

struct menu_entry entries_main[] = {
	{"    Normal Powerup", &menu_exec_normal, &menu_format_normal},
	{"    Recovery", &menu_exec_recovery, NULL},
	{"    GRUB", &menu_exec_grub, NULL},
	{"    Download Mode", &menu_dload_mode, NULL},
	{"    Settings...", &menu_settings, NULL},
	{"    Reboot", &menu_reboot, NULL},
	{"    Shutdown", &menu_shutdown, NULL},
	{NULL,NULL,NULL},
};
