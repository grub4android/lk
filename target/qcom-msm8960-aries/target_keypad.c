/*
 * Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of The Linux Foundation nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
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
 *
 */

#include <string.h>
#include <err.h>
#include <debug.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dev/keys.h>
#include <dev/ssbi.h>
#include <dev/gpio_keypad.h>
#include <dev/pmic/pm8921.h>
#include <platform/gpio.h>
#include <platform/smem.h>
#include <platform/board.h>
#include <platform/msm8960.h>

#define BITS_IN_ELEMENT(x) (sizeof(x) * 8)
#define KEYMAP_INDEX(row, col) (row)* BITS_IN_ELEMENT(unsigned int) + (col)

static unsigned int apq8064_qwerty_keymap[] = {
	[KEYMAP_INDEX(0, 0)] = KEY_VOLUMEUP,	/* Volume key on the device/CDP */
	[KEYMAP_INDEX(0, 1)] = KEY_VOLUMEDOWN,	/* Volume key on the device/CDP */
};

static unsigned int apq8064_pm8921_keys_gpiomap[] = {
	[KEYMAP_INDEX(0, 0)] = PM_GPIO(1),	/* Volume key on the device/CDP */
	[KEYMAP_INDEX(0, 1)] = PM_GPIO(2),	/* Volume key on the device/CDP */
};

static struct qwerty_keypad_info apq8064_pm8921_qwerty_keypad = {
	.keymap = apq8064_qwerty_keymap,
	.gpiomap = apq8064_pm8921_keys_gpiomap,
	.mapsize = ARRAY_SIZE(apq8064_qwerty_keymap),
	.key_gpio_get = &pm8921_gpio_get,
	.settle_time = 5 /* msec */ ,
	.poll_time = 20 /* msec */ ,
};

void apq8064_keypad_init(void)
{
	apq8064_keypad_gpio_init();
	ssbi_gpio_keypad_init(&apq8064_pm8921_qwerty_keypad);
}

static unsigned int msm8960_qwerty_keymap[] = {
	[KEYMAP_INDEX(0, 0)] = KEY_VOLUMEDOWN,	/* Volume key on the device/CDP */
	[KEYMAP_INDEX(0, 1)] = KEY_VOLUMEUP,	/* Volume key on the device/CDP */
};

static unsigned int msm8960_keys_gpiomap[] = {
	[KEYMAP_INDEX(0, 0)] = PM_GPIO(1),	/* Volume key on the device/CDP */
	[KEYMAP_INDEX(0, 1)] = PM_GPIO(2),	/* Volume key on the device/CDP */
};

static struct qwerty_keypad_info msm8960_qwerty_keypad = {
	.keymap = msm8960_qwerty_keymap,
	.gpiomap = msm8960_keys_gpiomap,
	.mapsize = ARRAY_SIZE(msm8960_qwerty_keymap),
	.key_gpio_get = &pm8921_gpio_get,
	.settle_time = 5 /* msec */ ,
	.poll_time = 20 /* msec */ ,
};

void msm8960_keypad_init(void)
{
	msm8960_keypad_gpio_init();
	ssbi_gpio_keypad_init(&msm8960_qwerty_keypad);
}

static struct pm8xxx_gpio_init pm8921_keypad_gpios[] = {
	/* keys GPIOs */
	PM8XXX_GPIO_INPUT(PM_GPIO(1), PM_GPIO_PULL_UP_31_5),
	PM8XXX_GPIO_INPUT(PM_GPIO(2), PM_GPIO_PULL_UP_31_5),
	PM8XXX_GPIO_OUTPUT(PM_GPIO(9), 0),
	PM8XXX_GPIO_OUTPUT(PM_GPIO(10), 0),
};

/* pm8921 GPIO configuration for APQ8064 keypad */
static struct pm8xxx_gpio_init pm8921_keypad_gpios_apq[] = {
	/* keys GPIOs */
	PM8XXX_GPIO_INPUT(PM_GPIO(1), PM_GPIO_PULL_UP_31_5),
	PM8XXX_GPIO_INPUT(PM_GPIO(2), PM_GPIO_PULL_UP_31_5),
	PM8XXX_GPIO_OUTPUT(PM_GPIO(9), 0),
	PM8XXX_GPIO_OUTPUT(PM_GPIO(10), 0),
};

void msm8960_keypad_gpio_init(void)
{
	int i = 0;
	int num = 0;

	num = ARRAY_SIZE(pm8921_keypad_gpios);

	for(i=0; i < num; i++)
	{
		pm8921_gpio_config(pm8921_keypad_gpios[i].gpio,
							&(pm8921_keypad_gpios[i].config));
	}
}

void apq8064_keypad_gpio_init(void)
{
	int i = 0;
	int num = 0;
	struct pm8xxx_gpio_init *gpio_array;

	num = ARRAY_SIZE(pm8921_keypad_gpios_apq);
	gpio_array = pm8921_keypad_gpios_apq;

	for(i = 0; i < num; i++)
	{
		pm8921_gpio_config(gpio_array[i].gpio,
							&(gpio_array[i].config));
	}
}
