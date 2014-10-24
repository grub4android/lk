/*
 * (C) Copyright 2007 Semihalf
 *
 * Written by: Rafal Jaworowski <raj@semihalf.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

//#include <config.h>
//#include <command.h>
//#include <common.h>
#include <malloc.h>
//#include <environment.h>
//#include <linux/types.h>
#include <string.h>
#include <platform.h>
#include <platform/timer.h>
#include <dev/uart.h>
#include <dev/fbcon.h>
#include <dev/keys.h>
#include <debug.h>
#include <arch/ops.h>
#include <include/boot_stats.h>
#include <kernel/thread.h>
#include <target.h>
#include <platform.h>
#include <mipi_dsi.h>
#include <api_public.h>

#if DEVICE_TREE
#include <libfdt.h>
#include <dev_tree.h>
#endif

#include "api_private.h"
#include "../grub.h"
#include "../bootimg.h"

extern unsigned int calculate_crc32(unsigned char *buffer, int len);
extern unsigned char *update_cmdline(const char * cmdline);
extern void generate_atags(unsigned *ptr, const char *cmdline,
                    void *ramdisk, unsigned ramdisk_size);
extern int check_aboot_addr_range_overlap(uint32_t start, uint32_t size);
extern void update_ker_tags_rdisk_addr(struct boot_img_hdr *hdr, bool is_arm64);



/*****************************************************************************
 *
 * This is the API core.
 *
 * API_ functions are part of U-Boot code and constitute the lowest level
 * calls:
 *
 *  - they know what values they need as arguments
 *  - their direct return value pertains to the API_ "shell" itself (0 on
 *    success, some error code otherwise)
 *  - if the call returns a value it is buried within arguments
 *
 ****************************************************************************/

typedef	int (*cfp_t)(va_list argp);

static int calls_no;

/*
 * pseudo signature:
 *
 * int API_getc(int *c)
 */
static int API_getc(va_list ap)
{
	int *c;

	if ((c = (int *)va_arg(ap, uint32_t)) == NULL)
		return API_EINVAL;

#if WITH_DEBUG_UART
	*c = uart_getc(0, 0);
#endif
	return 0;
}

/*
 * pseudo signature:
 *
 * int API_tstc(int *c)
 */
static int API_tstc(va_list ap)
{
	int *t;

	if ((t = (int *)va_arg(ap, uint32_t)) == NULL)
		return API_EINVAL;

#if WITH_DEBUG_UART
	*t = uart_tstc(0);
#else
	*t = 0;
#endif

	return 0;
}

/*
 * pseudo signature:
 *
 * int API_putc(char *ch)
 */
static int API_putc(va_list ap)
{
	char *c;

	if ((c = (char *)va_arg(ap, uint32_t)) == NULL)
		return API_EINVAL;

	_dputc(*c);
	return 0;
}

/*
 * pseudo signature:
 *
 * int API_puts(char **s)
 */
static int API_puts(va_list ap)
{
	char *s;

	if ((s = (char *)va_arg(ap, uint32_t)) == NULL)
		return API_EINVAL;

	_dputs(s);
	return 0;
}

/*
 * pseudo signature:
 *
 * int API_reset(void)
 */
static int API_reset(va_list ap)
{
	reboot_device(NORMAL_MODE);

	/* NOT REACHED */
	return 0;
}

/*
 * pseudo signature:
 *
 * int API_get_sys_info(struct sys_info *si)
 *
 * fill out the sys_info struct containing selected parameters about the
 * machine
 */
static int API_get_sys_info(va_list ap)
{
	struct sys_info *si;

	si = (struct sys_info *)va_arg(ap, uint32_t);
	if (si == NULL)
		return API_ENOMEM;

	// kernel memory
	platform_set_mr(si, BASE_ADDR, 44*1024*1024, MR_ATTR_DRAM);
	// scratch memory(LK uses this as a fastboot buffer)
	platform_set_mr(si, (unsigned long)target_get_scratch_address(), target_get_max_flash_size(), MR_ATTR_DRAM);

	return 0;
}

/*
 * pseudo signature:
 *
 * int API_udelay(unsigned long *udelay)
 */
static int API_udelay(va_list ap)
{
	unsigned long *d;

	if ((d = (unsigned long *)va_arg(ap, uint32_t)) == NULL)
		return API_EINVAL;

	spin(*d);
	return 0;
}

/*
 * pseudo signature:
 *
 * int API_get_timer(unsigned long *current, unsigned long *base)
 */
