/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2009-2014, The Linux Foundation. All rights reserved.
 * Copyright (c) 2011-2014, Xiaomi Corporation. All rights reserved.
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

#include <app.h>
#include <debug.h>
#include <assert.h>
#include <printf.h>
#include <ctype.h>
#include <arch/arm.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <kernel/thread.h>
#include <arch/ops.h>

#include <dev/flash.h>
#include <lib/ptable.h>
#include <dev/keys.h>
#include <dev/fbcon.h>
#include <baseband.h>
#include <target.h>
#include <mmc.h>
#include <partition_parser.h>
#include <platform.h>
#include <crypto_hash.h>
#include <malloc.h>
#include <boot_stats.h>
#include <sha.h>
#include <platform/iomap.h>
#include <platform/msm_shared.h>
#include <boot_device.h>
#include <boot_verifier.h>
#include <image_verify.h>
#include <lib/bio.h>
#include <lib/sysparam.h>
#if WITH_APP_DISPLAY_SERVER
#include <app/display_server.h>
#endif
#include <app/aboot.h>
#include <api_public.h>
#include "uboot_api/api_private.h"
#include "grub.h"

#if DEVICE_TREE
#include <libfdt.h>
#include <dev_tree.h>
#endif

#if BOOT_2NDSTAGE
#include <2ndstage_tools.h>
#endif

#include "image_verify.h"
#include "recovery.h"
#include "bootimg.h"
#include "fastboot.h"
#include "sparse_format.h"
#include "mmc.h"
#include "board.h"
#include "scm.h"

extern  bool target_use_signed_kernel(void);
extern void platform_uninit(void);
extern void target_uninit(void);
extern int get_target_boot_params(const char *cmdline, const char *part,
				  char *buf, int buflen);
extern void mmc_set_lun(uint8_t lun);
extern uint32_t mmc_page_size(void);
extern uint32_t mmc_get_device_blocksize(void);

static int aboot_save_boot_hash_mmc(uint32_t image_addr, uint32_t image_size);
static void bootcount_aboot_inc(void);

/* fastboot command function pointer */
typedef void (*fastboot_cmd_fn) (const char *, void *, unsigned);

struct fastboot_cmd_desc {
	char * name;
	fastboot_cmd_fn cb;
};

#define EXPAND(NAME) #NAME

#ifdef MEMBASE
#define EMMC_BOOT_IMG_HEADER_ADDR (0xFF000+(MEMBASE))
#else
#define EMMC_BOOT_IMG_HEADER_ADDR 0xFF000
#endif

#ifndef MEMSIZE
#define MEMSIZE 1024*1024
#endif

#define MAX_TAGS_SIZE   1024

/* make 4096 as default size to ensure EFS,EXT4's erasing */
#define DEFAULT_ERASE_SIZE  4096
#define MAX_PANEL_ID_LEN 64

#define UBI_MAGIC      "UBI#"
#define DISPLAY_DEFAULT_PREFIX "mdss_mdp"
#define UBI_MAGIC_SIZE 0x04
#define BOOT_DEV_MAX_LEN  64

#define IS_ARM64(ptr) (ptr->magic_64 == KERNEL64_HDR_MAGIC) ? true : false

#define ADD_OF(a, b) (UINT_MAX - b > a) ? (a + b) : UINT_MAX

#if UFS_SUPPORT || USE_BOOTDEV_CMDLINE
static const char *emmc_cmdline = " androidboot.bootdevice=";
#else
static const char *emmc_cmdline = " androidboot.emmc=true";
#endif
static const char *usb_sn_cmdline = " androidboot.serialno=";
static const char *androidboot_mode = " androidboot.mode=";
static const char *alarmboot_cmdline = " androidboot.alarmboot=true";
static const char *loglevel         = " quiet";
static const char *battchg_pause = " androidboot.mode=charger";
static const char *auth_kernel = " androidboot.authorized_kernel=true";
static const char *secondary_gpt_enable = " gpt";

static const char *baseband_apq     = " androidboot.baseband=apq";
static const char *baseband_msm     = " androidboot.baseband=msm";
static const char *baseband_csfb    = " androidboot.baseband=csfb";
static const char *baseband_svlte2a = " androidboot.baseband=svlte2a";
static const char *baseband_mdm     = " androidboot.baseband=mdm";
static const char *baseband_mdm2    = " androidboot.baseband=mdm2";
static const char *baseband_sglte   = " androidboot.baseband=sglte";
static const char *baseband_dsda    = " androidboot.baseband=dsda";
static const char *baseband_dsda2   = " androidboot.baseband=dsda2";
static const char *baseband_sglte2  = " androidboot.baseband=sglte2";
static const char *warmboot_cmdline = " qpnp-power-on.warm_boot=1";

#if BOOT_2NDSTAGE
static const char *sndstage_cmdline = " multiboot.2ndstage=1";
#endif

#if WITH_XIAOMI_DUALBOOT
/* keep them same length */
static const char *boot0_cmdline = " syspart=system ";
static const char *boot1_cmdline = " syspart=system1";
#endif

enum bootmode bootmode = BOOTMODE_AUTO;
enum bootmode bootmode_normal = BOOTMODE_AUTO;
static unsigned page_size = 0;
static unsigned page_mask = 0;
static char ffbm_mode_string[FFBM_MODE_BUF_SIZE];
static bool boot_into_ffbm;
static char target_boot_params[64];
static bool boot_reason_alarm;

/* Assuming unauthorized kernel image by default */
static int auth_kernel_img = 0;

struct atag_ptbl_entry
{
	char name[16];
	unsigned offset;
	unsigned size;
	unsigned flags;
};

/*
 * Partition info, required to be published
 * for fastboot
 */
struct getvar_partition_info {
	const char part_name[MAX_GPT_NAME_SIZE]; /* Partition name */
	char getvar_size[MAX_GET_VAR_NAME_SIZE]; /* fastboot get var name for size */
	char getvar_type[MAX_GET_VAR_NAME_SIZE]; /* fastboot get var name for type */
	char size_response[MAX_RSP_SIZE];        /* fastboot response for size */
	char type_response[MAX_RSP_SIZE];        /* fastboot response for type */
};

/*
 * Right now, we are publishing the info for only
 * three partitions
 */
struct getvar_partition_info part_info[] =
{
	{ "system"  , "partition-size:", "partition-type:", "", "ext4" },
	{ "userdata", "partition-size:", "partition-type:", "", "ext4" },
	{ "cache"   , "partition-size:", "partition-type:", "", "ext4" },
};

char max_download_size[MAX_RSP_SIZE];
char charger_screen_enabled[MAX_RSP_SIZE];
char sn_buf[13];
char panel_display_mode[MAX_RSP_SIZE];
char use_splash_partition[MAX_RSP_SIZE];
char screen_resolution[MAX_RSP_SIZE];

extern int emmc_recovery_init(void);

#if NO_KEYPAD_DRIVER
extern int fastboot_trigger(void);
#endif

void update_ker_tags_rdisk_addr(struct boot_img_hdr *hdr, bool is_arm64)
{
	/* overwrite the destination of specified for the project */
#ifdef ABOOT_IGNORE_BOOT_HEADER_ADDRS
	if (is_arm64)
		hdr->kernel_addr = ABOOT_FORCE_KERNEL64_ADDR;
	else
		hdr->kernel_addr = ABOOT_FORCE_KERNEL_ADDR;
	hdr->ramdisk_addr = ABOOT_FORCE_RAMDISK_ADDR;
	hdr->tags_addr = ABOOT_FORCE_TAGS_ADDR;
#endif
}

static void ptentry_to_tag(unsigned **ptr, struct ptentry *ptn)
{
	struct atag_ptbl_entry atag_ptn;

	memcpy(atag_ptn.name, ptn->name, 16);
	atag_ptn.name[15] = '\0';
	atag_ptn.offset = ptn->start;
	atag_ptn.size = ptn->length;
	atag_ptn.flags = ptn->flags;
	memcpy(*ptr, &atag_ptn, sizeof(struct atag_ptbl_entry));
	*ptr += sizeof(struct atag_ptbl_entry) / sizeof(unsigned);
}

char *update_cmdline(const char * cmdline)
{
	int cmdline_len = 0;
	int have_cmdline = 0;
	char *cmdline_final = NULL;
	int pause_at_bootup = 0;
	bool warm_boot = false;
	bool gpt_exists = partition_gpt_exists();
	int have_target_boot_params = 0;
	char *boot_dev_buf = NULL;

	const char* __display_panel = sysparm_read_str("display_panel");
	char* display_panel = __display_panel?strdup(__display_panel):NULL;

	if (cmdline && cmdline[0]) {
		cmdline_len = strlen(cmdline);
		have_cmdline = 1;
	}
	if (target_is_emmc_boot()) {
		cmdline_len += strlen(emmc_cmdline);
#if UFS_SUPPORT || USE_BOOTDEV_CMDLINE
		boot_dev_buf = (char *) malloc(sizeof(char) * BOOT_DEV_MAX_LEN);
		ASSERT(boot_dev_buf);
		platform_boot_dev_cmdline(boot_dev_buf);
		cmdline_len += strlen(boot_dev_buf);
#endif
	}

	cmdline_len += strlen(usb_sn_cmdline);
	cmdline_len += strlen(sn_buf);

	if (bootmode==BOOTMODE_RECOVERY && gpt_exists)
		cmdline_len += strlen(secondary_gpt_enable);

	if (boot_into_ffbm) {
		cmdline_len += strlen(androidboot_mode);
		cmdline_len += strlen(ffbm_mode_string);
		/* reduce kernel console messages to speed-up boot */
		cmdline_len += strlen(loglevel);
	} else if (boot_reason_alarm) {
		cmdline_len += strlen(alarmboot_cmdline);
	} else if (sysparam_read_bool("charger_screen") &&
			target_pause_for_battery_charge()) {
		pause_at_bootup = 1;
		cmdline_len += strlen(battchg_pause);
	}

#if WITH_XIAOMI_DUALBOOT
	/* bootX_cmdline is a mandatory parameter
	 * it should be there in any case
	 */
	int needs_xiaomi_syspart = 0;
	if(!strstr(cmdline, "syspart=")) {
		needs_xiaomi_syspart = 1;
		cmdline_len += strlen(boot0_cmdline);
	}
#endif

#if BOOT_2NDSTAGE
	cmdline_len += strlen(sndstage_cmdline);
#endif

	if(target_use_signed_kernel() && auth_kernel_img) {
		cmdline_len += strlen(auth_kernel);
	}

	if (get_target_boot_params(cmdline, bootmode==BOOTMODE_RECOVERY ? "recoveryfs" :
								 "system",
				   target_boot_params,
				   sizeof(target_boot_params)) == 0) {
		have_target_boot_params = 1;
		cmdline_len += strlen(target_boot_params);
	}

	/* Determine correct androidboot.baseband to use */
	switch(target_baseband())
	{
		case BASEBAND_APQ:
			cmdline_len += strlen(baseband_apq);
			break;

		case BASEBAND_MSM:
			cmdline_len += strlen(baseband_msm);
			break;

		case BASEBAND_CSFB:
			cmdline_len += strlen(baseband_csfb);
			break;

		case BASEBAND_SVLTE2A:
			cmdline_len += strlen(baseband_svlte2a);
			break;

		case BASEBAND_MDM:
			cmdline_len += strlen(baseband_mdm);
			break;

		case BASEBAND_MDM2:
			cmdline_len += strlen(baseband_mdm2);
			break;

		case BASEBAND_SGLTE:
			cmdline_len += strlen(baseband_sglte);
			break;

		case BASEBAND_SGLTE2:
			cmdline_len += strlen(baseband_sglte2);
			break;

		case BASEBAND_DSDA:
			cmdline_len += strlen(baseband_dsda);
			break;

		case BASEBAND_DSDA2:
			cmdline_len += strlen(baseband_dsda2);
			break;
	}

	if (cmdline) {
		if ((strstr(cmdline, DISPLAY_DEFAULT_PREFIX) == NULL) &&
			target_display_panel_node(display_panel,
			display_panel, MAX_PANEL_ID_LEN) &&
			strlen(display_panel)) {
			cmdline_len += strlen(display_panel);
		}
	}

	if (target_warm_boot()) {
		warm_boot = true;
		cmdline_len += strlen(warmboot_cmdline);
	}

	if (cmdline_len > 0) {
		const char *src;
		char *dst;

		cmdline_final = (char*) malloc((cmdline_len + 4) & (~3));
		ASSERT(cmdline_final != NULL);
		dst = cmdline_final;

		/* Save start ptr for debug print */
		if (have_cmdline) {
			src = cmdline;
			while ((*dst++ = *src++));
		}
		if (target_is_emmc_boot()) {
			src = emmc_cmdline;
			if (have_cmdline) --dst;
			have_cmdline = 1;
			while ((*dst++ = *src++));
#if UFS_SUPPORT  || USE_BOOTDEV_CMDLINE
			src = boot_dev_buf;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
#endif
		}

		src = usb_sn_cmdline;
		if (have_cmdline) --dst;
		have_cmdline = 1;
		while ((*dst++ = *src++));
		src = sn_buf;
		if (have_cmdline) --dst;
		have_cmdline = 1;
		while ((*dst++ = *src++));
		if (warm_boot) {
			if (have_cmdline) --dst;
			src = warmboot_cmdline;
			while ((*dst++ = *src++));
		}

		if (bootmode==BOOTMODE_RECOVERY && gpt_exists) {
			src = secondary_gpt_enable;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
		}

		if (boot_into_ffbm) {
			src = androidboot_mode;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
			src = ffbm_mode_string;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
			src = loglevel;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
		} else if (boot_reason_alarm) {
			src = alarmboot_cmdline;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
		} else if (pause_at_bootup) {
			src = battchg_pause;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
		}

#if WITH_XIAOMI_DUALBOOT
		if(needs_xiaomi_syspart) {
			if (bootmode == BOOTMODE_SECOND) {
				src = boot1_cmdline;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
			} else {
				src = boot0_cmdline;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
			}
		}
#endif

#if BOOT_2NDSTAGE
		src = sndstage_cmdline;
		if (have_cmdline) --dst;
		while ((*dst++ = *src++));
#endif

		if(target_use_signed_kernel() && auth_kernel_img) {
			src = auth_kernel;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
		}

		switch(target_baseband())
		{
			case BASEBAND_APQ:
				src = baseband_apq;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_MSM:
				src = baseband_msm;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_CSFB:
				src = baseband_csfb;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_SVLTE2A:
				src = baseband_svlte2a;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_MDM:
				src = baseband_mdm;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_MDM2:
				src = baseband_mdm2;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_SGLTE:
				src = baseband_sglte;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_SGLTE2:
				src = baseband_sglte2;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_DSDA:
				src = baseband_dsda;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;

			case BASEBAND_DSDA2:
				src = baseband_dsda2;
				if (have_cmdline) --dst;
				while ((*dst++ = *src++));
				break;
		}

		if (strlen(display_panel)) {
			src = display_panel;
			if (have_cmdline) --dst;
			while ((*dst++ = *src++));
		}

		if (have_target_boot_params) {
			if (have_cmdline) --dst;
			src = target_boot_params;
			while ((*dst++ = *src++));
		}
	}


	if (boot_dev_buf)
		free(boot_dev_buf);

	dprintf(INFO, "cmdline: %s\n", cmdline_final ? cmdline_final : "");

#if BOOT_2NDSTAGE
	char *extended_cmdline = sndstage_extend_cmdline((char*)cmdline_final);
	free(cmdline_final);
	cmdline_final = extended_cmdline;

	dprintf(INFO, "cmdline_extended: %s\n", cmdline_final);
#endif

	return cmdline_final;
}

