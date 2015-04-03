#ifndef _LIB_SPARSE_H_
#define _LIB_SPARSE_H_

#include <sys/types.h>
#include <stdbool.h>
#include <lib/bio.h>

bool sparse_validate(void*, uint32_t);
uint sparse_write_to_device(bdev_t* dev, void *data, uint32_t sz);

#endif /* ! LIB_SPARSE */
