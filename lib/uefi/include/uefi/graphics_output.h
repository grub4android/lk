/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EFI_GOP_HEADER
#define EFI_GOP_HEADER	1

/* Based on UEFI specification.  */

#define EFI_GOP_GUID \
  { 0x9042a9de, 0x23dc, 0x4a38, { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a }}

typedef enum {
	EFI_GOT_RGBA8,
	EFI_GOT_BGRA8,
	EFI_GOT_BITMASK
} efi_gop_pixel_format_t;

typedef enum {
	EFI_BLT_VIDEO_FILL,
	EFI_BLT_VIDEO_TO_BLT_BUFFER,
	EFI_BLT_BUFFER_TO_VIDEO,
	EFI_BLT_VIDEO_TO_VIDEO,
	EFI_BLT_OPERATION_MAX
} efi_gop_blt_operation_t;

struct efi_gop_blt_pixel {
	uint8_t blue;
	uint8_t green;
	uint8_t red;
	uint8_t reserved;
};

struct efi_gop_pixel_bitmask {
	uint32_t r;
	uint32_t g;
	uint32_t b;
	uint32_t a;
};

struct efi_gop_mode_info {
	efi_uint32_t version;
	efi_uint32_t width;
	efi_uint32_t height;
	efi_gop_pixel_format_t pixel_format;
	struct efi_gop_pixel_bitmask pixel_bitmask;
	efi_uint32_t pixels_per_scanline;
};

struct efi_gop_mode {
	efi_uint32_t max_mode;
	efi_uint32_t mode;
	struct efi_gop_mode_info *info;
	efi_uintn_t info_size;
	efi_physical_address_t fb_base;
	efi_uintn_t fb_size;
};

/* Forward declaration.  */
struct efi_gop;

typedef efi_status_t
    (*efi_gop_query_mode_t) (struct efi_gop * this,
			     efi_uint32_t mode_number,
			     efi_uintn_t * size_of_info,
			     struct efi_gop_mode_info ** info);

typedef efi_status_t
    (*efi_gop_set_mode_t) (struct efi_gop * this, efi_uint32_t mode_number);

typedef efi_status_t
    (*efi_gop_blt_t) (struct efi_gop * this,
		      void *buffer,
		      efi_uintn_t operation,
		      efi_uintn_t sx,
		      efi_uintn_t sy,
		      efi_uintn_t dx,
		      efi_uintn_t dy,
		      efi_uintn_t width, efi_uintn_t height, efi_uintn_t delta);

struct efi_gop {
	efi_gop_query_mode_t query_mode;
	efi_gop_set_mode_t set_mode;
	efi_gop_blt_t blt;
	struct efi_gop_mode *mode;
};

#endif