#if !DEVICE_TREE || DEVICE_TREE_FALLBACK
unsigned *atag_core(unsigned *ptr)
{
	/* CORE */
	*ptr++ = 2;
	*ptr++ = 0x54410001;

	return ptr;

}

unsigned *atag_ramdisk(unsigned *ptr, void *ramdisk,
							   unsigned ramdisk_size)
{
	if (ramdisk_size) {
		*ptr++ = 4;
		*ptr++ = 0x54420005;
		*ptr++ = (unsigned)ramdisk;
		*ptr++ = ramdisk_size;
	}

	return ptr;
}

unsigned *atag_ptable(unsigned **ptr_addr)
{
	int i;
	struct ptable *ptable;

	if ((ptable = flash_get_ptable()) && (ptable->count != 0)) {
		*(*ptr_addr)++ = 2 + (ptable->count * (sizeof(struct atag_ptbl_entry) /
							sizeof(unsigned)));
		*(*ptr_addr)++ = 0x4d534d70;
		for (i = 0; i < ptable->count; ++i)
			ptentry_to_tag(ptr_addr, ptable_get(ptable, i));
	}

	return (*ptr_addr);
}

unsigned *atag_cmdline(unsigned *ptr, const char *cmdline)
{
	int cmdline_length = 0;
	int n;
	char *dest;

	cmdline_length = strlen((const char*)cmdline);
	n = (cmdline_length + 4) & (~3);

	*ptr++ = (n / 4) + 2;
	*ptr++ = 0x54410009;
	dest = (char *) ptr;
	while ((*dest++ = *cmdline++));
	ptr += (n / 4);

	return ptr;
}

unsigned *atag_end(unsigned *ptr)
{
	/* END */
	*ptr++ = 0;
	*ptr++ = 0;

	return ptr;
}

void generate_atags(unsigned *ptr, const char *cmdline,
                    void *ramdisk, unsigned ramdisk_size)
{

	ptr = atag_core(ptr);
	ptr = atag_ramdisk(ptr, ramdisk, ramdisk_size);
	ptr = target_atag_mem(ptr);

	/* Skip NAND partition ATAGS for eMMC boot */
	if (!target_is_emmc_boot()){
		ptr = atag_ptable(&ptr);
	}

	ptr = atag_cmdline(ptr, cmdline);
	ptr = atag_end(ptr);
}
#endif

typedef void entry_func_ptr(unsigned, unsigned, unsigned*);
void boot_linux(void *kernel, unsigned *tags,
		const char *cmdline, unsigned machtype,
		void *ramdisk, unsigned ramdisk_size, int use_device_tree)
{
	char *final_cmdline;
#if DEVICE_TREE
	int ret = 0;
#endif

	void (*entry)(unsigned, unsigned, unsigned*) = (entry_func_ptr*)(PA((addr_t)kernel));
	uint32_t tags_phys = PA((addr_t)tags);
	struct kernel64_hdr *kptr = (struct kernel64_hdr*)kernel;

	ramdisk = (void *)PA((addr_t)ramdisk);

	final_cmdline = update_cmdline(cmdline);

#if DEVICE_TREE
	if(use_device_tree) {
		dprintf(INFO, "Updating device tree: start\n");

		/* Update the Device Tree */
		ret = update_device_tree((void *)tags, final_cmdline, ramdisk, ramdisk_size);
		if(ret)
		{
			dprintf(CRITICAL, "ERROR: Updating Device Tree Failed \n");
			ASSERT(0);
		}
		dprintf(INFO, "Updating device tree: done\n");
	}
	else
#endif
	{
#if !DEVICE_TREE || DEVICE_TREE_FALLBACK
		/* Generating the Atags */
		generate_atags(tags, final_cmdline, ramdisk, ramdisk_size);
#endif
	}

	free(final_cmdline);

#if VERIFIED_BOOT
	/* Write protect the device info */
	if (mmc_write_protect("devinfo", 1))
	{
		dprintf(INFO, "Failed to write protect dev info\n");
		ASSERT(0);
	}
#endif

	/* Perform target specific cleanup */
	target_uninit();

	/* Turn off splash screen if enabled */
#if DISPLAY_SPLASH_SCREEN
	target_display_shutdown();
#endif


	dprintf(INFO, "booting linux @ %p, ramdisk @ %p (%d), tags/device tree @ %p\n",
		entry, ramdisk, ramdisk_size, (void *)tags_phys);

#if WITH_APP_DISPLAY_SERVER
	display_server_stop();
#endif

	enter_critical_section();

	/* do any platform specific cleanup before kernel entry */
	platform_uninit();

	arch_disable_cache(UCACHE);

#if ARM_WITH_MMU
	arch_disable_mmu();
#endif
	bs_set_timestamp(BS_KERNEL_ENTRY);

	if (IS_ARM64(kptr))
		/* Jump to a 64bit kernel */
		scm_elexec_call((paddr_t)kernel, tags_phys);
	else
		/* Jump to a 32bit kernel */
		entry(0, machtype, (unsigned*)tags_phys);
}

/* Function to check if the memory address range falls within the aboot
 * boundaries.
 * start: Start of the memory region
 * size: Size of the memory region
 */
int check_aboot_addr_range_overlap(uint32_t start, uint32_t size)
{
	/* Check for boundary conditions. */
	if ((UINT_MAX - start) < size)
		return -1;

	/* Check for memory overlap. */
	if ((start < MEMBASE) && ((start + size) <= MEMBASE))
		return 0;
	else if (start >= (MEMBASE + MEMSIZE))
		return 0;
	else
		return -1;
}

#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))

BUF_DMA_ALIGN(buf, BOOT_IMG_MAX_PAGE_SIZE); //Equal to max-supported pagesize
#if DEVICE_TREE
BUF_DMA_ALIGN(dt_buf, BOOT_IMG_MAX_PAGE_SIZE);
#endif

#if WITH_XIAOMI_DUALBOOT
bool is_dualboot_supported(void)
{
	return (partition_get_index("boot1")!=INVALID_PTN)
		&& (partition_get_index("system1")!=INVALID_PTN)
		&& (partition_get_index("modem1")!=INVALID_PTN);
}
#define DUALBOOT_OFFSET_SECTORS 8	/* 4 pages, for recovery_message */
enum bootmode get_dualboot_mode(void)
{
	int index = INVALID_PTN;
	unsigned long long ptn = 0;
	struct dual_boot_message *dualboot_msg;
	enum bootmode rc = BOOTMODE_NORMAL;

	index = partition_get_index("misc");
	ptn = partition_get_offset(index);
	if (ptn == 0) {
		dprintf(CRITICAL, "ERROR: No misc found\n");
		goto out;
	}

	ptn += DUALBOOT_OFFSET_SECTORS * MMC_BOOT_RD_BLOCK_LEN;
	memset(buf, 0, sizeof(buf));
	if (mmc_read(ptn, (unsigned int *)buf, MMC_BOOT_RD_BLOCK_LEN)) {
		dprintf(CRITICAL, "ERROR: Cannot read misc\n");
		goto out;
	}

	dualboot_msg = (struct dual_boot_message *)buf;

	if (!strncmp(dualboot_msg->command, "boot-system", 11)) {
		if (dualboot_msg->command[11] == '1')
			rc = BOOTMODE_SECOND;
	} else
		dprintf(CRITICAL, "WARNING: dual_boot_message not set\n");

out:
	return rc;
}

void set_dualboot_mode(enum bootmode mode)
{
	int index = INVALID_PTN;
	unsigned long long ptn = 0;
	struct dual_boot_message dualboot_msg;

	if (mode==BOOTMODE_SECOND) {
		snprintf(dualboot_msg.command, 32, "boot-system1");
	}
	else if(mode==BOOTMODE_NORMAL) {
		snprintf(dualboot_msg.command, 32, "boot-system");
	}
	else {
		dprintf(CRITICAL, "Invalid Dualboot bootmode '%s'\n", strbootmode(mode));
	}

	index = partition_get_index("misc");
	ptn = partition_get_offset(index);
	if (ptn == 0) {
		dprintf(CRITICAL, "ERROR: No misc found\n");
		goto out;
	}

	ptn += DUALBOOT_OFFSET_SECTORS * MMC_BOOT_RD_BLOCK_LEN;
	memset(buf, 0, sizeof(buf));
	if (mmc_write(ptn, MMC_BOOT_RD_BLOCK_LEN, (unsigned int *)&dualboot_msg)) {
		dprintf(CRITICAL, "ERROR: Cannot read misc\n");
		goto out;
	}

out:
	return;
}
#endif

