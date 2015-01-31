/*
 * Copyright (c) 2008 Travis Geiselbrecht
 * Copyright (c) 2012, NVIDIA CORPORATION. All rights reserved
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
#ifndef __ARCH_ARM_MMU_H
#define __ARCH_ARM_MMU_H

#if defined(ARM_ISA_ARMV6) | defined(ARM_ISA_ARMV7)

#define MMU_MEMORY_L1_DESCRIPTOR_INVALID		(0x0 << 0)
#define MMU_MEMORY_L1_DESCRIPTOR_PAGE_TABLE		(0x1 << 0)
#define MMU_MEMORY_L1_DESCRIPTOR_SECTION		(0x2 << 0)
#define MMU_MEMORY_L1_DESCRIPTOR_SUPERSECTION		((0x2 << 0) | (0x1 << 18))

/* C, B and TEX[2:0] encodings without TEX remap (for first level descriptors) */
                                                          /* TEX      |    CB    */
#define MMU_MEMORY_L1_TYPE_STRONGLY_ORDERED              ((0x0 << 12) | (0x0 << 2))
#define MMU_MEMORY_L1_TYPE_DEVICE_SHARED                 ((0x0 << 12) | (0x1 << 2))
#define MMU_MEMORY_L1_TYPE_DEVICE_NON_SHARED             ((0x2 << 12) | (0x0 << 2))
#define MMU_MEMORY_L1_TYPE_NORMAL                        ((0x1 << 12) | (0x0 << 2))
#define MMU_MEMORY_L1_TYPE_NORMAL_WRITE_THROUGH          ((0x0 << 12) | (0x2 << 2))
#define MMU_MEMORY_L1_TYPE_NORMAL_WRITE_BACK_NO_ALLOCATE ((0x0 << 12) | (0x3 << 2))
#define MMU_MEMORY_L1_TYPE_NORMAL_WRITE_BACK_ALLOCATE    ((0x1 << 12) | (0x3 << 2))

/* C, B and TEX[2:0] encodings without TEX remap (for second level descriptors) */
                                                          /* TEX     |    CB    */
#define MMU_MEMORY_L2_TYPE_STRONGLY_ORDERED              ((0x0 << 6) | (0x0 << 2))
#define MMU_MEMORY_L2_TYPE_DEVICE_SHARED                 ((0x0 << 6) | (0x1 << 2))
#define MMU_MEMORY_L2_TYPE_DEVICE_NON_SHARED             ((0x2 << 6) | (0x0 << 2))
#define MMU_MEMORY_L2_TYPE_NORMAL                        ((0x1 << 6) | (0x0 << 2))
#define MMU_MEMORY_L2_TYPE_NORMAL_WRITE_THROUGH          ((0x0 << 6) | (0x2 << 2))
#define MMU_MEMORY_L2_TYPE_NORMAL_WRITE_BACK_NO_ALLOCATE ((0x0 << 6) | (0x3 << 2))
#define MMU_MEMORY_L2_TYPE_NORMAL_WRITE_BACK_ALLOCATE    ((0x1 << 6) | (0x3 << 2))

#define MMU_MEMORY_DOMAIN_MEM    	(0)

/*
 * AP (Access Permissions)
 * +-------------------------+
 * | AP        P         U   |
 * +-------------------------+
 * |                         |
 * | 00       NA        NA   |
 * |                         |
 * | 01       RW        NA   |
 * |                         |
 * | 10       RW        R    |
 * |                         |
 * | 11       RW        RW   |
 * |                         |
 * +-------------------------+
 *
 * NA = No Access
 * RW = Read/Write
 * R  = Read only
 *
 * P = Privileged modes
 * U = ~P
 *
 */
#define MMU_MEMORY_L1_AP_P_NA_U_NA	((0x0 << 15) | (0x0 << 10))
#define MMU_MEMORY_L1_AP_P_RW_U_RO	((0x0 << 15) | (0x2 << 10))
#define MMU_MEMORY_L1_AP_P_RW_U_RW	((0x0 << 15) | (0x3 << 10))
#define MMU_MEMORY_L1_AP_P_RW_U_NA	((0x0 << 15) | (0x1 << 10))

#define MMU_MEMORY_L2_AP_P_NA_U_NA	((0x0 << 9) | (0x0 << 4))
#define MMU_MEMORY_L2_AP_P_RW_U_RO	((0x0 << 9) | (0x2 << 4))
#define MMU_MEMORY_L2_AP_P_RW_U_RW	((0x0 << 9) | (0x3 << 4))
#define MMU_MEMORY_L2_AP_P_RW_U_NA	((0x0 << 9) | (0x1 << 4))

#define MMU_MEMORY_L1_PAGETABLE_NON_SECURE	(1 << 3)

#define MMU_MEMORY_L1_SECTION_NON_SECURE	(1 << 19)
#define MMU_MEMORY_L1_SECTION_SHAREABLE		(1 << 16)
#define MMU_MEMORY_L1_SECTION_NON_GLOBAL	(1 << 17)

#define MMU_MEMORY_L2_SHAREABLE		(1 << 10)
#define MMU_MEMORY_L2_NON_GLOBAL	(1 << 11)

#define MMU_MEMORY_L2_CB_SHIFT		2
#define MMU_MEMORY_L2_TEX_SHIFT		6

