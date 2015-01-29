#include <app.h>
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <arch/ops.h>
#include <dev/keys.h>
#include <app/aboot.h>
#include <platform.h>
#include <target.h>
#include <printf.h>

#include "menu_private.h"
#include "../aboot/grub.h"

static void menu_exec_normal(void) {
	bootmode=BOOTMODE_AUTO;
	aboot_continue_boot();
}
static void menu_format_normal(char** buf) {
	*buf = calloc(100, 1);
	enum bootmode bootmode = sysparam_read_bootmode("bootmode");

#if WITH_XIAOMI_DUALBOOT
	if(is_dualboot_supported() && bootmode==BOOTMODE_AUTO) {
		snprintf(*buf, 100, "    Normal Powerup [%s=%s]", strbootmode(bootmode), get_dualboot_mode()==BOOTMODE_SECOND?"System2":"System1");
		return;
	}
#endif

	snprintf(*buf, 100, "    Normal Powerup [%s]", strbootmode(bootmode));
}

static void menu_exec_android(void) {
	bootmode=BOOTMODE_NORMAL;

#if WITH_XIAOMI_DUALBOOT
	if(is_dualboot_supported() && get_dualboot_mode()==BOOTMODE_SECOND) {
		bootmode=BOOTMODE_SECOND;
	}
#endif

	aboot_continue_boot();
}
static void menu_format_android(char** buf) {
	*buf = calloc(100, 1);

#if WITH_XIAOMI_DUALBOOT
	if(is_dualboot_supported()) {
		snprintf(*buf, 100, "    Android [%s]", get_dualboot_mode()==BOOTMODE_SECOND?"System2":"System1");
		return;
	}
#endif

	snprintf(*buf, 100, "    Android");
}

#if WITH_XIAOMI_DUALBOOT
static void menu_exec_android2(void) {
	bootmode=BOOTMODE_SECOND;

	if(get_dualboot_mode()==BOOTMODE_SECOND) {
		bootmode=BOOTMODE_NORMAL;
	}

	aboot_continue_boot();
}
static void menu_format_android2(char** buf) {
	*buf = calloc(100, 1);

	snprintf(*buf, 100, "    Android [%s]", get_dualboot_mode()==BOOTMODE_SECOND?"System1":"System2");
}
static bool menu_hide_android2(void) {
	return !is_dualboot_supported();
}
#endif

static void menu_exec_recovery(void) {
	bootmode=BOOTMODE_RECOVERY;
	aboot_continue_boot();
}

static void menu_exec_grub(void) {
	bootmode=BOOTMODE_GRUB;
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
	{"    Normal Powerup", &menu_exec_normal, &menu_format_normal, NULL},
	{"    Android", &menu_exec_android, &menu_format_android, NULL},
#if WITH_XIAOMI_DUALBOOT
	{"    Android", &menu_exec_android2, &menu_format_android2, &menu_hide_android2},
#endif
	{"    Recovery", &menu_exec_recovery, NULL, NULL},
	{"    GRUB", &menu_exec_grub, NULL, NULL},
	{"    Download Mode", &menu_dload_mode, NULL, NULL},
	{"    Settings...", &menu_settings, NULL, NULL},
	{"    Reboot", &menu_reboot, NULL, NULL},
	{"    Shutdown", &menu_shutdown, NULL, NULL},
	{NULL,NULL,NULL,NULL},
};