static void verify_signed_bootimg(uint32_t bootimg_addr, uint32_t bootimg_size)
{
	int ret;
#if IMAGE_VERIF_ALGO_SHA1
	uint32_t auth_algo = CRYPTO_AUTH_ALG_SHA1;
#else
	uint32_t auth_algo = CRYPTO_AUTH_ALG_SHA256;
#endif

	/* Assume device is rooted at this time. */
	sysparam_write_bool("is_tampered", true);

	dprintf(INFO, "Authenticating boot image (%d): start\n", bootimg_size);

#if VERIFIED_BOOT
	if(bootmode==BOOTMODE_RECOVERY)
	{
		ret = boot_verify_image((unsigned char *)bootimg_addr,
				bootimg_size, "recovery");
	}
	else
	{
		ret = boot_verify_image((unsigned char *)bootimg_addr,
				bootimg_size, "boot");
	}
	boot_verify_print_state();
#else
	ret = image_verify((unsigned char *)bootimg_addr,
					   (unsigned char *)(bootimg_addr + bootimg_size),
					   bootimg_size,
					   auth_algo);
#endif
	dprintf(INFO, "Authenticating boot image: done return value = %d\n", ret);

	if (ret)
	{
		/* Authorized kernel */
		sysparam_write_bool("is_tampered", false);
		auth_kernel_img = 1;
	}

#if USE_PCOM_SECBOOT
	set_tamper_flag(sysparam_read_bool("is_tampered"));
#endif

	if(sysparam_read_bool("is_tampered"))
	{
		sysparam_write();
	#ifdef TZ_TAMPER_FUSE
		set_tamper_fuse_cmd();
	#endif
	#ifdef ASSERT_ON_TAMPER
		dprintf(CRITICAL, "Device is tampered. Asserting..\n");
		ASSERT(0);
	#endif
	}

#if VERIFIED_BOOT
	if(boot_verify_get_state() == RED)
	{
		if(bootmode!=BOOTMODE_RECOVERY)
		{
			dprintf(CRITICAL,
					"Device verification failed. Rebooting into recovery.\n");
			reboot_device(REBOOT_MODE_RECOVERY);
		}
		else
		{
			dprintf(CRITICAL,
					"Recovery image verification failed. Asserting..\n");
			ASSERT(0);
		}
	}
#endif
}

static bool check_format_bit(void)
{
	bool ret = false;
	int index;
	uint64_t offset;
	struct boot_selection_info *in = NULL;
	char *buf = NULL;

	index = partition_get_index("bootselect");
	if (index == INVALID_PTN)
	{
		dprintf(INFO, "Unable to locate /bootselect partition\n");
		return ret;
	}
	offset = partition_get_offset(index);
	if(!offset)
	{
		dprintf(INFO, "partition /bootselect doesn't exist\n");
		return ret;
	}
	buf = (char *) memalign(CACHE_LINE, ROUNDUP(page_size, CACHE_LINE));
	ASSERT(buf);
	if (mmc_read(offset, (uint32_t *)buf, page_size))
	{
		dprintf(INFO, "mmc read failure /bootselect %d\n", page_size);
		free(buf);
		return ret;
	}
	in = (struct boot_selection_info *) buf;
	if ((in->signature == BOOTSELECT_SIGNATURE) &&
			(in->version == BOOTSELECT_VERSION)) {
		if ((in->state_info & BOOTSELECT_FORMAT) &&
				!(in->state_info & BOOTSELECT_FACTORY))
			ret = true;
	} else {
		dprintf(CRITICAL, "Signature: 0x%08x or version: 0x%08x mismatched of /bootselect\n",
				in->signature, in->version);
		ASSERT(0);
	}
	free(buf);
	return ret;
}

void boot_verifier_init(void)
{

	uint32_t boot_state;
	/* Check if device unlock */
	if(sysparam_read_bool("is_unlocked"))
	{
		boot_verify_send_event(DEV_UNLOCK);
		boot_verify_print_state();
		dprintf(CRITICAL, "Device is unlocked! Skipping verification...\n");
		return;
	}
	else
	{
		boot_verify_send_event(BOOT_INIT);
	}

	/* Initialize keystore */
	boot_state = boot_verify_keystore_init();
	if(boot_state == YELLOW)
	{
		boot_verify_print_state();
		dprintf(CRITICAL, "Keystore verification failed! Continuing anyways...\n");
	}
}

int boot_linux_from_mmc(void)
{
	struct boot_img_hdr *hdr = (void*) buf;
	struct boot_img_hdr *uhdr;
	unsigned offset = 0;
	int rcode;
	unsigned long long ptn = 0;
	int index = INVALID_PTN;

	unsigned char *image_addr = 0;
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	unsigned imagesize_actual;
	unsigned second_actual __UNUSED = 0;

#if DEVICE_TREE
	struct dt_table *table;
	struct dt_entry dt_entry;
	unsigned dt_table_offset;
	uint32_t dt_actual;
	uint32_t dt_hdr_size;
#endif
	BUF_DMA_ALIGN(kbuf, BOOT_IMG_MAX_PAGE_SIZE);
	struct kernel64_hdr *kptr = (void*) kbuf;

	if (check_format_bit())
		bootmode = BOOTMODE_RECOVERY;

	if (bootmode!=BOOTMODE_RECOVERY) {
		memset(ffbm_mode_string, '\0', sizeof(ffbm_mode_string));
		rcode = get_ffbm(ffbm_mode_string, sizeof(ffbm_mode_string));
		if (rcode <= 0) {
			boot_into_ffbm = false;
			if (rcode < 0)
				dprintf(CRITICAL,"failed to get ffbm cookie");
		} else
			boot_into_ffbm = true;
	} else
		boot_into_ffbm = false;
	uhdr = (struct boot_img_hdr *)EMMC_BOOT_IMG_HEADER_ADDR;
	if (!memcmp(uhdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		dprintf(INFO, "Unified boot method!\n");
		hdr = uhdr;
		goto unified_boot;
	}

	const char* partname = NULL;
	int is_auto = bootmode==BOOTMODE_AUTO;
	switch(bootmode) {
		case BOOTMODE_AUTO:
		#if WITH_XIAOMI_DUALBOOT
			if(is_dualboot_supported() && get_dualboot_mode()==BOOTMODE_SECOND) {
				bootmode = BOOTMODE_SECOND;
				partname = "boot1";
				break;
			}
		#endif
		case BOOTMODE_NORMAL:
			partname = "boot";
			break;
	#if WITH_XIAOMI_DUALBOOT
		case BOOTMODE_SECOND:
			if(!is_dualboot_supported()) {
				dprintf(CRITICAL, "Dualboot is not supported!\n");
				return -1;
			}
			partname = "boot1";
			break;
	#endif
		case BOOTMODE_RECOVERY:
			partname = "recovery";
			break;

		default:
			dprintf(CRITICAL, "Invalid bootmode %d\n", bootmode);
			return -1;
	}

	if(is_auto)
		dprintf(INFO, "Automatic bootmode: %s\n", strbootmode(bootmode));

	index = partition_get_index(partname);
	ptn = partition_get_offset(index);
	if(ptn == 0) {
		dprintf(CRITICAL, "ERROR: partition '%s' not found\n", partname);
                return -1;
	}

	if (mmc_read(ptn + offset, (uint32_t *) buf, page_size)) {
		dprintf(CRITICAL, "ERROR: Cannot read boot image header\n");
                return -1;
	}

	if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		dprintf(CRITICAL, "ERROR: Invalid boot image header\n");
                return -1;
	}

	if (hdr->page_size && (hdr->page_size != page_size)) {

		if (hdr->page_size > BOOT_IMG_MAX_PAGE_SIZE) {
			dprintf(CRITICAL, "ERROR: Invalid page size\n");
			return -1;
		}
		page_size = hdr->page_size;
		page_mask = page_size - 1;
	}

	/* Read the next page to get kernel Image header
	 * which lives in the second page for arm64 targets.
	 */

	if (mmc_read(ptn + page_size, (uint32_t *) kbuf, page_size)) {
		dprintf(CRITICAL, "ERROR: Cannot read boot image header\n");
                return -1;
	}

	/*
	 * Update the kernel/ramdisk/tags address if the boot image header
	 * has default values, these default values come from mkbootimg when
	 * the boot image is flashed using fastboot flash:raw
	 */
	update_ker_tags_rdisk_addr(hdr, IS_ARM64(kptr));

	/* Get virtual addresses since the hdr saves physical addresses. */
	hdr->kernel_addr = VA((addr_t)(hdr->kernel_addr));
	hdr->ramdisk_addr = VA((addr_t)(hdr->ramdisk_addr));
	hdr->tags_addr = VA((addr_t)(hdr->tags_addr));

	kernel_actual  = ROUND_TO_PAGE(hdr->kernel_size,  page_mask);
	ramdisk_actual = ROUND_TO_PAGE(hdr->ramdisk_size, page_mask);

	/* Check if the addresses in the header are valid. */
	if (check_aboot_addr_range_overlap(hdr->kernel_addr, kernel_actual) ||
		check_aboot_addr_range_overlap(hdr->ramdisk_addr, ramdisk_actual))
	{
		dprintf(CRITICAL, "kernel/ramdisk addresses overlap with aboot addresses.\n");
		return -1;
	}

#if !DEVICE_TREE || DEVICE_TREE_FALLBACK
#if DEVICE_TREE_FALLBACK
	if(hdr->dt_size==0)
#endif
	{
		if (check_aboot_addr_range_overlap(hdr->tags_addr, MAX_TAGS_SIZE))
		{
			dprintf(CRITICAL, "Tags addresses overlap with aboot addresses.\n");
			return -1;
		}
	}
#endif

	/* Authenticate Kernel */
	dprintf(INFO, "use_signed_kernel=%d, is_unlocked=%d, is_tampered=%d.\n",
		(int) target_use_signed_kernel(),
		sysparam_read_bool("is_unlocked"),
		sysparam_read_bool("is_tampered"));

#if VERIFIED_BOOT
	boot_verifier_init();
#endif

	if(target_use_signed_kernel() && (!sysparam_read_bool("is_unlocked")))
	{
		offset = 0;

		image_addr = (unsigned char *)target_get_scratch_address();

#if DEVICE_TREE
#if DEVICE_TREE_FALLBACK
		if(hdr->dt_size>0)
#endif
		{
			dt_actual = ROUND_TO_PAGE(hdr->dt_size, page_mask);
			imagesize_actual = (page_size + kernel_actual + ramdisk_actual + dt_actual);

			if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_actual))
			{
				dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
				return -1;
			}
		}
#if DEVICE_TREE_FALLBACK
		else
#endif
#endif
#if !DEVICE_TREE || DEVICE_TREE_FALLBACK
		{
			imagesize_actual = (page_size + kernel_actual + ramdisk_actual);
		}
#endif

		dprintf(INFO, "Loading boot image (%d): start\n", imagesize_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_START);

		if (check_aboot_addr_range_overlap((uint32_t)image_addr, imagesize_actual))
		{
			dprintf(CRITICAL, "Boot image buffer address overlaps with aboot addresses.\n");
			return -1;
		}

		/* Read image without signature */
		if (mmc_read(ptn + offset, (void *)image_addr, imagesize_actual))
		{
			dprintf(CRITICAL, "ERROR: Cannot read boot image\n");
				return -1;
		}

		dprintf(INFO, "Loading boot image (%d): done\n", imagesize_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_DONE);

		offset = imagesize_actual;

		if (check_aboot_addr_range_overlap((uint32_t)image_addr + offset, page_size))
		{
			dprintf(CRITICAL, "Signature read buffer address overlaps with aboot addresses.\n");
			return -1;
		}

		/* Read signature */
		if(mmc_read(ptn + offset, (void *)(image_addr + offset), page_size))
		{
			dprintf(CRITICAL, "ERROR: Cannot read boot image signature\n");
			return -1;
		}

		verify_signed_bootimg((uint32_t)image_addr, imagesize_actual);

		/* Move kernel, ramdisk and device tree to correct address */
		memmove((void*) hdr->kernel_addr, (char *)(image_addr + page_size), hdr->kernel_size);
		memmove((void*) hdr->ramdisk_addr, (char *)(image_addr + page_size + kernel_actual), hdr->ramdisk_size);

		#if DEVICE_TREE
		if(hdr->dt_size) {
			dt_table_offset = ((uint32_t)image_addr + page_size + kernel_actual + ramdisk_actual + second_actual);
			table = (struct dt_table*) dt_table_offset;

			if (dev_tree_validate(table, hdr->page_size, &dt_hdr_size) != 0) {
				dprintf(CRITICAL, "ERROR: Cannot validate Device Tree Table \n");
				return -1;
			}

			/* Find index of device tree within device tree table */
			if(dev_tree_get_entry_info(table, &dt_entry) != 0){
				dprintf(CRITICAL, "ERROR: Device Tree Blob cannot be found\n");
				return -1;
			}

			/* Validate and Read device device tree in the "tags_add */
			if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_entry.size))
			{
				dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
				return -1;
			}

			memmove((void *)hdr->tags_addr, (char *)dt_table_offset + dt_entry.offset, dt_entry.size);
		} else {
			/*
			 * If appended dev tree is found, update the atags with
			 * memory address to the DTB appended location on RAM.
			 * Else update with the atags address in the kernel header
			 */
			void *dtb;
			dtb = dev_tree_appended((void*) hdr->kernel_addr,
						hdr->kernel_size,
						(void *)hdr->tags_addr);
			if (!dtb) {
				dprintf(CRITICAL, "ERROR: Appended Device Tree Blob not found\n");
#if !DEVICE_TREE_FALLBACK
				return -1;
#endif
			}
		}
		#endif
	}
	else
	{
		second_actual  = ROUND_TO_PAGE(hdr->second_size,  page_mask);

		image_addr = (unsigned char *)target_get_scratch_address();
#if DEVICE_TREE
#if DEVICE_TREE_FALLBACK
		if(hdr->dt_size>0)
#endif
		{
			dt_actual = ROUND_TO_PAGE(hdr->dt_size, page_mask);
			imagesize_actual = (page_size + kernel_actual + ramdisk_actual + dt_actual);

			if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_actual))
			{
				dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
				return -1;
			}
		}
