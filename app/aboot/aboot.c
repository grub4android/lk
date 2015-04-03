#include <app.h>
#include <debug.h>
#include <printf.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <lib/bio.h>
#include <dev/keys.h>
#include <lib/android.h>

#if WITH_LIB_SYSPARAM
#include <lib/sysparam.h>
#endif

static void aboot_init_sysparam(void) {
#if defined(SYSPARAM_PARTITION) && defined(WITH_LIB_SYSPARAM)
	bdev_t *dev = bio_open_by_label(SYSPARAM_PARTITION);
	if(!dev) {
		dprintf(CRITICAL, "sysparam partition not found\n");
		return;
	}

	if(sysparam_scan(dev, dev->size - dev->block_size, dev->block_size)!=(ssize_t)dev->block_size) {
		dprintf(SPEW, "error scanning sysparam partition\n");
	}

	//bootcount_aboot_inc();

	sysparam_dump(true);
#endif
}

static platform_halt_reason aboot_get_bootmode(void) {
	// get key states
	int key_volup = keys_get_state(KEY_VOLUMEUP);
	int key_voldown = keys_get_state(KEY_VOLUMEDOWN);
	int key_home = keys_get_state(KEY_HOME);
	int key_back = keys_get_state(KEY_BACK);
	dprintf(INFO, "keys: up:%d down:%d home:%d back:%d\n", key_volup, key_voldown, key_home, key_back);

	if(key_volup && key_voldown) {
		return HALT_REASON_SW_DOWNLOAD;
	}

	if(key_home || key_volup) {
		return HALT_REASON_SW_RECOVERY;
	}

	if(key_back || key_voldown) {
		return HALT_REASON_SW_BOOTLOADER;
	}

	platform_halt_reason reason = platform_get_reboot_reason();
	dprintf(INFO, "halt reason: %d\n", reason);
	return reason;
}

static void aboot_boot_partition(const char* name) {
	android_parsed_bootimg_t parsed;

	// parse bootimg
	if(android_parse_partition(name, &parsed)) {
		dprintf(CRITICAL, "error loading partition '%s'\n", name);
		return;
	}

	// boot
	android_do_boot(&parsed, false);
}

void aboot_boot(platform_halt_reason reason) {
	switch(reason) {
		case HALT_REASON_SW_RESET:
		case HALT_REASON_UNKNOWN:
			aboot_boot_partition("boot");
			break;
		case HALT_REASON_SW_RECOVERY:
			aboot_boot_partition("recovery");
			break;
		case HALT_REASON_SW_BOOTLOADER:
			platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_BOOTLOADER);
			break;

		default:
			dprintf(CRITICAL, "Unsupported boot mode %d\n", reason);
	}
}

static void aboot_init(const struct app_descriptor *app)
{
	aboot_init_sysparam();

	switch(aboot_get_bootmode()) {
		case HALT_REASON_SW_RESET:
		case HALT_REASON_ALARM:
		case HALT_REASON_UNKNOWN:
			aboot_boot_partition("boot");
			break;
		case HALT_REASON_SW_RECOVERY:
			aboot_boot_partition("recovery");
			break;

		default:
		case HALT_REASON_SW_PANIC:
			// maybe display last_kmsg on screen?
		case HALT_REASON_SW_BOOTLOADER:
			// fall through so fastboot can be started
			break;
	}
}

APP_START(aboot)
    .init = aboot_init,
APP_END
