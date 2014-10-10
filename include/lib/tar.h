#ifndef __LIB_TAR_H
#define __LIB_TAR_H

#define TMAGIC   "ustar"        /* ustar and a null */
#define TMAGLEN  6
//#define ALIGN(x, y)	(x + ((y - ((x) & (y-1))) & (y-1)))

struct posix_header
{                              /* byte offset */
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */
  char mtime[12];               /* 136 */
  char chksum[8];               /* 148 */
  char typeflag;                /* 156 */
  char linkname[100];           /* 157 */
  char magic[6];                /* 257 */
  char version[2];              /* 263 */
  char uname[32];               /* 265 */
  char gname[32];               /* 297 */
  char devmajor[8];             /* 329 */
  char devminor[8];             /* 337 */
  char prefix[155];             /* 345 */
                                /* 500 */
} __attribute__((__packed__)) ;

struct tar_fileinfo
{
	struct posix_header hd;
	ulong blkid;
	uint64_t size;
};

struct tar_io {
	ulong	lba;		/* number of blocks */
	ulong	blksz;		/* block size */
	ulong	(*block_read)(struct tar_io *tio,
				      ulong start,
				      ulong blkcnt,
				      void *buffer);
	void		*priv;		/* driver private struct pointer */
};

int tar_get_fileinfo(struct tar_io *tio, const char *path, struct tar_fileinfo *fi);
int tar_read_file(struct tar_io *tio, struct tar_fileinfo *fi, void* buf);

#endif
