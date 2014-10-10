/*
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2009-2011, The Linux Foundation. All rights reserved.
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
#include <sys/types.h>
#include <err.h>
#include <assert.h>
#include <trace.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/timer.h>
#include <platform/msm8960.h>
#include <platform/msm_shared/timer.h>

#define LOCAL_TRACE 0

#define GPT_ENABLE_CLR_ON_MATCH_EN        2
#define GPT_ENABLE_EN                     1
#define DGT_ENABLE_CLR_ON_MATCH_EN        2
#define DGT_ENABLE_EN                     1

#define SPSS_TIMER_STATUS_DGT_EN    (1 << 0)

static platform_timer_callback t_callback;
static lk_time_t periodic_interval;

static volatile uint ticks = 0;

static enum handler_return platform_tick(void *arg)
{
	ticks++;

	if (t_callback) {
		return t_callback(arg, current_time());
	} else {
		return INT_NO_RESCHEDULE;
	}
}

status_t
platform_set_periodic_timer(platform_timer_callback callback,
			 void *arg, lk_time_t interval)
{
	enter_critical_section();

    LTRACEF("callback %p, arg %p, interval %lu\n", callback, arg, interval);

    t_callback = callback;

    periodic_interval = interval;

    uint32_t tick_count = periodic_interval * platform_tick_rate() / 1000;

    writel(tick_count, DGT_MATCH_VAL);
	writel(0, DGT_CLEAR);
	writel(DGT_ENABLE_EN | DGT_ENABLE_CLR_ON_MATCH_EN, DGT_ENABLE);

	register_int_handler(INT_DEBUG_TIMER_EXP, &platform_tick, NULL);
	unmask_interrupt(INT_DEBUG_TIMER_EXP);

    exit_critical_section();

    return NO_ERROR;
}

lk_bigtime_t current_time_hires(void)
{
	lk_bigtime_t time;

    time = ticks * periodic_interval * 1000ULL;

    return time;
}

lk_time_t current_time(void)
{
	lk_time_t time;

    time = ticks * periodic_interval;

    return time;
}

static void wait_for_timer_op(void)
{
	while (readl(SPSS_TIMER_STATUS) & SPSS_TIMER_STATUS_DGT_EN) ;
}

void platform_uninit_timer(void)
{
	writel(0, DGT_ENABLE);
	wait_for_timer_op();
	writel(0, DGT_CLEAR);
	wait_for_timer_op();
}

void platform_timer_mdelay(unsigned msecs)
{
	msecs *= 33;

	writel(0, GPT_CLEAR);
	writel(0, GPT_ENABLE);
	while (readl(GPT_COUNT_VAL) != 0) ;

	writel(GPT_ENABLE_EN, GPT_ENABLE);
	while (readl(GPT_COUNT_VAL) < msecs) ;

	writel(0, GPT_ENABLE);
	writel(0, GPT_CLEAR);
}

void platform_timer_udelay(unsigned usecs)
{
	usecs = (usecs * 33 + 1000 - 33) / 1000;

	writel(0, GPT_CLEAR);
	writel(0, GPT_ENABLE);
	while (readl(GPT_COUNT_VAL) != 0) ;

	writel(GPT_ENABLE_EN, GPT_ENABLE);
	while (readl(GPT_COUNT_VAL) < usecs) ;

	writel(0, GPT_ENABLE);
	writel(0, GPT_CLEAR);
}
