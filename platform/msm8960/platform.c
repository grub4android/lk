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
#include <qgic.h>
#include <uart_dm.h>
#include <dev/fbcon.h>
#include <mmu.h>
#include <arch/arm/mmu.h>
#include <board.h>
#include <platform/msm_shared/timer.h>

static uint32_t ticks_per_sec = 0;

#define MSM_IOMAP_SIZE ((MSM_IOMAP_END - MSM_IOMAP_BASE)/MB)

/* LK memory - cacheable, write through */
#define LK_MEMORY         (MMU_MEMORY_TYPE_NORMAL_WRITE_THROUGH | \
                           MMU_MEMORY_AP_READ_WRITE)

/* Kernel region - cacheable, write through */
#define KERNEL_MEMORY     (MMU_MEMORY_TYPE_NORMAL_WRITE_THROUGH   | \
                           MMU_MEMORY_AP_READ_WRITE | MMU_MEMORY_XN)

/* Scratch region - cacheable, write through */
#define SCRATCH_MEMORY    (MMU_MEMORY_TYPE_NORMAL_WRITE_THROUGH   | \
                           MMU_MEMORY_AP_READ_WRITE | MMU_MEMORY_XN)

/* Peripherals - non-shared device */
#define IOMAP_MEMORY      (MMU_MEMORY_TYPE_DEVICE_NON_SHARED | \
                           MMU_MEMORY_AP_READ_WRITE | MMU_MEMORY_XN)

/* IMEM: Must set execute never bit to avoid instruction prefetch from TZ */
#define IMEM_MEMORY       (MMU_MEMORY_TYPE_STRONGLY_ORDERED | \
                           MMU_MEMORY_AP_READ_WRITE | MMU_MEMORY_XN)

mmu_section_t mmu_section_table[] = {
/*  Physical addr,    Virtual addr,    Size (in MB),    Flags */
	{MEMBASE, MEMBASE, (MEMSIZE / MB), LK_MEMORY},
	{MSM_IOMAP_BASE, MSM_IOMAP_BASE, MSM_IOMAP_SIZE, IOMAP_MEMORY},
	{MSM_IMEM_BASE, MSM_IMEM_BASE, 1, IMEM_MEMORY},
};

void platform_early_init(void)
{
	msm_clocks_init();
	qgic_init();
	platform_init_timer();
	board_init();
}

void platform_init(void)
{
	dprintf(INFO, "platform_init()\n");
}

void platform_uninit(void)
{
#if DISPLAY_SPLASH_SCREEN
	display_shutdown();
#endif

	platform_uninit_timer();
}

/* Setup memory for this platform */
void platform_init_mmu_mappings(void)
{
	uint32_t i;
	uint32_t table_size = ARRAY_SIZE(mmu_section_table);

	for (i = 0; i < table_size; i++) {
		platform_mmu_map_area(mmu_section_table[i].paddress, mmu_section_table[i].vaddress,
			mmu_section_table[i].num_of_sections, mmu_section_table[i].flags);
	}
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
