#include <debug.h>
#include <assert.h>
#include <stdlib.h>
#include <uefi/api.h>
#include <uefi/console_control.h>

static efi_screen_mode_t cc_mode = EFI_SCREEN_TEXT;
static efi_boolean_t cc_std_in_locked = 0;

static efi_status_t uefi_cc_get_mode(struct efi_console_control_protocol* this, efi_screen_mode_t* mode, efi_boolean_t* uga_exists, efi_boolean_t* std_in_locked) {
	if(mode)
		*mode = cc_mode;
	if(uga_exists)
		*uga_exists = 0;
	if(std_in_locked)
		*std_in_locked = cc_std_in_locked;
	return EFI_SUCCESS;
}

static efi_status_t uefi_cc_set_mode(struct efi_console_control_protocol* this, efi_screen_mode_t mode) {
	cc_mode = mode;
	return EFI_SUCCESS;
}

static efi_status_t uefi_cc_lock_std_in(struct efi_console_control_protocol* this, efi_char16_t* password) {
	cc_std_in_locked = 1;
	return EFI_SUCCESS;
}

efi_status_t uefi_api_console_init(void) {
	// create handle
	efi_handle_t handle = uefi_create_handle();

	// create protocol interface
	efi_console_control_protocol_t* cc = calloc(sizeof(efi_console_control_protocol_t), 1);
	cc->get_mode = uefi_cc_get_mode;
	cc->set_mode = uefi_cc_set_mode;
	cc->lock_std_in = uefi_cc_lock_std_in;

	// add gop protocol
	efi_guid_t guid_cc = EFI_CONSOLE_CONTROL_GUID;
	uefi_add_protocol_interface(handle, guid_cc, cc);

	return EFI_SUCCESS;
}
