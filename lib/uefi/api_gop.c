#include <debug.h>
#include <assert.h>
#include <stdlib.h>
#include <uefi/api.h>
#include <uefi/graphics_output.h>
#include <dev/fbcon.h>

static efi_status_t uefi_gop_query_mode(struct efi_gop* this,
			     efi_uint32_t mode_number,
			     efi_uintn_t* size_of_info,
			     struct efi_gop_mode_info** info) {
	*info = this->mode->info;
	return EFI_SUCCESS;
}

static efi_status_t uefi_gop_set_mode(struct efi_gop* this, efi_uint32_t mode_number) {
	return EFI_SUCCESS;
}

static efi_status_t uefi_gop_blt_buffer_to_video(struct efi_gop* this,
		      void *buffer,
		      efi_uintn_t sx,
		      efi_uintn_t sy,
		      efi_uintn_t dx,
		      efi_uintn_t dy,
		      efi_uintn_t width, efi_uintn_t height, efi_uintn_t delta) {

	// video buffer
	void* fb_base = (efi_uintn_t*)(efi_uintn_t)this->mode->fb_base;
	efi_uint32_t pixels_per_scanline = this->mode->info->pixels_per_scanline;

	// blt buffer
	efi_uint32_t src_pxsz = sizeof(struct efi_gop_blt_pixel);
	efi_uint32_t src_bytes_per_line = width * src_pxsz;
	if(delta && delta!=width*src_pxsz)
		src_bytes_per_line = delta;

	// copy buffer
	efi_uint32_t sl,dl,sp,dp;
	for(sl=sy,dl=dy; sl<sy+height; sl++,dl++) {
		for(sp=sx,dp=dx; sp<sx+width; sp++,dp++) {
			struct efi_gop_blt_pixel* px_src  = buffer + (sl*src_bytes_per_line) + (sp*src_pxsz);
			efi_uint8_t* px_dst = fb_base + (dl*pixels_per_scanline) + (dp*3);

			px_dst[0] = px_src->blue;
			px_dst[1] = px_src->green;
			px_dst[2] = px_src->red;
		}
	}

	// flush framebuffer
	fbcon_flush();

	return EFI_SUCCESS;
}

static efi_status_t uefi_gop_blt(struct efi_gop* this,
		      void *buffer,
		      efi_uintn_t operation,
		      efi_uintn_t sx,
		      efi_uintn_t sy,
		      efi_uintn_t dx,
		      efi_uintn_t dy,
		      efi_uintn_t width, efi_uintn_t height, efi_uintn_t delta) {
	switch (operation) {
		case EFI_BLT_VIDEO_FILL:
			DEBUG_ASSERT(0);
			return EFI_INVALID_PARAMETER;
		break;

		case EFI_BLT_VIDEO_TO_BLT_BUFFER:
			DEBUG_ASSERT(0);
			return EFI_INVALID_PARAMETER;
		break;

		case EFI_BLT_BUFFER_TO_VIDEO:
			return uefi_gop_blt_buffer_to_video(this, buffer, sx, sy, dx, dy, width, height, delta);
		break;

		case EFI_BLT_VIDEO_TO_VIDEO:
			DEBUG_ASSERT(0);
			return EFI_INVALID_PARAMETER;
		break;

		case EFI_BLT_OPERATION_MAX:
		default:
			DEBUG_ASSERT(0);
			return EFI_INVALID_PARAMETER;
		break;
	}

	return EFI_SUCCESS;
}

efi_status_t uefi_api_gop_init(void) {
	// open fbcon display
	struct fbcon_config* fbcon_config = fbcon_display();
	if(!fbcon_config) return EFI_UNSUPPORTED;

	// create handle
	efi_handle_t handle = uefi_create_handle();

	// allocate mode_info
	struct efi_gop_mode_info *mode_info = calloc(sizeof(struct efi_gop_mode_info), 1);	
	mode_info->version = 0x10000;
	mode_info->width = fbcon_config->width;
	mode_info->height = fbcon_config->height;
	mode_info->pixel_format = EFI_GOT_RGBA8;
	mode_info->pixel_bitmask.r = 0x00ff0000;
	mode_info->pixel_bitmask.g = 0x0000ff00;
	mode_info->pixel_bitmask.b = 0x000000ff;
	mode_info->pixel_bitmask.a = 0xFF000000;
	mode_info->pixels_per_scanline = mode_info->width * (fbcon_config->bpp/8);

	// allocate mode
	struct efi_gop_mode *mode = calloc(sizeof(struct efi_gop_mode), 1);
	mode->max_mode = 1;
	mode->mode = 0;
	mode->info = mode_info;
	mode->info_size = sizeof(*mode_info);
	mode->fb_base = (efi_physical_address_t)(uint32_t)fbcon_config->base;
	mode->fb_size = fbcon_config->width*fbcon_config->height*(fbcon_config->bpp/8);
	

	// create protocol interface
	struct efi_gop* gop = calloc(sizeof(struct efi_gop), 1);
	gop->query_mode = uefi_gop_query_mode;
	gop->set_mode = uefi_gop_set_mode;
	gop->blt = uefi_gop_blt;
	gop->mode = mode;

	// add gop protocol
	efi_guid_t guid_gop = EFI_GOP_GUID;
	uefi_add_protocol_interface(handle, guid_gop, gop);

	return EFI_SUCCESS;
}
