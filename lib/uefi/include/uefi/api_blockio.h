#ifndef EFI_BLOCKIO_HEADER
#define EFI_BLOCKIO_HEADER	1

#include <uefi/api.h>

efi_status_t uefi_api_blockio_init(void);
efi_status_t uefi_api_blockio_by_name(efi_handle_t image_handle, const char* name, efi_handle_t* dev_handle);

#endif
