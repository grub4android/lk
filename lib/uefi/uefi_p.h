#ifndef LIB_UEFI_P_H
#define LIB_UEFI_P_H

#include <list.h>
#include <kernel/timer.h>
#include <rand.h>

// GUID's
#define EFI_EDID_ACTIVE_GUID \
	{ 0xbd8c1056, 0x9f36, 0x44ec, { 0x92, 0xa8, 0xa6, 0x33, 0x7f, 0x81, 0x79, 0x86 }}
#define EFI_EDID_DISCOVERED_GUID \
	{ 0x1c0c34f6, 0xd380, 0x41fa, { 0xa0, 0x49, 0x8a, 0xd0, 0x6c, 0x1a, 0x66, 0xaa }}


// common
#define EFI_PAGE_SIZE 4096
#define EFI_MEMORY_DESCRIPTOR_VERSION 1

// signatures
#define EFI_SIGNATURE_16(A, B)        ((A) | (B << 8))
#define EFI_SIGNATURE_32(A, B, C, D)  (EFI_SIGNATURE_16 (A, B) | (EFI_SIGNATURE_16 (C, D) << 16))

// handles
#define EFI_HANDLE_SIGNATURE EFI_SIGNATURE_32('h','n','d','l')
struct efi_ihandle {
	efi_uintn_t signature;
	struct list_node protocols;
};
typedef struct efi_ihandle efi_ihandle_t;

// protocols
#define EFI_PROTOCOL_INTERFACE_SIGNATURE EFI_SIGNATURE_32('p','i','f','c')
struct efi_protocol_interface {
	struct list_node node;
	efi_uintn_t signature;
	efi_guid_t guid;
	void *interface;
};
typedef struct efi_protocol_interface protocol_interface_t;

struct efi_global_protocol_entry {
	struct list_node node;
	protocol_interface_t* protocol_interface;
};
typedef struct efi_global_protocol_entry efi_global_protocol_entry_t;

struct efi_handle_entry {
	struct list_node node;
	efi_ihandle_t* handle;
};
typedef struct efi_handle_entry efi_handle_entry_t;

#define GUID_MATCHES(x, y) (!memcmp(&x, &y, sizeof(efi_guid_t)))
#define GUID_ARGS(x) (x).data1, (x).data2, (x).data3, (x).data4[0], (x).data4[1], (x).data4[2], (x).data4[3], (x).data4[4], (x).data4[5], (x).data4[6], (x).data4[7]
#define GUID_FORMAT "%x-%x-%x-%x%x%x%x%x%x%x%x"

static inline void random_guid(efi_packed_guid_t* guid) {
	guid->data1 = rand();
	guid->data2 = rand();
	guid->data3 = rand();

	uint32_t i;
	for(i=0; i<8; i++)
		guid->data4[i] = rand();
}

#define HANDLE_IS_VALID(x) ((x) && ((efi_ihandle_t*)(x))->signature==EFI_HANDLE_SIGNATURE)
#define PROTOCOL_IS_VALID(x) ((x) && (x)->signature==EFI_PROTOCOL_INTERFACE_SIGNATURE)

static inline protocol_interface_t* get_prototcol_interface_from_guid(efi_ihandle_t* handle, efi_guid_t* guid) {
	protocol_interface_t *prot_if;
	list_for_every_entry(&handle->protocols, prot_if, protocol_interface_t, node) {
		ASSERT(PROTOCOL_IS_VALID(prot_if));
		if(GUID_MATCHES(prot_if->guid, *guid))
			return prot_if;
	}

	return NULL;
}

#define EFI_EVENT_SIGNATURE EFI_SIGNATURE_32('e','v','n','t')
#define EVENT_IS_VALID(x) ((x) && ((efi_ievent_t*)(x))->signature==EFI_EVENT_SIGNATURE)
struct efi_ievent {
	efi_uintn_t signature;
	efi_uint32_t type;
	efi_tpl_t notify_tpl;
	void (*notify_function)(efi_event_t event, void* context);
	void* notify_context;

	timer_t timer;
	bool timer_running;
};
typedef struct efi_ievent efi_ievent_t;

#endif
