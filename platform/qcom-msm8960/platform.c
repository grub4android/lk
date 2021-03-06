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

#include <arch.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <reg.h>
#include <err.h>
#include <debug.h>
#include <lk/init.h>
#include <kernel/vm.h>
#include <dev/ssbi.h>
#include <dev/uart.h>
#include <dev/keys.h>
#include <dev/pmic/pm8921.h>
#include <platform/qgic.h>
#include <platform.h>
#include <platform/gic.h>
#include <platform/smem.h>
#include <platform/qcom.h>
#include <platform/iomap.h>
#include <platform/board.h>
#include <platform/timer.h>
#include <platform/uart_dm.h>
#include <platform/baseband.h>
#include <platform/interrupts.h>
#include <platform/qcom_timer.h>
#include "platform_p.h"

struct mmu_initial_mapping mmu_initial_mappings[] = {
    { .phys = MEMBASE,
      .virt = KERNEL_BASE,
      .size = KERNEL_LOAD_OFFSET+MEMSIZE,
      .flags = 0,
      .name = "kernel"},

    { .phys = MSM_IOMAP_BASE,
      .virt = MSM_IOMAP_BASE,
      .size = MSM_IOMAP_SIZE,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "iomap"},

    { .phys = MSM_IMEM_BASE,
      .virt = MSM_IMEM_BASE,
      .size = MB,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "imem"},

    { .phys = MSM_SHARED_BASE,
      .virt = MSM_SHARED_BASE,
      .size = 2*MB,
      .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
      .name = "smem"},

	{ 0 },
};

void platform_early_init(void)
{
    msm_clocks_init();

    /* initialize the interrupt controller */
    qgic_init();

    platform_init_timer();

    board_init();

    platform_qcom_init_pmm();
}

__WEAK int target_power_key(void)
{
	uint8_t ret = 0;

	pm8921_pwrkey_status(&ret);
	return ret;
}

static int event_source_poll(key_event_source_t* source) {
	keys_set_report_key(source, KEY_RIGHT, target_power_key());
	return NO_ERROR;
}

static key_event_source_t event_source = {
	.poll = event_source_poll
};

void platform_init(void)
{
	unsigned base_addr;
	unsigned char slot;
	unsigned platform_id = board_platform_id();
	static pm8921_dev_t pmic;

	/* Initialize PMIC driver */
	pmic.read = (pm8921_read_func) & pa1_ssbi2_read_bytes;
	pmic.write = (pm8921_write_func) & pa1_ssbi2_write_bytes;

	pm8921_init(&pmic);
	keys_add_source(&event_source);

	switch(platform_id) {
	case MSM8960:
	case MSM8960AB:
	case APQ8060AB:
	case MSM8260AB:
	case MSM8660AB:
		msm8960_keypad_init();
		break;
	case MSM8130:
	case MSM8230:
	case MSM8630:
	case MSM8930:
	case MSM8130AA:
	case MSM8230AA:
	case MSM8630AA:
	case MSM8930AA:
	case MSM8930AB:
	case MSM8630AB:
	case MSM8230AB:
	case MSM8130AB:
	case APQ8030AB:
	case APQ8030:
	case APQ8030AA:
		msm8930_keypad_init();
		break;
	case APQ8064:
	case MPQ8064:
	case APQ8064AA:
	case APQ8064AB:
		apq8064_keypad_init();
		break;
	default:
		dprintf(CRITICAL,"Keyboard is not supported for platform: %d\n",platform_id);
	};

#ifdef QCOM_ENABLE_EMMC
	unsigned mmc_sdc_base[] = { MSM_SDC1_BASE, MSM_SDC2_BASE, MSM_SDC3_BASE, MSM_SDC4_BASE };

	/* Trying Slot 1 first */
	slot = 1;
	base_addr = mmc_sdc_base[slot - 1];
	if (mmc_boot_main(slot, base_addr)) {
		/* Trying Slot 3 next */
		slot = 3;
		base_addr = mmc_sdc_base[slot - 1];
		if (mmc_boot_main(slot, base_addr)) {
			dprintf(CRITICAL, "mmc init failed!");
			ASSERT(0);
		}
	}
#endif
}

void platform_quiesce(void)
{
    platform_stop_timer();
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

	/* initialize the timer block */
	arm_qcom_timer_init(6750000);
}

/* Return true if the pmic type matches */
uint8_t platform_pmic_type(uint32_t pmic_type)
{
	uint8_t ret = 0;
	uint8_t i = 0;
	uint8_t num_ent = 0;
	struct board_pmic_data pmic_info[SMEM_V7_SMEM_MAX_PMIC_DEVICES];

	num_ent = board_pmic_info(pmic_info, SMEM_V7_SMEM_MAX_PMIC_DEVICES);

	for(i = 0; i < num_ent; i++) {
		if (pmic_info[i].pmic_type == pmic_type) {
			ret = 1;
			break;
		}
	}

	return ret;
}

void shutdown_device(void)
{
	pm8921_config_reset_pwr_off(0);

	/* Actually reset the chip */
	writel(0, MSM_PSHOLD_CTL_SU);
	mdelay(5000);
}

void reboot_device(unsigned reboot_reason)
{
	writel(reboot_reason, RESTART_REASON_ADDR);

	/* Actually reset the chip */
	pm8921_config_reset_pwr_off(1);
	writel(0, MSM_PSHOLD_CTL_SU);
	mdelay(10000);

	dprintf(CRITICAL, "PSHOLD failed, trying watchdog reset\n");
	writel(1, MSM_WDT0_RST);
	writel(0, MSM_WDT0_EN);
	writel(0x31F3, MSM_WDT0_BT);
	writel(3, MSM_WDT0_EN);
	DMB;
	writel(3, MSM_TCSR_BASE + TCSR_WDOG_CFG);
	mdelay(10000);
}
