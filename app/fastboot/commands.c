/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
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

#include <app.h>
#include <debug.h>
#include <printf.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <target.h>
#include <dev/udc.h>
#include <platform.h>
#include <kernel/vm.h>
#include <kernel/event.h>
#include <kernel/thread.h>
#include <app/fastboot.h>
#include <lib/console.h>
#include <lk/init.h>

#if WITH_LIB_CONSOLE
static char fastboot_catcher_buf[MAX_RSP_SIZE];
static unsigned fastboot_catcher_len = 0;

static void fastboot_debug_catcher(char c) {
	if(c!='\n' && c!='\r')
		fastboot_catcher_buf[fastboot_catcher_len++] = c;

	if(fastboot_catcher_len==sizeof(fastboot_catcher_buf)-1-4 || c=='\n' || c=='\r') {
		fastboot_catcher_buf[fastboot_catcher_len] = 0;
		fastboot_info(fastboot_catcher_buf);
		fastboot_catcher_len = 0;
	}
}

void cmd_lkshell(const char *arg, void *data, unsigned sz) {
	fastboot_catcher_len = 0;
	debug_catcher_add(&fastboot_debug_catcher);
	int rc = console_run_command(arg);
	debug_catcher_remove(&fastboot_debug_catcher);

	if(rc) {
		snprintf(fastboot_catcher_buf, sizeof(fastboot_catcher_buf), "Command Failed. return code: %d", rc);
		fastboot_fail(fastboot_catcher_buf);
	}
	else fastboot_okay("");
}
#endif

static void fastboot_commands_init(uint level)
{
	fastboot_register("oem lkshell", cmd_lkshell);
}

LK_INIT_HOOK(virtio, &fastboot_commands_init, LK_INIT_LEVEL_THREADING);
