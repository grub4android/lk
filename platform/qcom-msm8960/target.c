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
#include <dev/uart.h>
#include <dev/pmic/pm8921.h>
#include <lk/init.h>
#include <kernel/vm.h>
#include <platform.h>
#include <platform/gic.h>
#include <platform/smem.h>
#include <platform/gpio.h>
#include <platform/qcom.h>
#include <platform/iomap.h>
#include <platform/board.h>
#include <platform/uart_dm.h>
#include <platform/baseband.h>
#include <platform/qcom_timer.h>
#include <platform/interrupts.h>
#include "platform_p.h"

#ifdef QCOM_ENABLE_UART
void platform_uart_init_auto(void)
{
	unsigned target_id = board_machtype();

	switch (target_id) {
	case LINUX_MACHTYPE_8960_SIM:
	case LINUX_MACHTYPE_8960_RUMI3:
	case LINUX_MACHTYPE_8960_CDP:
	case LINUX_MACHTYPE_8960_MTP:
	case LINUX_MACHTYPE_8960_FLUID:
	case LINUX_MACHTYPE_8960_APQ:
	case LINUX_MACHTYPE_8960_LIQUID:

		if(board_baseband() == BASEBAND_SGLTE || board_baseband() == BASEBAND_SGLTE2)
		{
			uart_dm_init(8, 0x1A000000, 0x1A040000);;
		}
		else
		{
			uart_dm_init(5, 0x16400000, 0x16440000);
		}
		break;

	case LINUX_MACHTYPE_8930_CDP:
	case LINUX_MACHTYPE_8930_MTP:
	case LINUX_MACHTYPE_8930_FLUID:

		uart_dm_init(5, 0x16400000, 0x16440000);
		break;

	case LINUX_MACHTYPE_8064_SIM:
	case LINUX_MACHTYPE_8064_RUMI3:
		uart_dm_init(3, 0x16200000, 0x16240000);
		break;

	case LINUX_MACHTYPE_8064_CDP:
	case LINUX_MACHTYPE_8064_MTP:
	case LINUX_MACHTYPE_8064_LIQUID:
		uart_dm_init(7, 0x16600000, 0x16640000);
		break;

	case LINUX_MACHTYPE_8064_EP:
		uart_dm_init(2, 0x12480000, 0x12490000);
		break;

	case LINUX_MACHTYPE_8064_MPQ_CDP:
	case LINUX_MACHTYPE_8064_MPQ_HRD:
	case LINUX_MACHTYPE_8064_MPQ_DTV:
	case LINUX_MACHTYPE_8064_MPQ_DMA:
		uart_dm_init(5, 0x1A200000, 0x1A240000);
		break;

	case LINUX_MACHTYPE_8627_CDP:
	case LINUX_MACHTYPE_8627_MTP:

		uart_dm_init(5, 0x16400000, 0x16440000);
		break;

	default:
		dprintf(CRITICAL, "uart gsbi not defined for target: %d\n",
			target_id);
	}
}

__WEAK int target_uart_gpio_config(uint8_t id)
{
	return 0;
}
#endif