static int API_get_timer(va_list ap)
{
	unsigned long *base, *cur;

	cur = (unsigned long *)va_arg(ap, uint32_t);
	if (cur == NULL)
		return API_EINVAL;

	base = (unsigned long *)va_arg(ap, uint32_t);
	if (base == NULL)
		return API_EINVAL;

	*cur = current_time() + *base;
	return 0;
}


/*****************************************************************************
 *
 * pseudo signature:
 *
 * int API_dev_enum(struct device_info *)
 *
 *
 * cookies uniqely identify the previously enumerated device instance and
 * provide a hint for what to inspect in current enum iteration:
 *
 *   - net: &eth_device struct address from list pointed to by eth_devices
 *
 *   - storage: block_dev_desc_t struct address from &ide_dev_desc[n],
 *     &scsi_dev_desc[n] and similar tables
 *
 ****************************************************************************/

static int API_dev_enum(va_list ap)
{
	struct uboot_device_info *di;

	/* arg is ptr to the device_info struct we are going to fill out */
	di = (struct uboot_device_info *)va_arg(ap, uint32_t);
	if (di == NULL)
		return API_EINVAL;

	if (di->cookie == NULL) {
		/* start over - clean up enumeration */
		dev_enum_reset();	/* XXX shouldn't the name contain 'stor'? */
		dprintf(SPEW, "RESTART ENUM\n");

		/* net device enumeration first */
		//if (dev_enum_net(di))
			//return 0;
	}

	/*
	 * The hidden assumption is there can only be one active network
	 * device and it is identified upon enumeration (re)start, so there's
	 * no point in trying to find network devices in other cases than the
	 * (re)start and hence the 'next' device can only be storage
	 */
	if (!dev_enum_storage(di))
		/* make sure we mark there are no more devices */
		di->cookie = NULL;

	return 0;
}


static int API_dev_open(va_list ap)
{
	struct uboot_device_info *di;
	int err = 0;

	/* arg is ptr to the device_info struct */
	di = (struct uboot_device_info *)va_arg(ap, uint32_t);
	if (di == NULL)
		return API_EINVAL;

	/* Allow only one consumer of the device at a time */
	if (di->state == DEV_STA_OPEN)
		return API_EBUSY;

	if (di->cookie == NULL)
		return API_ENODEV;

	if (di->type & DEV_TYP_STOR)
		err = dev_open_stor(di->cookie);

	//else if (di->type & DEV_TYP_NET)
		//err = dev_open_net(di->cookie);
	else
		err = API_ENODEV;

	if (!err)
		di->state = DEV_STA_OPEN;

	return err;
}


static int API_dev_close(va_list ap)
{
	struct uboot_device_info *di;
	int err = 0;

	/* arg is ptr to the device_info struct */
	di = (struct uboot_device_info *)va_arg(ap, uint32_t);
	if (di == NULL)
		return API_EINVAL;

	if (di->state == DEV_STA_CLOSED)
		return 0;

	if (di->cookie == NULL)
		return API_ENODEV;

	if (di->type & DEV_TYP_STOR)
		err = dev_close_stor(di->cookie);

	//else if (di->type & DEV_TYP_NET)
		//err = dev_close_net(di->cookie);
	else
		/*
		 * In case of unknown device we cannot change its state, so
		 * only return error code
		 */
		err = API_ENODEV;

	if (!err)
		di->state = DEV_STA_CLOSED;

	return err;
}


/*
 * Notice: this is for sending network packets only, as U-Boot does not
 * support writing to storage at the moment (12.2007)
 *
 * pseudo signature:
 *
 * int API_dev_write(
 *	struct device_info *di,
 *	void *buf,
 *	int *len
 * )
 *
 * buf:	ptr to buffer from where to get the data to send
 *
 * len: length of packet to be sent (in bytes)
 *
 */
static int API_dev_write(va_list ap)
{
	struct uboot_device_info *di;
	void *buf;
	int *len;
	int err = 0;

	/* 1. arg is ptr to the device_info struct */
	di = (struct uboot_device_info *)va_arg(ap, uint32_t);
	if (di == NULL)
		return API_EINVAL;

	/* XXX should we check if device is open? i.e. the ->state ? */

	if (di->cookie == NULL)
		return API_ENODEV;

	/* 2. arg is ptr to buffer from where to get data to write */
	buf = (void *)va_arg(ap, uint32_t);
	if (buf == NULL)
		return API_EINVAL;

	/* 3. arg is length of buffer */
	len = (int *)va_arg(ap, uint32_t);
	if (len == NULL)
		return API_EINVAL;
	if (*len <= 0)
		return API_EINVAL;

	if (di->type & DEV_TYP_STOR)
		/*
		 * write to storage is currently not supported by U-Boot:
		 * no storage device implements block_write() method
		 */
		return API_ENODEV;

	//else if (di->type & DEV_TYP_NET)
		//err = dev_write_net(di->cookie, buf, *len);
	else
		err = API_ENODEV;

	return err;
}


