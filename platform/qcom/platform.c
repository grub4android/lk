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

static pmm_arena_t arenas[4];

void platform_qcom_init_pmm(void) {
	struct smem_ram_ptable ram_ptable;
	uint32_t i, j = 0;

	/* Make sure RAM partition table is initialized */
	ASSERT(smem_ram_ptable_init(&ram_ptable));

	/* Calculating the size of the mem_info_ptr */
	for (i = 0 ; i < ram_ptable.len; i++) {
		if(ram_ptable.parts[i].category==SDRAM && ram_ptable.parts[i].type==SYS_MEMORY) {
			pmm_arena_t* arena = &arenas[j++];
			memset(arena, 0, sizeof(*arena));

			arena->name = "sdram";
			arena->base = ram_ptable.parts[i].start;
			arena->size = ram_ptable.parts[i].size;
			arena->flags = PMM_ARENA_FLAG_KMAP;

			pmm_add_arena(arena);
		}
	}

	// reserve SMEM area
	struct list_node list;
	list_initialize(&list);
	pmm_alloc_range(kvaddr_to_paddr((void*)platform_get_smem_base_addr()), 2*MB/PAGE_SIZE, &list);
}

void platform_init_mmu_mappings(void)
{
}

__WEAK uint32_t platform_get_smem_base_addr(void)
{
	return (uint32_t)paddr_to_kvaddr(MSM_SHARED_BASE);
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