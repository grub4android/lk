#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <mmc.h>
#include <partition_parser.h>
#include <malloc.h>
#include <lib/tar.h>
#include <platform.h>

#include "grub.h"
#include "uboot_api/uboot_part.h"

struct tar_io_priv {
	unsigned long long index;
	unsigned long long ptn;
};

unsigned long grub_tar_read(struct tar_io *tio, ulong start, ulong blkcnt, void *buffer) {
	struct tar_io_priv *priv = (struct tar_io_priv*) tio->priv;

	unsigned long long ptn = ((unsigned long long) start)*BLOCK_SIZE;
	return mmc_read(priv->ptn + ptn, buffer, blkcnt*BLOCK_SIZE);
}

static struct tar_io_priv priv;
static struct tar_fileinfo fi;
static struct tar_io tio = {
	.block_read = &grub_tar_read,
	.priv = (void*)&priv,
};

struct tar_io* grub_tar_get_tio(void) {
	return &tio;
}

int grub_boot(void)
{
	// prepare block api
	priv.index = partition_get_index("aboot");
	priv.ptn = partition_get_offset(priv.index) + 1024*1024; // 1MB offset to aboot
	tio.blksz = BLOCK_SIZE;
	tio.lba = partition_get_size(priv.index) / tio.blksz;

	// search file
	if(tar_get_fileinfo(&tio, "./boot/grub/core.img", &fi)) {
		dprintf(CRITICAL, "%s: couldn't find core.img!\n", __func__);
		return -1;
	}

	// load file into RAM
	void* buf = (void*) GRUB_LOADING_ADDRESS;
	if(tar_read_file(&tio, &fi, buf)) {
		dprintf(CRITICAL, "%s: couldn't read core.img!\n", __func__);
		return -1;
	}

	// BOOT !
	void (*entry)(unsigned, unsigned, unsigned*) = (void*)buf;
	dprintf(INFO, "booting GRUB @ %p\n", entry);
	entry(0, board_machtype(), NULL);
	
	return 0;
}

