#include <err.h>
#include <pow2.h>
#include <trace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <lib/bio.h>
#include <kernel/vm.h>
#include <lib/android.h>
#include <lib/android/cmdline.h>
#include <platform/qcom.h>
#include <platform/board.h>
#include <platform/baseband.h>
#include <platform/msm_panel.h>
#include <dev/gcdb/panel_display.h>
#include <lib/sysparam.h>
#include <app/fastboot.h>
#include <lk/init.h>

#include "atags.h"

#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))

#pragma GCC diagnostic ignored "-Wtype-limits"
static int internal_allocate_mem(vaddr_t linux_virt, size_t size, uint32_t addr, void** result) {
	// check if start addr is inside linux range
	if(!(addr>=LINUX_BASE && addr<=LINUX_BASE+LINUX_SIZE))
		return ERR_NO_MEMORY;

	// check if end addr i inside linux range
	if(!(addr+size>=LINUX_BASE && addr+size<=LINUX_BASE+LINUX_SIZE))
		return ERR_NO_MEMORY;

	*result = (void*)(addr-LINUX_BASE+linux_virt);

	return NO_ERROR;
}

static int internal_read_image_from_dev(bdev_t* dev, off_t offset, size_t size, void** buf) {
	// allocate memory
	*buf = malloc(size);
	if(!*buf) return ERR_NO_MEMORY;

	// read data
	if((size_t)bio_read(dev, *buf, offset, size)!=size) {
		free(*buf);
		*buf = NULL;
		return ERR_IO;
	}
	return NO_ERROR;
}

int internal_prepare_cmdline(android_parsed_bootimg_t* parsed) {
	boot_img_hdr_t* hdr = parsed->hdr;

	// terminate cmdlines
	hdr->cmdline[BOOT_ARGS_SIZE-1] = 0;
	hdr->extra_cmdline[BOOT_EXTRA_ARGS_SIZE-1] = 0;

	// create cmdline
	size_t len_cmdline = strlen((char*)hdr->cmdline);
	size_t len_cmdline_extra = strlen((char*)hdr->extra_cmdline);
	parsed->cmdline = malloc(len_cmdline + len_cmdline_extra + 1);
	strncpy(parsed->cmdline, (char*)hdr->cmdline, len_cmdline+1);
	strncpy(parsed->cmdline + len_cmdline, (char*)hdr->extra_cmdline, len_cmdline_extra+1);
	parsed->cmdline[len_cmdline + len_cmdline_extra] = 0;

	return NO_ERROR;
}

int android_parse_bootimg(void* ptr, size_t size, android_parsed_bootimg_t* parsed) {
	// check data ptr
	if(!ptr)
		return ERR_INVALID_ARGS;

	// check size
	if(size<sizeof(boot_img_hdr_t))
		return ERR_INVALID_ARGS;

	// check parsed ptr
	if(!parsed)
		return ERR_INVALID_ARGS;

	// initialize parsed data
	memset(parsed, 0, sizeof(*parsed));

	// get bootimg header
	boot_img_hdr_t* hdr = ptr;
	parsed->hdr = hdr;

	// set image pointers
	parsed->kernel = ptr+hdr->page_size;
	parsed->ramdisk = parsed->kernel + ALIGN(hdr->kernel_size, hdr->page_size);
	parsed->second = parsed->ramdisk + ALIGN(hdr->second_size, hdr->page_size);
	parsed->tags = parsed->second + ALIGN(hdr->dt_size, hdr->page_size);

	// prepare cmdline
	return internal_prepare_cmdline(parsed);
}

int android_parse_partition(const char* name, android_parsed_bootimg_t* parsed) {
	// check parsed ptr
	if(!parsed)
		return ERR_INVALID_ARGS;

	// initialize parsed data
	memset(parsed, 0, sizeof(*parsed));

	// open dev
	bdev_t* dev = bio_open_by_label(name);
	if(!dev) {
		return ERR_NOT_FOUND;
	}

	// set bio flag
	parsed->from_bio = true;

	// read bootimgheader
	boot_img_hdr_t* hdr = malloc(sizeof(boot_img_hdr_t));
	parsed->hdr = hdr;
	if(bio_read(dev, hdr, 0, sizeof(*hdr))!=sizeof(*hdr)) {
		return ERR_IO;
	}

	// calculate offsets
	off_t off_kernel = hdr->page_size;
	off_t off_ramdisk = off_kernel + ALIGN(hdr->kernel_size, hdr->page_size);
	off_t off_second = off_ramdisk + ALIGN(hdr->second_size, hdr->page_size);
	off_t off_tags = off_second + ALIGN(hdr->dt_size, hdr->page_size);

	// read images
	internal_read_image_from_dev(dev, off_kernel, hdr->kernel_size, &parsed->kernel);
	internal_read_image_from_dev(dev, off_ramdisk, hdr->ramdisk_size, &parsed->ramdisk);
	internal_read_image_from_dev(dev, off_second, hdr->second_size, &parsed->second);
	internal_read_image_from_dev(dev, off_tags, hdr->dt_size, &parsed->tags);

	// prepare cmdline
	return internal_prepare_cmdline(parsed);
}

