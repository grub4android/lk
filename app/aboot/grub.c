#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <mmc.h>
#include <partition_parser.h>
#include <malloc.h>
#include <lib/tar.h>
#include <platform.h>
#include <ext4.h>
#include <ext4_types.h>
#include <ext4_fs.h>
#include <ext4_mmcdev.h>
#include <kernel/thread.h>

#if WITH_APP_DISPLAY_SERVER
#include <app/display_server.h>
#endif

#include "grub.h"
#include "stat.h"
#include "uboot_api/uboot_part.h"
#include "bootimg.h"

struct tar_io_priv {
	unsigned long long index;
	unsigned long long ptn;
	int is_ramdisk;
};

unsigned long grub_tar_read(struct tar_io *tio, ulong start, ulong blkcnt, void *buffer) {
	struct tar_io_priv *priv = (struct tar_io_priv*) tio->priv;
	unsigned long long ptn = ((unsigned long long) start)*BLOCK_SIZE;

	if(priv->is_ramdisk) {
		memcpy(buffer, (void*)(unsigned long)(priv->ptn+ptn), blkcnt*BLOCK_SIZE);
	}
	else {
		if(mmc_read(priv->ptn + ptn, buffer, blkcnt*BLOCK_SIZE))
			return 0;
	}
	return blkcnt*BLOCK_SIZE;
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

static char* grub_bootpath = NULL;
void grub_get_bootpath(char **value) {
	*value = grub_bootpath;
}

static int grub_found_tar = 0;
int grub_has_tar(void) {
	return grub_found_tar;
}

#ifdef GRUB_BOOT_PARTITION
#define GRUB_MOUNTPOINT "/"GRUB_BOOT_PARTITION"/"
#define GRUB_PATH GRUB_BOOT_PATH_PREFIX"boot/grub"
#define HAS_FLAG(m, mask)	(((m) & mask) == mask)
#define ALLOW_BOOT(m) ( \
	S_ISREG((m)) \
	&& !HAS_FLAG((m), S_IXUSR) \
\
	&& !HAS_FLAG((m), S_IXGRP) \
	&& !HAS_FLAG((m), S_IWGRP) \
\
	&& !HAS_FLAG((m), S_IXOTH) \
	&& !HAS_FLAG((m), S_IWOTH) \
)
static int grub_mmc_check_permissions(const char *path, int *allow) {
	int ret;
	ext4_file f;
	struct ext4_inode_ref ref;

	// open file
	ret = ext4_fopen(&f, path, "rb");
	if(ret != EOK){
		dprintf(CRITICAL, "ext4_fopen ERROR = %d\n", ret);
		return -1;
	}

	// get inode ref
	ret = ext4_fs_get_inode_ref(&f.mp->fs, f.inode, &ref);
	if(ret != EOK){
		dprintf(CRITICAL, "ext4_fs_get_inode_ref ERROR = %d\n", ret);
		goto err_close_file;
	}

	// check permissions
	if(ref.inode->uid==0 && ref.inode->gid==0 && ALLOW_BOOT(ref.inode->mode))
		*allow = 1;
	else
		*allow = 0;

	if(*allow==0) {
		ref.inode->mode |= S_IXUGO;
		dprintf(CRITICAL, "file %s has wrong permissions!\n", path);
	}

	ext4_fclose(&f);
	return 0;

err_close_file:
	ext4_fclose(&f);
	return -1;
}
static int grub_load_from_mmc(void) {
	ext4_file f;
	uint32_t bytes_read;
	int ret = 0, allow = 0;

	dprintf(INFO, "%s: part=[%s] path=[%s]\n", __func__, GRUB_BOOT_PARTITION, GRUB_PATH);

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

	// check permissions of crucial files
	if(grub_mmc_check_permissions(GRUB_MOUNTPOINT GRUB_PATH "/core.img", &allow)==EOK && !allow)
		return -1;
	if(grub_mmc_check_permissions(GRUB_MOUNTPOINT GRUB_PATH "/grub.cfg", &allow)==EOK && !allow)
		return -1;
	if(grub_mmc_check_permissions(GRUB_MOUNTPOINT GRUB_PATH "/grubenv", &allow)==EOK && !allow)
		return -1;
	dprintf(INFO, "Permissions are good\n");

	// open file
	ret = ext4_fopen(&f, GRUB_MOUNTPOINT GRUB_PATH "/core.img", "rb");
	if(ret != EOK){
		dprintf(CRITICAL, "ext4_fopen ERROR = %d\n", ret);
		return -1;
	}

	// read file
	ret = ext4_fread(&f, (void*)GRUB_LOADING_ADDRESS_VIRT, ext4_fsize(&f), &bytes_read);
	if(ret != EOK){
		dprintf(CRITICAL, "ext4_fread ERROR = %d\n", ret);
		return -1;
	}

	// close file
	ret = ext4_fclose(&f);
	if(ret != EOK){
		dprintf(CRITICAL, "ext4_fclose ERROR = %d\n", ret);
		return -1;
	}

	// unmount partition
	ret = ext4_umount(GRUB_MOUNTPOINT);
	if(ret != EOK){
		dprintf(CRITICAL, "ext4_umount ERROR = %d\n", ret);
		return -1;
	}

	// store bootdev
	unsigned int index = (unsigned int) partition_get_index(GRUB_BOOT_PARTITION);
	char buf[20];
	sprintf(buf, "hd0,%u", index+1);
	grub_bootdev = strdup(buf);
	grub_bootpath = strdup(GRUB_BOOT_PATH_PREFIX "boot/grub");

	dprintf(INFO, "Loaded GRUB from MMC\n");
	return 0;
}
#endif

