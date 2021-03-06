/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <sys/types.h>
#include <stdio.h>
#include <rand.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <app/tests.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <kernel/event.h>
#include <platform.h>

#if ARCH_ARM
const uint BUFSIZE = (1024*1024);
const uint ITER = 1024;

void bench_set_overhead(void)
{
	uint32_t *buf = malloc(BUFSIZE);

	uint count = arch_cycle_count();
	for (uint i = 0; i < ITER; i++) {
//		for (uint j = 0; j < BUFSIZE / sizeof(*buf); j++) {
			__asm__ volatile(
				"nop"
			);
//		}
	}
	count = arch_cycle_count() - count;

	printf("took %u cycles overhead to loop %u times\n",
	       count, ITER);

	free(buf);
}

void bench_memset(void)
{
	void *buf = malloc(BUFSIZE);

	uint count = arch_cycle_count();
	for (uint i = 0; i < ITER; i++) {
		memset(buf, 0, BUFSIZE);
	}
	count = arch_cycle_count() - count;

	printf("took %u cycles to memset a buffer of size %u %d times (%u bytes)\n",
	       count, BUFSIZE, ITER, BUFSIZE * ITER);

	free(buf);
}

#define bench_cset(type) \
void bench_cset_##type(void) \
{ \
	type *buf = malloc(BUFSIZE); \
 \
	uint count = arch_cycle_count(); \
	for (uint i = 0; i < ITER; i++) { \
		for (uint j = 0; j < BUFSIZE / sizeof(*buf); j++) { \
			buf[j] = 0; \
		} \
	} \
	count = arch_cycle_count() - count; \
 \
	printf("took %u cycles to manually clear a buffer using wordsize %d of size %u %d times (%u bytes)\n", \
	       count, sizeof(*buf), BUFSIZE, ITER, BUFSIZE * ITER); \
 \
	free(buf); \
}

bench_cset(uint8_t)
bench_cset(uint16_t)
bench_cset(uint32_t)
bench_cset(uint64_t)

void bench_cset_wide(void)
{
	uint32_t *buf = malloc(BUFSIZE);

	uint count = arch_cycle_count();
	for (uint i = 0; i < ITER; i++) {
		for (uint j = 0; j < BUFSIZE / sizeof(*buf) / 8; j++) {
			buf[j*8] = 0;
			buf[j*8+1] = 0;
			buf[j*8+2] = 0;
			buf[j*8+3] = 0;
			buf[j*8+4] = 0;
			buf[j*8+5] = 0;
			buf[j*8+6] = 0;
			buf[j*8+7] = 0;
		}
	}
	count = arch_cycle_count() - count;

	printf("took %u cycles to manually clear a buffer of size %u %d times 8 words at a time (%u bytes)\n",
	       count, BUFSIZE, ITER, BUFSIZE * ITER);

	free(buf);
}

void bench_cset_stm(void)
{
	uint32_t *buf = malloc(BUFSIZE);

	uint count = arch_cycle_count();
	for (uint i = 0; i < ITER; i++) {
		for (uint j = 0; j < BUFSIZE / sizeof(*buf) / 8; j++) {
			__asm__ volatile(
				"stm	%0, {r0-r7};"
				:: "r" (&buf[j*8])
			);
		}
	}
	count = arch_cycle_count() - count;

	printf("took %u cycles to manually clear a buffer of size %u %d times 8 words at a time using stm (%u bytes)\n",
	       count, BUFSIZE, ITER, BUFSIZE * ITER);

	free(buf);
}


void bench_memcpy(void)
{
	uint8_t *buf = malloc(BUFSIZE);

	uint count = arch_cycle_count();
	for (uint i = 0; i < ITER; i++) {
		memcpy(buf, buf + BUFSIZE / 2, BUFSIZE / 2);
	}
	count = arch_cycle_count() - count;

	printf("took %u cycles to memcpy a buffer of size %u %d times (%u bytes)\n",
	       count, BUFSIZE / 2, ITER, BUFSIZE * ITER);

	free(buf);
}
#endif

#if WITH_LIB_LIBM
#include <math.h>

__NO_INLINE static void bench_sincos(void)
{
    printf("touching the floating point unit\n");
    __UNUSED volatile double _hole = sin(0);

    uint count = arch_cycle_count();
    __UNUSED double a = sin(2.0);
    count = arch_cycle_count() - count;
    printf("took %u cycles for sin()\n", count);

    count = arch_cycle_count();
    a = cos(2.0);
    count = arch_cycle_count() - count;
    printf("took %u cycles for cos()\n", count);

    count = arch_cycle_count();
    a = sinf(2.0);
    count = arch_cycle_count() - count;
    printf("took %u cycles for sinf()\n", count);

    count = arch_cycle_count();
    a = cosf(2.0);
    count = arch_cycle_count() - count;
    printf("took %u cycles for cosf()\n", count);

    count = arch_cycle_count();
    a = sqrt(1234567.0);
    count = arch_cycle_count() - count;
    printf("took %u cycles for sqrt()\n", count);

    count = arch_cycle_count();
    a = sqrtf(1234567.0f);
    count = arch_cycle_count() - count;
    printf("took %u cycles for sqrtf()\n", count);
}

#endif

void benchmarks(void)
{
#if ARCH_ARM
	bench_set_overhead();
	bench_memset();
	bench_cset_uint8_t();
	bench_cset_uint16_t();
	bench_cset_uint32_t();
	bench_cset_uint64_t();
	bench_cset_wide();
	bench_cset_stm();
	bench_memcpy();
#endif
#if WITH_LIB_LIBM
    bench_sincos();
#endif
}