#if DEVICE_TREE_FALLBACK
		else
#endif
#endif
#if !DEVICE_TREE || DEVICE_TREE_FALLBACK
		{
			imagesize_actual = (page_size + kernel_actual + ramdisk_actual);
		}
#endif
		if (check_aboot_addr_range_overlap((unsigned)image_addr, imagesize_actual))
		{
			dprintf(CRITICAL, "Boot image buffer address overlaps with aboot addresses.\n");
			return -1;
		}

		dprintf(INFO, "Loading boot image (%d): start\n",
				imagesize_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_START);

		offset = 0;

		/* Load the entire boot image */
		if (mmc_read(ptn + offset, (void *)image_addr, imagesize_actual)) {
			dprintf(CRITICAL, "ERROR: Cannot read boot image\n");
					return -1;
		}

		dprintf(INFO, "Loading boot image (%d): done\n",
				imagesize_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_DONE);

		#ifdef TZ_SAVE_KERNEL_HASH
		aboot_save_boot_hash_mmc(image_addr, imagesize_actual);
		#endif /* TZ_SAVE_KERNEL_HASH */

		/* Move kernel, ramdisk and device tree to correct address */
		memmove((void*) hdr->kernel_addr, (char *)(image_addr + page_size), hdr->kernel_size);
		memmove((void*) hdr->ramdisk_addr, (char *)(image_addr + page_size + kernel_actual), hdr->ramdisk_size);

		#if DEVICE_TREE
		if(hdr->dt_size) {
			dt_table_offset = ((uint32_t)image_addr + page_size + kernel_actual + ramdisk_actual + second_actual);
			table = (struct dt_table*) dt_table_offset;

			if (dev_tree_validate(table, hdr->page_size, &dt_hdr_size) != 0) {
				dprintf(CRITICAL, "ERROR: Cannot validate Device Tree Table \n");
				return -1;
			}

			/* Find index of device tree within device tree table */
			if(dev_tree_get_entry_info(table, &dt_entry) != 0){
				dprintf(CRITICAL, "ERROR: Getting device tree address failed\n");
				return -1;
			}

			/* Validate and Read device device tree in the tags_addr */
			if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_entry.size))
			{
				dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
				return -1;
			}

			memmove((void *)hdr->tags_addr, (char *)dt_table_offset + dt_entry.offset, dt_entry.size);
		} else {
			/* Validate the tags_addr */
			if (check_aboot_addr_range_overlap(hdr->tags_addr, kernel_actual))
			{
				dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
				return -1;
			}
			/*
			 * If appended dev tree is found, update the atags with
			 * memory address to the DTB appended location on RAM.
			 * Else update with the atags address in the kernel header
			 */
			void *dtb;
			dtb = dev_tree_appended((void*) hdr->kernel_addr,
						kernel_actual,
						(void *)hdr->tags_addr);
			if (!dtb) {
				dprintf(CRITICAL, "ERROR: Appended Device Tree Blob not found\n");
#if !DEVICE_TREE_FALLBACK
				return -1;
#endif
			}
		}
		#endif
	}

	if (bootmode==BOOTMODE_RECOVERY && !sysparam_read_bool("is_unlocked") && !sysparam_read_bool("is_tampered"))
		target_load_ssd_keystore();

unified_boot:

	boot_linux((void *)hdr->kernel_addr, (void *)hdr->tags_addr,
		   (const char *)hdr->cmdline, board_machtype(),
		   (void *)hdr->ramdisk_addr, hdr->ramdisk_size, hdr->dt_size>0);

	return 0;
}

int boot_linux_from_flash(void)
{
	struct boot_img_hdr *hdr = (void*) buf;
	struct ptentry *ptn;
	struct ptable *ptable;
	unsigned offset = 0;

	unsigned char *image_addr __UNUSED = 0;
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	unsigned imagesize_actual __UNUSED;
	unsigned second_actual;

#if DEVICE_TREE
	struct dt_table *table;
	struct dt_entry dt_entry;
	uint32_t dt_actual;
	uint32_t dt_hdr_size;
#endif

	if (target_is_emmc_boot()) {
		hdr = (struct boot_img_hdr *)EMMC_BOOT_IMG_HEADER_ADDR;
		if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
			dprintf(CRITICAL, "ERROR: Invalid boot image header\n");
			return -1;
		}
		goto continue_boot;
	}

	ptable = flash_get_ptable();
	if (ptable == NULL) {
		dprintf(CRITICAL, "ERROR: Partition table not found\n");
		return -1;
	}

	const char* partname = NULL;
	int is_auto = bootmode==BOOTMODE_AUTO;
	switch(bootmode) {
		case BOOTMODE_AUTO:
		#if WITH_XIAOMI_DUALBOOT
			if(is_dualboot_supported() && get_dualboot_mode()==BOOTMODE_SECOND) {
				partname = "boot1";
				break;
			}
			dprintf(INFO, "Dualboot choosing: %s\n", strbootmode(bootmode));
		#endif
		case BOOTMODE_NORMAL:
			partname = "boot";
			break;
	#if WITH_XIAOMI_DUALBOOT
		case BOOTMODE_SECOND:
			if(!is_dualboot_supported()) {
				dprintf(CRITICAL, "Dualboot is not supported!\n");
				return -1;
			}
			partname = "boot1";
			break;
	#endif
		case BOOTMODE_RECOVERY:
			partname = "recovery";
			break;

		default:
			dprintf(CRITICAL, "Invalid bootmode %d\n", bootmode);
			return -1;
	}

	if(is_auto)
		dprintf(INFO, "Automatic bootmode: %s\n", strbootmode(bootmode));

    ptn = ptable_find(ptable, partname);
    if (ptn == NULL) {
        dprintf(CRITICAL, "ERROR: partition '%s' not found\n", partname);
        return -1;
    }

	if (flash_read(ptn, offset, buf, page_size)) {
		dprintf(CRITICAL, "ERROR: Cannot read boot image header\n");
		return -1;
	}

	if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		dprintf(CRITICAL, "ERROR: Invalid boot image header\n");
		return -1;
	}

	if (hdr->page_size != page_size) {
		dprintf(CRITICAL, "ERROR: Invalid boot image pagesize. Device pagesize: %d, Image pagesize: %d\n",page_size,hdr->page_size);
		return -1;
	}

	/*
	 * Update the kernel/ramdisk/tags address if the boot image header
	 * has default values, these default values come from mkbootimg when
	 * the boot image is flashed using fastboot flash:raw
	 */
	update_ker_tags_rdisk_addr(hdr, false);

	/* Get virtual addresses since the hdr saves physical addresses. */
	hdr->kernel_addr = VA((addr_t)(hdr->kernel_addr));
	hdr->ramdisk_addr = VA((addr_t)(hdr->ramdisk_addr));
	hdr->tags_addr = VA((addr_t)(hdr->tags_addr));

	kernel_actual  = ROUND_TO_PAGE(hdr->kernel_size,  page_mask);
	ramdisk_actual = ROUND_TO_PAGE(hdr->ramdisk_size, page_mask);

	/* Check if the addresses in the header are valid. */
	if (check_aboot_addr_range_overlap(hdr->kernel_addr, kernel_actual) ||
		check_aboot_addr_range_overlap(hdr->ramdisk_addr, ramdisk_actual))
	{
		dprintf(CRITICAL, "kernel/ramdisk addresses overlap with aboot addresses.\n");
		return -1;
	}

#if !DEVICE_TREE || DEVICE_TREE_FALLBACK
#if DEVICE_TREE_FALLBACK
	if(hdr->dt_size==0)
#endif
	{
		if (check_aboot_addr_range_overlap(hdr->tags_addr, MAX_TAGS_SIZE))
		{
			dprintf(CRITICAL, "Tags addresses overlap with aboot addresses.\n");
			return -1;
		}
	}
#endif

	/* Authenticate Kernel */
	if(target_use_signed_kernel() && (!sysparam_read_bool("is_unlocked")))
	{
		image_addr = (unsigned char *)target_get_scratch_address();
		offset = 0;

#if DEVICE_TREE
#if DEVICE_TREE_FALLBACK
		if(hdr->dt_size>0)
#endif
		{
			dt_actual = ROUND_TO_PAGE(hdr->dt_size, page_mask);
			imagesize_actual = (page_size + kernel_actual + ramdisk_actual + dt_actual);

			if (check_aboot_addr_range_overlap(hdr->tags_addr, hdr->dt_size))
			{
				dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
				return -1;
			}
		}
#if DEVICE_TREE_FALLBACK
		else
#endif
#endif
#if !DEVICE_TREE || DEVICE_TREE_FALLBACK
		{
			imagesize_actual = (page_size + kernel_actual + ramdisk_actual);
		}
#endif

		dprintf(INFO, "Loading boot image (%d): start\n", imagesize_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_START);

		/* Read image without signature */
		if (flash_read(ptn, offset, (void *)image_addr, imagesize_actual))
		{
			dprintf(CRITICAL, "ERROR: Cannot read boot image\n");
				return -1;
		}

		dprintf(INFO, "Loading boot image (%d): done\n", imagesize_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_DONE);

		offset = imagesize_actual;
		/* Read signature */
		if (flash_read(ptn, offset, (void *)(image_addr + offset), page_size))
		{
			dprintf(CRITICAL, "ERROR: Cannot read boot image signature\n");
			return -1;
		}

		verify_signed_bootimg((uint32_t)image_addr, imagesize_actual);

		/* Move kernel and ramdisk to correct address */
		memmove((void*) hdr->kernel_addr, (char *)(image_addr + page_size), hdr->kernel_size);
		memmove((void*) hdr->ramdisk_addr, (char *)(image_addr + page_size + kernel_actual), hdr->ramdisk_size);
#if DEVICE_TREE
#if DEVICE_TREE_FALLBACK
		if(hdr->dt_size>0)
#endif
		{
			/* Validate and Read device device tree in the "tags_add */
			if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_entry.size))
			{
				dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
				return -1;
			}

			memmove((void*) hdr->tags_addr, (char *)(image_addr + page_size + kernel_actual + ramdisk_actual), hdr->dt_size);
		}
#endif

		/* Make sure everything from scratch address is read before next step!*/
		if(sysparam_read_bool("is_tampered"))
		{
			sysparam_write();
		}
#if USE_PCOM_SECBOOT
		set_tamper_flag(sysparam_read_bool("is_tampered"));