/*
 * pseudo signature:
 *
 * int API_dev_read(
 *	struct device_info *di,
 *	void *buf,
 *	size_t *len,
 *	unsigned long *start
 *	size_t *act_len
 * )
 *
 * buf:	ptr to buffer where to put the read data
 *
 * len: ptr to length to be read
 *      - network: len of packet to read (in bytes)
 *      - storage: # of blocks to read (can vary in size depending on define)
 *
 * start: ptr to start block (only used for storage devices, ignored for
 *        network)
 *
 * act_len: ptr to where to put the len actually read
 */
static int API_dev_read(va_list ap)
{
	struct uboot_device_info *di;
	void *buf;
	lbasize_t *len_stor, *act_len_stor;
	lbastart_t *start;
	//int *len_net, *act_len_net;

	/* 1. arg is ptr to the device_info struct */
	di = (struct uboot_device_info *)va_arg(ap, uint32_t);
	if (di == NULL)
		return API_EINVAL;

	/* XXX should we check if device is open? i.e. the ->state ? */

	if (di->cookie == NULL)
		return API_ENODEV;

	/* 2. arg is ptr to buffer from where to put the read data */
	buf = (void *)va_arg(ap, uint32_t);
	if (buf == NULL)
		return API_EINVAL;

	if (di->type & DEV_TYP_STOR) {
		/* 3. arg - ptr to var with # of blocks to read */
		len_stor = (lbasize_t *)va_arg(ap, uint32_t);
		if (!len_stor)
			return API_EINVAL;
		if (*len_stor <= 0)
			return API_EINVAL;

		/* 4. arg - ptr to var with start block */
		start = (lbastart_t *)va_arg(ap, uint32_t);

		/* 5. arg - ptr to var where to put the len actually read */
		act_len_stor = (lbasize_t *)va_arg(ap, uint32_t);
		if (!act_len_stor)
			return API_EINVAL;

		*act_len_stor = dev_read_stor(di->cookie, buf, *len_stor, *start);

	} else
		return API_ENODEV;

	return 0;
}


/*
 * pseudo signature:
 *
 * int API_env_get(const char *name, char **value)
 *
 * name: ptr to name of env var
 */
static int API_env_get(va_list ap)
{
	char *name, **value;

	if ((name = (char *)va_arg(ap, uint32_t)) == NULL)
		return API_EINVAL;
	if ((value = (char **)va_arg(ap, uint32_t)) == NULL)
		return API_EINVAL;

	if(strcmp("grub_bootdev", name)==0)
		grub_get_bootdev(value);
	else if(strcmp("grub_bootpath", name)==0)
		grub_get_bootpath(value);
	else {
		dprintf(INFO, "%s: %s\n", __func__, name);
		*value = NULL;
	}

	return 0;
}

/*
 * pseudo signature:
 *
 * int API_env_set(const char *name, const char *value)
 *
 * name: ptr to name of env var
 *
 * value: ptr to value to be set
 */
static int API_env_set(va_list ap)
{
	char *name, *value;

	if ((name = (char *)va_arg(ap, uint32_t)) == NULL)
		return API_EINVAL;
	if ((value = (char *)va_arg(ap, uint32_t)) == NULL)
		return API_EINVAL;

	dprintf(INFO, "%s: %s=%s\n", __func__, name, value);

	return 0;
}

/*
 * pseudo signature:
 *
 * int API_env_enum(const char *last, char **next)
 *
 * last: ptr to name of env var found in last iteration
 */
static int API_env_enum(va_list ap)
{
	dprintf(INFO, "%s\n", __func__);
	return 0;
}

/*
 * pseudo signature:
 *
 * int API_display_get_info(int type, struct display_info *di)
 */
static int API_display_get_info(va_list ap)
{
	int type;
	struct display_info *di;

	type = va_arg(ap, int);
	di = va_arg(ap, struct display_info *);

	return display_get_info(type, di);
}

/*
 * pseudo signature:
 *
 * int API_display_draw_bitmap(ulong bitmap, int x, int y)
 */
static int API_display_draw_bitmap(va_list ap)
{
	ulong bitmap;
	int x, y;

	bitmap = va_arg(ap, ulong);
	x = va_arg(ap, int);
	y = va_arg(ap, int);

	return display_draw_bitmap(bitmap, x, y);
}

