/*
 * (C) Copyright 2007-2008 Semihalf
 *
 * Written by: Rafal Jaworowski <raj@semihalf.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <debug.h>
#include <string.h>
#include <malloc.h>
#include <mmc.h>
#include <partition_parser.h>
#include <lib/tar.h>

#include "uboot_part.h"
#include "../grub.h"

static unsigned long block_read(int dev, lbaint_t start, lbaint_t blkcnt, void *buffer);
static unsigned long block_write(int dev, lbaint_t start, lbaint_t blkcnt, const void *buffer);
static int initialized = 0;

static block_dev_desc_t mmcdev = {
	.type = DEV_TYPE_HARDDISK,

	.lba = 0,
	.blksz = BLOCK_SIZE,

	.block_read = block_read,
	.block_write = block_write,
};

static block_dev_desc_t tardev = {
	.type = DEV_TYPE_HARDDISK,

	.lba = 0,
	.blksz = BLOCK_SIZE,

	.block_read = block_read,
	.block_write = block_write,
};

static unsigned long block_read(int dev, lbaint_t start, lbaint_t blkcnt, void *buffer) {
	// NAND
	if(dev==0) {
		unsigned long long ptn = ((unsigned long long) start)*BLOCK_SIZE;
		if(mmc_read(ptn, buffer, blkcnt*BLOCK_SIZE))
			return 0;
		return blkcnt*BLOCK_SIZE;
	}
	// BOOT-TAR
	else if(dev==1 && grub_has_tar()) {
		struct tar_io *tio = grub_tar_get_tio();
		return tio->block_read(tio, start, blkcnt, buffer);
	}

	return 0;
}

static unsigned long block_write(int dev, lbaint_t start, lbaint_t blkcnt, const void *buffer) {
	return 0;
}

static int initialize(void) {
	struct tar_io *tio = grub_tar_get_tio();

	mmcdev.lba = (mmc_get_device_capacity()) / BLOCK_SIZE;
	tardev.lba = tio->lba;

	initialized = 1;
	return 0;
}

block_dev_desc_t *get_dev(const char *ifname, int dev) {
	if(!initialized && initialize())
		return NULL;

	if(strcmp(ifname, "mmc")==0) {
		// NAND
		if(dev==0) return &mmcdev;
		// BOOT-TAR
		else if(dev==1 && grub_has_tar()) return &tardev;
	}

	return NULL;
}