/* Detect the target type */
void target_detect(struct board_data *board)
{
	uint32_t platform;
	uint32_t platform_hw;
	uint32_t target_id;

	platform = board->platform;
	platform_hw = board->platform_hw;

	/* Detect the board we are running on */
	if ((platform == MSM8960) || (platform == MSM8960AB) ||
		(platform == APQ8060AB) || (platform == MSM8260AB) ||
		(platform == MSM8660AB) ||(platform == MSM8660A) ||
		(platform == MSM8260A) || (platform == APQ8060A)) {
		switch (platform_hw) {
		case HW_PLATFORM_SURF:
			target_id = LINUX_MACHTYPE_8960_CDP;
			break;
		case HW_PLATFORM_MTP:
			target_id = LINUX_MACHTYPE_8960_MTP;
			break;
		case HW_PLATFORM_FLUID:
			target_id = LINUX_MACHTYPE_8960_FLUID;
			break;
		case HW_PLATFORM_LIQUID:
			target_id = LINUX_MACHTYPE_8960_LIQUID;
			break;
		default:
			target_id = LINUX_MACHTYPE_8960_CDP;
		}
	} else if ((platform == MSM8130)           ||
			   (platform == MSM8130AA) || (platform == MSM8130AB) ||
			   (platform == MSM8230)   || (platform == MSM8630)   ||
			   (platform == MSM8930)   || (platform == MSM8230AA) ||
			   (platform == MSM8630AA) || (platform == MSM8930AA) ||
			   (platform == MSM8930AB) || (platform == MSM8630AB) ||
			   (platform == MSM8230AB) || (platform == APQ8030AB) ||
			   (platform == APQ8030) || platform == APQ8030AA) {
		switch (platform_hw) {
		case HW_PLATFORM_SURF:
			target_id = LINUX_MACHTYPE_8930_CDP;
			break;
		case HW_PLATFORM_MTP:
			target_id = LINUX_MACHTYPE_8930_MTP;
			break;
		case HW_PLATFORM_FLUID:
			target_id = LINUX_MACHTYPE_8930_FLUID;
			break;
		case HW_PLATFORM_QRD:
			target_id = LINUX_MACHTYPE_8930_EVT;
			break;
		default:
			target_id = LINUX_MACHTYPE_8930_CDP;
		}
	} else if ((platform == MSM8227) || (platform == MSM8627) ||
			   (platform == MSM8227AA) || (platform == MSM8627AA)) {
		switch (platform_hw) {
		case HW_PLATFORM_SURF:
			target_id = LINUX_MACHTYPE_8627_CDP;
			break;
		case HW_PLATFORM_MTP:
			target_id = LINUX_MACHTYPE_8627_MTP;
			break;
		default:
			target_id = LINUX_MACHTYPE_8627_CDP;
		}
	} else if (platform == MPQ8064) {
		switch (platform_hw) {
		case HW_PLATFORM_SURF:
			target_id = LINUX_MACHTYPE_8064_MPQ_CDP;
			break;
		case HW_PLATFORM_HRD:
			target_id = LINUX_MACHTYPE_8064_MPQ_HRD;
			break;
		case HW_PLATFORM_DTV:
			target_id = LINUX_MACHTYPE_8064_MPQ_DTV;
			break;
		case HW_PLATFORM_DMA:
			target_id = LINUX_MACHTYPE_8064_MPQ_DMA;
			break;
		default:
			target_id = LINUX_MACHTYPE_8064_MPQ_CDP;
		}
	} else if ((platform == APQ8064) || (platform == APQ8064AA)
					 || (platform == APQ8064AB)) {
		switch (platform_hw) {
		case HW_PLATFORM_SURF:
			target_id = LINUX_MACHTYPE_8064_CDP;
			break;
		case HW_PLATFORM_MTP:
			target_id = LINUX_MACHTYPE_8064_MTP;
			break;
		case HW_PLATFORM_LIQUID:
			target_id = LINUX_MACHTYPE_8064_LIQUID;
			break;
		case HW_PLATFORM_BTS:
			target_id = LINUX_MACHTYPE_8064_EP;
			break;
		default:
			target_id = LINUX_MACHTYPE_8064_CDP;
		}
	} else {
		dprintf(CRITICAL, "platform (%d) is not identified.\n",
			platform);
		ASSERT(0);
	}
	board->target = target_id;
}

/* Detect the modem type */
void target_baseband_detect(struct board_data *board)
{
	uint32_t baseband;
	uint32_t platform;
	uint32_t platform_subtype;

	platform         = board->platform;
	platform_subtype = board->platform_subtype;

	/* Check for baseband variants. Default to MSM */
	if (platform_subtype == HW_PLATFORM_SUBTYPE_MDM)
		baseband = BASEBAND_MDM;
	else if (platform_subtype == HW_PLATFORM_SUBTYPE_SGLTE)
		baseband = BASEBAND_SGLTE;
	else if (platform_subtype == HW_PLATFORM_SUBTYPE_DSDA)
		baseband = BASEBAND_DSDA;
	else if (platform_subtype == HW_PLATFORM_SUBTYPE_DSDA2)
		baseband = BASEBAND_DSDA2;
	else if (platform_subtype == HW_PLATFORM_SUBTYPE_SGLTE2)
		baseband = BASEBAND_SGLTE2;
	else {
		switch(platform) {
		case APQ8060:
		case APQ8064:
		case APQ8064AA:
		case APQ8064AB:
		case APQ8030AB:
		case MPQ8064:
		case APQ8030:
		case APQ8030AA:
			baseband = BASEBAND_APQ;
			break;
		default:
			baseband = BASEBAND_MSM;
		};
	}
	board->baseband = baseband;
}

/*
 * Function to set the capabilities for the host
 */
void target_mmc_caps(struct mmc_host *host)
{
	host->caps.ddr_mode = 1;
	host->caps.hs200_mode = 1;
	host->caps.bus_width = MMC_BOOT_BUS_WIDTH_8_BIT;
	host->caps.hs_clk_rate = MMC_CLK_96MHZ;
}

unsigned board_machtype(void)
{
	return board_target_id();
}

enum baseband target_baseband(void)
{
	return board_baseband();
}

static void apq8064_ext_3p3V_enable(void)
{
	/* Configure GPIO for output */
	gpio_tlmm_config(77, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA, GPIO_ENABLE);

	/* Output High */
	gpio_set(77, 2);
}

/* Do any target specific intialization needed before entering fastboot mode */
void target_fastboot_init(void)
{
	/* Set the BOOT_DONE flag in PM8921 */
	pm8921_boot_done();
}

/* Do target specific usb initialization */
void target_usb_init(void)
{
	if(board_target_id() == LINUX_MACHTYPE_8064_LIQUID)
	{
		apq8064_ext_3p3V_enable();
	}
}
