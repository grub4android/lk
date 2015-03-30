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

#undef LK_DEBUGLEVEL
#define LK_DEBUGLEVEL 2

#include <debug.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <platform.h>
#include <kernel/vm.h>
#include <kernel/thread.h>
#include <lk/init.h>
#include <pow2.h>
#include <uefi/pe32.h>
#include <uefi/api.h>
#include <uefi/api_simpletext.h>
#include <uefi/api_blockio.h>
#include <uefi/api_gop.h>
#include <uefi/console_control.h>

#include "uefi_p.h"

static efi_guid_t ignored_protocols[] = {
	EFI_EDID_ACTIVE_GUID,
	EFI_EDID_DISCOVERED_GUID,
	EFI_SIMPLE_NETWORK_GUID,
	EFI_SERIAL_IO_GUID,
	EFI_DEVICE_PATH_GUID,
};

static struct list_node uefi_global_handle_list;

static efi_tpl_t efi_raise_tpl(efi_tpl_t new_tpl)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static void efi_restore_tpl(efi_tpl_t old_tpl)
{
	DEBUG_ASSERT(0);
}

static efi_status_t efi_allocate_pages(efi_allocate_type_t type,
				       efi_memory_type_t memory_type,
				       efi_uintn_t pages,
				       efi_physical_address_t * memory)
{
	// check type
	if((efi_uint32_t)type>=EFI_MAX_ALLOCATION_TYPE)
		return EFI_INVALID_PARAMETER;

	// check memory type
	if(memory_type >= EFI_MAX_MEMORY_TYPE || memory_type == EFI_CONVENTIONAL_MEMORY)
		return EFI_INVALID_PARAMETER;

	// check memory pointer
	if(!memory)
		return EFI_INVALID_PARAMETER;

	// other types currently are unsupported
	if(memory_type!=EFI_LOADER_DATA) {
		DEBUG_ASSERT(0);
		return EFI_UNSUPPORTED;
	}

	// allocate memory
	void* buf;
	size_t len = pages*EFI_PAGE_SIZE;
	if (vmm_alloc_contiguous(vmm_get_kernel_aspace(), __func__, len, &buf, log2_uint(len), 0, ARCH_MMU_FLAG_CACHED) < 0) {
		return EFI_OUT_OF_RESOURCES;
	}

	*memory = (efi_physical_address_t)(uint32_t)buf;
	return EFI_SUCCESS;
}

static efi_status_t efi_free_pages(efi_physical_address_t memory,
				   efi_uintn_t pages)
{
	vmm_free_region(vmm_get_kernel_aspace(), memory);
	return EFI_SUCCESS;
}

struct efi_map_pdata {
	efi_memory_descriptor_t* map;
	size_t count;
};

static int efi_pmm_range_cb(void* _pdata, paddr_t addr, size_t size) {
	struct efi_map_pdata* pdata = _pdata;

	// allocate memory for new map entry
	pdata->map = realloc(pdata->map, sizeof(efi_memory_descriptor_t)*(++pdata->count));
	ASSERT(pdata->map);
	efi_memory_descriptor_t* desc = &pdata->map[pdata->count-1];

	// write new map entry
	desc->type = EFI_CONVENTIONAL_MEMORY;
	desc->padding = 0;
	desc->physical_start = addr;
	desc->virtual_start = addr;
	desc->num_pages = size/EFI_PAGE_SIZE;
	desc->attribute = 0;

	return 0;
}

static efi_uintn_t efi_create_memory_map(efi_memory_descriptor_t ** efi_map) {
#if 0
	// create map
	struct efi_map_pdata pdata = {NULL, 0};
	pmm_get_free_ranges(&pdata, efi_pmm_range_cb);
	*efi_map = pdata.map;
	return pdata.count;
#else
	// report a fake map starting at 0x0 because it seems like GRUB uses the first entry only
	*efi_map = malloc(sizeof(efi_memory_descriptor_t));
	efi_memory_descriptor_t* desc = &(*efi_map)[0];
	desc->type = EFI_CONVENTIONAL_MEMORY;
	desc->padding = 0;
	desc->physical_start = 0x0;
	desc->virtual_start = 0x0;
	desc->num_pages = pmm_get_free_space()/EFI_PAGE_SIZE;
	desc->attribute = 0;
	return 1;
#endif
}

