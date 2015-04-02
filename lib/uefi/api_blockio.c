#include <debug.h>
#include <assert.h>
#include <stdlib.h>
#include <uefi/api.h>
#include <lib/bio.h>

#include "uefi_p.h"
#include "charset.h"

#define MAX_DEVICES 100

typedef struct {
	bdev_t* parent;
	efi_vendor_device_path_t* parent_path;
	uint32_t index;
} bio_subdev_pdata_t;

static bdev_t* media_devices[MAX_DEVICES];
static uint32_t num_devices = 0;

static efi_status_t efi_bio_reset(struct efi_block_io* this, efi_boolean_t extended_verification) {
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}
static efi_status_t efi_bio_read_blocks(struct efi_block_io* this, efi_uint32_t media_id, efi_lba_t lba, efi_uintn_t buffer_size, void* buffer) {
	// check media id
	if(this->media->media_id>=num_devices)
		return EFI_INVALID_PARAMETER;

	// get bio dev
	bdev_t* dev = media_devices[this->media->media_id];

	// read
	bio_read_block(dev, buffer, lba, buffer_size/dev->block_size);

	return EFI_SUCCESS;
}
static efi_status_t efi_bio_write_blocks(struct efi_block_io* this, efi_uint32_t media_id, efi_lba_t lba, efi_uintn_t buffer_size, void* buffer) {
	return EFI_WRITE_PROTECTED;
}
static efi_status_t efi_bio_flush_blocks(struct efi_block_io* this) {
	return EFI_SUCCESS;
}

static void bio_subdev_cb(const char* name, void* _pdata) {
	bio_subdev_pdata_t* pdata = _pdata;
	const char* parent_name = pdata->parent->name;

	// check for free slot
	if(num_devices>=MAX_DEVICES)
		return;

	// ignore device which are not the parent itself or a subdevice of it
	if(strncmp(parent_name, name, strlen(parent_name)))
		return;

	// check if this device is the parent
	bool is_parent = !strcmp(name, parent_name);
	bool is_memdisk = !strcmp(parent_name, "grub_ramdisk");

	// open disk
	bdev_t* dev;
	if(is_parent)
		dev = pdata->parent;
	else
		dev= bio_open(name);
	if(!dev) return;

	// get partition offset
	efi_lba_t partition_start = 0;
	if(dev->is_subdev) {
		subdev_t* subdev = (subdev_t*)dev;
		partition_start = subdev->offset;
	}

	// add to media devices
	uint32_t media_id = num_devices++;
	media_devices[media_id] = dev;

	// create handle
	efi_handle_t handle = uefi_create_handle();

	// allocate media device
	efi_block_io_media_t* media = calloc(sizeof(efi_block_io_media_t), 1);
	media->media_id = media_id;
	media->removable_media = false;
	media->media_present = true;
	media->logical_partition = !is_parent;
	media->read_only = true;
	media->write_caching = false;
	media->block_size = dev->block_size;
	media->io_align = 4;
	media->last_block = dev->block_count;

	// create blockio interface
	efi_block_io_t* bio = calloc(sizeof(efi_block_io_t), 1);
	bio->revision = 0x10000;
	bio->media = media;
	bio->reset = efi_bio_reset;
	bio->read_blocks = efi_bio_read_blocks;
	bio->write_blocks = efi_bio_write_blocks;
	bio->flush_blocks = efi_bio_flush_blocks;

	// add blockio protocol
	efi_guid_t guid_block = EFI_BLOCK_IO_GUID;
	uefi_add_protocol_interface(handle, guid_block, bio);

	uint32_t path_len = pdata->parent_path->header.length;
	if(is_memdisk)
		path_len+=sizeof(efi_cdrom_device_path_t);
	else
		path_len+=sizeof(efi_hard_drive_device_path_t);

	// create devicepath based on parent's path
	void* _dp = uefi_create_device_path(path_len);
	memcpy(_dp, pdata->parent_path, pdata->parent_path->header.length);
	
	if(is_memdisk) {
		// create CDROM device path
		efi_cdrom_device_path_t* dp = _dp + pdata->parent_path->header.length;
		dp->header.type = EFI_MEDIA_DEVICE_PATH_TYPE;
		dp->header.subtype = EFI_CDROM_DEVICE_PATH_SUBTYPE;
		dp->header.length = sizeof(*dp);
		dp->partition_start = partition_start;
		dp->partition_size = dev->size/dev->block_size;
		++pdata->index;
	}
	else {
		// create HD device path
		efi_hard_drive_device_path_t* dp = _dp + pdata->parent_path->header.length;
		dp->header.type = EFI_MEDIA_DEVICE_PATH_TYPE;
		dp->header.subtype = EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE;
		dp->header.length = sizeof(*dp);
		dp->partition_number = pdata->index++;
		dp->partition_start = partition_start;
		dp->partition_size = dev->size/dev->block_size;
	}

	// add device path protocol
	efi_guid_t guid_dp = EFI_DEVICE_PATH_GUID;
	uefi_add_protocol_interface(handle, guid_dp, _dp);
}

