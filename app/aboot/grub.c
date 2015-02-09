#include <err.h>
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <printf.h>
#include <malloc.h>
#include <lib/bio.h>
#include <platform.h>
#include <kernel/thread.h>
#include <app/aboot.h>

#if WITH_APP_DISPLAY_SERVER
#include <app/display_server.h>
#endif

#include "grub.h"
#include <api_public.h>
#include "uboot_api/api_private.h"
#include "uboot_api/uboot_part.h"
#include "bootimg.h"

static char* grub_bootdev = NULL;
void grub_get_bootdev(char **value) {
	*value = grub_bootdev;
}

static char* grub_bootpath = NULL;
void grub_get_bootpath(char **value) {
	*value = grub_bootpath;
}

#ifdef GRUB_BOOT_PARTITION
#define GRUB_MOUNTPOINT "/"GRUB_BOOT_PARTITION"/"
#define GRUB_PATH GRUB_BOOT_PATH_PREFIX"grub"

static int set_mmc_as_bootpart(void) {
	// store bootdev
	bdev_t* dev = bio_open_by_label(GRUB_BOOT_PARTITION);
	if (dev) {
		grub_bootdev = strdup(dev->name);
		grub_bootpath = strdup("/" GRUB_BOOT_PATH_PREFIX "grub");
		bio_close(dev);

		return NO_ERROR;
	}

	return ERR_NOT_FOUND;
}

static int grub_load_from_mmc(void) {
	dprintf(INFO, "%s: part=[%s] path=[%s]\n", __func__, GRUB_BOOT_PARTITION, GRUB_PATH);

	// open device
	bdev_t* dev = bio_open_by_label(ABOOT_PARTITION);
	if(!dev) {
		dprintf(CRITICAL, "Couldn't find partition %s\n", ABOOT_PARTITION);
		return ERR_NOT_FOUND;
	}

	// read core.img
	off_t off = GRUB_PARTITION_OFFSET;
	void* entry = (void*)GRUB_LOADING_ADDRESS_VIRTUAL;
	if(dev->read(dev, entry, off, dev->size-off)<=0) {
		dprintf(CRITICAL, "BIO read error\n");
		return ERR_IO;
	}

	// close device
	bio_close(dev);

	// set bootpart
	set_mmc_as_bootpart();

	dprintf(INFO, "Loaded GRUB from MMC\n");
	return NO_ERROR;
}
#endif

static int grub_sideload_handler(void *data)
{
	char* memdisk_name = NULL;
	struct boot_img_hdr *hdr =  (struct boot_img_hdr *)data;
	unsigned kernel_size = ALIGN(hdr->kernel_size, hdr->page_size);
	void* local_kernel_addr = data + hdr->page_size;
	void* local_ramdisk_addr = local_kernel_addr + kernel_size;

	// translate ramdisk_addr to phys
	hdr->ramdisk_addr = hdr->ramdisk_addr-GRUB_LOADING_ADDRESS_VIRTUAL+GRUB_LOADING_ADDRESS;

	// Load kernel & ramdisk (to phys locations)
	memmove((void*) hdr->kernel_addr-GRUB_LOADING_ADDRESS_VIRTUAL+GRUB_LOADING_ADDRESS, local_kernel_addr, hdr->kernel_size);
	memmove((void*) hdr->ramdisk_addr, local_ramdisk_addr, hdr->ramdisk_size);

	// use ramdisk
	if(hdr->ramdisk_size>0) {
		// register ramdisk
		memdisk_name = malloc(20);
		snprintf(memdisk_name, sizeof(memdisk_name), "hd%d", bio_num_devices(false));
		create_membdev(memdisk_name, (void*)hdr->ramdisk_addr, hdr->ramdisk_size);
		dev_stor_scan_devices();

		// set bootdev
		grub_bootdev = strdup(memdisk_name);
		grub_bootpath = strdup("/grub");
	}
#ifdef GRUB_BOOT_PARTITION
	else set_mmc_as_bootpart();
#endif
	dprintf(INFO, "bootdev: %s\n", grub_bootdev);
	dprintf(INFO, "bootpath: %s\n", grub_bootpath);

	// BOOT !
	void (*entry)(unsigned, unsigned, unsigned*) = (void*)hdr->kernel_addr;
	dprintf(INFO, "booting GRUB from sideload @ %p ramdisk @ %p\n", entry, (void*)hdr->ramdisk_addr);

#if WITH_APP_DISPLAY_SERVER
	display_server_stop();
#endif

	entry(0, board_machtype(), (void*)uboot_api_sig);

	// delete ramdisk in cae we used one
	if(memdisk_name) {
		bdev_t* dev = bio_open(memdisk_name);
		if (dev) {
			bio_unregister_device(dev);
			bio_close(dev);
		}
		memdisk_name = NULL;
	}

	return NO_ERROR;
}

int grub_load_from_sideload(void* data) {
	thread_t *thr;

	thr = thread_create("grub_sideload", grub_sideload_handler, data, DEFAULT_PRIORITY, 4096);
	if (!thr)
	{
		return ERR_GENERIC;
	}
	thread_resume(thr);
	return NO_ERROR;
}

int grub_boot(void)
{
	void (*entry)(unsigned, unsigned, unsigned*) = (void*)GRUB_LOADING_ADDRESS_VIRTUAL;

	// load grub into RAM
#ifdef GRUB_BOOT_PARTITION
	if(grub_load_from_mmc()) {
		dprintf(CRITICAL, "%s: failed to load grub from mmc.\n", __func__);
	}
	else goto boot;
#endif

	dprintf(CRITICAL, "%s: Couldn't find grub at any known location!\n", __func__);
	return ERR_NOT_FOUND;

boot:
	// BOOT !
	dprintf(INFO, "booting GRUB @ %p\n", entry);

#if WITH_APP_DISPLAY_SERVER
	display_server_stop();
#endif

	entry(0, board_machtype(), (void*)uboot_api_sig);
	
	return NO_ERROR;
}

