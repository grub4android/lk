/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2009-2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of The Linux Foundation nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <malloc.h>
#include <lib/bio.h>
#include <lib/sparse.h>
#include <arch/defines.h>
#include <app/fastboot.h>
#include "sparse_format.h"

bool sparse_validate(void *data, uint32_t sz)
{
	sparse_header_t *hdr = data;
	return (sz >= sizeof(*hdr) && hdr->magic == SPARSE_HEADER_MAGIC);
}

uint sparse_write_to_device(bdev_t * dev, void *data, uint32_t sz)
{
	uint32_t chunk;
	uint32_t chunk_data_sz;
	uint32_t *fill_buf = NULL;
	uint32_t fill_val;
	uint32_t chunk_blk_cnt = 0;
	sparse_header_t *sparse_header;
	chunk_header_t *chunk_header;
	uint32_t total_blocks = 0;
	uint32_t i;

	/* Read and skip over sparse image header */
	sparse_header = (sparse_header_t *) data;
	if ((sparse_header->total_blks * sparse_header->blk_sz) > dev->size) {
		fastboot_fail("size too large");
		return -1;
	}

	data += sparse_header->file_hdr_sz;
	if (sparse_header->file_hdr_sz > sizeof(sparse_header_t)) {
		/* Skip the remaining bytes in a header that is longer than
		 * we expected.
		 */
		data += (sparse_header->file_hdr_sz - sizeof(sparse_header_t));
	}

	dprintf(SPEW, "=== Sparse Image Header ===\n");
	dprintf(SPEW, "magic: 0x%x\n", sparse_header->magic);
	dprintf(SPEW, "major_version: 0x%x\n", sparse_header->major_version);
	dprintf(SPEW, "minor_version: 0x%x\n", sparse_header->minor_version);
	dprintf(SPEW, "file_hdr_sz: %d\n", sparse_header->file_hdr_sz);
	dprintf(SPEW, "chunk_hdr_sz: %d\n", sparse_header->chunk_hdr_sz);
	dprintf(SPEW, "blk_sz: %d\n", sparse_header->blk_sz);
	dprintf(SPEW, "total_blks: %d\n", sparse_header->total_blks);
	dprintf(SPEW, "total_chunks: %d\n", sparse_header->total_chunks);

	/* Start processing chunks */
	for (chunk = 0; chunk < sparse_header->total_chunks; chunk++) {
		/* Read and skip over chunk header */
		chunk_header = (chunk_header_t *) data;
		data += sizeof(chunk_header_t);

		dprintf(SPEW, "=== Chunk Header ===\n");
		dprintf(SPEW, "chunk_type: 0x%x\n", chunk_header->chunk_type);
		dprintf(SPEW, "chunk_data_sz: 0x%x\n", chunk_header->chunk_sz);
		dprintf(SPEW, "total_size: 0x%x\n", chunk_header->total_sz);

		if (sparse_header->chunk_hdr_sz > sizeof(chunk_header_t)) {
			/* Skip the remaining bytes in a header that is longer than
			 * we expected.
			 */
			data +=
			    (sparse_header->chunk_hdr_sz -
			     sizeof(chunk_header_t));
		}

		chunk_data_sz = sparse_header->blk_sz * chunk_header->chunk_sz;
		switch (chunk_header->chunk_type) {
		case CHUNK_TYPE_RAW:
			if (chunk_header->total_sz !=
			    (sparse_header->chunk_hdr_sz + chunk_data_sz)) {
				fastboot_fail
				    ("Bogus chunk size for chunk type Raw");
				return -1;
			}

			if (bio_write
			    (dev, data,
			     ((uint64_t) total_blocks * sparse_header->blk_sz),
			     chunk_data_sz)) {
				fastboot_fail("flash write failure");
				return -1;
			}
			total_blocks += chunk_header->chunk_sz;
			data += chunk_data_sz;
			break;

		case CHUNK_TYPE_FILL:
			if (chunk_header->total_sz !=
			    (sparse_header->chunk_hdr_sz + sizeof(uint32_t))) {
				fastboot_fail
				    ("Bogus chunk size for chunk type FILL");
				return -1;
			}

			fill_buf =
			    (uint32_t *) memalign(CACHE_LINE,
						  ALIGN(sparse_header->blk_sz,
							CACHE_LINE));
			if (!fill_buf) {
				fastboot_fail
				    ("Malloc failed for: CHUNK_TYPE_FILL");
				return -1;
			}

			fill_val = *(uint32_t *) data;
			data = (char *)data + sizeof(uint32_t);
			chunk_blk_cnt = chunk_data_sz / sparse_header->blk_sz;

			for (i = 0;
			     i < (sparse_header->blk_sz / sizeof(fill_val));
			     i++) {
				fill_buf[i] = fill_val;
			}

			for (i = 0; i < chunk_blk_cnt; i++) {
				if (bio_write
				    (dev, fill_buf,
				     ((uint64_t) total_blocks *
				      sparse_header->blk_sz),
				     sparse_header->blk_sz)) {
					fastboot_fail("flash write failure");
					free(fill_buf);
					return -1;
				}

				total_blocks++;
			}

			free(fill_buf);
			break;

		case CHUNK_TYPE_DONT_CARE:
			total_blocks += chunk_header->chunk_sz;
			break;

		case CHUNK_TYPE_CRC32:
			if (chunk_header->total_sz !=
			    sparse_header->chunk_hdr_sz) {
				fastboot_fail
				    ("Bogus chunk size for chunk type Dont Care");
				return -1;
			}
			total_blocks += chunk_header->chunk_sz;
			data += chunk_data_sz;
			break;

		default:
			dprintf(CRITICAL, "Unkown chunk type: %x\n",
				chunk_header->chunk_type);
			fastboot_fail("Unknown chunk type");
			return -1;
		}
	}

	dprintf(INFO, "Wrote %d blocks, expected to write %d blocks\n",
		total_blocks, sparse_header->total_blks);

	if (total_blocks != sparse_header->total_blks) {
		fastboot_fail("sparse image write failure");
		return -1;
	}

	fastboot_okay("");
	return 0;
}
