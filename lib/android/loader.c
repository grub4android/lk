#include <err.h>
#include <pow2.h>
#include <trace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <kernel/vm.h>
#include <lib/android.h>
#include <platform/qcom.h>
#include <platform/board.h>

#include "atags.h"

#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))

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

	// terminate cmdlines
	hdr->cmdline[BOOT_ARGS_SIZE-1] = 0;
	hdr->extra_cmdline[BOOT_EXTRA_ARGS_SIZE-1] = 0;

	// set image pointers
	parsed->kernel = ptr+hdr->page_size;
	parsed->ramdisk = parsed->kernel + ALIGN(hdr->kernel_size, hdr->page_size);
	parsed->second = parsed->ramdisk + ALIGN(hdr->second_size, hdr->page_size);
	parsed->tags = parsed->second + ALIGN(hdr->dt_size, hdr->page_size);

	// create cmdline
	size_t len_cmdline = strlen((char*)hdr->cmdline);
	size_t len_cmdline_extra = strlen((char*)hdr->extra_cmdline);
	parsed->cmdline = malloc(len_cmdline + len_cmdline_extra + 1);
	strncpy(parsed->cmdline, (char*)hdr->cmdline, len_cmdline+1);
	strncpy(parsed->cmdline + len_cmdline, (char*)hdr->extra_cmdline, len_cmdline_extra+1);
	parsed->cmdline[len_cmdline + len_cmdline_extra] = 0;

	return NO_ERROR;
}

int android_free_parsed_bootimg(android_parsed_bootimg_t* parsed) {
	// check parsed ptr
	if(!parsed)
		return ERR_INVALID_ARGS;

	if(parsed->cmdline)
		free(parsed->cmdline);

	if(parsed->linux_mem)
		vmm_free_region(vmm_get_kernel_aspace(), parsed->linux_mem);

	return NO_ERROR;
}

int android_allocate_boot_memory(android_parsed_bootimg_t* parsed) {
	int rc = NO_ERROR;

	// allocate predefined physical linux memory
	if(vmm_alloc_physical(vmm_get_kernel_aspace(), __func__, LINUX_SIZE, (void**)&parsed->linux_mem, LINUX_BASE, 0, ARCH_MMU_FLAG_UNCACHED_DEVICE)<0) {
		return ERR_NO_MEMORY;
	}

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

static void* mmap_iteration_cb(void* pdata, paddr_t addr, size_t size) {
	struct tag *tag = pdata;

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
