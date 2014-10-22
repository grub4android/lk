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

void menu_back(void) {
	menu_leave();
}

#if WITH_XIAOMI_DUALBOOT
void menu_exec_dualbootmode(void) {
	set_dualboot_mode(dual_boot_sign==DUALBOOT_BOOT_SECOND?DUALBOOT_BOOT_FIRST:DUALBOOT_BOOT_SECOND);
}
void menu_format_dualbootmode(char** buf) {
	*buf = calloc(100, 1);
	snprintf(*buf, 100, "    Dualboot mode [%s]", dual_boot_sign==DUALBOOT_BOOT_SECOND?"System2":"System1");
}
#endif

void menu_chargerscreen(void) {
	device.charger_screen_enabled = !device.charger_screen_enabled;
	write_device_info(&device);
}
void menu_chargerscreen_format(char** buf) {
	*buf = calloc(100, 1);
	snprintf(*buf, 100, "    Charger Screen [%s]", device.charger_screen_enabled?"enabled":"disabled");
}

void menu_splash(void) {
	device.use_splash_partition = !device.use_splash_partition;
	write_device_info(&device);
}
void menu_splash_format(char** buf) {
	*buf = calloc(100, 1);
	snprintf(*buf, 100, "    Splash Logo [%s]", device.use_splash_partition?"enabled":"disabled");
}

void menu_forcefastboot(void) {
	device.force_fastboot = !device.force_fastboot;
	write_device_info(&device);
}
void menu_forcefastboot_format(char** buf) {
	*buf = calloc(100, 1);
	snprintf(*buf, 100, "    Force Fastboot Mode [%s]", device.force_fastboot?"enabled":"disabled");
}

struct menu_entry entries_settings[] = {
	{"    <-- Back", &menu_back, NULL},
#if WITH_XIAOMI_DUALBOOT
	{"", &menu_exec_dualbootmode, &menu_format_dualbootmode},
#endif
	{"", &menu_chargerscreen, &menu_chargerscreen_format},
	{"", &menu_splash, &menu_splash_format},
	{"", &menu_forcefastboot, &menu_forcefastboot_format},
	{NULL,NULL,NULL},
};
