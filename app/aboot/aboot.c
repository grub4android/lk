#include <app.h>
#include <err.h>
#include <debug.h>
#include <printf.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <lib/bio.h>
#include <dev/keys.h>
#include <app/aboot.h>
#include <lib/android.h>
#include <lib/console.h>
#include <kernel/thread.h>

#if WITH_LIB_SYSPARAM
#include <lib/sysparam.h>
#endif

#if WITH_LIB_UEFI
#include <uefi/pe32.h>
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

static bootmode_t aboot_initial_bootmode(void) {
	// get key states
	int key_volup = keys_get_state(KEY_VOLUMEUP);
	int key_voldown = keys_get_state(KEY_VOLUMEDOWN);
	int key_home = keys_get_state(KEY_HOME);
	int key_back = keys_get_state(KEY_BACK);
	dprintf(INFO, "keys: up:%d down:%d home:%d back:%d\n", key_volup, key_voldown, key_home, key_back);

	if(key_volup && key_voldown) {
		return BOOTMODE_DOWNLOAD;
	}

	if(key_home || key_volup) {
		return BOOTMODE_RECOVERY;
	}

	if(key_back || key_voldown) {
		return BOOTMODE_FASTBOOT;
	}

	switch(platform_get_reboot_reason()) {
		case HALT_REASON_SW_UPDATE:
			return BOOTMODE_RECOVERY;
		case HALT_REASON_SW_PANIC:
			return BOOTMODE_PANIC;
		case HALT_REASON_SW_BOOTLOADER:
			return BOOTMODE_FASTBOOT;
		case HALT_REASON_ALARM:
			return BOOTMODE_ANDROID;

		default:
			return BOOTMODE_AUTO;

	}
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

static uint aboot_boot_grub(void) {
	bdev_t* dev = bio_open("grub");
	if(!dev) {
		dprintf(CRITICAL, "grub partition not found\n");
		return ERR_INVALID_ARGS;
	}

	return peloader_load(dev, NULL);
}

void aboot_boot(bootmode_t bootmode) {
	switch(bootmode) {
		case BOOTMODE_AUTO:
			if(!aboot_boot_grub()) {
				thread_become_idle();
				break;
			}
			aboot_boot_partition("boot");
			break;
		case BOOTMODE_ANDROID:
			aboot_boot_partition("boot");
			break;
		case BOOTMODE_RECOVERY:
			aboot_boot_partition("recovery");
			break;
		case BOOTMODE_DOWNLOAD:
			// TODO implement dload
			break;
		case BOOTMODE_PANIC:
			// TODO display last_kmsg on screen
			break;
		case BOOTMODE_GRUB:
			aboot_boot_grub();
			break;

		default:
			dprintf(CRITICAL, "Unsupported boot mode %d\n", bootmode);
	}
}

static void aboot_init(const struct app_descriptor *app)
{
#ifdef GRUB_PARTITION_NAME
	bdev_t* dev = bio_open_by_label(GRUB_PARTITION_NAME);
	if(dev) {
		bnum_t start_block = 0;

#ifdef GRUB_PARTITION_OFFSET
		start_block = ALIGN(GRUB_PARTITION_OFFSET, dev->block_size)/dev->block_size;
#endif

		if(!bio_publish_subdevice(dev->name, "grub", start_block, dev->block_count - start_block - dev->block_size)) {
			bdev_t* grubdev = bio_open("grub");
			grubdev->label = strdup("grub");
			grubdev->is_virtual = true;
			bio_close(grubdev);
		}
	}
#endif

	aboot_init_sysparam();

	bootmode_t bootmode = aboot_initial_bootmode();
	switch(bootmode) {
		case BOOTMODE_FASTBOOT:
			// fall through so fastboot can be started
			break;

		default:
			aboot_boot(bootmode);
	}
}

APP_START(aboot)
    .init = aboot_init,
APP_END