#define MMU_MEMORY_NON_CACHEABLE		0
#define MMU_MEMORY_WRITE_BACK_ALLOCATE		1
#define MMU_MEMORY_WRITE_THROUGH_NO_ALLOCATE	2
#define MMU_MEMORY_WRITE_BACK_NO_ALLOCATE	3

#define MMU_MEMORY_SET_L2_INNER(val)	    (((val) & 0x3) << MMU_MEMORY_L2_CB_SHIFT)
#define MMU_MEMORY_SET_L2_OUTER(val)	    (((val) & 0x3) << MMU_MEMORY_L2_TEX_SHIFT)
#define MMU_MEMORY_SET_L2_CACHEABLE_MEM	    (0x4 << MMU_MEMORY_L2_TEX_SHIFT)

#define MMU_MEMORY_TTBR_RGN(x)		(((x) & 0x3) << 3)
/* IRGN[1:0] is encoded as: IRGN[0] in TTBRx[6], and IRGN[1] in TTBRx[0] */
#define MMU_MEMORY_TTBR_IRGN(x)		((((x) & 0x1) << 6) | \
					 ((((x) >> 1) & 0x1) << 0))

#if WITH_MMU_RELOC
/* Default configuration for main kernel page table:
 *    - section mappings for memory
 *    - do cached translation walks
 */

/* Enable cached page table walks:
 * inner/outer (IRGN/RGN): write-back + write-allocate
 */
#define MMU_TTBRx_FLAGS		\
	(MMU_MEMORY_TTBR_RGN(MMU_MEMORY_WRITE_BACK_ALLOCATE) |\
	 MMU_MEMORY_TTBR_IRGN(MMU_MEMORY_WRITE_BACK_ALLOCATE))

/* Section mapping, TEX[2:0]=001, CB=11, S=1, AP[2:0]=001 */
#define MMU_KERNEL_L1_PTE_FLAGS		\
	(MMU_MEMORY_L1_DESCRIPTOR_SECTION | \
	 MMU_MEMORY_L1_SECTION_SHAREABLE | \
	 MMU_MEMORY_L1_TYPE_NORMAL_WRITE_BACK_ALLOCATE | \
	 MMU_MEMORY_L1_AP_P_RW_U_NA)
#endif

/* C, B and TEX[2:0] encodings without TEX remap */
                                                       /* TEX      |    CB    */
#define MMU_MEMORY_TYPE_STRONGLY_ORDERED              ((0x0 << 12) | (0x0 << 2))
#define MMU_MEMORY_TYPE_DEVICE_SHARED                 ((0x0 << 12) | (0x1 << 2))
#define MMU_MEMORY_TYPE_DEVICE_NON_SHARED             ((0x2 << 12) | (0x0 << 2))
#define MMU_MEMORY_TYPE_NORMAL                        ((0x1 << 12) | (0x0 << 2))
#define MMU_MEMORY_TYPE_NORMAL_WRITE_THROUGH          ((0x0 << 12) | (0x2 << 2))
#define MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_NO_ALLOCATE ((0x0 << 12) | (0x3 << 2))
#define MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE    ((0x1 << 12) | (0x3 << 2))

#define MMU_MEMORY_AP_NO_ACCESS     (0x0 << 10)
#define MMU_MEMORY_AP_READ_ONLY     (0x7 << 10)
#define MMU_MEMORY_AP_READ_WRITE    (0x3 << 10)

#define MMU_MEMORY_XN               (0x1 << 4)

#else

#define MMU_FLAG_CACHED			0x1
#define MMU_FLAG_BUFFERED		0x2
#define MMU_FLAG_READWRITE		0x4

#define MMU_MEMORY_DOMAIN_MEM    	(0)
#endif

#ifndef ASSEMBLY

#include <sys/types.h>
#include <assert.h>
#include <compiler.h>

__BEGIN_CDECLS

void arm_mmu_init(void);

void arm_mmu_map_section(addr_t paddr, addr_t vaddr, uint flags);
void arm_mmu_unmap_section(addr_t vaddr);
void platform_mmu_map_area(addr_t paddress, addr_t vaddress, uint32_t num_of_sections, uint32_t flags);
void arm_mmu_map_smem_table(void);

#if WITH_MMU_RELOC
static inline void validate_kvaddr(void *ptr)
{
	extern void _start(void);
	extern unsigned int _heap_end;
	ASSERT((void *)_start <= ptr && ptr <= (void *)(_heap_end - 1));
}

extern uintptr_t phys_offset;
static inline paddr_t kvaddr_to_paddr(void *ptr)
{
	validate_kvaddr(ptr);
	return (paddr_t)ptr + phys_offset;
}

static inline void *paddr_to_kvaddr(paddr_t paddr)
{
	void *ptr = (void *)(paddr - phys_offset);
	validate_kvaddr(ptr);
	return ptr;
}
#else
static inline __ALWAYS_INLINE paddr_t kvaddr_to_paddr(void *ptr)
{
	return (paddr_t)ptr;
}

static inline __ALWAYS_INLINE void *paddr_to_kvaddr(paddr_t paddr)
{
	return (void *)paddr;
}
#endif

__END_CDECLS

#endif /* ASSEMBLY */

#endif
