/*
 * Copyright (c) 2014 Travis Geiselbrecht
 * Copyright (c) 2014 Chris Anderson
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

#include <debug.h>
#include <platform/qcom.h>
#include <platform/gpio.h>
#include <platform/gsbi.h>
#include <platform/board.h>
#include <platform/msm_panel.h>

#ifdef QCOM_ENABLE_UART
#define MSM_BOOT_UART_SWITCH_GPIO_MITWOA	33
#define MSM_BOOT_UART_SWITCH_GPIO_MITWO		62

int target_uart_gpio_config(uint8_t id) {
	// configure UART switch
	if(board_target_id() == LINUX_MACHTYPE_8960_CDP)
	{
		gpio_tlmm_config(MSM_BOOT_UART_SWITCH_GPIO_MITWOA, 0, GPIO_OUTPUT,
				GPIO_NO_PULL, GPIO_16MA, GPIO_ENABLE);
		gpio_direction(MSM_BOOT_UART_SWITCH_GPIO_MITWOA, GPIO_OUTPUT);
		gpio_set(MSM_BOOT_UART_SWITCH_GPIO_MITWOA, GPIO_IN_OUT_HIGH);

	} else if (board_target_id() == LINUX_MACHTYPE_8064_MTP
			|| board_target_id() == LINUX_MACHTYPE_8064_MITWO) {
		gpio_tlmm_config(MSM_BOOT_UART_SWITCH_GPIO_MITWO, 0, GPIO_OUTPUT,
				GPIO_NO_PULL, GPIO_16MA, GPIO_ENABLE);
		gpio_direction(MSM_BOOT_UART_SWITCH_GPIO_MITWO, GPIO_OUTPUT);
		gpio_set(MSM_BOOT_UART_SWITCH_GPIO_MITWO, GPIO_IN_OUT_HIGH);
	}

	if(board_platform_id() == MPQ8064 || board_platform_id() == MSM8260AB)
	{
		switch (id) {

		case GSBI_ID_5:
			/* configure rx gpio */
			gpio_tlmm_config(23, 1, GPIO_INPUT, GPIO_NO_PULL,
							 GPIO_8MA, GPIO_DISABLE);
			/* configure tx gpio */
			gpio_tlmm_config(22, 1, GPIO_OUTPUT, GPIO_NO_PULL,
							 GPIO_8MA, GPIO_DISABLE);
			break;

		default:
			ASSERT(0);
		}

		return 1;
	}

	// fall back to platform code
	return 0;
}
#endif

void target_early_init(void)
{
#ifdef QCOM_ENABLE_UART
	platform_uart_init_auto();
#endif
}

void target_init(void)
{
	target_display_init("");
}