/*
 * pseudo signature:
 *
 * void API_display_clear(void)
 */
static int API_display_clear(va_list ap)
{
	display_clear();
	return 0;
}

/*
 * pseudo signature:
 *
 * void API_display_fb_get(unsigned long* fb)
 */
static int API_display_fb_get(va_list ap)
{
	unsigned long *fb;
	struct fbcon_config* config =  fbcon_display();

	if ((fb = (unsigned long *)va_arg(ap, uint32_t)) == NULL)
		return API_EINVAL;

	*fb = (unsigned long)config->base;
	return 0;
}

/*
 * pseudo signature:
 *
 * void API_display_fb_flush(void)
 */
static int API_display_fb_flush(va_list ap)
{
	fbcon_flush();
	return 0;
}

static int keymap[MAX_KEYS];
#define CHECK_AND_REPORT_KEY(code, value) \
	if(value) { \
		if(!keymap[code]){ \
			keymap[code] = 1; \
			return code; \
		} \
	} else{keymap[code]=0;}

/*
 * pseudo signature:
 *
 * int API_input_getkey(void)
 */
static int API_input_getkey(va_list ap)
{
	// small delay to prevent unwanted keypresses
	spin(1000);

	CHECK_AND_REPORT_KEY(KEY_UP, target_volume_up());
	CHECK_AND_REPORT_KEY(KEY_DOWN, target_volume_down());
	CHECK_AND_REPORT_KEY(KEY_RIGHT, target_power_key());

	return 0;
}

/*
 * pseudo signature:
 *
 * void API_boot_update_addresses(struct tag_info *info, int is_arm64)
 */
static int API_boot_update_addresses(va_list ap)
{
	struct boot_img_hdr *hdr = va_arg(ap, struct boot_img_hdr *);
	int is_arm64 = va_arg(ap, int);

	update_ker_tags_rdisk_addr(hdr, is_arm64);
	return 0;
}


#if DEVICE_TREE
static int copy_dtb(struct tags_info *info)
{
	struct dt_table *table;
	struct dt_entry dt_entry;
	uint32_t dt_hdr_size;

	if(info->dt_size != 0) {
		/* offset now point to start of dt.img */
		table = (struct dt_table*)(info->dt);

		if (dev_tree_validate(table, info->page_size, &dt_hdr_size) != 0) {
			dprintf(CRITICAL, "ERROR: Cannot validate Device Tree Table \n");
			return -1;
		}
		/* Find index of device tree within device tree table */
		if(dev_tree_get_entry_info(table, &dt_entry) != 0){
			dprintf(CRITICAL, "ERROR: Getting device tree address failed\n");
			return -1;
		}

		/* Validate and Read device device tree in the "tags_add */
		if (check_aboot_addr_range_overlap((uint32_t)info->tags_addr, dt_entry.size))
		{
			dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
			return -1;
		}

		/* Read device device tree in the "tags_add */
		memmove((void*) info->tags_addr,
				info->dt +  dt_entry.offset,
				dt_entry.size);
	} else
		return -1;

	/* Everything looks fine. Return success. */
	return 0;
}
#endif

/*
 * pseudo signature:
 *
 * int API_boot_create_tags(struct tag_info *info)
 */
static int API_boot_create_tags(va_list ap)
{
	struct tags_info *info = va_arg(ap, struct tags_info *);
	unsigned char *final_cmdline = update_cmdline(info->cmdline);

#if DEVICE_TREE
	if(info->dt_size>0) {
		// copy dtb to final destination
		if(copy_dtb(info)) {
			dprintf(CRITICAL, "Couldn't find DTB.\n");
			return API_EINVAL;
		}

		// Update the Device Tree
		if(update_device_tree(info->tags_addr, (const char *)final_cmdline, info->ramdisk, info->ramdisk_size)) {
			dprintf(CRITICAL, "ERROR: Updating Device Tree Failed\n");
			return API_EINVAL;
		}
	}
	else
#endif
	{
#if !DEVICE_TREE || DEVICE_TREE_FALLBACK
		generate_atags(info->tags_addr, (const char*)final_cmdline, info->ramdisk, info->ramdisk_size);
#endif
	}

	return 0;
}

/*
 * pseudo signature:
 *
 * int API_boot_prepare(void)
 */
static int API_boot_prepare(va_list ap)
{
	/* Perform target specific cleanup */
	target_uninit();

	/* Turn off splash screen if enabled */
	#if DISPLAY_SPLASH_SCREEN
	target_display_shutdown();
	#endif

	enter_critical_section();

	/* do any platform specific cleanup before kernel entry */
	platform_uninit();

	arch_disable_cache(UCACHE);

	#if ARM_WITH_MMU
	arch_disable_mmu();
	#endif
	bs_set_timestamp(BS_KERNEL_ENTRY);

	return 0;
}