#endif
	}
	else
	{
		offset = page_size;

		kernel_actual = ROUND_TO_PAGE(hdr->kernel_size, page_mask);
		ramdisk_actual = ROUND_TO_PAGE(hdr->ramdisk_size, page_mask);
		second_actual = ROUND_TO_PAGE(hdr->second_size, page_mask);

		dprintf(INFO, "Loading boot image (%d): start\n",
				kernel_actual + ramdisk_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_START);

		if (flash_read(ptn, offset, (void *)hdr->kernel_addr, kernel_actual)) {
			dprintf(CRITICAL, "ERROR: Cannot read kernel image\n");
			return -1;
		}
		offset += kernel_actual;

		if (flash_read(ptn, offset, (void *)hdr->ramdisk_addr, ramdisk_actual)) {
			dprintf(CRITICAL, "ERROR: Cannot read ramdisk image\n");
			return -1;
		}
		offset += ramdisk_actual;

		dprintf(INFO, "Loading boot image (%d): done\n",
				kernel_actual + ramdisk_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_DONE);

		if(hdr->second_size != 0) {
			offset += second_actual;
			/* Second image loading not implemented. */
			ASSERT(0);
		}

#if DEVICE_TREE
		if(hdr->dt_size != 0) {

			/* Read the device tree table into buffer */
			if(flash_read(ptn, offset, (void *) dt_buf, page_size)) {
				dprintf(CRITICAL, "ERROR: Cannot read the Device Tree Table\n");
				return -1;
			}

			table = (struct dt_table*) dt_buf;

			if (dev_tree_validate(table, hdr->page_size, &dt_hdr_size) != 0) {
				dprintf(CRITICAL, "ERROR: Cannot validate Device Tree Table \n");
				return -1;
			}

			table = (struct dt_table*) memalign(CACHE_LINE, dt_hdr_size);
			if (!table)
				return -1;

			/* Read the entire device tree table into buffer */
			if(flash_read(ptn, offset, (void *)table, dt_hdr_size)) {
				dprintf(CRITICAL, "ERROR: Cannot read the Device Tree Table\n");
				return -1;
			}


			/* Find index of device tree within device tree table */
			if(dev_tree_get_entry_info(table, &dt_entry) != 0){
				dprintf(CRITICAL, "ERROR: Getting device tree address failed\n");
				return -1;
			}

			/* Validate and Read device device tree in the "tags_add */
			if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_entry.size))
			{
				dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
				return -1;
			}

			/* Read device device tree in the "tags_add */
			if(flash_read(ptn, offset + dt_entry.offset,
						 (void *)hdr->tags_addr, dt_entry.size)) {
				dprintf(CRITICAL, "ERROR: Cannot read device tree\n");
				return -1;
			}
		}
#endif

	}
continue_boot:

	/* TODO: create/pass atags to kernel */

	boot_linux((void *)hdr->kernel_addr, (void *)hdr->tags_addr,
		   (const char *)hdr->cmdline, board_machtype(),
		   (void *)hdr->ramdisk_addr, hdr->ramdisk_size, hdr->dt_size>0);

	return 0;
}

static int validate_device_info(void)
{
	if(sysparam_get_ptr("is_unlocked", NULL, NULL))
		sysparam_write_bool("is_unlocked", false);
	if(sysparam_get_ptr("is_tampered", NULL, NULL))
		sysparam_write_bool("is_tampered", false);
	if(sysparam_get_ptr("charger_screen", NULL, NULL))
		sysparam_write_bool("charger_screen", false);
	if(sysparam_get_ptr("bootmode", NULL, NULL))
		sysparam_write_bootmode("bootmode", BOOTMODE_AUTO);
#if WITH_XIAOMI_DUALBOOT
	if(!is_dualboot_supported() && sysparam_read_bootmode("bootmode")==BOOTMODE_SECOND)
	    sysparam_write_bootmode("bootmode", BOOTMODE_NORMAL);
#endif
	if(sysparam_get_ptr("splash_partition", NULL, NULL))
		sysparam_write_bool("splash_partition", false);

	if(sysparam_get_ptr("display_panel", NULL, NULL))
		sysparm_write_str("display_panel", "");
	else {
		const char* display_panel = sysparm_read_str("display_panel");
		if(!isprint(display_panel[0]) && !display_panel[0]=='\0')
			sysparm_write_str("display_panel", display_panel);
	}

	return 1;
}

void read_device_info(void)
{
#if VERIFIED_BOOT
	bdev_t *spdev = bio_open_by_label("devinfo");
#else
	bdev_t *spdev = bio_open_by_label(ABOOT_PARTITION);
#endif
	if(!spdev) {
		dprintf(CRITICAL, "aboot partition not found!\n");
		return;
	}

	sysparam_scan(spdev, spdev->size-spdev->block_size, spdev->block_size);
	if(validate_device_info()) {
		sysparam_write();
	}

	bootcount_aboot_inc();

	sysparam_dump(true);
}

void reset_device_info(void)
{
	dprintf(ALWAYS, "reset_device_info called.");
	sysparam_write_bool("is_tampered", false);
	sysparam_write();
}

void set_device_root(void)
{
	dprintf(ALWAYS, "set_device_root called.");
	sysparam_write_bool("is_tampered", true);
	sysparam_write();
}

#if DEVICE_TREE
int copy_dtb(uint8_t *boot_image_start)
{
	uint32 dt_image_offset = 0;
	uint32_t n;
	struct dt_table *table;
	struct dt_entry dt_entry;
	uint32_t dt_hdr_size;

	struct boot_img_hdr *hdr = (struct boot_img_hdr *) (boot_image_start);

	if(hdr->dt_size != 0) {

		/* add kernel offset */
		dt_image_offset += page_size;
		n = ROUND_TO_PAGE(hdr->kernel_size, page_mask);
		dt_image_offset += n;

		/* add ramdisk offset */
		n = ROUND_TO_PAGE(hdr->ramdisk_size, page_mask);
		dt_image_offset += n;

		/* add second offset */
		if(hdr->second_size != 0) {
			n = ROUND_TO_PAGE(hdr->second_size, page_mask);
			dt_image_offset += n;
		}

		/* offset now point to start of dt.img */
		table = (struct dt_table*)(boot_image_start + dt_image_offset);

		if (dev_tree_validate(table, hdr->page_size, &dt_hdr_size) != 0) {
			dprintf(CRITICAL, "ERROR: Cannot validate Device Tree Table \n");
			return -1;
		}
		/* Find index of device tree within device tree table */
		if(dev_tree_get_entry_info(table, &dt_entry) != 0){
			dprintf(CRITICAL, "ERROR: Getting device tree address failed\n");
			return -1;
		}

		/* Validate and Read device device tree in the "tags_add */
		if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_entry.size))
		{
			dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
			return -1;
		}

		/* Read device device tree in the "tags_add */
		memmove((void*) hdr->tags_addr,
				boot_image_start + dt_image_offset +  dt_entry.offset,
				dt_entry.size);
	} else
		return -1;

	/* Everything looks fine. Return success. */
	return 0;
}
#endif

void cmd_boot(const char *arg, void *data, unsigned sz)
{
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	uint32_t image_actual;
	uint32_t dt_actual = 0;
	uint32_t sig_actual = SIGNATURE_SIZE;
	struct boot_img_hdr *hdr;
	struct kernel64_hdr *kptr;
	char *ptr = ((char*) data);
	int ret __UNUSED = 0;
	uint8_t dtb_copied __UNUSED = 0;

#if VERIFIED_BOOT
	if(!sysparam_read_bool("is_unlocked"))
	{
		fastboot_fail("unlock device to use this command");
		return;
	}
#endif

	if (sz < sizeof(hdr)) {
		fastboot_fail("invalid bootimage header");
		return;
	}

	hdr = (struct boot_img_hdr *)data;

	if(strcmp((const char*)hdr->name, "GRUB")==0) {
		grub_load_from_sideload(data);
		fastboot_okay("");
		return;
	}

	/* ensure commandline is terminated */
	hdr->cmdline[BOOT_ARGS_SIZE-1] = 0;

	if(target_is_emmc_boot() && hdr->page_size) {
		page_size = hdr->page_size;
		page_mask = page_size - 1;
	}

	kernel_actual = ROUND_TO_PAGE(hdr->kernel_size, page_mask);
	ramdisk_actual = ROUND_TO_PAGE(hdr->ramdisk_size, page_mask);
#if DEVICE_TREE
	dt_actual = ROUND_TO_PAGE(hdr->dt_size, page_mask);
#endif

	image_actual = ADD_OF(page_size, kernel_actual);
	image_actual = ADD_OF(image_actual, ramdisk_actual);
	image_actual = ADD_OF(image_actual, dt_actual);

	if (target_use_signed_kernel() && (!sysparam_read_bool("is_unlocked")))
		image_actual = ADD_OF(image_actual, sig_actual);

	/* sz should have atleast raw boot image */
	if (image_actual > sz) {
		fastboot_fail("bootimage: incomplete or not signed");
		return;
	}

	/* Verify the boot image
	 * device & page_size are initialized in aboot_init
	 */
	if (target_use_signed_kernel() && (!sysparam_read_bool("is_unlocked")))
		/* Pass size excluding signature size, otherwise we would try to
		 * access signature beyond its length
		 */
		verify_signed_bootimg((uint32_t)data, (image_actual - sig_actual));

	/*
	 * Update the kernel/ramdisk/tags address if the boot image header
	 * has default values, these default values come from mkbootimg when
	 * the boot image is flashed using fastboot flash:raw
	 */
	kptr = (struct kernel64_hdr*)((char*) data + page_size);
	update_ker_tags_rdisk_addr(hdr, IS_ARM64(kptr));

	/* Get virtual addresses since the hdr saves physical addresses. */
	hdr->kernel_addr = (unsigned)VA(hdr->kernel_addr);
	hdr->ramdisk_addr = (unsigned)VA(hdr->ramdisk_addr);
	hdr->tags_addr = (unsigned)VA(hdr->tags_addr);

	/* Check if the addresses in the header are valid. */
	if (check_aboot_addr_range_overlap(hdr->kernel_addr, kernel_actual) ||
		check_aboot_addr_range_overlap(hdr->ramdisk_addr, ramdisk_actual))
	{
		dprintf(CRITICAL, "kernel/ramdisk addresses overlap with aboot addresses.\n");
		return;
	}

#if DEVICE_TREE
#if DEVICE_TREE_FALLBACK
	if(hdr->dt_size>0)
#endif
	{
		/* find correct dtb and copy it to right location */
		ret = copy_dtb(data);

		dtb_copied = !ret ? 1 : 0;
	}
#if DEVICE_TREE_FALLBACK
	else
#endif
#endif
#if !DEVICE_TREE || DEVICE_TREE_FALLBACK
	{
		if (check_aboot_addr_range_overlap(hdr->tags_addr, MAX_TAGS_SIZE))
		{
			dprintf(CRITICAL, "Tags addresses overlap with aboot addresses.\n");
			return;
		}
	}
#endif

	/* Load ramdisk & kernel */
	memmove((void*) hdr->ramdisk_addr, ptr + page_size + kernel_actual, hdr->ramdisk_size);
	memmove((void*) hdr->kernel_addr, ptr + page_size, hdr->kernel_size);

#if DEVICE_TREE
#if DEVICE_TREE_FALLBACK
	if(hdr->dt_size>0)
#endif
	{
		/*
		 * If dtb is not found look for appended DTB in the kernel.
		 * If appended dev tree is found, update the atags with
		 * memory address to the DTB appended location on RAM.
		 * Else update with the atags address in the kernel header
		 */
		if (!dtb_copied) {
			void *dtb;
			dtb = dev_tree_appended((void *)hdr->kernel_addr, hdr->kernel_size,
						(void *)hdr->tags_addr);
			if (!dtb) {
				fastboot_fail("dtb not found");
				return;
			}
		}
	}
#if DEVICE_TREE_FALLBACK
	else
#endif
#endif
#if !DEVICE_TREE || DEVICE_TREE_FALLBACK
	{
		if (check_aboot_addr_range_overlap(hdr->tags_addr, MAX_TAGS_SIZE))
		{
			dprintf(CRITICAL, "Tags addresses overlap with aboot addresses.\n");
			return;
		}
	}
#endif

	fastboot_okay("");
	fastboot_stop();

	boot_linux((void*) hdr->kernel_addr, (void*) hdr->tags_addr,
		   (const char*) hdr->cmdline, board_machtype(),
		   (void*) hdr->ramdisk_addr, hdr->ramdisk_size, hdr->dt_size>0);
}