static void bio_iter_cb(const char* name, void* pdata) {
	// check for free slot
	if(num_devices>=MAX_DEVICES)
		return;

	// open disk
	bdev_t* dev = bio_open(name);
	if(!dev) return;

	// add to media devices
	uint32_t media_id = num_devices++;
	media_devices[media_id] = dev;

	// create handle
	efi_handle_t handle = uefi_create_handle();

	// allocate media device
	efi_block_io_media_t* media = calloc(sizeof(efi_block_io_media_t), 1);
	media->media_id = media_id;
	media->removable_media = false;
	media->media_present = true;
	media->logical_partition = dev->is_subdev;
	media->read_only = true;
	media->write_caching = false;
	media->block_size = dev->block_size;
	media->io_align = 0;
	media->last_block = dev->block_count;

	// create blockio interface
	efi_block_io_t* bio = calloc(sizeof(efi_block_io_t), 1);
	bio->revision = 0x10000;
	bio->media = media;
	bio->reset = efi_bio_reset;
	bio->read_blocks = efi_bio_read_blocks;
	bio->write_blocks = efi_bio_write_blocks;
	bio->flush_blocks = efi_bio_flush_blocks;

	// add blockio protocol
	efi_guid_t guid_block = EFI_BLOCK_IO_GUID;
	uefi_add_protocol_interface(handle, guid_block, bio);

	// add vendor device path with a random GUID
	efi_vendor_device_path_t* dp = uefi_create_device_path(sizeof(efi_vendor_device_path_t));
	dp->header.type = EFI_HARDWARE_DEVICE_PATH_TYPE;
	dp->header.subtype = EFI_VENDOR_DEVICE_PATH_SUBTYPE;
	dp->header.length = sizeof(*dp);
	random_guid(&dp->vendor_guid);

	// add partitions
	bio_subdev_pdata_t subdev_pdata = {dev, dp, 0};
	bio_foreach(bio_subdev_cb, true, &subdev_pdata);

	// add device path protocol
	efi_guid_t guid_dp = EFI_DEVICE_PATH_GUID;
	uefi_add_protocol_interface(handle, guid_dp, dp);
}

efi_status_t uefi_api_blockio_init(void) {
	bio_foreach(bio_iter_cb, false, NULL);

	return EFI_SUCCESS;
}

efi_status_t uefi_api_blockio_by_name(efi_handle_t image_handle, const char* name, efi_handle_t* dev_handle) {
	// get boot device handle
	efi_handle_t handles[MAX_DEVICES];
	efi_uintn_t bufsize = ARRAY_SIZE(handles)*sizeof(*handles);
	efi_guid_t guid_block = EFI_BLOCK_IO_GUID;
	if(efi_system_table.boot_services->locate_handle(EFI_BY_PROTOCOL, &guid_block, 0, &bufsize, handles)==EFI_SUCCESS) {
		uint32_t i;
		for(i=0; i<bufsize/sizeof(*handles); i++) {
			efi_block_io_t* bio;
			if(efi_system_table.boot_services->open_protocol(handles[i], &guid_block, (void**)&bio, image_handle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL)!=EFI_SUCCESS)
				continue;

			// get bio dev
			bdev_t* dev = media_devices[bio->media->media_id]; 

			if(!strcmp(dev->name, name)) {
				*dev_handle = handles[i];
				return EFI_SUCCESS;
			}
		}
	}

	return EFI_NOT_FOUND;
}
