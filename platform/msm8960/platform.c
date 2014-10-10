/*
 * Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <debug.h>
#include <reg.h>
#include <platform/iomap.h>
#include <platform/msm8960.h>
#include <platform.h>
#include <platform/msm_shared.h>
#include <platform/msm_shared/qgic.h>
#include <platform/msm_shared/uart_dm.h>
#include <dev/fbcon.h>
#include <kernel/vm.h>
#include <arch/arm/mmu.h>
#include <platform/msm_shared/board.h>

#include "platform_p.h"

static uint32_t ticks_per_sec = 0;

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

    /* Kernel memory */
    { .phys = BASE_ADDR,
      .virt = BASE_ADDR,
      .size = 44*MB,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "kernel" },

    /* Scratch memory */
    { .phys = SCRATCH_ADDR,
      .virt = SCRATCH_ADDR,
      .size = 768*MB,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "scratch" },

    /* IOMAP memory */
    { .phys = MSM_IOMAP_BASE,
      .virt = MSM_IOMAP_BASE,
      .size = MSM_IOMAP_SIZE*MB,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "iomap" },

    /* IMEM memory */
    { .phys = MSM_IMEM_BASE,
      .virt = MSM_IMEM_BASE,
      .size = 1*MB,
      .flags = MMU_INITIAL_MAPPING_FLAG_UNCACHED,
      .name = "imem" },

    /* identity map to let the boot code run */
   /* { .phys = MEMBASE,
      .virt = MEMBASE,
      .size = MEMSIZE,
      .flags = MMU_INITIAL_MAPPING_TEMPORARY },*/

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
	msm_clocks_init();
	qgic_init();
	platform_init_timer();
	board_init();

    /* add the main memory arena */
	pmm_add_arena(&sram_arena);
}

void platform_init(void)
{
	dprintf(INFO, "platform_init()\n");
}

void platform_quiesce(void)
{
#if DISPLAY_SPLASH_SCREEN
	display_shutdown();
#endif

	platform_uninit_timer();
}

/* Setup memory for this platform */
void platform_init_mmu_mappings(void)
{
}

/* Initialize DGT timer */
void platform_init_timer(void)
{
	/* disable timer */
	writel(0, DGT_ENABLE);

	/* DGT uses LPXO source which is 27MHz.
	 * Set clock divider to 4.
	 */
	writel(3, DGT_CLK_CTL);

	ticks_per_sec = 6750000;	/* (27 MHz / 4) */
}

/* Returns timer ticks per sec */
uint32_t platform_tick_rate(void)
{
	return ticks_per_sec;
}

/* Return true if the pmic type matches */
uint8_t platform_pmic_type(uint32_t pmic_type)
{
	uint8_t ret = 0;
	uint8_t i = 0;
	uint8_t num_ent = 0;
	struct board_pmic_data pmic_info[SMEM_V7_SMEM_MAX_PMIC_DEVICES];

	num_ent = board_pmic_info((struct board_pmic_data*)&pmic_info, SMEM_V7_SMEM_MAX_PMIC_DEVICES);

	for(i = 0; i < num_ent; i++) {
		if (pmic_info[i].pmic_type == pmic_type) {
			ret = 1;
			break;
		}
	}

	return ret;
}
