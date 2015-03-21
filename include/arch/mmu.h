/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#pragma once

#include <arch.h>
#include <sys/types.h>
#include <compiler.h>

__BEGIN_CDECLS

#define ARCH_MMU_FLAG_CACHED            (0<<0)
#define ARCH_MMU_FLAG_UNCACHED          (1<<0)
#define ARCH_MMU_FLAG_UNCACHED_DEVICE   (2<<0) /* only exists on some arches, otherwise UNCACHED */
#define ARCH_MMU_FLAG_CACHE_MASK        (3<<0)

#define ARCH_MMU_FLAG_PERM_USER         (1<<2)
#define ARCH_MMU_FLAG_PERM_RO           (1<<3)
#define ARCH_MMU_FLAG_PERM_NO_EXECUTE   (1<<4)
#define ARCH_MMU_FLAG_NS                (1<<5) /* NON-SECURE */

int arch_mmu_map(vaddr_t vaddr, paddr_t paddr, uint count, uint flags);
int arch_mmu_unmap(vaddr_t vaddr, uint count);
status_t arch_mmu_query(vaddr_t vaddr, paddr_t *paddr, uint *flags);
status_t arch_mmu_query_reverse(paddr_t paddr, vaddr_t *vaddr, uint *flags);

void arch_disable_mmu(void);

__END_CDECLS

