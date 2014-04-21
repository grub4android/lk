#ifndef GZIP_H
#define GZIP_H

int gzip(void *dst, unsigned long *lenp,
		unsigned char *src, unsigned long srclen);

int gunzip(void *dst, int dstlen, unsigned char *src, unsigned long *lenp);

#endif /* GZIP_H */
