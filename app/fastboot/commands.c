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
#include <lib/sparse.h>
#include <lib/android.h>
#include <app/aboot.h>
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

#define ISPART(x) (!strcmp((pname), (x)))
#define PRINT_PARSED_INFO(x) \
	dprintf(SPEW, "%s: kernel=0x%08x(0x%08x) ramdisk=0x%08x(0x%08x) second=0x%08x(0x%08x) tags=0x%08x(0x%08x) cmdline=[%s]\n", __func__, \
		(x).hdr->kernel_addr, (x).hdr->kernel_size, \
		(x).hdr->ramdisk_addr, (x).hdr->ramdisk_size, \
		(x).hdr->second_addr, (x).hdr->second_size, \
		(x).hdr->tags_addr, (x).hdr->dt_size, \
		(x).cmdline);

#if WITH_LIB_CONSOLE
static char fastboot_catcher_buf[MAX_RSP_SIZE];
static unsigned fastboot_catcher_len = 0;

static void print_cb(print_callback_t *cb, const char *str, size_t len) {
	size_t i;
	for(i=0; i<len; i++) {
		char c = str[i];

		if(c!='\n' && c!='\r')
			fastboot_catcher_buf[fastboot_catcher_len++] = c;

		if(fastboot_catcher_len==sizeof(fastboot_catcher_buf)-1-4 || c=='\n' || c=='\r') {
			fastboot_catcher_buf[fastboot_catcher_len] = 0;
			fastboot_info(fastboot_catcher_buf);
			fastboot_catcher_len = 0;
		}
	}
}

static print_callback_t printcb_data =  {
	.print = print_cb
};

static void cmd_oem_lkshell(const char *arg, void *data, unsigned sz) {
	fastboot_catcher_len = 0;
	register_print_callback(&printcb_data);
	int rc = console_run_command(arg);
	unregister_print_callback(&printcb_data);

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

static void cmd_flash(const char *arg, void *data, unsigned sz)
{
	// duplicate arg for strtok
	char tmp[strlen(arg)+1];
	strcpy(tmp, arg);

	// get partition name
	char *saveptr = NULL;
	char *pch = strtok_r(tmp, ":", &saveptr);
	char pname[strlen(pch)];
	strcpy(pname, pch);

	// get LUN
	pch = strtok_r(NULL, ":", &saveptr);
	if(pch) {
		// UFS LUN
		fastboot_fail("Unsupported");
	}

	// flash partition table
	if(ISPART("partition")) {
		fastboot_fail("Unsupported");
		return;
	}

	// open device
	bdev_t* dev = bio_open_by_label(pname);
	if(!dev) {
		fastboot_fail("partition not found");
		return;
	}

	// check size
	if(sz > dev->size) {
		fastboot_fail("image is too large");
		goto finish;
	}

	// sparse flash
	if(sparse_validate(data, sz)) {
		if(sparse_write_to_device(dev, data, sz)) {
			goto finish;
		}
	}

	// normal flash
	else {
		// validate data
		if(ISPART("boot") || ISPART("boot1") || ISPART("recovery")) {
			// parse bootimg
			if(!android_is_bootimg(data, sz)) {
				fastboot_fail("not a boot image");
				goto finish;
			}
		}

		// write
		if(bio_write(dev, data, 0, sz)!=(ssize_t)sz) {
			fastboot_fail("write failure");
			goto finish;
		}

		// send OK
		fastboot_okay("");
	}

finish:
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
	platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_UPDATE);
}

static void cmd_boot(const char *arg, void *data, unsigned sz)
{
	android_parsed_bootimg_t parsed;

	// parse bootimg
	if(android_parse_bootimg(data, sz, &parsed)) {
		fastboot_fail("error parsing bootimg");
		return;
	}
	PRINT_PARSED_INFO(parsed);

#if WITH_LIB_UEFI
	// create devices for kernel and ramdisk
	bdev_t* kernel_dev = create_membdev("", parsed.kernel, parsed.hdr->kernel_size, false);
	bdev_t* ramdisk_dev = NULL;
	if(parsed.hdr->ramdisk_size>0)
		ramdisk_dev = create_membdev("", parsed.ramdisk, parsed.hdr->ramdisk_size, false);

	// try to load as PE image
	int rc = peloader_load(kernel_dev, ramdisk_dev);
	if(rc!=ERR_INVALID_ARGS) {
		if(rc==NO_ERROR) fastboot_okay("");
		else fastboot_fail("failed to load PE image");
		goto done;
	}
#endif

	// boot
	android_do_boot(&parsed, true);

done:
	if(ramdisk_dev)
		delete_membdev(ramdisk_dev);
	delete_membdev(kernel_dev);
}

static void cmd_continue(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
	aboot_boot(HALT_REASON_UNKNOWN);
}


static void fastboot_commands_init(uint level)
{
#if WITH_LIB_CONSOLE
	fastboot_register_desc("oem lkshell", "Run commands in the LK shell", cmd_oem_lkshell);
#endif
#if WITH_LIB_BIO
	fastboot_register_desc("oem dump-partition", "download partition data", cmd_oem_dump_partition);
	fastboot_register("flash:", cmd_flash);
#endif

	fastboot_register("reboot", cmd_reboot);
	fastboot_register("reboot-bootloader", cmd_reboot_bootloader);
	fastboot_register_desc("oem reboot-recovery", "Reboot to recovery mode", cmd_oem_reboot_recovery);
	fastboot_register("boot", cmd_boot);
	fastboot_register("continue", cmd_continue);
}

LK_INIT_HOOK(fastboot, &fastboot_commands_init, LK_INIT_LEVEL_THREADING);