int android_free_parsed_bootimg(android_parsed_bootimg_t* parsed) {
	// check parsed ptr
	if(!parsed)
		return ERR_INVALID_ARGS;

	if(parsed->cmdline) {
		free(parsed->cmdline);
		parsed->cmdline = NULL;
	}

	if(parsed->linux_mem) {
		parsed->linux_mem = 0;
	}

	if(parsed->from_bio) {
		if(parsed->kernel) {
			free(parsed->kernel);
			parsed->kernel = NULL;
		}
		if(parsed->ramdisk) {
			free(parsed->kernel);
			parsed->ramdisk = NULL;
		}
		if(parsed->second) {
			free(parsed->second);
			parsed->second = NULL;
		}
		if(parsed->tags) {
			free(parsed->tags);
			parsed->tags = NULL;
		}
		if(parsed->hdr) {
			free(parsed->hdr);
			parsed->hdr = NULL;
		}
	}

	return NO_ERROR;
}

int android_allocate_boot_memory(android_parsed_bootimg_t* parsed) {
	int rc = NO_ERROR;

	// allocate predefined physical linux memory
	parsed->linux_mem = LINUX_BASE;

	// kernel
	rc = internal_allocate_mem(parsed->linux_mem, parsed->hdr->kernel_size, parsed->hdr->kernel_addr, &parsed->kernel_loaded);
	if(rc) return rc;

	// ramdisk
	rc = internal_allocate_mem(parsed->linux_mem, parsed->hdr->ramdisk_size, parsed->hdr->ramdisk_addr, &parsed->ramdisk_loaded);
	if(rc) return rc;

	// second
	rc = internal_allocate_mem(parsed->linux_mem, parsed->hdr->second_size, parsed->hdr->second_addr, &parsed->second_loaded);
	if(rc) return rc;

	// tags
	rc = internal_allocate_mem(parsed->linux_mem, parsed->hdr->dt_size, parsed->hdr->tags_addr, &parsed->tags_loaded);
	if(rc) return rc;

	return rc;
}

int android_load_images(android_parsed_bootimg_t* parsed) {
	if(parsed->hdr->kernel_size>0 && parsed->kernel_loaded)
		memmove(parsed->kernel_loaded, parsed->kernel, parsed->hdr->kernel_size);

	if(parsed->hdr->ramdisk_size>0 && parsed->ramdisk_loaded)
		memmove(parsed->ramdisk_loaded, parsed->ramdisk, parsed->hdr->ramdisk_size);

	if(parsed->hdr->second_size>0 && parsed->second_loaded)
		memmove(parsed->second_loaded, parsed->second, parsed->hdr->second_size);

	if(parsed->hdr->dt_size>0 && parsed->tags_loaded)
		memmove(parsed->tags_loaded, parsed->tags, parsed->hdr->dt_size);

	return NO_ERROR;
}

static void* mmap_iteration_cb(void* pdata, paddr_t addr, size_t size, bool reserved) {
	struct tag *tag = pdata;

	if(reserved) return tag;

	tag = tag_next(tag);
	tag->hdr.tag = ATAG_MEM;
	tag->hdr.size = tag_size(tag_mem32);
	tag->u.mem.size = size;
	tag->u.mem.start = addr;

	return tag;
}

static int android_generate_atags(android_parsed_bootimg_t* parsed) {
	// TODO: add devicetree support
	if(parsed->hdr->dt_size>0)
		return ERR_NOT_IMPLEMENTED;

	// allocate tag list
	parsed->tags = malloc(ANDROID_MAX_ATAGS_SIZE);
	if(!parsed->tags)
		return ERR_NO_MEMORY;

	// CORE
	struct tag *tag = parsed->tags;
	tag->hdr.tag  = ATAG_CORE;
	tag->hdr.size = sizeof(struct tag_header)>>2; // we don't have a rootdev

	// initrd
	tag = tag_next(tag);
	tag->hdr.tag = ATAG_INITRD2;
	tag->hdr.size = tag_size(tag_initrd);
	tag->u.initrd.start = (uint32_t)kvaddr_to_paddr(parsed->ramdisk_loaded);
	tag->u.initrd.size  = parsed->hdr->ramdisk_size;

	// memory map
	tag = platform_get_mmap(tag, mmap_iteration_cb);

	// TODO: ptable

	// cmdline
	tag = tag_next(tag);
	tag->hdr.tag = ATAG_CMDLINE;
	tag->hdr.size = (strlen(parsed->cmdline) + 3 +
			 sizeof(struct tag_header)) >> 2;
	strcpy(tag->u.cmdline.cmdline, parsed->cmdline);

	// end
	tag = tag_next(tag);
	tag->hdr.tag = ATAG_NONE;
	tag->hdr.size = 0;

	// set tag size
	parsed->hdr->dt_size = ((void*)tag + sizeof(struct tag_header)) - parsed->tags;
	ASSERT(parsed->hdr->dt_size<=ANDROID_MAX_ATAGS_SIZE);

	// add machtype info
	parsed->machtype = board_machtype();

	return NO_ERROR;
}

