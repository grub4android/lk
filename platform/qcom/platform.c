/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
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

#include <printf.h>
#include <string.h>
#include <compiler.h>
#include <kernel/vm.h>
#include <arch/arm/mmu.h>
#include <platform/qcom.h>
#include <platform/iomap.h>
#include <platform/smem.h>

#ifdef QCOM_ENABLE_SDHCI
#include <platform/sdhci_msm.h>
#endif

#if WITH_LIB_BIO
#include <lib/bio.h>
#endif

#ifdef QCOM_ENABLE_DISPLAY
#include <platform/msm_panel.h>
#endif

static char bssbuf[(sizeof(pmm_arena_t)+15)*30];
static uintptr_t bssbuf_end = (uintptr_t)bssbuf;

static void* bss_alloc(size_t len) {
    uintptr_t ptr = ALIGN(bssbuf_end, 8);

	if(ptr+len>(uintptr_t)bssbuf+ARRAY_SIZE(bssbuf))
		return NULL;

    bssbuf_end = (ptr + ALIGN(len, 8));

	memset((void*)ptr, 0, len);
	return (void*)ptr;
}

static pmm_arena_t arena_lk = {
	.name = "kernel",
	.base = MEMBASE + KERNEL_LOAD_OFFSET,
	.size = MEMSIZE,
	.flags = PMM_ARENA_FLAG_KMAP,
	.priority = 1
};

static void* mmap_callback(void* pdata, paddr_t addr, size_t size, bool reserved) {
	uint32_t* sdcount = pdata;
	uint32_t lk_end = arena_lk.base+arena_lk.size;
	uint flags = reserved?PMM_ARENA_FLAG_RESERVED:PMM_ARENA_FLAG_SDRAM;

	// allocate
	pmm_arena_t* arena = bss_alloc(sizeof(pmm_arena_t));
	ASSERT(arena);

	// name
	if(reserved)
		arena->name = "reserved";
	else {
		arena->name = bss_alloc(10);
		snprintf((char*)arena->name, 10, "sdram%u", (*sdcount)++);
	}

	// configure
	arena->base = addr;
	arena->size = size;
	arena->flags = flags;
	uint32_t arena_end = arena->base+arena->size;

	// overlap check
	if(MAX(arena_lk.base,arena->base) <= MIN(lk_end,arena_end)) {
		// the LK arena contains this arena
		if(arena_lk.base<=arena->base && lk_end>=arena_end)
			return pdata;

		// start
		if(arena->base<arena_lk.base) {
			// allocate
			pmm_arena_t* arena_pre = bss_alloc(sizeof(pmm_arena_t));
			ASSERT(arena_pre);

			// name
			if(reserved)
				arena->name = "reserved-pre";
			else {
				arena_pre->name = bss_alloc(15);
				snprintf((char*)arena_pre->name, 15, "sdram%u-pre", (*sdcount)-1);
			}

			// configure
			arena_pre->base = arena->base;
			arena_pre->size = arena_lk.base - arena->base;
			arena_pre->flags = flags;

			// add
			pmm_add_arena(arena_pre);
		}

		// end
		if(arena_end>lk_end) {
			// allocate
			pmm_arena_t* arena_post = bss_alloc(sizeof(pmm_arena_t));
			ASSERT(arena_post);

			// name
			if(reserved)
				arena->name = "reserved-post";
			else {
				arena_post->name = bss_alloc(15);
				snprintf((char*)arena_post->name, 15, "sdram%u-post", (*sdcount)-1);
			}

			// configure
			arena_post->base = lk_end;
			arena_post->size = arena_end - lk_end;
			arena_post->flags = flags;

			// add
			pmm_add_arena(arena_post);
		}

		return pdata;
	}

	// add
	pmm_add_arena(arena);
	return pdata;
}

void platform_qcom_init_pmm(void) {
	uint32_t sdcount = 0;

	// add kernel arena
	pmm_add_arena(&arena_lk);

	// add sdram arenas
	platform_get_mmap(&sdcount, mmap_callback);

	// reserve SMEM area
	struct list_node list;
	list_initialize(&list);
	pmm_alloc_range(kvaddr_to_paddr((void*)platform_get_smem_base_addr()), 2*MB/PAGE_SIZE, &list);

	// reserve linux memory
	pmm_alloc_map_addr(LINUX_BASE, LINUX_SIZE, ARCH_MMU_FLAG_UNCACHED_DEVICE);
	pmm_set_type_ptr((void*)LINUX_BASE, VM_PAGE_TYPE_LINUX);

#if defined(ENABLE_2NDSTAGE_BOOT) && defined(QCOM_ENABLE_DISPLAY)
	msm_display_2ndstagefb_reserve();
#endif
}

void platform_init_mmu_mappings(void)
{
}

__WEAK uint32_t platform_get_smem_base_addr(void)
{
	return MSM_SHARED_BASE;
}

__WEAK int boot_device_mask(int val)
{
	return ((val & 0x3E) >> 1);
}

__WEAK uint32_t platform_detect_panel(void)
{
	return 0;
}

__WEAK uint32_t platform_get_boot_dev(void)
{
	return 0;
}

__WEAK void shutdown_device(void)
{
}

__WEAK void reboot_device(unsigned reboot_reason)
{
}

/* Default target specific initialization before using USB */
__WEAK void target_usb_init(void)
{
}

/* Default target specific usb shutdown */
__WEAK void target_usb_stop(void)
{
}

__WEAK void target_fastboot_init(void)
{
}

__WEAK const char* target_serialno(void)
{
	const char* ret = NULL;
#if WITH_LIB_BIO
	bdev_t* dev = bio_open_first_dev();
	if(dev && dev->serial)
		ret = dev->serial;
	else
#endif
	ret = PROJECT;

	return ret;
}

/* default usb controller to be used. */
__WEAK const char* target_usb_controller(void)
{
	return "ci";
}

#ifdef QCOM_ENABLE_SDHCI
__WEAK void clock_config_cdc(uint32_t slot)
{

}

/* Default CFG delay value */
__WEAK uint32_t target_ddr_cfg_val(void)
{
	return DDR_CONFIG_VAL;
}
#endif

__WEAK void* platform_get_mmap(void* pdata, platform_mmap_cb_t cb) {
	uint32_t i;
	ram_partition part;

	// Make sure RAM partition table is initialized
	ASSERT(smem_ram_ptable_init_v1());
	for(i=0; i<smem_get_ram_ptable_len(); i++) {
		smem_get_ram_ptable_entry(&part, i);

		if(part.category==SDRAM && part.type==SYS_MEMORY) {
			/* Pass along all other usable memory regions to Linux */
			pdata = cb(pdata, (paddr_t) part.start, (size_t)part.size, false);
		}
	}

	return pdata;
}

__WEAK enum baseband target_baseband(void)
{
	return BASEBAND_UNKNOWN;
}
