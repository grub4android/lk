/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
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
#include <stdarg.h>
#include <reg.h>
#include <stdio.h>
#include <dev/uart.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/qcom.h>
#include <platform/debug.h>
#include <target/debugconfig.h>

/* DEBUG_UART must be defined to 0 or 1 */
#if defined(DEBUG_UART) && DEBUG_UART == 0
#define DEBUG_UART_BASE UART0_BASE
#elif defined(DEBUG_UART) && DEBUG_UART == 1
#define DEBUG_UART_BASE UART1_BASE
#else
#error define DEBUG_UART to something valid
#endif

#define REBOOT_MODE_BOOTLOADER 0x77665500
#define REBOOT_MODE_NORMAL     0x77665501
#define REBOOT_MODE_RECOVERY   0x77665502
#define REBOOT_MODE_ALARM      0x77665503
#define REBOOT_MODE_PANIC      0x6d630100

void platform_dputc(char c)
{
#ifdef QCOM_ENABLE_UART
    if (c == '\n')
        uart_putc(DEBUG_UART, '\r');
    uart_putc(DEBUG_UART, c);
#endif
}

int platform_dgetc(char *c, bool wait)
{
#ifdef QCOM_ENABLE_UART
    int ret = uart_getc(DEBUG_UART, wait);
    if (ret == -1)
        return -1;
    *c = ret;
    return 0;
#endif

    return -1;
}

int platform_dtstc(void)
{
#ifdef QCOM_ENABLE_UART
    return uart_tstc(DEBUG_UART);
#endif

    return 0;
}

void platform_halt(platform_halt_action suggested_action,
                          platform_halt_reason reason)
{
    switch (suggested_action) {
        default:
        case HALT_ACTION_SHUTDOWN:
        case HALT_ACTION_HALT:
            printf("HALT: shutdown device... (reason = %d)\n", reason);
            shutdown_device();
            printf("SHUTDOWN FAILED - spinning forever...\n");
			arch_disable_ints();
            for(;;)
				arch_idle();
            break;
        case HALT_ACTION_REBOOT:
            printf("REBOOT\n");
            if(reason==HALT_REASON_SW_BOOTLOADER)
                reboot_device(REBOOT_MODE_BOOTLOADER);
            else if(reason==HALT_REASON_SW_UPDATE)
                reboot_device(REBOOT_MODE_RECOVERY);
            else
                reboot_device(REBOOT_MODE_NORMAL);

            printf("REBOOT FAILED - spinning forever...\n");
			arch_disable_ints();
            for(;;)
				arch_idle();
            break;
    }
}

platform_halt_reason platform_get_reboot_reason(void) {
	switch(qcom_get_reboot_reason()) {
		case REBOOT_MODE_BOOTLOADER:
			return HALT_REASON_SW_BOOTLOADER;
		case REBOOT_MODE_NORMAL:
			return HALT_REASON_SW_RESET;
		case REBOOT_MODE_RECOVERY:
			return HALT_REASON_SW_UPDATE;
		case REBOOT_MODE_ALARM:
			return HALT_REASON_ALARM;
		case REBOOT_MODE_PANIC:
			return HALT_REASON_SW_PANIC;

		default:
			return HALT_REASON_UNKNOWN;
	}
}
