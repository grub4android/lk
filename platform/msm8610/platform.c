/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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
 */

#include <debug.h>
#include <reg.h>
#include <platform/iomap.h>
#include <platform/msm_shared.h>
#include <qgic.h>
#include <qtimer.h>
#include <platform/clock.h>
#include <kernel/vm.h>
#include <arch/arm/mmu.h>
#include <smem.h>
#include <board.h>
#include <boot_stats.h>

#define MSM_IOMAP_SIZE ((MSM_IOMAP_END - MSM_IOMAP_BASE)/MB)

/* initial memory mappings. parsed by start.S */
struct mmu_initial_mapping mmu_initial_mappings[] = {
	{ .phys = 0,
      .virt = 0,
      .size = 2048*MB,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "identity"  },

	{ .phys = 2048*MB,
      .virt = 2048*MB,
      .size = 2048*MB,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "identity"  },

    /* LK memory */
    { .phys = MEMBASE,
      .virt = KERNEL_BASE,
      .size = MEMSIZE,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "memory" },

    /* IOMAP memory */
    { .phys = MSM_IOMAP_BASE,
      .virt = MSM_IOMAP_BASE,
      .size = MSM_IOMAP_SIZE*MB,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "iomap" },

    /* IMEM memory */
    { .phys = SYSTEM_IMEM_BASE,
      .virt = SYSTEM_IMEM_BASE,
      .size = 1*MB,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "imem" },

    /* null entry to terminate the list */
    { 0 }
};

static pmm_arena_t sram_arena = {
    .name = "sdram",
    .base = MEMBASE,
    .size = MEMSIZE,
    .priority = 1,
    .flags = PMM_ARENA_FLAG_KMAP
};

void platform_early_init(void)
{
	board_init();
	platform_clock_init();
	qgic_init();
	qtimer_init();

    /* add the main memory arena */
	pmm_add_arena(&sram_arena);
}

void platform_init(void)
{
	dprintf(INFO, "platform_init()\n");
}

uint32_t platform_get_sclk_count(void)
{
	return readl(MPM2_MPM_SLEEP_TIMETICK_COUNT_VAL);
}

addr_t get_bs_info_addr()
{
	return ((addr_t)BS_INFO_ADDR);
}

void platform_quiesce(void)
{
#if DISPLAY_SPLASH_SCREEN
	display_shutdown();
#endif

	qtimer_uninit();
}

/* Setup memory for this platform */
void platform_init_mmu_mappings(void)
{
}
