#undef LK_DEBUGLEVEL
#define LK_DEBUGLEVEL 2

#include <err.h>
#include <debug.h>
#include <assert.h>
#include <malloc.h>
#include <lib/rtc.h>
#include <platform.h>
#include <lib/sysparam.h>
#include <uefi/api.h>
#include <uefi/api_simpletext.h>

#include "uefi_p.h"

efi_status_t efi_get_time(efi_time_t *time, efi_time_capabilities_t *capabilities)
{
	// check time
	if(!time)
		return EFI_INVALID_PARAMETER;

	// use timer clock as time source
	// TODO: use real RTC drivers
	struct rtc_time t;
	rtc_time_to_tm(current_time()/1000, &t);
	
	time->year = t.tm_year;
	time->month = t.tm_mon+1;
	time->day = t.tm_mday;
	time->hour = t.tm_hour;
	time->minute = t.tm_min;
	time->second = t.tm_sec;
	time->nanosecond = 0;
	time->time_zone = EFI_UNSPECIFIED_TIMEZONE;
	time->daylight = 0;

	if(capabilities) {
		capabilities->resolution = 1;
		capabilities->accuracy = 50000000;
		capabilities->sets_to_zero = false;
	}

	return EFI_SUCCESS;
}

efi_status_t efi_set_time (efi_time_t *time) {
	DEBUG_ASSERT(0);
	return EFI_SUCCESS;
}

/* Following 2 functions are vendor specific. So announce it as unsupported */
efi_status_t efi_get_wakeup_time (efi_boolean_t *enabled, efi_boolean_t *pending, efi_time_t *time)
{
	DEBUG_ASSERT(0);
	return EFI_SUCCESS;
}

efi_status_t efi_set_wakeup_time (efi_boolean_t enabled, efi_time_t *time)
{
	DEBUG_ASSERT(0);
	return EFI_SUCCESS;
}

efi_status_t efi_set_virtual_address_map (efi_uintn_t memory_map_size,
				  efi_uintn_t descriptor_size,
				  efi_uint32_t descriptor_version,
				  efi_memory_descriptor_t *virtual_map)
{
	DEBUG_ASSERT(0);
	return EFI_SUCCESS;
}

/* since efi_set_virtual_address_map corrects all the pointers
   we don't need efi_convert_pointer */
efi_status_t efi_convert_pointer(efi_uintn_t debug_disposition, void **address)
{
	DEBUG_ASSERT(0);
	return EFI_SUCCESS;
}

efi_status_t efi_get_variable(efi_char16_t *variable_name,
				const efi_guid_t *vendor_guid,
				efi_uint32_t *attributes,
				efi_uintn_t *data_size,
				void *data)
{
	efi_status_t status = EFI_SUCCESS;

	// check name
	if(!variable_name)
		return EFI_INVALID_PARAMETER;
	// check guid
	if(!vendor_guid)
		return EFI_INVALID_PARAMETER;
	// check name
	if(!data_size)
		return EFI_INVALID_PARAMETER;

	// check GUID
	efi_guid_t guid_gv = EFI_GLOBAL_VARIABLE_GUID;
	if(!GUID_MATCHES(*vendor_guid, guid_gv)) {
		dprintf(SPEW, "%s: unsupported GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(*vendor_guid));
	  	DEBUG_ASSERT(0);
		return EFI_NOT_FOUND;
	}

	// convert name
	efi_char8_t* name8 = malloc(strlen16(variable_name)+1);
	SafeUnicodeStrToAsciiStr(variable_name, name8);

	// try to read variable
	ssize_t len = sysparam_read((const char *)name8, data, *data_size);

	// not found
	if(len==ERR_NOT_FOUND) {
		status = EFI_NOT_FOUND;
		goto out;
	}

	// buffer is too small
	else if(len<(ssize_t)data_size) {
		status = EFI_BUFFER_TOO_SMALL;
		goto out;
	}

	// OK
	else {
		if(attributes)
			*attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS;

		*data_size = len;
	}
	
out:
	free(name8);
	return status;
}

efi_status_t efi_get_next_variable_name(efi_uintn_t *variable_name_size,
				 efi_char16_t *variable_name,
				 efi_guid_t *vendor_guid)
{
	DEBUG_ASSERT(0);
	return EFI_SUCCESS;
}

efi_status_t efi_set_variable(efi_char16_t *variable_name,
				const efi_guid_t *vendor_guid,
				efi_uint32_t attributes,
				efi_uintn_t data_size,
				void *data)
{
  	DEBUG_ASSERT(0);
	return EFI_SUCCESS;
}

efi_status_t efi_get_next_high_monotonic_count(efi_uint32_t *high_count)
{
 	DEBUG_ASSERT(0);
	return EFI_SUCCESS;
}

/* To implement it with APM we need to go to real mode. It's too much hassle
   Besides EFI specification says that this function shouldn't be used
   on systems supporting ACPI
 */
void efi_reset_system(efi_reset_type_t reset_type,
				   efi_status_t reset_status,
				   efi_uintn_t data_size,
				   efi_char16_t *reset_data)
{
	platform_halt_action action = HALT_ACTION_REBOOT;
	if(reset_type==EFI_RESET_SHUTDOWN)
		action = HALT_ACTION_SHUTDOWN;

	platform_halt(action, HALT_REASON_SW_RESET);
	ASSERT(0);
}

struct efi_runtime_services efi_runtime_services =
{
	.hdr = {
		.signature = EFI_RUNTIME_SERVICES_SIGNATURE,
		.revision = 0x0001000a,
		.header_size = sizeof (struct efi_runtime_services),
		.crc32 = 0, /* filled later*/
		.reserved = 0
	},
	.get_time = efi_get_time,
	.set_time = efi_set_time,
	.get_wakeup_time = efi_get_wakeup_time,
	.set_wakeup_time = efi_set_wakeup_time,

	.set_virtual_address_map = efi_set_virtual_address_map,
	.convert_pointer = efi_convert_pointer,

	.get_variable = efi_get_variable,
	.get_next_variable_name = efi_get_next_variable_name,
	.set_variable = efi_set_variable,
	.get_next_high_monotonic_count = efi_get_next_high_monotonic_count,

	.reset_system = efi_reset_system
};