int android_add_board_info(android_parsed_bootimg_t* parsed) {
	int rc = NO_ERROR;

	// add atags
	rc = android_generate_atags(parsed);
	if(rc) goto err;

	// TODO patch cmdline

	return NO_ERROR;

err:
	return rc;
}

#define PRERR(x) {if(fastboot_control) fastboot_fail((x)); else dprintf(CRITICAL, (x));}
int android_do_boot(android_parsed_bootimg_t* parsed, bool fastboot_control) {
	// TODO: second loader support
	if(parsed->hdr->second_size>0) {
		PRERR("second loaders are not supported");
		return ERR_NOT_SUPPORTED;
	}

	// allocate memory
	if(android_allocate_boot_memory(parsed)) {
		PRERR("error allocating memory");
		goto err;
	}
	dprintf(SPEW, "%s: kernel=%p(0x%08lx) ramdisk=%p(0x%08lx) second=%p(0x%08lx) tags=%p(0x%08lx)\n", __func__,
		parsed->kernel_loaded, parsed->kernel_loaded?kvaddr_to_paddr(parsed->kernel_loaded):0,
		parsed->ramdisk_loaded, parsed->ramdisk_loaded?kvaddr_to_paddr(parsed->ramdisk_loaded):0,
		parsed->second_loaded, parsed->second_loaded?kvaddr_to_paddr(parsed->second_loaded):0,
		parsed->tags_loaded, parsed->tags_loaded?kvaddr_to_paddr(parsed->tags_loaded):0);

	// generate tags
	if(android_add_board_info(parsed)) {
		PRERR("error generating tags");
		goto err;
	}

	// load images
	if(android_load_images(parsed)) {
		PRERR("error loading images");
		goto err;
	}

	// boot
	if(fastboot_control) {
		fastboot_okay("");
		fastboot_stop();
	}
	arch_chain_load(parsed->kernel_loaded, 0, parsed->machtype, kvaddr_to_paddr(parsed->tags_loaded), 0);

err:
	android_free_parsed_bootimg(parsed);

	return ERR_GENERIC;
}

ssize_t android_sysparam_cb(const char* name, void* data, size_t* len) {
	// get len
	size_t cmdline_len = cmdline_length();

	size_t read = 0;
	if(data)
		read = cmdline_generate(data, *len);

	// return length
	if(len)
		*len = cmdline_len;

	return read;
}

static void android_init(uint level)
{
	//
	// CMDLINE
	//

#ifdef ANDROID_BOOTTYPE_EMMC
	cmdline_add("androidboot.emmc", "true");
#endif
	cmdline_add("androidboot.serialno", target_serialno());
	// GPT
	// FFBM cookie + loglevel
	// ALARMBOOT
	// BATTCHARGE-PAUSE
	// DUALBOOT
	// SIGNED-KERNEL
	// TARGET-BOOTPARAMS

	// BASEBAND
	switch(target_baseband()) {
		CMDLINE_BASEBAND(BASEBAND_APQ, "apq");
		CMDLINE_BASEBAND(BASEBAND_MSM, "msm");
		CMDLINE_BASEBAND(BASEBAND_CSFB, "csfb");
		CMDLINE_BASEBAND(BASEBAND_SVLTE1, "svlte1");
		CMDLINE_BASEBAND(BASEBAND_SVLTE2A, "svlte2a");
		CMDLINE_BASEBAND(BASEBAND_MDM, "mdm");
		CMDLINE_BASEBAND(BASEBAND_MDM2, "mdm2");
		CMDLINE_BASEBAND(BASEBAND_SGLTE, "sglte");
		CMDLINE_BASEBAND(BASEBAND_SGLTE2, "sglte2");
		CMDLINE_BASEBAND(BASEBAND_DSDA, "dsda");
		CMDLINE_BASEBAND(BASEBAND_DSDA2, "dsda2");
		default:
			break;
	}

	// DISPLAY
	char display_panel[MAX_PANEL_ID_LEN];
	target_display_panel_node("", display_panel, MAX_PANEL_ID_LEN);
	cmdline_add(display_panel, NULL);

	// WARMBOOT

	// add dynamic cmdline param
	sysparam_add_dynamic("android_additional_cmdline", android_sysparam_cb);
}

LK_INIT_HOOK(android, &android_init, LK_INIT_LEVEL_TARGET);