void cmd_erase_nand(const char *arg, void *data, unsigned sz)
{
	struct ptentry *ptn;
	struct ptable *ptable;

	ptable = flash_get_ptable();
	if (ptable == NULL) {
		fastboot_fail("partition table doesn't exist");
		return;
	}

	ptn = ptable_find(ptable, arg);
	if (ptn == NULL) {
		fastboot_fail("unknown partition name");
		return;
	}

	if (flash_erase(ptn)) {
		fastboot_fail("failed to erase partition");
		return;
	}
	fastboot_okay("");
}


void cmd_erase_mmc(const char *arg, void *data, unsigned sz)
{
	unsigned long long ptn = 0;
	unsigned long long size = 0;
	int index = INVALID_PTN;
	uint8_t lun = 0;

#if VERIFIED_BOOT
	if(!strcmp(arg, KEYSTORE_PTN_NAME))
	{
		if(!sysparam_read_bool("is_unlocked"))
		{
			fastboot_fail("unlock device to erase keystore");
			return;
		}
	}
#endif

	index = partition_get_index(arg);
	ptn = partition_get_offset(index);
	size = partition_get_size(index);

	if(ptn == 0) {
		fastboot_fail("Partition table doesn't exist\n");
		return;
	}

	lun = partition_get_lun(index);
	mmc_set_lun(lun);

#if MMC_SDHCI_SUPPORT
	if (mmc_erase_card(ptn, size)) {
		fastboot_fail("failed to erase partition\n");
		return;
	}
#else
	BUF_DMA_ALIGN(out, DEFAULT_ERASE_SIZE);
	size = partition_get_size(index);
	if (size > DEFAULT_ERASE_SIZE)
		size = DEFAULT_ERASE_SIZE;

	/* Simple inefficient version of erase. Just writing
       0 in first several blocks */
	if (mmc_write(ptn , size, (unsigned int *)out)) {
		fastboot_fail("failed to erase partition");
		return;
	}
#endif
	fastboot_okay("");
}

void cmd_erase(const char *arg, void *data, unsigned sz)
{
	if(target_is_emmc_boot())
		cmd_erase_mmc(arg, data, sz);
	else
		cmd_erase_nand(arg, data, sz);
}