static efi_status_t efi_get_memory_map(efi_uintn_t * memory_map_size,
				       efi_memory_descriptor_t * memory_map,
				       efi_uintn_t * map_key,
				       efi_uintn_t * descriptor_size,
				       efi_uint32_t * descriptor_version)
{
	static efi_uintn_t global_map_key = 0;
	efi_status_t rc = EFI_SUCCESS;
	efi_uintn_t size;
	efi_uintn_t num_entries;
	efi_uintn_t buffer_size;
	efi_memory_descriptor_t* generated_map = NULL;

	// check map size pointer
	if(!memory_map_size)
		return EFI_INVALID_PARAMETER;

	// count map entries
	num_entries = efi_create_memory_map(&generated_map);
	

	// get descriptor size
	size = sizeof(*memory_map);

	//
	// Make sure Size != sizeof(EFI_MEMORY_DESCRIPTOR). This will
	// prevent people from having pointer math bugs in their code.
	// now you have to use *DescriptorSize to make things work.
	//
	size += sizeof(efi_uint64_t) - (size % sizeof (efi_uint64_t));

	// set descriptor size
	if (descriptor_size)
		*descriptor_size = size;

	// set descriptor version
	if (descriptor_version)
		*descriptor_version = EFI_MEMORY_DESCRIPTOR_VERSION;

	//
	// Compute the buffer size needed to fit the entire map
	//
	buffer_size = size * num_entries;

	// check buffer size
	if(*memory_map_size < buffer_size) {
		rc = EFI_BUFFER_TOO_SMALL;
		goto out;
	}

	// check memory map pointer
	if(!memory_map) {
		rc = EFI_INVALID_PARAMETER;
		goto out;
	}

	// build map
	memcpy(memory_map, generated_map, buffer_size);

out:
	if(generated_map)
		free(generated_map);

	//
	// Update the map key finally
	//
	if(map_key)
		*map_key = global_map_key++;


	// set map size
	*memory_map_size = buffer_size;

	return rc;
}

