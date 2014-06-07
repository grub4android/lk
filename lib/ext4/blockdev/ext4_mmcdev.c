/*
 * Copyright (c) 2013 Grzegorz Kostka (kostka.grzegorz@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _FILE_OFFSET_BITS 64

#include <ext4_config.h>
#include <ext4_blockdev.h>
#include <ext4_errno.h>
#include <ext4_mmcdev.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <partition_parser.h>


/**********************BLOCKDEV INTERFACE**************************************/
static int mmcdev_open(struct ext4_blockdev *bdev);
static int mmcdev_bread(struct ext4_blockdev *bdev, void *buf, uint64_t blk_id,
    uint32_t blk_cnt);
static int mmcdev_bwrite(struct ext4_blockdev *bdev, const void *buf,
    uint64_t blk_id, uint32_t blk_cnt);
static int mmcdev_close(struct  ext4_blockdev *bdev);

/******************************************************************************/
static struct private_mmc_data* mmcdev_get_privatedata(struct ext4_blockdev *bdev) {
	return (struct private_mmc_data*)bdev->private_data;
}

/******************************************************************************/
static int mmcdev_open(struct ext4_blockdev *bdev)
{
	struct private_mmc_data* mmcdata = mmcdev_get_privatedata(bdev);
	unsigned long long index = INVALID_PTN;
	unsigned long long size;

	// get partition ptn
	index = partition_get_index(mmcdata->partname);
	mmcdata->ptn = partition_get_offset(index);
	if(mmcdata->ptn == 0) return EIO;

	// get size
	size = partition_get_size(index);
	bdev->ph_bsize = mmc_get_device_blocksize();
	bdev->ph_bcnt = size / bdev->ph_bsize;
	bdev->ph_bbuf = (uint8_t*)malloc(sizeof(uint8_t)*bdev->ph_bsize);

	return EOK;
}

/******************************************************************************/

static int mmcdev_bread(struct  ext4_blockdev *bdev, void *buf, uint64_t blk_id,
    uint32_t blk_cnt)
{
	struct private_mmc_data* mmcdata = mmcdev_get_privatedata(bdev);

	uint64_t offset = blk_id * bdev->ph_bsize;
	if (mmc_read(mmcdata->ptn + offset, buf, bdev->ph_bsize * blk_cnt))
	{
		dprintf(CRITICAL, "ERROR: mmc_read() fail.\n");
		return EIO;
	}

	return EOK;
}

/******************************************************************************/
static int mmcdev_bwrite(struct ext4_blockdev *bdev, const void *buf,
    uint64_t blk_id, uint32_t blk_cnt)
{
	return EOK;
}
/******************************************************************************/
static int mmcdev_close(struct  ext4_blockdev *bdev)
{
	struct private_mmc_data* mmcdata = mmcdev_get_privatedata(bdev);

	free(bdev->ph_bbuf);
	mmcdata->ptn = 0;
	return EOK;
}

/******************************************************************************/
struct ext4_blockdev* ext4_mmcdev_get(const char *partname)
{
	struct ext4_blockdev *dev = (void*)malloc(sizeof(struct ext4_blockdev));
	memset(dev, 0, sizeof(struct ext4_blockdev));

	dev->open = mmcdev_open;
	dev->bread = mmcdev_bread;
	dev->bwrite = mmcdev_bwrite;
	dev->close = mmcdev_close;
	dev->ph_bcnt = 0;
	dev->ph_bbuf = 0;

	struct private_mmc_data *mmcdata = (void*) malloc(sizeof(struct private_mmc_data));
	dev->private_data = mmcdata;
	mmcdata->partname = partname;

	return dev;
}
/******************************************************************************/