void cmd_flash_mmc_img(const char *arg, void *data, unsigned sz)
{
	unsigned long long ptn = 0;
	unsigned long long size = 0;
	int index = INVALID_PTN;
	char *token = NULL;
	char *pname = NULL;
	uint8_t lun = 0;
	bool lun_set = false;
	char tmp[2048];
	strcpy(tmp, arg);

	token = strtok(tmp, ":");
	pname = token;
	token = strtok(NULL, ":");
	if(token)
	{
		lun = atoi(token);
		mmc_set_lun(lun);
		lun_set = true;
	}

	if (pname)
	{
		if (!strcmp(pname, "partition"))
		{
			dprintf(INFO, "Attempt to write partition image.\n");
			if (write_partition(sz, (unsigned char *) data)) {
				fastboot_fail("failed to write partition");
				return;
			}
		}
		else
		{
#if VERIFIED_BOOT
			if(!strcmp(pname, KEYSTORE_PTN_NAME))
			{
				if(!sysparam_read_bool("is_unlocked"))
				{
					fastboot_fail("unlock device to flash keystore");
					return;
				}
				if(!boot_verify_validate_keystore((unsigned char *)data))
				{
					fastboot_fail("image is not a keystore file");
					return;
				}
			}
#endif
			index = partition_get_index(pname);
			ptn = partition_get_offset(index);
			if(ptn == 0) {
				fastboot_fail("partition table doesn't exist");
				return;
			}

			if (!strcmp(pname, "boot")
#if WITH_XIAOMI_DUALBOOT
				|| !strcmp(pname, "boot1")
#endif
				|| !strcmp(pname, "recovery")) {
				if (memcmp((void *)data, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
					fastboot_fail("image is not a boot image");
					return;
				}
			}

			if(!lun_set)
			{
				lun = partition_get_lun(index);
				mmc_set_lun(lun);
			}

			size = partition_get_size(index);
			if (ROUND_TO_PAGE(sz,511) > size) {
				fastboot_fail("size too large");
				return;
			}
			else if (mmc_write(ptn , sz, (unsigned int *)data)) {
				fastboot_fail("flash write failure");
				return;
			}
		}
	}
	fastboot_okay("");
	return;
}

void cmd_flash_mmc_sparse_img(const char *arg, void *data, unsigned sz)
{
	unsigned int chunk;
	unsigned int chunk_data_sz;
	uint32_t *fill_buf = NULL;
	uint32_t fill_val;
	uint32_t chunk_blk_cnt = 0;
	sparse_header_t *sparse_header;
	chunk_header_t *chunk_header;
	uint32_t total_blocks = 0;
	unsigned long long ptn = 0;
	unsigned long long size = 0;
	int index = INVALID_PTN;
	uint32_t i;
	uint8_t lun = 0;

	index = partition_get_index(arg);
	ptn = partition_get_offset(index);
	if(ptn == 0) {
		fastboot_fail("partition table doesn't exist");
		return;
	}

	size = partition_get_size(index);
	if (ROUND_TO_PAGE(sz,511) > size) {
		fastboot_fail("size too large");
		return;
	}

	lun = partition_get_lun(index);
	mmc_set_lun(lun);

	/* Read and skip over sparse image header */
	sparse_header = (sparse_header_t *) data;
	if ((sparse_header->total_blks * sparse_header->blk_sz) > size) {
		fastboot_fail("size too large");
		return;
	}

	data += sparse_header->file_hdr_sz;
	if(sparse_header->file_hdr_sz > sizeof(sparse_header_t))
	{
		/* Skip the remaining bytes in a header that is longer than
		 * we expected.
		 */
		data += (sparse_header->file_hdr_sz - sizeof(sparse_header_t));
	}

	dprintf (SPEW, "=== Sparse Image Header ===\n");
	dprintf (SPEW, "magic: 0x%x\n", sparse_header->magic);
	dprintf (SPEW, "major_version: 0x%x\n", sparse_header->major_version);
	dprintf (SPEW, "minor_version: 0x%x\n", sparse_header->minor_version);
	dprintf (SPEW, "file_hdr_sz: %d\n", sparse_header->file_hdr_sz);
	dprintf (SPEW, "chunk_hdr_sz: %d\n", sparse_header->chunk_hdr_sz);
	dprintf (SPEW, "blk_sz: %d\n", sparse_header->blk_sz);
	dprintf (SPEW, "total_blks: %d\n", sparse_header->total_blks);
	dprintf (SPEW, "total_chunks: %d\n", sparse_header->total_chunks);

	/* Start processing chunks */
	for (chunk=0; chunk<sparse_header->total_chunks; chunk++)
	{
		/* Read and skip over chunk header */
		chunk_header = (chunk_header_t *) data;
		data += sizeof(chunk_header_t);

		dprintf (SPEW, "=== Chunk Header ===\n");
		dprintf (SPEW, "chunk_type: 0x%x\n", chunk_header->chunk_type);
		dprintf (SPEW, "chunk_data_sz: 0x%x\n", chunk_header->chunk_sz);
		dprintf (SPEW, "total_size: 0x%x\n", chunk_header->total_sz);

		if(sparse_header->chunk_hdr_sz > sizeof(chunk_header_t))
		{
			/* Skip the remaining bytes in a header that is longer than
			 * we expected.
			 */
			data += (sparse_header->chunk_hdr_sz - sizeof(chunk_header_t));
		}

		chunk_data_sz = sparse_header->blk_sz * chunk_header->chunk_sz;
		switch (chunk_header->chunk_type)
		{
			case CHUNK_TYPE_RAW:
			if(chunk_header->total_sz != (sparse_header->chunk_hdr_sz +
											chunk_data_sz))
			{
				fastboot_fail("Bogus chunk size for chunk type Raw");
				return;
			}

			if(mmc_write(ptn + ((uint64_t)total_blocks*sparse_header->blk_sz),
						chunk_data_sz,
						(unsigned int*)data))
			{
				fastboot_fail("flash write failure");
				return;
			}
			total_blocks += chunk_header->chunk_sz;
			data += chunk_data_sz;
			break;

			case CHUNK_TYPE_FILL:
			if(chunk_header->total_sz != (sparse_header->chunk_hdr_sz +
											sizeof(uint32_t)))
			{
				fastboot_fail("Bogus chunk size for chunk type FILL");
				return;
			}

			fill_buf = (uint32_t *)memalign(CACHE_LINE, ROUNDUP(sparse_header->blk_sz, CACHE_LINE));
			if (!fill_buf)
			{
				fastboot_fail("Malloc failed for: CHUNK_TYPE_FILL");
				return;
			}

			fill_val = *(uint32_t *)data;
			data = (char *) data + sizeof(uint32_t);
			chunk_blk_cnt = chunk_data_sz / sparse_header->blk_sz;

			for (i = 0; i < (sparse_header->blk_sz / sizeof(fill_val)); i++)
			{
				fill_buf[i] = fill_val;
			}

			for (i = 0; i < chunk_blk_cnt; i++)
			{
				if(mmc_write(ptn + ((uint64_t)total_blocks*sparse_header->blk_sz),
							sparse_header->blk_sz,
							fill_buf))
				{
					fastboot_fail("flash write failure");
					free(fill_buf);
					return;
				}

				total_blocks++;
			}

			free(fill_buf);
			break;

			case CHUNK_TYPE_DONT_CARE:
			total_blocks += chunk_header->chunk_sz;
			break;

			case CHUNK_TYPE_CRC:
			if(chunk_header->total_sz != sparse_header->chunk_hdr_sz)
			{
				fastboot_fail("Bogus chunk size for chunk type Dont Care");
				return;
			}
			total_blocks += chunk_header->chunk_sz;
			data += chunk_data_sz;
			break;

			default:
			dprintf(CRITICAL, "Unkown chunk type: %x\n",chunk_header->chunk_type);
			fastboot_fail("Unknown chunk type");
			return;
		}
	}

	dprintf(INFO, "Wrote %d blocks, expected to write %d blocks\n",
					total_blocks, sparse_header->total_blks);

	if(total_blocks != sparse_header->total_blks)
	{
		fastboot_fail("sparse image write failure");
	}

	fastboot_okay("");
	return;
}

void cmd_flash_mmc(const char *arg, void *data, unsigned sz)
{
	sparse_header_t *sparse_header;
	/* 8 Byte Magic + 2048 Byte xml + Encrypted Data */
	unsigned int *magic_number = (unsigned int *) data;

#ifdef SSD_ENABLE
	int              ret=0;
	uint32           major_version=0;
	uint32           minor_version=0;

	ret = scm_svc_version(&major_version,&minor_version);
	if(!ret)
	{
		if(major_version >= 2)
		{
			if( !strcmp(arg, "ssd") || !strcmp(arg, "tqs") )
			{
				ret = encrypt_scm((uint32 **) &data, &sz);
				if (ret != 0) {
					dprintf(CRITICAL, "ERROR: Encryption Failure\n");
					return;
				}

				/* Protect only for SSD */
				if (!strcmp(arg, "ssd")) {
					ret = scm_protect_keystore((uint32 *) data, sz);
					if (ret != 0) {
						dprintf(CRITICAL, "ERROR: scm_protect_keystore Failed\n");
						return;
					}
				}
			}
			else
			{
				ret = decrypt_scm_v2((uint32 **) &data, &sz);
				if(ret != 0)
				{
					dprintf(CRITICAL,"ERROR: Decryption Failure\n");
					return;
				}
			}
		}
		else
		{
			if (magic_number[0] == DECRYPT_MAGIC_0 &&
			magic_number[1] == DECRYPT_MAGIC_1)
			{
				ret = decrypt_scm((uint32 **) &data, &sz);
				if (ret != 0) {
					dprintf(CRITICAL, "ERROR: Invalid secure image\n");
					return;
				}
			}
			else if (magic_number[0] == ENCRYPT_MAGIC_0 &&
				magic_number[1] == ENCRYPT_MAGIC_1)
			{
				ret = encrypt_scm((uint32 **) &data, &sz);
				if (ret != 0) {
					dprintf(CRITICAL, "ERROR: Encryption Failure\n");
					return;
				}
			}
		}
	}
	else
	{
		dprintf(CRITICAL,"INVALID SVC Version\n");
		return;
	}
#endif /* SSD_ENABLE */

#if VERIFIED_BOOT
	if(!sysparam_read_bool("is_unlocked") && !sysparam_read_bool("is_verified"))
	{
		fastboot_fail("device is locked. Cannot flash images");
		return;
	}
	if(!sysparam_read_bool("is_unlocked") && sysparam_read_bool("is_verified"))
	{
		if(!boot_verify_flash_allowed(arg))
		{
			fastboot_fail("cannot flash this partition in verified state");
			return;
		}
	}
#endif

	sparse_header = (sparse_header_t *) data;
	if (sparse_header->magic != SPARSE_HEADER_MAGIC)
		cmd_flash_mmc_img(arg, data, sz);
	else
		cmd_flash_mmc_sparse_img(arg, data, sz);
	return;
}

void cmd_flash_nand(const char *arg, void *data, unsigned sz)
{
	struct ptentry *ptn;
	struct ptable *ptable;
	unsigned extra = 0;

	ptable = flash_get_ptable();
	if (ptable == NULL) {
		fastboot_fail("partition table doesn't exist");
		return;
	}

	ptn = ptable_find(ptable, arg);
	if (ptn == NULL) {
		fastboot_fail("unknown partition name");
		return;
	}

	if (!strcmp(ptn->name, "boot") || !strcmp(ptn->name, "recovery")) {
		if (memcmp((void *)data, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
			fastboot_fail("image is not a boot image");
			return;
		}
	}

	if (!strcmp(ptn->name, "system")
		|| !strcmp(ptn->name, "userdata")
		|| !strcmp(ptn->name, "persist")
		|| !strcmp(ptn->name, "recoveryfs")
		|| !strcmp(ptn->name, "modem"))
	{
		if (memcmp((void *)data, UBI_MAGIC, UBI_MAGIC_SIZE))
			extra = 1;
		else
			extra = 0;
	}
	else
		sz = ROUND_TO_PAGE(sz, page_mask);

	dprintf(INFO, "writing %d bytes to '%s'\n", sz, ptn->name);
	if (flash_write(ptn, extra, data, sz)) {
		fastboot_fail("flash write failure");
		return;
	}
	dprintf(INFO, "partition '%s' updated\n", ptn->name);
	fastboot_okay("");
}

void cmd_flash(const char *arg, void *data, unsigned sz)
{
	if(target_is_emmc_boot())
		cmd_flash_mmc(arg, data, sz);
	else
		cmd_flash_nand(arg, data, sz);
}

void cmd_continue(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");

	bootmode = BOOTMODE_AUTO;
	aboot_continue_boot();
}

void cmd_reboot(const char *arg, void *data, unsigned sz)
{
	dprintf(INFO, "rebooting the device\n");
	fastboot_okay("");
	reboot_device(REBOOT_MODE_NORMAL);
}

void cmd_reboot_bootloader(const char *arg, void *data, unsigned sz)
{
	dprintf(INFO, "rebooting the device\n");
	fastboot_okay("");
	reboot_device(REBOOT_MODE_FASTBOOT);
}

void cmd_reboot_recovery(const char *arg, void *data, unsigned sz)
{
	dprintf(INFO, "rebooting the device\n");
	fastboot_okay("");
	reboot_device(REBOOT_MODE_RECOVERY);
}

void cmd_oem_enable_charger_screen(const char *arg, void *data, unsigned size)
{
	dprintf(INFO, "Enabling charger screen check\n");
	sysparam_write_bool("charger_screen", true);
	sysparam_write();
	fastboot_okay("");
}

void cmd_oem_disable_charger_screen(const char *arg, void *data, unsigned size)
{
	dprintf(INFO, "Disabling charger screen check\n");
	sysparam_write_bool("charger_screen", false);
	sysparam_write();
	fastboot_okay("");
}

void cmd_oem_enable_use_splash_partition(const char *arg, void *data, unsigned size)
{
	dprintf(INFO, "Enabling using splash partition\n");
	sysparam_write_bool("splash_partition", true);
	sysparam_write();
	fastboot_okay("");
}

void cmd_oem_disable_use_splash_partition(const char *arg, void *data, unsigned size)
{
	dprintf(INFO, "Disabling using splash partition\n");
	sysparam_write_bool("splash_partition", false);
	sysparam_write();
	fastboot_okay("");
}

void cmd_oem_select_display_panel(const char *arg, void *data, unsigned size)
{
	dprintf(INFO, "Selecting display panel %s\n", arg);

	if (arg)
		sysparm_write_str("display_panel", arg);
	sysparam_write();
	fastboot_okay("");
}

void cmd_oem_unlock(const char *arg, void *data, unsigned sz)
{
	/* TODO: Wipe user data */
	if(!sysparam_read_bool("is_unlocked") || sysparam_read_bool("is_verified"))
	{
		sysparam_write_bool("is_unlocked", true);
		sysparam_write_bool("is_verified", false);
		sysparam_write();
	}
	fastboot_okay("");
}

void cmd_oem_lock(const char *arg, void *data, unsigned sz)
{
	/* TODO: Wipe user data */
	if(sysparam_read_bool("is_unlocked") || sysparam_read_bool("is_verified"))
	{
		sysparam_write_bool("is_unlocked", false);
		sysparam_write_bool("is_verified", false);
		sysparam_write();
	}
	fastboot_okay("");
}

void cmd_oem_verified(const char *arg, void *data, unsigned sz)
{
	/* TODO: Wipe user data */
	if(sysparam_read_bool("is_unlocked") || !sysparam_read_bool("is_verified"))
	{
		sysparam_write_bool("is_unlocked", false);
		sysparam_write_bool("is_verified", true);
		sysparam_write();
	}
	fastboot_okay("");
}

void cmd_oem_devinfo(const char *arg, void *data, unsigned sz)
{
	char response[128];
	snprintf(response, sizeof(response), "\tDevice tampered: %s", (sysparam_read_bool("is_tampered") ? "true" : "false"));
	fastboot_info(response);
	snprintf(response, sizeof(response), "\tDevice unlocked: %s", (sysparam_read_bool("is_unlocked") ? "true" : "false"));
	fastboot_info(response);
	snprintf(response, sizeof(response), "\tCharger screen enabled: %s", (sysparam_read_bool("charger_screen") ? "true" : "false"));
	fastboot_info(response);
	snprintf(response, sizeof(response), "\tDisplay panel: %s", sysparm_read_str("display_panel"));
	fastboot_info(response);
	snprintf(response, sizeof(response), "\tUsing splash partition: %s", (sysparam_read_bool("splash_partition") ? "true" : "false"));
	fastboot_info(response);
	fastboot_okay("");
}

#if WITH_DEBUG_LOG_BUF
void cmd_oem_lk_log(const char *arg, void *data, unsigned sz)
{
	char* pch;
	char* buf = strdup(lk_log_getbuf());

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
#endif

void cmd_oem_screenshot(const char *arg, void *unused, unsigned sz)
{
	struct fbcon_config* config = fbcon_display();
	char *data = config->base;
	unsigned size = config->width*config->height*config->bpp/8;

	fastboot_send_data(data, size);

	fastboot_okay("");
}

void cmd_preflash(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
}

struct fbimage* splash_screen_flash(void);

int splash_screen_check_header(struct fbimage *logo)
{
	if (memcmp(logo->header.magic, LOGO_IMG_MAGIC, 8))
		return -1;
	if (logo->header.width == 0 || logo->header.height == 0)
		return -1;
	return 0;
}

struct fbimage* splash_screen_flash(void)
{
	struct ptentry *ptn;
	struct ptable *ptable;
	struct fbcon_config *fb_display = NULL;
	struct fbimage *logo = NULL;


	logo = (struct fbimage *) malloc(ROUNDUP(page_size, sizeof(struct fbimage)));
	ASSERT(logo);

	ptable = flash_get_ptable();
	if (ptable == NULL) {
	dprintf(CRITICAL, "ERROR: Partition table not found\n");
	goto err;
	}
	ptn = ptable_find(ptable, SPLASH_PARTITION_NAME);
	if (ptn == NULL) {
		dprintf(CRITICAL, "ERROR: splash Partition not found\n");
		goto err;
	}

	if (flash_read(ptn, 0,(unsigned int *) logo, sizeof(logo->header))) {
		dprintf(CRITICAL, "ERROR: Cannot read boot image header\n");
		goto err;
	}

	if (splash_screen_check_header(logo)) {
		dprintf(CRITICAL, "ERROR: Boot image header invalid\n");
		goto err;
	}

	fb_display = fbcon_display();
	if (fb_display) {
		uint8_t *base = (uint8_t *) fb_display->base;
		if (logo->header.width != fb_display->width || logo->header.height != fb_display->height) {
				base += LOGO_IMG_OFFSET;
		}

		if (flash_read(ptn + sizeof(logo->header), 0,
			base,
			((((logo->header.width * logo->header.height * fb_display->bpp/8) + 511) >> 9) << 9))) {
			fbcon_clear();
			dprintf(CRITICAL, "ERROR: Cannot read splash image from partition\n");
			goto err;
		}

#if DISPLAY_USE_BGR
		unsigned i;
		for(i=0; i<(logo->header.width*fb_display->height); i++) {
			int r,g,b;
			char* color = (char*) base + i*3;

			r = color[0];
			g = color[1];
			b = color[2];

			color[0] = b;
			color[1] = g;
			color[2] = r;
		}
#endif

		logo->image = base;
	}

	return logo;

err:
	free(logo);
	return NULL;
}

struct fbimage* splash_screen_mmc(void)
{
	int index = INVALID_PTN;
	unsigned long long ptn = 0;
	struct fbcon_config *fb_display = NULL;
	struct fbimage *logo = NULL;
	uint32_t blocksize;
	uint32_t readsize;
	uint32_t ptn_size;

	index = partition_get_index(SPLASH_PARTITION_NAME);
	if (index == 0) {
		dprintf(CRITICAL, "ERROR: splash Partition table not found\n");
		return NULL;
	}

	ptn = partition_get_offset(index);
	if (ptn == 0) {
		dprintf(CRITICAL, "ERROR: splash Partition invalid\n");
		return NULL;
	}

	ptn_size = partition_get_size(index);
	blocksize = mmc_get_device_blocksize();
	readsize = ROUNDUP(sizeof(logo->header), blocksize);

	logo = (struct fbimage *)memalign(CACHE_LINE, ROUNDUP(readsize, CACHE_LINE));
	ASSERT(logo);

	if (mmc_read(ptn, (uint32_t *) logo, readsize)) {
		dprintf(CRITICAL, "ERROR: Cannot read splash image header\n");
		goto err;
	}

	if (splash_screen_check_header(logo)) {
		dprintf(CRITICAL, "ERROR: Splash image header invalid\n");
		goto err;
	}

	fb_display = fbcon_display();
	if (fb_display) {
		uint8_t *base = (uint8_t *) fb_display->base;
		uint32_t hdrsz = readsize;
		readsize = ROUNDUP((logo->header.width * logo->header.height * fb_display->bpp/8), blocksize);
		if (logo->header.width != fb_display->width || logo->header.height != fb_display->height)
				base += LOGO_IMG_OFFSET;

		if (readsize > ptn_size)
		{
			dprintf(CRITICAL, "@%d:Invalid logo header readsize:%u exceeds ptn_size:%u\n", __LINE__, readsize,ptn_size);
			goto err;
		}

		if (mmc_read(ptn + hdrsz,(uint32_t *)base, readsize)) {
			fbcon_clear();
			dprintf(CRITICAL, "ERROR: Cannot read splash image from partition\n");
			goto err;
		}

#if DISPLAY_USE_BGR
		unsigned i;
		for(i=0; i<(logo->header.width*fb_display->height); i++) {
			int r,g,b;
			char* color = (char*) base + i*3;

			r = color[0];
			g = color[1];
			b = color[2];

			color[0] = b;
			color[1] = g;
			color[2] = r;
		}
#endif

		logo->image = base;
	}

	return logo;

err:
	free(logo);
	return NULL;
}


struct fbimage* fetch_image_from_partition(void)
{
	if (!sysparam_read_bool("splash_partition"))
		return NULL;

	if (target_is_emmc_boot()) {
		return splash_screen_mmc();
	} else {
		return splash_screen_flash();
	}
}

/* Get the size from partiton name */
static void get_partition_size(const char *arg, char *response)
{
	uint64_t ptn = 0;
	uint64_t size;
	int index = INVALID_PTN;

	index = partition_get_index(arg);

	if (index == INVALID_PTN)
	{
		dprintf(CRITICAL, "Invalid partition index\n");
		return;
	}

	ptn = partition_get_offset(index);

	if(!ptn)
	{
		dprintf(CRITICAL, "Invalid partition name %s\n", arg);
		return;
	}

	size = partition_get_size(index);

	snprintf(response, MAX_RSP_SIZE, "\t 0x%llx", size);
	return;
}

/*
 * Publish the partition type & size info
 * fastboot getvar will publish the required information.
 * fastboot getvar partition_size:<partition_name>: partition size in hex
 * fastboot getvar partition_type:<partition_name>: partition type (ext/fat)
 */
static void publish_getvar_partition_info(struct getvar_partition_info *info, uint8_t num_parts)
{
	uint8_t i;

	for (i = 0; i < num_parts; i++) {
		get_partition_size(info[i].part_name, info[i].size_response);

		if (strlcat(info[i].getvar_size, info[i].part_name, MAX_GET_VAR_NAME_SIZE) >= MAX_GET_VAR_NAME_SIZE)
		{
			dprintf(CRITICAL, "partition size name truncated\n");
			return;
		}
		if (strlcat(info[i].getvar_type, info[i].part_name, MAX_GET_VAR_NAME_SIZE) >= MAX_GET_VAR_NAME_SIZE)
		{
			dprintf(CRITICAL, "partition type name truncated\n");
			return;
		}

		/* publish partition size & type info */
		fastboot_publish((const char *) info[i].getvar_size, (const char *) info[i].size_response);
		fastboot_publish((const char *) info[i].getvar_type, (const char *) info[i].type_response);
	}
}

/* register commands and variables for fastboot */
void aboot_fastboot_register_commands(void)
{
	int i;
	struct fbcon_config* config = fbcon_display();

	struct fastboot_cmd_desc cmd_list[] = {
		/* By default the enabled list is empty. */
		{"", NULL},
		/* move commands enclosed within the below ifndef to here
		 * if they need to be enabled in user build.
		 */
#ifndef DISABLE_FASTBOOT_CMDS
		/* Register the following commands only for non-user builds */
		{"flash:", cmd_flash},
		{"erase:", cmd_erase},
		{"boot", cmd_boot},
		{"continue", cmd_continue},
		{"reboot", cmd_reboot},
		{"reboot-bootloader", cmd_reboot_bootloader},
		{"reboot-recovery", cmd_reboot_recovery},
		{"oem unlock", cmd_oem_unlock},
		{"oem lock", cmd_oem_lock},
		{"oem verified", cmd_oem_verified},
		{"oem device-info", cmd_oem_devinfo},
	#if WITH_DEBUG_LOG_BUF
		{"oem lk_log", cmd_oem_lk_log},
	#endif
		{"oem screenshot", cmd_oem_screenshot},
		{"preflash", cmd_preflash},
		{"oem enable-charger-screen", cmd_oem_enable_charger_screen},
		{"oem disable-charger-screen", cmd_oem_disable_charger_screen},
		{"oem select-display-panel", cmd_oem_select_display_panel},
		{"oem enable-using-splash-partition", cmd_oem_enable_use_splash_partition},
		{"oem disable-using-splash-partition", cmd_oem_disable_use_splash_partition},
#endif
	};

	int fastboot_cmds_count = ARRAY_SIZE(cmd_list);
	for (i = 1; i < fastboot_cmds_count; i++)
		fastboot_register(cmd_list[i].name,cmd_list[i].cb);

	/* publish variables and their values */
	fastboot_publish("product",  TARGET);
	fastboot_publish("kernel",   "lk");
	fastboot_publish("serialno", sn_buf);

	/*
	 * partition info is supported only for emmc partitions
	 * Calling this for NAND prints some error messages which
	 * is harmless but misleading. Avoid calling this for NAND
	 * devices.
	 */
	if (target_is_emmc_boot())
		publish_getvar_partition_info(part_info, ARRAY_SIZE(part_info));

	/* Max download size supported */
	snprintf(max_download_size, MAX_RSP_SIZE, "\t0x%x",
			target_get_max_flash_size());
	fastboot_publish("max-download-size", (const char *) max_download_size);
	/* Is the charger screen check enabled */
	snprintf(charger_screen_enabled, MAX_RSP_SIZE, "%d",
			sysparam_read_bool("charger_screen"));
	fastboot_publish("charger-screen-enabled",
			(const char *) charger_screen_enabled);
	snprintf(panel_display_mode, MAX_RSP_SIZE, "%s",
			sysparm_read_str("display_panel"));
	fastboot_publish("display-panel",
			(const char *) panel_display_mode);

	snprintf(use_splash_partition, MAX_RSP_SIZE, "%d",
			sysparam_read_bool("splash_partition"));
	fastboot_publish("using-splash-partition",
			(const char *) use_splash_partition);

	snprintf(screen_resolution, MAX_RSP_SIZE, "%dx%d",
			config->width, config->height);
	fastboot_publish("screen-resolution",
			(const char *) screen_resolution);
}

void aboot_continue_boot(void) {
	static int fastboot_initialized = 0;
	int error = 0;

	// allow to override the bootmode
	if(bootmode==BOOTMODE_AUTO) {
		enum bootmode tmp = sysparam_read_bootmode("bootmode");
		dprintf(INFO, "Overwrite bootmode: %s->%s\n", strbootmode(bootmode), strbootmode(tmp));
		bootmode = tmp;
	}

	dprintf(INFO, "Booting %s...\n", strbootmode(bootmode));

	switch(bootmode) {
		case BOOTMODE_AUTO:
		case BOOTMODE_NORMAL:
	#if WITH_XIAOMI_DUALBOOT
		case BOOTMODE_SECOND:
	#endif
		case BOOTMODE_RECOVERY:
			if (target_is_emmc_boot())
			{
				if(emmc_recovery_init())
					dprintf(ALWAYS,"error in emmc_recovery_init\n");

				if(target_use_signed_kernel())
				{
					if((sysparam_read_bool("is_unlocked")) || (sysparam_read_bool("is_tampered")))
					{
					#ifdef TZ_TAMPER_FUSE
						set_tamper_fuse_cmd();
					#endif
					#if USE_PCOM_SECBOOT
						set_tamper_flag(sysparam_read_bool("is_tampered"));
					#endif
					}
				}
				boot_linux_from_mmc();
			}
			else
			{
				recovery_init();
			#if USE_PCOM_SECBOOT
				if((sysparam_read_bool("is_unlocked")) || (sysparam_read_bool("is_tampered")))
					set_tamper_flag(sysparam_read_bool("is_tampered"));
			#endif
					boot_linux_from_flash();
				}
			break;
		case BOOTMODE_DLOAD:
			if (set_download_mode(EMERGENCY_DLOAD))
			{
				dprintf(CRITICAL,"dload mode not supported by target\n");
			}
			else
			{
				reboot_device(DLOAD);
				dprintf(CRITICAL,"Failed to reboot into dload mode\n");
			}
			error = 1;
			break;
		case BOOTMODE_GRUB:
			grub_boot();
			break;

		case BOOTMODE_FASTBOOT:
		default:
			error = 1;
			break;
	}

	if(error && !fastboot_initialized) {
		/* register aboot specific fastboot commands */
		aboot_fastboot_register_commands();

		/* dump partition table for debug info */
		partition_dump();

		/* initialize and start fastboot */
		fastboot_init(target_get_scratch_address(), target_get_max_flash_size());

	#if WITH_APP_DISPLAY_SERVER
		display_server_start();
	#endif

		fastboot_initialized = 1;
	}
}

const char* strbootmode(enum bootmode bm) {
	switch(bm) {
		case BOOTMODE_AUTO:
			return "Auto";
		case BOOTMODE_NORMAL:
			return "Normal";
		#if WITH_XIAOMI_DUALBOOT
		case BOOTMODE_SECOND:
			return "Second";
		#endif
		case BOOTMODE_RECOVERY:
			return "Recovery";
		case BOOTMODE_FASTBOOT:
			return "Fastboot";
		case BOOTMODE_DLOAD:
			return "Download";
		case BOOTMODE_GRUB:
			return "GRUB";

		default:
			return "unknown";
	}
}

static void bootcount_aboot_inc(void) {
	sysparam_write_u64("bootcount_aboot", sysparam_read_u64("bootcount_aboot")+1);
	sysparam_write();
}

void aboot_init(const struct app_descriptor *app)
{
	unsigned reboot_mode = 0;

	/* Setup page size information for nv storage */
	if (target_is_emmc_boot())
	{
		page_size = mmc_page_size();
		page_mask = page_size - 1;
	}
	else
	{
		page_size = flash_page_size();
		page_mask = page_size - 1;
	}

	ASSERT((MEMBASE + MEMSIZE) > MEMBASE);

	read_device_info();

	/* Display splash screen if enabled */
#if DISPLAY_SPLASH_SCREEN
	dprintf(SPEW, "Display Init: Start\n");
	target_display_init(sysparm_read_str("display_panel"));
	dprintf(SPEW, "Display Init: Done\n");
#endif

#if BOOT_2NDSTAGE
	dprintf(INFO, "Original ATAGS: %p\n", original_atags);
	board_parse_original_atags();
#endif

	/* initialize uboot api */
	api_init();

	target_serialno((unsigned char *) sn_buf);
	dprintf(SPEW,"serial number: %s\n",sn_buf);

	/*
	 * Check power off reason if user force reset,
	 * if yes phone will do normal boot.
	 */
	if (is_user_force_reset())
		goto normal_boot;

	/* Check if we should do something other than booting up */
	if (keys_get_state(KEY_VOLUMEUP) && keys_get_state(KEY_VOLUMEDOWN))
	{
		bootmode = BOOTMODE_DLOAD;
	}
	else
	{
		if (keys_get_state(KEY_HOME) || keys_get_state(KEY_VOLUMEUP))
			bootmode = BOOTMODE_RECOVERY;
		if (bootmode!=BOOTMODE_RECOVERY &&
			(keys_get_state(KEY_BACK) || keys_get_state(KEY_VOLUMEDOWN)))
			bootmode = BOOTMODE_FASTBOOT;
	}
	#if NO_KEYPAD_DRIVER
	if (fastboot_trigger())
		bootmode = BOOTMODE_FASTBOOT;
	#endif

	reboot_mode = check_reboot_mode();
	if (reboot_mode == REBOOT_MODE_RECOVERY) {
		bootmode = BOOTMODE_RECOVERY;
	} else if(reboot_mode == REBOOT_MODE_FASTBOOT) {
		bootmode = BOOTMODE_FASTBOOT;
	} else if(reboot_mode == REBOOT_MODE_ALARM) {
		boot_reason_alarm = true;
	}
#if WITH_XIAOMI_DUALBOOT
	else if (is_dualboot_supported()) {
		if (reboot_mode == REBOOT_MODE_BOOT0) {
			bootmode = BOOTMODE_NORMAL;
		} else if (reboot_mode == REBOOT_MODE_BOOT1) {
			bootmode = BOOTMODE_SECOND;
		}
	}
#endif

	dprintf(ALWAYS, "Selected bootmode: %s\n", strbootmode(bootmode));

normal_boot:
	bootmode_normal = bootmode;
	aboot_continue_boot();
}

uint32_t get_page_size(void)
{
	return page_size;
}

/*
 * Calculated and save hash (SHA256) for non-signed boot image.
 *
 * @param image_addr - Boot image address
 * @param image_size - Size of the boot image
 *
 * @return int - 0 on success, negative value on failure.
 */
static int aboot_save_boot_hash_mmc(uint32_t image_addr, uint32_t image_size)
{
	unsigned int digest[8];
#if IMAGE_VERIF_ALGO_SHA1
	uint32_t auth_algo = CRYPTO_AUTH_ALG_SHA1;
#else
	uint32_t auth_algo = CRYPTO_AUTH_ALG_SHA256;
#endif

	target_crypto_init_params();
	hash_find((unsigned char *)image_addr, image_size, (unsigned char *)&digest, auth_algo);

	save_kernel_hash_cmd(digest);
	dprintf(INFO, "aboot_save_boot_hash_mmc: imagesize_actual size %d bytes.\n", (int) image_size);

	return 0;
}

APP_START(aboot)
	.init = aboot_init,
APP_END