static cfp_t calls_table[API_MAXCALL] = { NULL, };

/*
 * The main syscall entry point - this is not reentrant, only one call is
 * serviced until finished.
 *
 * e.g. syscall(1, int *, uint32_t, uint32_t, uint32_t, uint32_t);
 *
 * call:	syscall number
 *
 * retval:	points to the return value placeholder, this is the place the
 *		syscall puts its return value, if NULL the caller does not
 *		expect a return value
 *
 * ...		syscall arguments (variable number)
 *
 * returns:	0 if the call not found, 1 if serviced
 */
int syscall(int call, int *retval, ...)
{
	va_list	ap;
	int rv;

	if (call < 0 || call >= calls_no) {
		dprintf(CRITICAL, "invalid call #%d\n", call);
		return 0;
	}

	if (calls_table[call] == NULL) {
		dprintf(CRITICAL, "syscall #%d does not have a handler\n", call);
		return 0;
	}

	va_start(ap, retval);
	rv = calls_table[call](ap);
	if (retval != NULL)
		*retval = rv;

	return 1;
}

void api_init(void)
{
	struct api_signature *sig = NULL;
	char* api_sig_magic = API_SIG_MAGIC;

	/* TODO put this into linker set one day... */
	calls_table[API_RSVD] = NULL;
	calls_table[API_GETC] = &API_getc;
	calls_table[API_PUTC] = &API_putc;
	calls_table[API_TSTC] = &API_tstc;
	calls_table[API_PUTS] = &API_puts;
	calls_table[API_RESET] = &API_reset;
	calls_table[API_GET_SYS_INFO] = &API_get_sys_info;
	calls_table[API_UDELAY] = &API_udelay;
	calls_table[API_GET_TIMER] = &API_get_timer;
	calls_table[API_DEV_ENUM] = &API_dev_enum;
	calls_table[API_DEV_OPEN] = &API_dev_open;
	calls_table[API_DEV_CLOSE] = &API_dev_close;
	calls_table[API_DEV_READ] = &API_dev_read;
	calls_table[API_DEV_WRITE] = &API_dev_write;
	calls_table[API_ENV_GET] = &API_env_get;
	calls_table[API_ENV_SET] = &API_env_set;
	calls_table[API_ENV_ENUM] = &API_env_enum;
	calls_table[API_DISPLAY_GET_INFO] = &API_display_get_info;
	calls_table[API_DISPLAY_DRAW_BITMAP] = &API_display_draw_bitmap;
	calls_table[API_DISPLAY_CLEAR] = &API_display_clear;
	calls_table[API_DISPLAY_FB_GET] = &API_display_fb_get;
	calls_table[API_DISPLAY_FB_FLUSH] = &API_display_fb_flush;
	calls_table[API_INPUT_GETKEY] = &API_input_getkey;
	calls_table[API_BOOT_UPDATE_ADDRESSES] = &API_boot_update_addresses;
	calls_table[API_BOOT_CREATE_TAGS] = &API_boot_create_tags;
	calls_table[API_BOOT_PREPARE] = &API_boot_prepare;
	calls_no = API_MAXCALL;

	dprintf(INFO, "API initialized with %d calls\n", calls_no);

	dev_stor_init();

	/*
	 * Produce the signature so the API consumers can find it
	 */
	sig = memalign(8, sizeof(struct api_signature));
	if (sig == NULL) {
		dprintf(CRITICAL, "API: could not allocate memory for the signature!\n");
		return;
	}

	dprintf(INFO, "API sig @ %p\n", sig);
	memcpy(sig->magic, api_sig_magic, 8);
	sig->version = API_SIG_VERSION;
	sig->syscall = &syscall;
	sig->checksum = 0;
	sig->checksum = calculate_crc32((unsigned char *)sig,
			      sizeof(struct api_signature));
	dprintf(INFO, "syscall entry: %p\n", sig->syscall);

	memset(api_sig_magic, 0, strlen(api_sig_magic));
}

void platform_set_mr(struct sys_info *si, unsigned long start, unsigned long size,
			int flags)
{
	int i;

	if (!si->mr || !size || (flags == 0))
		return;

	/* find free slot */
	for (i = 0; i < si->mr_no; i++)
		if (si->mr[i].flags == 0) {
			/* insert new mem region */
			si->mr[i].start = start;
			si->mr[i].size = size;
			si->mr[i].flags = flags;
			return;
		}
}
