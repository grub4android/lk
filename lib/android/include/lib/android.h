#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <lib/android/bootimg.h>

typedef struct {
	boot_img_hdr_t* hdr;
	vaddr_t linux_mem;
	uint32_t machtype;

	void* kernel;
	void* ramdisk;
	void* second;
	void* tags;
	char* cmdline;

	void* kernel_loaded;
	void* ramdisk_loaded;
	void* second_loaded;
	void* tags_loaded;	
} android_parsed_bootimg_t;

int android_parse_bootimg(void* ptr, size_t size, android_parsed_bootimg_t* parsed);
int android_free_parsed_bootimg(android_parsed_bootimg_t* parsed);
int android_allocate_boot_memory(android_parsed_bootimg_t* parsed);
int android_load_images(android_parsed_bootimg_t* parsed);
int android_add_board_info(android_parsed_bootimg_t* parsed);

#define ANDROID_MAX_ATAGS_SIZE (1*1024*1024)
