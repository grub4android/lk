#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <mmc.h>
#include <partition_parser.h>
#include <malloc.h>
#include <lib/tar.h>
#include <platform.h>
#include <ext4.h>
#include <ext4_mmcdev.h>

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

static char* grub_bootdev = NULL;
void grub_get_bootdev(char **value) {
	*value = grub_bootdev;
}

static int grub_found_tar = 0;
int grub_has_tar(void) {
	return grub_found_tar;
}

#ifdef GRUB_BOOT_PARTITION
#define GRUB_MOUNTPOINT "/"GRUB_BOOT_PARTITION"/"
static int grub_load_from_mmc(void) {
	ext4_file f;
	uint32_t bytes_read;
	int ret = 0;

	dprintf(INFO, "%s: part=[%s]\n", __func__, GRUB_BOOT_PARTITION);

	// create ext4 device
	struct ext4_blockdev* mmcdev = ext4_mmcdev_get(GRUB_BOOT_PARTITION);
	if(!mmcdev){
		dprintf(CRITICAL, "MMC device ERROR\n");
		return -1;
	}

	// register it
	ret = ext4_device_register(mmcdev, 0, GRUB_BOOT_PARTITION);
	if(ret != EOK){
		dprintf(CRITICAL, "ext4_device_register ERROR = %d\n", ret);
		return ret;
	}

	// mount it
	ret = ext4_mount(GRUB_BOOT_PARTITION, GRUB_MOUNTPOINT);
	if(ret != EOK){
		dprintf(CRITICAL, "ext4_mount ERROR = %d\n", ret);
		return -1;
	}

	// open file
	ret = ext4_fopen(&f, GRUB_MOUNTPOINT"boot/grub/core.img", "rb");
	if(ret != EOK){
		dprintf(CRITICAL, "ext4_fopen ERROR = %d\n", ret);
		return -1;
	}

	// read file
	ret = ext4_fread(&f, GRUB_LOADING_ADDRESS, ext4_fsize(&f), &bytes_read);
	if(ret != EOK){
		dprintf(CRITICAL, "ext4_fread ERROR = %d\n", ret);
		return -1;
	}

	// close file
	ret = ext4_fclose(&f);
	if(ret != EOK){
		printf("ext4_fclose ERROR = %d\n", ret);
		return -1;
	}

	// unmount partition
	ret = ext4_umount(GRUB_MOUNTPOINT);
	if(ret != EOK){
		printf("ext4_umount ERROR = %d\n", ret);
		return -1;
	}

	// store bootdev
	unsigned int index = (unsigned int) partition_get_index(GRUB_BOOT_PARTITION);
	char buf[20];
	sprintf(buf, "hd0,%u", index+1);
	grub_bootdev = strdup(buf);

	dprintf(INFO, "Loaded GRUB from MMC\n");
	return 0;
}
#endif

static int grub_load_from_tar(void) {
	// prepare block api
	priv.index = partition_get_index("aboot");
	priv.ptn = partition_get_offset(priv.index) + 1024*1024; // 1MB offset to aboot
	tio.blksz = BLOCK_SIZE;
	tio.lba = partition_get_size(priv.index) / tio.blksz - 1;

	// search file
	if(tar_get_fileinfo(&tio, "./boot/grub/core.img", &fi)) {
		dprintf(CRITICAL, "%s: couldn't find core.img!\n", __func__);
		return -1;
	}

	// load file into RAM
	if(tar_read_file(&tio, &fi, GRUB_LOADING_ADDRESS)) {
		dprintf(CRITICAL, "%s: couldn't read core.img!\n", __func__);
		return -1;
	}

	grub_bootdev = strdup("hd1");
	grub_found_tar = 1;

	dprintf(INFO, "Loaded GRUB from TAR\n");
	return 0;
}

int grub_boot(void)
{
	// load grub into RAM
#ifdef GRUB_BOOT_PARTITION
	if(grub_load_from_mmc()) {
		dprintf(CRITICAL, "%s: failed to load grub from mmc, trying tar now.\n", __func__);
#else
	{
#endif
		if(grub_load_from_tar()) {
			dprintf(CRITICAL, "%s: failed to load grub from tar.\n", __func__);
			return -1;
		}
	}

	// BOOT !
	void (*entry)(unsigned, unsigned, unsigned*) = (void*)GRUB_LOADING_ADDRESS;
	dprintf(INFO, "booting GRUB @ %p\n", entry);
	entry(0, board_machtype(), NULL);
	
	return 0;
}

