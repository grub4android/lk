#include <app.h>
#include <debug.h>
#include <string.h>
#include <printf.h>
#include <stdlib.h>
#include <limits.h>
#include <arch/ops.h>
#include <dev/keys.h>
#include <app/aboot.h>
#include <platform.h>

#include "menu_private.h"

static void menu_exec_back(void) {
	menu_leave();
}

#if WITH_XIAOMI_DUALBOOT
static enum bootmode dualboot_mode = BOOTMODE_NORMAL;
static void menu_exec_dualbootmode(void) {
	dualboot_mode = dualboot_mode==BOOTMODE_SECOND?BOOTMODE_NORMAL:BOOTMODE_SECOND;
	set_dualboot_mode(dualboot_mode);
}
static void menu_format_dualbootmode(char** buf) {
	dualboot_mode = get_dualboot_mode();
	*buf = calloc(100, 1);
	snprintf(*buf, 100, "    Dualboot mode [%s]", dualboot_mode==BOOTMODE_SECOND?"System2":"System1");
}
static bool menu_hide_dualbootmode(void) {
	return !is_dualboot_supported();
}
#endif

static void menu_exec_bootmode(void) {
	if(device.bootmode<BOOTMODE_MAX-1)
		device.bootmode++;
	else device.bootmode = 0;

#if WITH_XIAOMI_DUALBOOT
	if(!is_dualboot_supported() && device.bootmode==BOOTMODE_SECOND)
		device.bootmode++;
#endif

	write_device_info(&device);
}
static void menu_format_bootmode(char** buf) {
	*buf = calloc(100, 1);
	snprintf(*buf, 100, "    Bootmode [%s]", strbootmode(device.bootmode));
}

static void menu_exec_chargerscreen(void) {
	device.charger_screen_enabled = !device.charger_screen_enabled;
	write_device_info(&device);
}
static void menu_format_chargerscreen(char** buf) {
	*buf = calloc(100, 1);
	snprintf(*buf, 100, "    Charger Screen [%s]", device.charger_screen_enabled?"enabled":"disabled");
}

static void menu_exec_splash(void) {
	device.use_splash_partition = !device.use_splash_partition;
	write_device_info(&device);
}
static void menu_format_splash(char** buf) {
	*buf = calloc(100, 1);
	snprintf(*buf, 100, "    Splash Logo [%s]", device.use_splash_partition?"enabled":"disabled");
}

struct menu_entry entries_settings[] = {
	{"    <-- Back", &menu_exec_back, NULL, NULL},
#if WITH_XIAOMI_DUALBOOT
	{"", &menu_exec_dualbootmode, &menu_format_dualbootmode, &menu_hide_dualbootmode},
#endif
	{"", &menu_exec_bootmode, &menu_format_bootmode, NULL},
	{"", &menu_exec_chargerscreen, &menu_format_chargerscreen, NULL},
	{"", &menu_exec_splash, &menu_format_splash, NULL},
	{NULL,NULL,NULL,NULL},
};