static int grub_load_from_tar(void) {
	// prepare block api
	priv.index = partition_get_index("aboot");
	priv.ptn = partition_get_offset(priv.index) + 1024*1024; // 1MB offset to aboot
	priv.is_ramdisk = 0;
	tio.blksz = BLOCK_SIZE;
	tio.lba = partition_get_size(priv.index) / tio.blksz - 1;

	// search file
	if(tar_get_fileinfo(&tio, "./boot/grub/core.img", &fi)) {
		dprintf(CRITICAL, "%s: couldn't find core.img!\n", __func__);
		return -1;
	}

	// load file into RAM
	if(tar_read_file(&tio, &fi, (void*)GRUB_LOADING_ADDRESS_VIRT)) {
		dprintf(CRITICAL, "%s: couldn't read core.img!\n", __func__);
		return -1;
	}

	grub_bootdev = strdup("hd1");
	grub_bootpath = strdup("/boot/grub");
	grub_found_tar = 1;

	dprintf(INFO, "Loaded GRUB from TAR\n");
	return 0;
}

static int grub_sideload_handler(void *data)
{
	struct boot_img_hdr *hdr =  (struct boot_img_hdr *)data;
	unsigned kernel_size = ALIGN(hdr->kernel_size, hdr->page_size);
	void* local_kernel_addr = data + hdr->page_size;
	void* local_ramdisk_addr = local_kernel_addr + kernel_size;

	// Load ramdisk & kernel
	memmove((void*) hdr->kernel_addr, local_kernel_addr, hdr->kernel_size);
	memmove((void*) hdr->ramdisk_addr, local_ramdisk_addr, hdr->ramdisk_size);

	// use ramdisk
	if(hdr->ramdisk_size>0) {
		// prepare block api
		priv.index = 0;
		priv.ptn = (unsigned) hdr->ramdisk_addr;
		priv.is_ramdisk = 1;
		tio.blksz = BLOCK_SIZE;
		tio.lba = hdr->ramdisk_size;
		// set bootdev
		grub_bootdev = strdup("hd1");
		grub_bootpath = strdup("/boot/grub");
		grub_found_tar = 1;
	}
	else {
		// set bootdev
		unsigned int index = (unsigned int) partition_get_index(GRUB_BOOT_PARTITION);
		char buf[20];
		sprintf(buf, "hd0,%u", index+1);
		grub_bootdev = strdup(buf);
		grub_bootpath = strdup(GRUB_BOOT_PATH_PREFIX "boot/grub");
	}
	dprintf(INFO, "bootdev: %s\n", grub_bootdev);
	dprintf(INFO, "bootpath: %s\n", grub_bootpath);

	// BOOT !
	void (*entry)(unsigned, unsigned, unsigned*) = (void*)hdr->kernel_addr;
	dprintf(INFO, "booting GRUB from sideload @ %p ramdisk @ %p\n", entry, (void*)hdr->ramdisk_addr);

#if WITH_APP_DISPLAY_SERVER
	display_server_stop();
#endif

	entry(0, board_machtype(), NULL);

	return 0;
}

int grub_load_from_sideload(void* data) {
	thread_t *thr;

	thr = thread_create("grub_sideload", grub_sideload_handler, data, DEFAULT_PRIORITY, 4096);
	if (!thr)
	{
		return -1;
	}
	thread_resume(thr);
	return 0;
}

int grub_boot(void)
{
	void (*entry)(unsigned, unsigned, unsigned*) = (void*)GRUB_LOADING_ADDRESS_VIRT;

	// load grub into RAM
#ifdef GRUB_BOOT_PARTITION
	if(grub_load_from_mmc()) {
		dprintf(CRITICAL, "%s: failed to load grub from mmc.\n", __func__);
	}
	else goto boot;
#endif

#if !BOOT_2NDSTAGE
	if(grub_load_from_tar()) {
		dprintf(CRITICAL, "%s: failed to load grub from tar.\n", __func__);
	}
	else goto boot;
#endif

	dprintf(CRITICAL, "%s: Couldn't find grub at any known location!\n", __func__);
	return -1;

boot:
	// BOOT !
	dprintf(INFO, "booting GRUB @ %p\n", entry);

#if WITH_APP_DISPLAY_SERVER
	display_server_stop();
#endif

	entry(0, board_machtype(), NULL);
	
	return 0;
}

