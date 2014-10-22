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
static void menu_exec_dualbootmode(void) {
	set_dualboot_mode(dual_boot_sign==DUALBOOT_BOOT_SECOND?DUALBOOT_BOOT_FIRST:DUALBOOT_BOOT_SECOND);
}
static void menu_format_dualbootmode(char** buf) {
	*buf = calloc(100, 1);
	snprintf(*buf, 100, "    Dualboot mode [%s]", dual_boot_sign==DUALBOOT_BOOT_SECOND?"System2":"System1");
}
#endif

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

static void menu_exec_forcefastboot(void) {
	device.force_fastboot = !device.force_fastboot;
	write_device_info(&device);
}
static void menu_format_forcefastboot(char** buf) {
	*buf = calloc(100, 1);
	snprintf(*buf, 100, "    Force Fastboot Mode [%s]", device.force_fastboot?"enabled":"disabled");
}

struct menu_entry entries_settings[] = {
	{"    <-- Back", &menu_exec_back, NULL},
#if WITH_XIAOMI_DUALBOOT
	{"", &menu_exec_dualbootmode, &menu_format_dualbootmode},
#endif
	{"", &menu_exec_chargerscreen, &menu_format_chargerscreen},
	{"", &menu_exec_splash, &menu_format_splash},
	{"", &menu_exec_forcefastboot, &menu_format_forcefastboot},
	{NULL,NULL,NULL},
};
