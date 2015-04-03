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

#include <ctype.h>
#include <debug.h>
#include <stdlib.h>
#include <printf.h>
#include <stdio.h>
#include <list.h>
#include <string.h>
#include <arch/ops.h>
#include <platform.h>
#include <platform/debug.h>
#include <kernel/thread.h>

#if WITH_LIB_SM
#define PRINT_LOCK_FLAGS SPIN_LOCK_FLAG_IRQ_FIQ
#else
#define PRINT_LOCK_FLAGS SPIN_LOCK_FLAG_INTERRUPTS
#endif

static spin_lock_t print_spin_lock = 0;
static struct list_node print_callbacks = LIST_INITIAL_VALUE(print_callbacks);
/* print lock must be held when invoking out, outs, outc */
static void out_count(const char *str, size_t len)
{
	print_callback_t *cb;
	size_t i;

	/* print to any registered loggers */
	list_for_every_entry(&print_callbacks, cb, print_callback_t, entry) {
		if (cb->print)
			cb->print(cb, str, len);
	}

	/* write out the serial port */
	for (i = 0; i < len; i++) {
		platform_dputc(str[i]);
	}
}

static void out_string(const char *str)
{
	out_count(str, strlen(str));
}

static void out_char(char c)
{
	out_count(&c, 1);
}

static int input_char(char *c)
{
	return platform_dgetc(c, true);
}

void register_print_callback(print_callback_t *cb)
{
	spin_lock_saved_state_t state;

	spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);
	list_add_head(&print_callbacks, &cb->entry);
	spin_unlock_restore(&print_spin_lock, state, PRINT_LOCK_FLAGS);
}

void unregister_print_callback(print_callback_t *cb)
{
	spin_lock_saved_state_t state;

	spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);
	list_delete(&cb->entry);
	spin_unlock_restore(&print_spin_lock, state, PRINT_LOCK_FLAGS);
}

void spin(uint32_t usecs)
{
	lk_bigtime_t start = current_time_hires();

	while ((current_time_hires() - start) < usecs)
		;
}

void _panic(void *caller, const char *fmt, ...)
{
	dprintf(ALWAYS, "panic (caller %p): ", caller);

	va_list ap;
	va_start(ap, fmt);
	_dvprintf(fmt, ap);
	va_end(ap);

	platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

void abort(void)
{
	panic("abort");
}

static int __debug_stdio_fputc(void *ctx, int c)
{
	_dputc(c);
	return 0;
}

static int __debug_stdio_fputs(void *ctx, const char *s)
{
	return _dputs(s);
}

static int __debug_stdio_fgetc(void *ctx)
{
	char c;
	int err;

	err = input_char(&c);
	if (err < 0)
		return err;
	return (unsigned char)c;
}

static int __debug_stdio_vfprintf(void *ctx, const char *fmt, va_list ap)
{
	return _dvprintf(fmt, ap);
}

#define DEFINE_STDIO_DESC(id)						\
	[(id)]	= {							\
		.ctx		= &__stdio_FILEs[(id)],			\
		.fputc		= __debug_stdio_fputc,			\
		.fputs		= __debug_stdio_fputs,			\
		.fgetc		= __debug_stdio_fgetc,			\
		.vfprintf	= __debug_stdio_vfprintf,		\
	}

FILE __stdio_FILEs[3] = {
	DEFINE_STDIO_DESC(0), /* stdin */
	DEFINE_STDIO_DESC(1), /* stdout */
	DEFINE_STDIO_DESC(2), /* stderr */
};
#undef DEFINE_STDIO_DESC

#if !DISABLE_DEBUG_OUTPUT

void _dputc(char c)
{
	spin_lock_saved_state_t state;

	spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);
	out_char(c);
	spin_unlock_restore(&print_spin_lock, state, PRINT_LOCK_FLAGS);
}

int _dputs(const char *str)
{
	spin_lock_saved_state_t state;

	spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);
	out_string(str);
	spin_unlock_restore(&print_spin_lock, state, PRINT_LOCK_FLAGS);

	return 0;
}

int _dwrite(const char *ptr, size_t len)
{
	spin_lock_saved_state_t state;

	spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);
	out_count(ptr, len);
	spin_unlock_restore(&print_spin_lock, state, PRINT_LOCK_FLAGS);

	return 0;
}

static int _dprintf_output_func(const char *str, size_t len, void *state)
{
	size_t n = strnlen(str, len);

	out_count(str, n);
	return n;
}

int _dprintf(const char *fmt, ...)
{
	spin_lock_saved_state_t state;
	int err;
	va_list ap;

	va_start(ap, fmt);
	spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);
	err = _printf_engine(&_dprintf_output_func, NULL, fmt, ap);
	spin_unlock_restore(&print_spin_lock, state, PRINT_LOCK_FLAGS);
	va_end(ap);

	return err;
}

int _dvprintf(const char *fmt, va_list ap)
{
	spin_lock_saved_state_t state;
	int err;

	spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);
	err = _printf_engine(&_dprintf_output_func, NULL, fmt, ap);
	spin_unlock_restore(&print_spin_lock, state, PRINT_LOCK_FLAGS);

	return err;
}

void hexdump(const void *ptr, size_t len)
{
	addr_t address = (addr_t)ptr;
	size_t count;

	for (count = 0 ; count < len; count += 16) {
		union {
			uint32_t buf[4];
			uint8_t  cbuf[16];
		} u;
		size_t s = ROUNDUP(MIN(len - count, 16), 4);
		size_t i;

		printf("0x%08lx: ", address);
		for (i = 0; i < s / 4; i++) {
			u.buf[i] = ((const uint32_t *)address)[i];
			printf("%08x ", u.buf[i]);
		}
		for (; i < 4; i++) {
			printf("         ");
		}
		printf("|");

		for (i=0; i < 16; i++) {
			char c = u.cbuf[i];
			if (i < s && isprint(c)) {
				printf("%c", c);
			} else {
				printf(".");
			}
		}
		printf("|\n");
		address += 16;
	}
}

void hexdump8(const void *ptr, size_t len)
{
	addr_t address = (addr_t)ptr;
	size_t count;
	size_t i;

	for (count = 0 ; count < len; count += 16) {
		printf("0x%08lx: ", address);
		for (i=0; i < MIN(len - count, 16); i++) {
			printf("0x%02hhx ", *(const uint8_t *)(address + i));
		}
		printf("\n");
		address += 16;
	}
}

#endif // !DISABLE_DEBUG_OUTPUT

// vim: set noexpandtab:
