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
#include <arch.h>
#include <err.h>
#include <debug.h>
#include <printf.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <target.h>
#include <lk/init.h>
#include <platform.h>
#include <kernel/vm.h>
#include <lib/android.h>
#include <app/fastboot.h>

#if WITH_LIB_UEFI
#include <uefi/pe32.h>
#endif

#if WITH_LIB_CONSOLE
#include <lib/console.h>
#endif

#if WITH_LIB_BIO
#include <lib/bio.h>
#endif

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

static void cmd_oem_lkshell(const char *arg, void *data, unsigned sz) {
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

#if WITH_LIB_BIO
static off_t fastboot_dump_partition_callback(void* pdata, void* buf, off_t offset, off_t len) {
	bdev_t* dev = (bdev_t*)pdata;

	ssize_t rc = bio_read(dev, buf, offset, len);
	if (rc!=len) {
		return -1;
	}

	return len;
}

static void cmd_oem_dump_partition(const char *arg, void *unused, unsigned sz)
{
	bdev_t* dev = bio_open_by_label(arg+1);
	if(!dev) {
		fastboot_fail("invalid partition");
		return;
	}

	if(fastboot_send_data_cb(&fastboot_dump_partition_callback, dev, dev->size)) {
		dprintf(CRITICAL, "Error sending Data\n");
		fastboot_fail("unknown error");
	} else fastboot_okay("");

	bio_close(dev);
}
#endif

static void cmd_reboot(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
	platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_RESET);
}

static void cmd_reboot_bootloader(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
	platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_BOOTLOADER);
}

static void cmd_oem_reboot_recovery(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
	platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_RECOVERY);
}

static void cmd_boot(const char *arg, void *data, unsigned sz)
{
	android_parsed_bootimg_t parsed;

	// parse bootimg
	android_parse_bootimg(data, sz, &parsed);
	dprintf(SPEW, "%s: kernel=0x%08x(0x%08x) ramdisk=0x%08x(0x%08x) second=0x%08x(0x%08x) tags=0x%08x(0x%08x) cmdline=[%s]\n", __func__, 
		parsed.hdr->kernel_addr, parsed.hdr->kernel_size,
		parsed.hdr->ramdisk_addr, parsed.hdr->ramdisk_size,
		parsed.hdr->second_addr, parsed.hdr->second_size,
		parsed.hdr->tags_addr, parsed.hdr->dt_size,
		parsed.cmdline);

#if WITH_LIB_UEFI
	// try to load as PE image
	int rc = peloader_load(parsed.kernel, parsed.hdr->kernel_size);
	if(rc!=ERR_INVALID_ARGS) {
		if(rc==NO_ERROR) fastboot_okay("");
		else fastboot_fail("failed to load PE image");
		return;
	}
#endif

	// TODO: second loader support
	if(parsed.hdr->second_size>0) {
		fastboot_fail("second loaders are not supported");
		return;
	}

	// allocate memory
	if(android_allocate_boot_memory(&parsed)) {
		fastboot_fail("error allocating memory");
		goto err;
	}
	dprintf(SPEW, "%s: kernel=%p(0x%08lx) ramdisk=%p(0x%08lx) second=%p(0x%08lx) tags=%p(0x%08lx)\n", __func__, 
		parsed.kernel_loaded, parsed.kernel_loaded?kvaddr_to_paddr(parsed.kernel_loaded):0,
		parsed.ramdisk_loaded, parsed.ramdisk_loaded?kvaddr_to_paddr(parsed.ramdisk_loaded):0,
		parsed.second_loaded, parsed.second_loaded?kvaddr_to_paddr(parsed.second_loaded):0,
		parsed.tags_loaded, parsed.tags_loaded?kvaddr_to_paddr(parsed.tags_loaded):0);

	// generate tags
	if(android_add_board_info(&parsed)) {
		fastboot_fail("error generating tags");
		goto err;
	}

	// load images
	if(android_load_images(&parsed)) {
		fastboot_fail("error loading images");
		goto err;
	}

	// boot
	fastboot_okay("");
	arch_chain_load(parsed.kernel_loaded, 0, parsed.machtype, kvaddr_to_paddr(parsed.tags_loaded), 0);

err:
	android_free_parsed_bootimg(&parsed);
}


static void fastboot_commands_init(uint level)
{
#if WITH_LIB_CONSOLE
	fastboot_register_desc("oem lkshell", "Run commands in the LK shell", cmd_oem_lkshell);
#endif
#if WITH_LIB_BIO
	fastboot_register_desc("oem dump-partition", "download partition data", cmd_oem_dump_partition);
#endif

	fastboot_register("reboot", cmd_reboot);
	fastboot_register("reboot-bootloader", cmd_reboot_bootloader);
	fastboot_register_desc("oem reboot-recovery", "Reboot to recovery mode", cmd_oem_reboot_recovery);
	fastboot_register("boot", cmd_boot);
}

LK_INIT_HOOK(fastboot, &fastboot_commands_init, LK_INIT_LEVEL_THREADING);
