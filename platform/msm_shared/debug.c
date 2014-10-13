/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 * Copyright (c) 2009-2014, The Linux Foundation. All rights reserved.
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

#include <string.h>
#include <stdlib.h>
#include <debug.h>
#include <printf.h>
#include <arch/arm/dcc.h>
#include <dev/fbcon.h>
#include <dev/uart.h>
#include <platform/timer.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/msm_shared.h>
#include <platform/msm_shared/timer.h>
#include <lk/init.h>
#include <app/fastboot.h>

#if PON_VIB_SUPPORT
#include <vibrator.h>
#endif

static void write_dcc(char c)
{
	uint32_t timeout = 10;

	/* Note: Smallest sampling rate for DCC is 50us.
	 * This can be changed by SNOOPer.Rate on T32 window.
	 */
	while (timeout)
	{
		if (dcc_putc(c) == 0)
			break;
		udelay(50);
		timeout--;
	}
}

#if WITH_DEBUG_LOG_BUF

#ifndef LK_LOG_BUF_SIZE
#define LK_LOG_BUF_SIZE    (4096) /* align on 4k */
#endif

#define LK_LOG_COOKIE    0x474f4c52 /* "RLOG" in ASCII */

struct lk_log {
	struct lk_log_header {
		unsigned cookie;
		unsigned max_size;
		unsigned size_written;
		unsigned idx;
	} header;
	char data[LK_LOG_BUF_SIZE];
};

static struct lk_log log = {
	.header = {
		.cookie = LK_LOG_COOKIE,
		.max_size = sizeof(log.data),
		.size_written = 0,
		.idx = 0,
	},
	.data = {0}
};

static void log_putc(char c)
{
	log.data[log.header.idx++] = c;
	log.header.size_written++;
	if (unlikely(log.header.idx >= log.header.max_size))
		log.header.idx = 0;
}

void cmd_oem_lk_log(const char *arg, void *data, unsigned sz)
{
	char* pch;
	char* buf = strdup(log.data);

	pch = strtok(buf, "\n\r");
	while (pch != NULL) {
		char* ptr = pch;
		while(ptr!=NULL) {
			fastboot_info(ptr);
			if(strlen(ptr)>MAX_RSP_SIZE-5)
				ptr+=MAX_RSP_SIZE-5;
			else ptr=NULL;
		}

		pch = strtok(NULL, "\n\r");
	}

	free(buf);
	fastboot_okay("");
}

#endif /* WITH_DEBUG_LOG_BUF */

void platform_dputc(char c)
{
#if WITH_DEBUG_LOG_BUF
	log_putc(c);
#endif
#if WITH_DEBUG_DCC
	if (c == '\n') {
		write_dcc('\r');
	}
	write_dcc(c) ;
#endif
#if WITH_DEBUG_UART
	uart_putc(0, c);
#endif
#if WITH_DEBUG_FBCON && WITH_DEV_FBCON
	fbcon_putc(c);
#endif
#if WITH_DEBUG_JTAG
	jtag_dputc(c);
#endif
}

int platform_dgetc(char *c, bool wait)
{
	int n;
#if WITH_DEBUG_DCC
	n = dcc_getc();
#elif WITH_DEBUG_UART
	n = uart_getc(0, 0);
#else
	n = -1;
#endif
	if (n < 0) {
		return -1;
	} else {
		*c = n;
		return 0;
	}
}

void platform_halt(platform_halt_action suggested_action,
                          platform_halt_reason reason)
{
#if PON_VIB_SUPPORT
	vib_turn_off();
#endif

    switch (suggested_action) {
        default:
        case HALT_ACTION_SHUTDOWN:
        case HALT_ACTION_HALT:
            printf("HALT: (reason = %d)\n", reason);
            enter_critical_section();
			shutdown_device();
            for(;;);
            break;
        case HALT_ACTION_REBOOT:
            printf("REBOOT: (reason = %d)\n", reason);
            enter_critical_section();
			reboot_device(REBOOT_MODE_NORMAL);
            for (;;);
            break;
    }
}

static void platform_fastboot_init(uint level)
{
#if WITH_DEBUG_LOG_BUF
	fastboot_register("oem lk_log", cmd_oem_lk_log);
#endif
}

LK_INIT_HOOK(platform_msm, &platform_fastboot_init, LK_INIT_LEVEL_TARGET);
