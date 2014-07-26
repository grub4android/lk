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

#include "uboot_part.h"

static unsigned long block_read(int dev, lbaint_t start, lbaint_t blkcnt, void *buffer);
static unsigned long block_write(int dev, lbaint_t start, lbaint_t blkcnt, const void *buffer);
static int initialized = 0;

static block_dev_desc_t mmcdev = {
	.type = DEV_TYPE_HARDDISK,

	.lba = 128,
	.blksz = BLOCK_SIZE,
	
	.block_read = block_read,
	.block_write = block_write,
};

static unsigned long block_read(int dev, lbaint_t start, lbaint_t blkcnt, void *buffer) {
	if(dev==0) {
		return mmc_read(start*BLOCK_SIZE, buffer, blkcnt*BLOCK_SIZE);
	}

	return 0;
}

static unsigned long block_write(int dev, lbaint_t start, lbaint_t blkcnt, const void *buffer) {

	return 0;
}

static int initialize(void) {
	struct mmc_boot_host *mmc_host;
	struct mmc_boot_card *mmc_card;

	mmc_host = get_mmc_host();
	mmc_card = get_mmc_card();

	mmcdev.lba = (mmc_card->capacity) / BLOCK_SIZE;
	return 0;
}

block_dev_desc_t *get_dev(const char *ifname, int dev) {
	if(!initialized && initialize())
		return NULL;

	if(strcmp(ifname, "mmc")==0 && dev==0) {
		return &mmcdev;
	}

	return NULL;
}