static efi_status_t efi_allocate_pool(efi_memory_type_t pool_type,
				      efi_uintn_t size, void **buffer)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_free_pool(void *buffer)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_create_event(efi_uint32_t type,
				     efi_tpl_t notify_tpl,
				     void (*notify_function) (efi_event_t event,
							      void *context),
				     void *notify_context, efi_event_t * event)
{
	if(type==(EFI_EVT_TIMER | EFI_EVT_NOTIFY_SIGNAL) && notify_tpl==EFI_TPL_CALLBACK) {
		// allocate event
		efi_ievent_t* ievent = malloc(sizeof(efi_ievent_t));
		ievent->signature = EFI_EVENT_SIGNATURE;
		ievent->type = type;
		ievent->notify_tpl = notify_tpl;
		ievent->notify_function = notify_function;
		ievent->notify_context = notify_context;
		timer_initialize(&ievent->timer);
		ievent->timer_running = false;

		// return event
		*event = ievent;

		return EFI_SUCCESS;
	}

	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static enum handler_return efi_timer_callback(struct timer* timer, lk_time_t now, void *arg) {
	efi_ievent_t* ievent = (efi_ievent_t*)arg;
	ievent->notify_function(ievent, ievent->notify_context);
	return INT_NO_RESCHEDULE;
}

static efi_status_t efi_set_timer(efi_event_t event,
				  efi_timer_delay_t type,
				  efi_uint64_t trigger_time)
{
	// check event
	if (!EVENT_IS_VALID(event)) {
		DEBUG_ASSERT(0);
		return EFI_INVALID_PARAMETER;
	}

	// get internal event
	efi_ievent_t* ievent = (efi_ievent_t*)event;

	if(type==EFI_TIMER_PERIODIC) {
		// cancel running timer
		if(ievent->timer_running)
			timer_cancel(&ievent->timer);

		if(trigger_time>0) {
			timer_set_periodic(&ievent->timer, trigger_time/10000, efi_timer_callback, ievent);
			ievent->timer_running = true;
		}
		else ievent->timer_running = false;

		return EFI_SUCCESS;
	}

	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_wait_for_event(efi_uintn_t num_events,
				       efi_event_t * event, efi_uintn_t * index)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_signal_event(efi_event_t event)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_close_event(efi_event_t event)
{
	// check event
	if (!EVENT_IS_VALID(event)) {
		DEBUG_ASSERT(0);
		return EFI_INVALID_PARAMETER;
	}

	// get internal event
	efi_ievent_t* ievent = (efi_ievent_t*)event;

	// cancel running timer
	if(ievent->timer_running)
		timer_cancel(&ievent->timer);

	// free memory
	free(ievent);
	
	return EFI_SUCCESS;
}

static efi_status_t efi_check_event(efi_event_t event)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_install_protocol_interface(efi_handle_t * handle,
						   efi_guid_t * protocol,
						   efi_interface_type_t
						   protocol_interface_type,
						   void *protocol_interface)
{
	dprintf(CRITICAL, "%s: unsupported GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(*protocol));
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_reinstall_protocol_interface(efi_handle_t handle,
						     efi_guid_t * protocol,
						     void *old_interface,
						     void *new_interface)
{
	dprintf(CRITICAL, "%s: unsupported GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(*protocol));
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_uninstall_protocol_interface(efi_handle_t handle,
						     efi_guid_t * protocol,
						     void *protocol_interface)
{
	dprintf(CRITICAL, "%s: unsupported GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(*protocol));
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_handle_protocol(efi_handle_t handle,
					efi_guid_t * protocol,
					void **protocol_interface)
{
	dprintf(CRITICAL, "%s: unsupported GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(*protocol));
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_register_protocol_notify(efi_guid_t * protocol,
						 efi_event_t event,
						 void **registration)
{
	dprintf(CRITICAL, "%s: unsupported GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(*protocol));
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_locate_handle(efi_locate_search_type_t search_type,
				      efi_guid_t * protocol,
				      void *search_key,
				      efi_uintn_t * buffer_size,
				      efi_handle_t * buffer)
{
	// check buffer size
	if(!buffer_size)
		return EFI_INVALID_PARAMETER;

	if((*buffer_size > 0) && !buffer)
		return EFI_INVALID_PARAMETER;

	int32_t num_results = 0;
	int32_t max_results = (*buffer_size)/sizeof(*buffer);

	switch(search_type) {
		case EFI_ALL_HANDLES:
			break;

		case EFI_BY_REGISTER_NOTIFY:
			DEBUG_ASSERT(0);
			break;

		case EFI_BY_PROTOCOL:
			// check protocol
			if(!protocol)
				return EFI_INVALID_PARAMETER;
			break;

		default:
			return EFI_INVALID_PARAMETER;
	}

	// iterate over all known handles
	efi_handle_entry_t *handle_entry;
	list_for_every_entry(&uefi_global_handle_list, handle_entry, efi_handle_entry_t, node) {
		// check if this handle supports the requested protocol
		bool protocol_matches = false;
		if(search_type==EFI_BY_PROTOCOL) {
			protocol_interface_t* prot_if = get_prototcol_interface_from_guid(handle_entry->handle, protocol);
			if(prot_if) {
				protocol_matches = true;
			}
		}

		// write handle to result buffer
		if(search_type==EFI_ALL_HANDLES || (search_type==EFI_BY_PROTOCOL && protocol_matches)) {
			if(num_results<max_results)
				buffer[num_results] = handle_entry->handle;

			// continue counting results, we need to return the number
			num_results++;
		}
	}

	// no results
	if(num_results<=0) {
		// report unsupported GUID
		if(search_type==EFI_BY_PROTOCOL) {
			uint32_t i;
			for(i=0; i<ARRAY_SIZE(ignored_protocols); i++) {
				if(GUID_MATCHES(*protocol, ignored_protocols[i])) {
					dprintf(SPEW, "%s: ignored GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(ignored_protocols[i]));
					return EFI_NOT_FOUND;
				}
			}

			dprintf(CRITICAL, "%s: unsupported GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(*protocol));
			DEBUG_ASSERT(0);
		}

		return EFI_NOT_FOUND;
	}
	else {
		// set result size
		*buffer_size = num_results*sizeof(*buffer);

		// buffer was too small
		if (num_results>max_results) {
			return EFI_BUFFER_TOO_SMALL;
		}
	}

	return EFI_SUCCESS;
}

static efi_status_t efi_locate_device_path(efi_guid_t * protocol,
					   efi_device_path_t ** device_path,
					   efi_handle_t * device)
{
	dprintf(CRITICAL, "%s: unsupported GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(*protocol));
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_install_configuration_table(efi_guid_t * guid,
						    void *table)
{
	dprintf(CRITICAL, "%s: unsupported GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(*guid));
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_load_image(efi_boolean_t boot_policy,
				   efi_handle_t parent_image_handle,
				   efi_device_path_t * file_path,
				   void *source_buffer,
				   efi_uintn_t source_size,
				   efi_handle_t * image_handle)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_start_image(efi_handle_t image_handle,
				    efi_uintn_t * exit_data_size,
				    efi_char16_t ** exit_data)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}


static efi_status_t efi_exit(efi_handle_t image_handle,
			     efi_status_t exit_status,
			     efi_uintn_t exit_data_size,
			     efi_char16_t * exit_data) __attribute__((noreturn));
static efi_status_t efi_exit(efi_handle_t image_handle,
			     efi_status_t exit_status,
			     efi_uintn_t exit_data_size,
			     efi_char16_t * exit_data)
{
	// TODO correctly cleanup image
	DEBUG_ASSERT(0);

	// exit
	thread_exit(0);
}

static efi_status_t efi_unload_image(efi_handle_t image_handle)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_exit_boot_services(efi_handle_t image_handle,
					   efi_uintn_t map_key)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_get_next_monotonic_count(efi_uint64_t * count)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_stall(efi_uintn_t microseconds)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_set_watchdog_timer(efi_uintn_t timeout,
					   efi_uint64_t watchdog_code,
					   efi_uintn_t data_size,
					   efi_char16_t * watchdog_data)
{
	dprintf(CRITICAL, "%s()\n", __func__);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_connect_controller(efi_handle_t controller_handle,
					   efi_handle_t * driver_image_handle,
					   efi_device_path_protocol_t *
					   remaining_device_path,
					   efi_boolean_t recursive)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_disconnect_controller(efi_handle_t controller_handle,
					      efi_handle_t driver_image_handle,
					      efi_handle_t child_handle)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_open_protocol(efi_handle_t handle,
				      efi_guid_t * protocol,
				      void **protocol_interface,
				      efi_handle_t agent_handle,
				      efi_handle_t controller_handle,
				      efi_uint32_t attributes)
{
	// check protocol
	if(!protocol) return EFI_INVALID_PARAMETER;

	// check interface
	if (attributes!=EFI_OPEN_PROTOCOL_TEST_PROTOCOL) {
		if (!protocol_interface)
			return EFI_INVALID_PARAMETER;
		else *protocol_interface = NULL;
	}

	// check handle
	if (!HANDLE_IS_VALID(handle)) {
		DEBUG_ASSERT(0);
		return EFI_INVALID_PARAMETER;
	}

	// check attributes
	switch (attributes) {
		case EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER:
			if(!HANDLE_IS_VALID(agent_handle))
				return EFI_INVALID_PARAMETER;
			if(!HANDLE_IS_VALID(controller_handle))
				return EFI_INVALID_PARAMETER;
			if(handle == controller_handle)
				return EFI_INVALID_PARAMETER;
			break;
		case EFI_OPEN_PROTOCOL_BY_DRIVER:
		case EFI_OPEN_PROTOCOL_BY_DRIVER | EFI_OPEN_PROTOCOL_BY_EXCLUSIVE:
			if(!HANDLE_IS_VALID(agent_handle))
				return EFI_INVALID_PARAMETER;
			if(!HANDLE_IS_VALID(controller_handle))
				return EFI_INVALID_PARAMETER;
			break;
		case EFI_OPEN_PROTOCOL_BY_EXCLUSIVE:
			if(!HANDLE_IS_VALID(agent_handle))
				return EFI_INVALID_PARAMETER;
			break;
		case EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL:
		case EFI_OPEN_PROTOCOL_GET_PROTOCOL:
		case EFI_OPEN_PROTOCOL_TEST_PROTOCOL:
			break;
		default:
		return EFI_INVALID_PARAMETER;
	}

	// XXX: exlusive locks are not supported yet
	if(attributes & EFI_OPEN_PROTOCOL_BY_EXCLUSIVE) {
		dprintf(CRITICAL, "%s: exclusive locks are not supported\n", __func__);
		DEBUG_ASSERT(0);
		return EFI_UNSUPPORTED;
	}
	// XXX: driver protocols are not supported yet
	if(controller_handle) {
		dprintf(CRITICAL, "%s: driver protocols are not supported\n", __func__);
		DEBUG_ASSERT(0);
		return EFI_UNSUPPORTED;
	}

	// get protocol interface by handle+guid
	protocol_interface_t* prot_if = get_prototcol_interface_from_guid(handle, protocol);
	if(!prot_if) {
		uint32_t i;
		for(i=0; i<ARRAY_SIZE(ignored_protocols); i++) {
			if(GUID_MATCHES(*protocol, ignored_protocols[i])) {
				dprintf(SPEW, "%s: ignored GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(ignored_protocols[i]));
				return EFI_NOT_FOUND;
			}
		}

		dprintf(CRITICAL, "%s: unsupported GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(*protocol));
		DEBUG_ASSERT(0);
		return EFI_UNSUPPORTED;
	}

	// return interface
	if (attributes!=EFI_OPEN_PROTOCOL_TEST_PROTOCOL) {
		*protocol_interface = prot_if->interface;
	}

	return EFI_SUCCESS;
}

static efi_status_t efi_close_protocol(efi_handle_t handle,
				       efi_guid_t * protocol,
				       efi_handle_t agent_handle,
				       efi_handle_t controller_handle)
{
	dprintf(CRITICAL, "%s: unsupported GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(*protocol));
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_open_protocol_information(efi_handle_t handle,
						  efi_guid_t * protocol,
						  efi_open_protocol_information_entry_t
						  ** entry_buffer,
						  efi_uintn_t * entry_count)
{
	dprintf(CRITICAL, "%s: unsupported GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(*protocol));
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_protocols_per_handle(efi_handle_t user_handle,
					     efi_packed_guid_t*** protocol_buffer,
					     efi_uintn_t* protocol_buffer_count)
{
	// check handle
	if (!HANDLE_IS_VALID(user_handle)) {
		DEBUG_ASSERT(0);
		return EFI_INVALID_PARAMETER;
	}

	efi_ihandle_t* handle = user_handle;

	// check buffer
	if(!protocol_buffer)
		return EFI_INVALID_PARAMETER;

	// check count
	if(!protocol_buffer_count)
		return EFI_INVALID_PARAMETER;

	// set count to 0
	*protocol_buffer_count = 0;

	// count protocols for this handle
	uint32_t protocol_count = 0;
	protocol_interface_t *prot_if;
	list_for_every_entry(&handle->protocols, prot_if, protocol_interface_t, node) {
		ASSERT(PROTOCOL_IS_VALID(prot_if));
		protocol_count++;
	}

	//
	// If there are no protocol interfaces installed on Handle, then Handle is not a valid EFI_HANDLE
	//
	if(!protocol_count)
		return EFI_INVALID_PARAMETER;

	// allocate result buffer
	efi_packed_guid_t** buffer = malloc(sizeof(efi_packed_guid_t*) * protocol_count);
	if(!buffer)
		return EFI_OUT_OF_RESOURCES;

	// set info
	*protocol_buffer = buffer;
	*protocol_buffer_count = protocol_count;

	// add protocols to buffer
	protocol_count = 0;
	list_for_every_entry(&handle->protocols, prot_if, protocol_interface_t, node) {
		ASSERT(PROTOCOL_IS_VALID(prot_if));
		buffer[protocol_count++] = (efi_packed_guid_t*)&prot_if->guid;
	}

	return EFI_SUCCESS;
}

static efi_status_t efi_locate_handle_buffer(efi_locate_search_type_t
					     search_type, efi_guid_t * protocol,
					     void *search_key,
					     efi_uintn_t * no_handles,
					     efi_handle_t ** buffer)
{
	dprintf(CRITICAL, "%s: unsupported GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(*protocol));
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_locate_protocol(efi_guid_t * protocol,
					void *registration,
					void **protocol_interface)
{
	// check interface
	if(!protocol_interface)
		return EFI_INVALID_PARAMETER;

	// check protocol guid
	if (!protocol)
		return EFI_NOT_FOUND;

	*protocol_interface = NULL;

	// iterate over all known handles
	efi_handle_entry_t *handle_entry;
	list_for_every_entry(&uefi_global_handle_list, handle_entry, efi_handle_entry_t, node) {
		// get protocol interface by handle+guid
		protocol_interface_t* prot_if = get_prototcol_interface_from_guid(handle_entry->handle, protocol);
		if(prot_if) {
			*protocol_interface = prot_if->interface;
			return EFI_SUCCESS;
		}
	}

	uint32_t i;
	for(i=0; i<ARRAY_SIZE(ignored_protocols); i++) {
		if(GUID_MATCHES(*protocol, ignored_protocols[i])) {
			dprintf(SPEW, "%s: ignored GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(ignored_protocols[i]));
			return EFI_NOT_FOUND;
		}
	}

	// not found
	dprintf(CRITICAL, "%s: unsupported GUID: "GUID_FORMAT"\n", __func__, GUID_ARGS(*protocol));
	DEBUG_ASSERT(0);
	return EFI_NOT_FOUND;
}

static efi_status_t efi_install_multiple_protocol_interfaces(efi_handle_t *
							     handle, ...)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_uninstall_multiple_protocol_interfaces(efi_handle_t
							       handle, ...)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static efi_status_t efi_calculate_crc32(void *data,
					efi_uintn_t data_size,
					efi_uint32_t * crc32)
{
	DEBUG_ASSERT(0);
	return EFI_UNSUPPORTED;
}

static void efi_copy_mem(void *destination, void *source, efi_uintn_t length)
{
	DEBUG_ASSERT(0);
}

static void efi_set_mem(void *buffer, efi_uintn_t size, efi_uint8_t value)
{
	DEBUG_ASSERT(0);
}

static struct efi_boot_services efi_boot_services = {
	.hdr = {
		.signature = EFI_BOOT_SERVICES_SIGNATURE,
		.revision = 0x0001000a,
		.header_size = sizeof(struct efi_boot_services),
		.crc32 = 0,	/* filled later */
		.reserved = 0
	},
	.raise_tpl = efi_raise_tpl,
	.restore_tpl = efi_restore_tpl,
	.allocate_pages = efi_allocate_pages,
	.free_pages = efi_free_pages,
	.get_memory_map = efi_get_memory_map,
	.allocate_pool = efi_allocate_pool,
	.free_pool = efi_free_pool,
	.create_event = efi_create_event,
	.set_timer = efi_set_timer,
	.wait_for_event = efi_wait_for_event,
	.signal_event = efi_signal_event,
	.close_event = efi_close_event,
	.check_event = efi_check_event,
	.install_protocol_interface = efi_install_protocol_interface,
	.reinstall_protocol_interface = efi_reinstall_protocol_interface,
	.uninstall_protocol_interface = efi_uninstall_protocol_interface,
	.handle_protocol = efi_handle_protocol,
	.register_protocol_notify = efi_register_protocol_notify,
	.locate_handle = efi_locate_handle,
	.locate_device_path = efi_locate_device_path,
	.install_configuration_table = efi_install_configuration_table,
	.load_image = efi_load_image,
	.start_image = efi_start_image,
	.exit = efi_exit,
	.unload_image = efi_unload_image,
	.exit_boot_services = efi_exit_boot_services,
	.get_next_monotonic_count = efi_get_next_monotonic_count,
	.stall = efi_stall,
	.set_watchdog_timer = efi_set_watchdog_timer,
	.connect_controller = efi_connect_controller,
	.disconnect_controller = efi_disconnect_controller,
	.open_protocol = efi_open_protocol,
	.close_protocol = efi_close_protocol,
	.open_protocol_information = efi_open_protocol_information,
	.protocols_per_handle = efi_protocols_per_handle,
	.locate_handle_buffer = efi_locate_handle_buffer,
	.locate_protocol = efi_locate_protocol,
	.install_multiple_protocol_interfaces = efi_install_multiple_protocol_interfaces,
	.uninstall_multiple_protocol_interfaces = efi_uninstall_multiple_protocol_interfaces,
	.calculate_crc32 = efi_calculate_crc32,
	.copy_mem = efi_copy_mem,
	.set_mem = efi_set_mem,
};

static uint16_t efi_vendor[] = {
	'L', 'K', ' ', 'E', 'F', 'I', ' ',
	'R', 'U', 'N', 'T', 'I', 'M', 'E', 0
};

extern struct efi_runtime_services efi_runtime_services;
struct efi_system_table efi_system_table = {
	.hdr = {
		.signature = EFI_SYSTEM_TABLE_SIGNATURE,
		.revision = 0x0001000a,
		.header_size = sizeof(struct efi_system_table),
		.crc32 = 0,	/* filled later */
		.reserved = 0
	},
	.firmware_vendor = efi_vendor,
	.firmware_revision = 0x0001000a,
	.console_in_handler = 0,
	.con_in = &efi_simpletext_con_in,
	.console_out_handler = 0,
	.con_out = &efi_simpletext_con_out,
	.standard_error_handle = 0,
	.std_err = 0,
	.runtime_services = &efi_runtime_services,
	.boot_services = &efi_boot_services,
	.num_table_entries = 0,
	.configuration_table = 0
};

static void uefi_init(uint level) {
	// initialize global handle list
	list_initialize(&uefi_global_handle_list);

	uefi_api_console_init();
	uefi_api_gop_init();

	efi_memory_descriptor_t * map = NULL;
	efi_create_memory_map(&map);
}

efi_handle_t uefi_create_handle(void) {
	// allocate handle
	efi_ihandle_t* handle = malloc(sizeof(efi_ihandle_t));
	if(!handle) return NULL;

	// initialize handle
	handle->signature = EFI_HANDLE_SIGNATURE;
	list_initialize(&handle->protocols);

	// add handle to global ist
	efi_handle_entry_t* entry = malloc(sizeof(efi_handle_entry_t));
	entry->handle = handle;
	list_add_head(&uefi_global_handle_list, &entry->node);

	return handle;
}

efi_status_t uefi_add_protocol_interface(efi_handle_t handle, efi_guid_t guid, void* interface) {
	ASSERT(HANDLE_IS_VALID(handle));

	// allocate protocol interface entry
	protocol_interface_t* pif = malloc(sizeof(protocol_interface_t));
	pif->signature = EFI_PROTOCOL_INTERFACE_SIGNATURE;
	pif->guid = guid;
	pif->interface = interface;

	// add protocol to the handle's list
	list_add_head(&((efi_ihandle_t*)handle)->protocols, &pif->node);

	return EFI_SUCCESS;
}

void* uefi_create_device_path(size_t size) {
	void* dp = calloc(size + sizeof(efi_device_path_t), 1);
	efi_device_path_t* dp_end = dp + size;
	dp_end->type = EFI_END_DEVICE_PATH_TYPE;
	dp_end->subtype = EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
	dp_end->length = sizeof(*dp_end);

	return dp;
}

LK_INIT_HOOK(uefi, &uefi_init, LK_INIT_LEVEL_TARGET);
