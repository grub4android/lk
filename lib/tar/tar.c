#include <stdlib.h>
#include <string.h>
#include <debug.h>
#include <lib/tar.h>

static uint64_t str2number(const char *str, uint64_t size) {
  uint64_t ret = 0;

  while (size-- && *str >= '0' && *str <= '7')
    ret = (ret << 3) | (*str++ & 0xf);

  return ret;
}

int tar_get_fileinfo(struct tar_io *tio, const char *path, struct tar_fileinfo *fi) {
	int ret, blkid = 0;
	char blkbuf[tio->blksz];
	struct posix_header *hd = (void*)blkbuf;

	if(tio->blksz!=512) {
		dprintf(CRITICAL, "%s: Unsupported block size '%lu'!\n", __func__, tio->blksz);
		return -1;
	}

	while(1) {
		ret = tio->block_read(tio, blkid, 1, blkbuf);
		if(ret) {
			dprintf(CRITICAL, "%s: Error reading from device!\n", __func__);
			return -1;
		}

		// not a valid archive or end of archive reached
		if(memcmp(hd->magic, TMAGIC, TMAGLEN-1)!=0)
			break;

		uint64_t size = str2number(hd->size, sizeof(hd->size));

		// found :)
		if(strcmp(hd->name, path)==0) {
			fi->hd = *hd;
			fi->blkid = blkid+1;
			fi->size = size;
			return 0;
		}

		// skip header
		blkid++;

		// skip content
		blkid+=ALIGN(size, 512)/512;
	}

	return -1;
}

int tar_read_file(struct tar_io *tio, struct tar_fileinfo *fi, void* buf) {
	if(tio->blksz!=512) {
		dprintf(CRITICAL, "%s: Unsupported block size '%lu'!\n", __func__, tio->blksz);
		return -1;
	}

	return tio->block_read(tio, fi->blkid, ALIGN(fi->size, tio->blksz)/tio->blksz, buf);
}
