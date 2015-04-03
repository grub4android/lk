/* efi.h - declare EFI types and functions */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008,2009  Free Software Foundation, Inc.
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

#ifndef EFI_API_HEADER
#define EFI_API_HEADER	1

#include <stdint.h>
#include <string.h>
#include <uefi/pe32.h>

/* For consistency and safety, we name the EFI-defined types differently.
   All names are transformed into lower case, _t appended, and
   efi_ prepended.  */

/* Constants.  */
#define EFI_EVT_TIMER				0x80000000
#define EFI_EVT_RUNTIME				0x40000000
#define EFI_EVT_RUNTIME_CONTEXT			0x20000000
#define EFI_EVT_NOTIFY_WAIT			0x00000100
#define EFI_EVT_NOTIFY_SIGNAL			0x00000200
#define EFI_EVT_SIGNAL_EXIT_BOOT_SERVICES		0x00000201
#define EFI_EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE	0x60000202

#define EFI_TPL_APPLICATION	4
#define EFI_TPL_CALLBACK		8
#define EFI_TPL_NOTIFY		16
#define EFI_TPL_HIGH_LEVEL		31

#define EFI_MEMORY_UC	0x0000000000000001LL
#define EFI_MEMORY_WC	0x0000000000000002LL
#define EFI_MEMORY_WT	0x0000000000000004LL
#define EFI_MEMORY_WB	0x0000000000000008LL
#define EFI_MEMORY_UCE	0x0000000000000010LL
#define EFI_MEMORY_WP	0x0000000000001000LL
#define EFI_MEMORY_RP	0x0000000000002000LL
#define EFI_MEMORY_XP	0x0000000000004000LL
#define EFI_MEMORY_RUNTIME	0x8000000000000000LL

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL	0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL		0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL		0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER	0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER		0x00000010
#define EFI_OPEN_PROTOCOL_BY_EXCLUSIVE		0x00000020

#define EFI_OS_INDICATIONS_BOOT_TO_FW_UI	0x0000000000000001ULL

#define EFI_VARIABLE_NON_VOLATILE		0x0000000000000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS	0x0000000000000002
#define EFI_VARIABLE_RUNTIME_ACCESS	0x0000000000000004

#define EFI_TIME_ADJUST_DAYLIGHT	0x01
#define EFI_TIME_IN_DAYLIGHT	0x02

#define EFI_UNSPECIFIED_TIMEZONE	0x07FF

#define EFI_OPTIONAL_PTR	0x00000001

#define EFI_LOADED_IMAGE_GUID	\
  { 0x5b1b31a1, 0x9562, 0x11d2, \
    { 0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define EFI_DISK_IO_GUID	\
  { 0xce345171, 0xba0b, 0x11d2, \
    { 0x8e, 0x4f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define EFI_BLOCK_IO_GUID	\
  { 0x964e5b21, 0x6459, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define EFI_SERIAL_IO_GUID \
  { 0xbb25cf6f, 0xf1d4, 0x11d2, \
    { 0x9a, 0x0c, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0xfd } \
  }

#define EFI_SIMPLE_NETWORK_GUID	\
  { 0xa19832b9, 0xac25, 0x11d3, \
    { 0x9a, 0x2d, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define EFI_PXE_GUID	\
  { 0x03c4e603, 0xac28, 0x11d3, \
    { 0x9a, 0x2d, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define EFI_DEVICE_PATH_GUID	\
  { 0x09576e91, 0x6d3f, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID \
  { 0x387477c1, 0x69c7, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID \
  { 0xdd9e7534, 0x7762, 0x4698, \
    { 0x8c, 0x14, 0xf5, 0x85, 0x17, 0xa6, 0x25, 0xaa } \
  }

#define EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID \
  { 0x387477c2, 0x69c7, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define EFI_SIMPLE_POINTER_PROTOCOL_GUID \
  { 0x31878c87, 0xb75, 0x11d5, \
    { 0x9a, 0x4f, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define EFI_ABSOLUTE_POINTER_PROTOCOL_GUID \
  { 0x8D59D32B, 0xC655, 0x4AE9, \
    { 0x9B, 0x15, 0xF2, 0x59, 0x04, 0x99, 0x2A, 0x43 } \
  }

#define EFI_DRIVER_BINDING_PROTOCOL_GUID \
  { 0x18A031AB, 0xB443, 0x4D1A, \
    { 0xA5, 0xC0, 0x0C, 0x09, 0x26, 0x1E, 0x9F, 0x71 } \
  }

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
  { 0x5B1B31A1, 0x9562, 0x11d2, \
    { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } \
  }

#define EFI_LOAD_FILE_PROTOCOL_GUID \
  { 0x56EC3091, 0x954C, 0x11d2, \
    { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } \
  }

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
  { 0x0964e5b22, 0x6459, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define EFI_TAPE_IO_PROTOCOL_GUID \
  { 0x1e93e633, 0xd65a, 0x459e, \
    { 0xab, 0x84, 0x93, 0xd9, 0xec, 0x26, 0x6d, 0x18 } \
  }

#define EFI_UNICODE_COLLATION_PROTOCOL_GUID \
  { 0x1d85cd7f, 0xf43d, 0x11d2, \
    { 0x9a, 0x0c, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define EFI_SCSI_IO_PROTOCOL_GUID \
  { 0x932f47e6, 0x2362, 0x4002, \
    { 0x80, 0x3e, 0x3c, 0xd5, 0x4b, 0x13, 0x8f, 0x85 } \
  }

#define EFI_USB2_HC_PROTOCOL_GUID \
  { 0x3e745226, 0x9818, 0x45b6, \
    { 0xa2, 0xac, 0xd7, 0xcd, 0x0e, 0x8b, 0xa2, 0xbc } \
  }

#define EFI_DEBUG_SUPPORT_PROTOCOL_GUID \
  { 0x2755590C, 0x6F3C, 0x42FA, \
    { 0x9E, 0xA4, 0xA3, 0xBA, 0x54, 0x3C, 0xDA, 0x25 } \
  }

#define EFI_DEBUGPORT_PROTOCOL_GUID \
  { 0xEBA4E8D2, 0x3858, 0x41EC, \
    { 0xA2, 0x81, 0x26, 0x47, 0xBA, 0x96, 0x60, 0xD0 } \
  }

#define EFI_DECOMPRESS_PROTOCOL_GUID \
  { 0xd8117cfe, 0x94a6, 0x11d4, \
    { 0x9a, 0x3a, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID \
  { 0x8b843e20, 0x8132, 0x4852, \
    { 0x90, 0xcc, 0x55, 0x1a, 0x4e, 0x4a, 0x7f, 0x1c } \
  }

#define EFI_DEVICE_PATH_UTILITIES_PROTOCOL_GUID \
  { 0x379be4e, 0xd706, 0x437d, \
    { 0xb0, 0x37, 0xed, 0xb8, 0x2f, 0xb7, 0x72, 0xa4 } \
  }

#define EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL_GUID \
  { 0x5c99a21, 0xc70f, 0x4ad2, \
    { 0x8a, 0x5f, 0x35, 0xdf, 0x33, 0x43, 0xf5, 0x1e } \
  }

#define EFI_ACPI_TABLE_PROTOCOL_GUID \
  { 0xffe06bdd, 0x6107, 0x46a6, \
    { 0x7b, 0xb2, 0x5a, 0x9c, 0x7e, 0xc5, 0x27, 0x5c} \
  }

#define EFI_HII_CONFIG_ROUTING_PROTOCOL_GUID \
  { 0x587e72d7, 0xcc50, 0x4f79, \
    { 0x82, 0x09, 0xca, 0x29, 0x1f, 0xc1, 0xa1, 0x0f } \
  }

#define EFI_HII_DATABASE_PROTOCOL_GUID \
  { 0xef9fc172, 0xa1b2, 0x4693, \
    { 0xb3, 0x27, 0x6d, 0x32, 0xfc, 0x41, 0x60, 0x42 } \
  }

#define EFI_HII_STRING_PROTOCOL_GUID \
  { 0xfd96974, 0x23aa, 0x4cdc, \
    { 0xb9, 0xcb, 0x98, 0xd1, 0x77, 0x50, 0x32, 0x2a } \
  }

#define EFI_HII_IMAGE_PROTOCOL_GUID \
  { 0x31a6406a, 0x6bdf, 0x4e46, \
    { 0xb2, 0xa2, 0xeb, 0xaa, 0x89, 0xc4, 0x9, 0x20 } \
  }

#define EFI_HII_FONT_PROTOCOL_GUID \
  { 0xe9ca4775, 0x8657, 0x47fc, \
    { 0x97, 0xe7, 0x7e, 0xd6, 0x5a, 0x8, 0x43, 0x24 } \
  }

#define EFI_HII_CONFIGURATION_ACCESS_PROTOCOL_GUID \
  { 0x330d4706, 0xf2a0, 0x4e4f, \
    { 0xa3, 0x69, 0xb6, 0x6f, 0xa8, 0xd5, 0x43, 0x85 } \
  }

#define EFI_COMPONENT_NAME2_PROTOCOL_GUID \
  { 0x6a7a5cff, 0xe8d9, 0x4f70, \
    { 0xba, 0xda, 0x75, 0xab, 0x30, 0x25, 0xce, 0x14} \
  }

#define EFI_USB_IO_PROTOCOL_GUID \
  { 0x2B2F68D6, 0x0CD2, 0x44cf, \
    { 0x8E, 0x8B, 0xBB, 0xA2, 0x0B, 0x1B, 0x5B, 0x75 } \
  }

#define EFI_MPS_TABLE_GUID	\
  { 0xeb9d2d2f, 0x2d88, 0x11d3, \
    { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define EFI_ACPI_TABLE_GUID	\
  { 0xeb9d2d30, 0x2d88, 0x11d3, \
    { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define EFI_ACPI_20_TABLE_GUID	\
  { 0x8868e871, 0xe4f1, 0x11d3, \
    { 0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

#define EFI_SMBIOS_TABLE_GUID	\
  { 0xeb9d2d31, 0x2d88, 0x11d3, \
    { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define EFI_SAL_TABLE_GUID \
  { 0xeb9d2d32, 0x2d88, 0x11d3, \
      { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define EFI_HCDP_TABLE_GUID \
  { 0xf951938d, 0x620b, 0x42ef, \
      { 0x82, 0x79, 0xa8, 0x4b, 0x79, 0x61, 0x78, 0x98 } \
  }

#define EFI_DEVICE_TREE_GUID \
  { 0xb1b621d5, 0xf19c, 0x41a5, \
      { 0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0 } \
  }

#define EFI_VENDOR_APPLE_GUID \
  { 0x2B0585EB, 0xD8B8, 0x49A9,	\
      { 0x8B, 0x8C, 0xE2, 0x1B, 0x01, 0xAE, 0xF2, 0xB7 } \
  }

struct efi_sal_system_table {
	uint32_t signature;
	uint32_t total_table_len;
	uint16_t sal_rev;
	uint16_t entry_count;
	uint8_t checksum;
	uint8_t reserved1[7];
	uint16_t sal_a_version;
	uint16_t sal_b_version;
	uint8_t oem_id[32];
	uint8_t product_id[32];
	uint8_t reserved2[8];
	uint8_t entries[0];
};

enum {
	EFI_SAL_SYSTEM_TABLE_TYPE_ENTRYPOINT_DESCRIPTOR = 0,
	EFI_SAL_SYSTEM_TABLE_TYPE_MEMORY_DESCRIPTOR = 1,
	EFI_SAL_SYSTEM_TABLE_TYPE_PLATFORM_FEATURES = 2,
	EFI_SAL_SYSTEM_TABLE_TYPE_TRANSLATION_REGISTER_DESCRIPTOR = 3,
	EFI_SAL_SYSTEM_TABLE_TYPE_PURGE_TRANSLATION_COHERENCE = 4,
	EFI_SAL_SYSTEM_TABLE_TYPE_AP_WAKEUP = 5
};

struct efi_sal_system_table_entrypoint_descriptor {
	uint8_t type;
	uint8_t pad[7];
	uint64_t pal_proc_addr;
	uint64_t sal_proc_addr;
	uint64_t global_data_ptr;
	uint64_t reserved[2];
};

struct efi_sal_system_table_memory_descriptor {
	uint8_t type;
	uint8_t sal_used;
	uint8_t attr;
	uint8_t ar;
	uint8_t attr_mask;
	uint8_t mem_type;
	uint8_t usage;
	uint8_t unknown;
	uint64_t addr;
	uint64_t len;
	uint64_t unknown2;
};

struct efi_sal_system_table_platform_features {
	uint8_t type;
	uint8_t flags;
	uint8_t reserved[14];
};

struct efi_sal_system_table_translation_register_descriptor {
	uint8_t type;
	uint8_t register_type;
	uint8_t register_number;
	uint8_t reserved[5];
	uint64_t addr;
	uint64_t page_size;
	uint64_t reserver;
};

struct efi_sal_system_table_purge_translation_coherence {
	uint8_t type;
	uint8_t reserved[3];
	uint32_t ndomains;
	uint64_t coherence;
};

struct efi_sal_system_table_ap_wakeup {
	uint8_t type;
	uint8_t mechanism;
	uint8_t reserved[6];
	uint64_t vector;
};

enum {
	EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_BUSLOCK = 1,
	EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_IRQREDIRECT = 2,
	EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_IPIREDIRECT = 4,
	EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_ITCDRIFT = 8,
};

typedef enum efi_parity_type {
	EFI_SERIAL_DEFAULT_PARITY,
	EFI_SERIAL_NO_PARITY,
	EFI_SERIAL_EVEN_PARITY,
	EFI_SERIAL_ODD_PARITY
} efi_parity_type_t;

typedef enum efi_stop_bits {
	EFI_SERIAL_DEFAULT_STOP_BITS,
	EFI_SERIAL_1_STOP_BIT,
	EFI_SERIAL_1_5_STOP_BITS,
	EFI_SERIAL_2_STOP_BITS
} efi_stop_bits_t;

/* Enumerations.  */
enum efi_timer_delay {
	EFI_TIMER_CANCEL,
	EFI_TIMER_PERIODIC,
	EFI_TIMER_RELATIVE
};
typedef enum efi_timer_delay efi_timer_delay_t;

enum efi_allocate_type {
	EFI_ALLOCATE_ANY_PAGES,
	EFI_ALLOCATE_MAX_ADDRESS,
	EFI_ALLOCATE_ADDRESS,
	EFI_MAX_ALLOCATION_TYPE
};
typedef enum efi_allocate_type efi_allocate_type_t;

enum efi_memory_type {
	EFI_RESERVED_MEMORY_TYPE,
	EFI_LOADER_CODE,
	EFI_LOADER_DATA,
	EFI_BOOT_SERVICES_CODE,
	EFI_BOOT_SERVICES_DATA,
	EFI_RUNTIME_SERVICES_CODE,
	EFI_RUNTIME_SERVICES_DATA,
	EFI_CONVENTIONAL_MEMORY,
	EFI_UNUSABLE_MEMORY,
	EFI_ACPI_RECLAIM_MEMORY,
	EFI_ACPI_MEMORY_NVS,
	EFI_MEMORY_MAPPED_IO,
	EFI_MEMORY_MAPPED_IO_PORT_SPACE,
	EFI_PAL_CODE,
	EFI_MAX_MEMORY_TYPE
};
typedef enum efi_memory_type efi_memory_type_t;

enum efi_interface_type {
	EFI_NATIVE_INTERFACE
};
typedef enum efi_interface_type efi_interface_type_t;

enum efi_locate_search_type {
	EFI_ALL_HANDLES,
	EFI_BY_REGISTER_NOTIFY,
	EFI_BY_PROTOCOL
};
typedef enum efi_locate_search_type efi_locate_search_type_t;

enum efi_reset_type {
	EFI_RESET_COLD,
	EFI_RESET_WARM,
	EFI_RESET_SHUTDOWN
};
typedef enum efi_reset_type efi_reset_type_t;

/* Types.  */
typedef char efi_boolean_t;
#if CPU_SIZEOF_VOID_P == 8
typedef int64_t efi_intn_t;
typedef uint64_t efi_uintn_t;
#else
typedef int32_t efi_intn_t;
typedef uint32_t efi_uintn_t;
#endif
typedef int8_t efi_int8_t;
typedef uint8_t efi_uint8_t;
typedef int16_t efi_int16_t;
typedef uint16_t efi_uint16_t;
typedef int32_t efi_int32_t;
typedef uint32_t efi_uint32_t;
typedef int64_t efi_int64_t;
typedef uint64_t efi_uint64_t;
typedef uint8_t efi_char8_t;
typedef uint16_t efi_char16_t;

typedef efi_intn_t efi_status_t;

#define EFI_ERROR_CODE(value)	\
  ((((efi_status_t) 1) << (sizeof (efi_status_t) * 8 - 1)) | (value))

#define EFI_WARNING_CODE(value)	(value)

#define EFI_SUCCESS		0

#define EFI_LOAD_ERROR		EFI_ERROR_CODE (1)
#define EFI_INVALID_PARAMETER	EFI_ERROR_CODE (2)
#define EFI_UNSUPPORTED		EFI_ERROR_CODE (3)
#define EFI_BAD_BUFFER_SIZE	EFI_ERROR_CODE (4)
#define EFI_BUFFER_TOO_SMALL	EFI_ERROR_CODE (5)
#define EFI_NOT_READY		EFI_ERROR_CODE (6)
#define EFI_DEVICE_ERROR		EFI_ERROR_CODE (7)
#define EFI_WRITE_PROTECTED	EFI_ERROR_CODE (8)
#define EFI_OUT_OF_RESOURCES	EFI_ERROR_CODE (9)
#define EFI_VOLUME_CORRUPTED	EFI_ERROR_CODE (10)
#define EFI_VOLUME_FULL		EFI_ERROR_CODE (11)
#define EFI_NO_MEDIA		EFI_ERROR_CODE (12)
#define EFI_MEDIA_CHANGED		EFI_ERROR_CODE (13)
#define EFI_NOT_FOUND		EFI_ERROR_CODE (14)
#define EFI_ACCESS_DENIED		EFI_ERROR_CODE (15)
#define EFI_NO_RESPONSE		EFI_ERROR_CODE (16)
#define EFI_NO_MAPPING		EFI_ERROR_CODE (17)
#define EFI_TIMEOUT		EFI_ERROR_CODE (18)
#define EFI_NOT_STARTED		EFI_ERROR_CODE (19)
#define EFI_ALREADY_STARTED	EFI_ERROR_CODE (20)
#define EFI_ABORTED		EFI_ERROR_CODE (21)
#define EFI_ICMP_ERROR		EFI_ERROR_CODE (22)
#define EFI_TFTP_ERROR		EFI_ERROR_CODE (23)
#define EFI_PROTOCOL_ERROR		EFI_ERROR_CODE (24)
#define EFI_INCOMPATIBLE_VERSION	EFI_ERROR_CODE (25)
#define EFI_SECURITY_VIOLATION	EFI_ERROR_CODE (26)
#define EFI_CRC_ERROR		EFI_ERROR_CODE (27)

#define EFI_WARN_UNKNOWN_GLYPH	EFI_WARNING_CODE (1)
#define EFI_WARN_DELETE_FAILURE	EFI_WARNING_CODE (2)
#define EFI_WARN_WRITE_FAILURE	EFI_WARNING_CODE (3)
#define EFI_WARN_BUFFER_TOO_SMALL	EFI_WARNING_CODE (4)

typedef void *efi_handle_t;
typedef void *efi_event_t;
typedef efi_uint64_t efi_lba_t;
typedef efi_uintn_t efi_tpl_t;
typedef uint8_t efi_mac_address_t[32];
typedef uint8_t efi_ipv4_address_t[4];
typedef uint16_t efi_ipv6_address_t[8];
typedef uint8_t efi_ip_address_t[8] __attribute__ ((aligned(4)));
typedef efi_uint64_t efi_physical_address_t;
typedef efi_uint64_t efi_virtual_address_t;

struct efi_guid {
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	uint8_t data4[8];
} __attribute__ ((aligned(8)));
typedef struct efi_guid efi_guid_t;

struct efi_packed_guid {
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	uint8_t data4[8];
} __attribute__ ((packed));
typedef struct efi_packed_guid efi_packed_guid_t;

/* XXX although the spec does not specify the padding, this actually
   must have the padding!  */
struct efi_memory_descriptor {
	efi_uint32_t type;
	efi_uint32_t padding;
	efi_physical_address_t physical_start;
	efi_virtual_address_t virtual_start;
	efi_uint64_t num_pages;
	efi_uint64_t attribute;
} __attribute__ ((packed));
typedef struct efi_memory_descriptor efi_memory_descriptor_t;

/* Device Path definitions.  */
struct efi_device_path {
	efi_uint8_t type;
	efi_uint8_t subtype;
	efi_uint16_t length;
} __attribute__ ((packed));
typedef struct efi_device_path efi_device_path_t;
/* XXX EFI does not define EFI_DEVICE_PATH_PROTOCOL but uses it.
   It seems to be identical to EFI_DEVICE_PATH.  */
typedef struct efi_device_path efi_device_path_protocol_t;

#define EFI_DEVICE_PATH_TYPE(dp)		((dp)->type & 0x7f)
#define EFI_DEVICE_PATH_SUBTYPE(dp)	((dp)->subtype)
#define EFI_DEVICE_PATH_LENGTH(dp)		((dp)->length)

/* The End of Device Path nodes.  */
#define EFI_END_DEVICE_PATH_TYPE			(0xff & 0x7f)

#define EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE		0xff
#define EFI_END_THIS_DEVICE_PATH_SUBTYPE		0x01

#define EFI_END_ENTIRE_DEVICE_PATH(dp)	\
  (EFI_DEVICE_PATH_TYPE (dp) == EFI_END_DEVICE_PATH_TYPE \
   && (EFI_DEVICE_PATH_SUBTYPE (dp) \
       == EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE))

#define EFI_NEXT_DEVICE_PATH(dp)	\
  ((efi_device_path_t *) ((char *) (dp) \
                               + EFI_DEVICE_PATH_LENGTH (dp)))

/* Hardware Device Path.  */
#define EFI_HARDWARE_DEVICE_PATH_TYPE		1

#define EFI_PCI_DEVICE_PATH_SUBTYPE		1

struct efi_pci_device_path {
	efi_device_path_t header;
	efi_uint8_t function;
	efi_uint8_t device;
} __attribute__ ((packed));
typedef struct efi_pci_device_path efi_pci_device_path_t;

#define EFI_PCCARD_DEVICE_PATH_SUBTYPE		2

struct efi_pccard_device_path {
	efi_device_path_t header;
	efi_uint8_t function;
} __attribute__ ((packed));
typedef struct efi_pccard_device_path efi_pccard_device_path_t;

#define EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE	3

struct efi_memory_mapped_device_path {
	efi_device_path_t header;
	efi_uint32_t memory_type;
	efi_physical_address_t start_address;
	efi_physical_address_t end_address;
} __attribute__ ((packed));
typedef struct efi_memory_mapped_device_path
 efi_memory_mapped_device_path_t;

#define EFI_VENDOR_DEVICE_PATH_SUBTYPE		4

struct efi_vendor_device_path {
	efi_device_path_t header;
	efi_packed_guid_t vendor_guid;
	efi_uint8_t vendor_defined_data[0];
} __attribute__ ((packed));
typedef struct efi_vendor_device_path efi_vendor_device_path_t;

#define EFI_CONTROLLER_DEVICE_PATH_SUBTYPE		5

struct efi_controller_device_path {
	efi_device_path_t header;
	efi_uint32_t controller_number;
} __attribute__ ((packed));
typedef struct efi_controller_device_path
 efi_controller_device_path_t;

/* ACPI Device Path.  */
#define EFI_ACPI_DEVICE_PATH_TYPE			2

#define EFI_ACPI_DEVICE_PATH_SUBTYPE		1

struct efi_acpi_device_path {
	efi_device_path_t header;
	efi_uint32_t hid;
	efi_uint32_t uid;
} __attribute__ ((packed));
typedef struct efi_acpi_device_path efi_acpi_device_path_t;

#define EFI_EXPANDED_ACPI_DEVICE_PATH_SUBTYPE	2

struct efi_expanded_acpi_device_path {
	efi_device_path_t header;
	efi_uint32_t hid;
	efi_uint32_t uid;
	efi_uint32_t cid;
	char hidstr[0];
} __attribute__ ((packed));
typedef struct efi_expanded_acpi_device_path
 efi_expanded_acpi_device_path_t;

#define EFI_EXPANDED_ACPI_HIDSTR(dp)	\
  (((efi_expanded_acpi_device_path_t *) dp)->hidstr)
#define EFI_EXPANDED_ACPI_UIDSTR(dp)	\
  (EFI_EXPANDED_ACPI_HIDSTR(dp) \
   + strlen (EFI_EXPANDED_ACPI_HIDSTR(dp)) + 1)
#define EFI_EXPANDED_ACPI_CIDSTR(dp)	\
  (EFI_EXPANDED_ACPI_UIDSTR(dp) \
   + strlen (EFI_EXPANDED_ACPI_UIDSTR(dp)) + 1)

/* Messaging Device Path.  */
#define EFI_MESSAGING_DEVICE_PATH_TYPE		3

#define EFI_ATAPI_DEVICE_PATH_SUBTYPE		1

struct efi_atapi_device_path {
	efi_device_path_t header;
	efi_uint8_t primary_secondary;
	efi_uint8_t slave_master;
	efi_uint16_t lun;
} __attribute__ ((packed));
typedef struct efi_atapi_device_path efi_atapi_device_path_t;

#define EFI_SCSI_DEVICE_PATH_SUBTYPE		2

struct efi_scsi_device_path {
	efi_device_path_t header;
	efi_uint16_t pun;
	efi_uint16_t lun;
} __attribute__ ((packed));
typedef struct efi_scsi_device_path efi_scsi_device_path_t;

#define EFI_FIBRE_CHANNEL_DEVICE_PATH_SUBTYPE	3

struct efi_fibre_channel_device_path {
	efi_device_path_t header;
	efi_uint32_t reserved;
	efi_uint64_t wwn;
	efi_uint64_t lun;
} __attribute__ ((packed));
typedef struct efi_fibre_channel_device_path
 efi_fibre_channel_device_path_t;

#define EFI_1394_DEVICE_PATH_SUBTYPE		4

struct efi_1394_device_path {
	efi_device_path_t header;
	efi_uint32_t reserved;
	efi_uint64_t guid;
} __attribute__ ((packed));
typedef struct efi_1394_device_path efi_1394_device_path_t;

#define EFI_USB_DEVICE_PATH_SUBTYPE		5

struct efi_usb_device_path {
	efi_device_path_t header;
	efi_uint8_t parent_port_number;
	efi_uint8_t usb_interface;
} __attribute__ ((packed));
typedef struct efi_usb_device_path efi_usb_device_path_t;

#define EFI_USB_CLASS_DEVICE_PATH_SUBTYPE		15

struct efi_usb_class_device_path {
	efi_device_path_t header;
	efi_uint16_t vendor_id;
	efi_uint16_t product_id;
	efi_uint8_t device_class;
	efi_uint8_t device_subclass;
	efi_uint8_t device_protocol;
} __attribute__ ((packed));
typedef struct efi_usb_class_device_path efi_usb_class_device_path_t;

#define EFI_I2O_DEVICE_PATH_SUBTYPE		6

struct efi_i2o_device_path {
	efi_device_path_t header;
	efi_uint32_t tid;
} __attribute__ ((packed));
typedef struct efi_i2o_device_path efi_i2o_device_path_t;

#define EFI_MAC_ADDRESS_DEVICE_PATH_SUBTYPE	11

struct efi_mac_address_device_path {
	efi_device_path_t header;
	efi_mac_address_t mac_address;
	efi_uint8_t if_type;
} __attribute__ ((packed));
typedef struct efi_mac_address_device_path
 efi_mac_address_device_path_t;

#define EFI_IPV4_DEVICE_PATH_SUBTYPE		12

struct efi_ipv4_device_path {
	efi_device_path_t header;
	efi_ipv4_address_t local_ip_address;
	efi_ipv4_address_t remote_ip_address;
	efi_uint16_t local_port;
	efi_uint16_t remote_port;
	efi_uint16_t protocol;
	efi_uint8_t static_ip_address;
} __attribute__ ((packed));
typedef struct efi_ipv4_device_path efi_ipv4_device_path_t;

#define EFI_IPV6_DEVICE_PATH_SUBTYPE		13

struct efi_ipv6_device_path {
	efi_device_path_t header;
	efi_ipv6_address_t local_ip_address;
	efi_ipv6_address_t remote_ip_address;
	efi_uint16_t local_port;
	efi_uint16_t remote_port;
	efi_uint16_t protocol;
	efi_uint8_t static_ip_address;
} __attribute__ ((packed));
typedef struct efi_ipv6_device_path efi_ipv6_device_path_t;

#define EFI_INFINIBAND_DEVICE_PATH_SUBTYPE		9

struct efi_infiniband_device_path {
	efi_device_path_t header;
	efi_uint32_t resource_flags;
	efi_uint8_t port_gid[16];
	efi_uint64_t remote_id;
	efi_uint64_t target_port_id;
	efi_uint64_t device_id;
} __attribute__ ((packed));
typedef struct efi_infiniband_device_path
 efi_infiniband_device_path_t;

#define EFI_UART_DEVICE_PATH_SUBTYPE		14

struct efi_uart_device_path {
	efi_device_path_t header;
	efi_uint32_t reserved;
	efi_uint64_t baud_rate;
	efi_uint8_t data_bits;
	efi_uint8_t parity;
	efi_uint8_t stop_bits;
} __attribute__ ((packed));
typedef struct efi_uart_device_path efi_uart_device_path_t;

#define EFI_SATA_DEVICE_PATH_SUBTYPE		18

struct efi_sata_device_path {
	efi_device_path_t header;
	efi_uint16_t hba_port;
	efi_uint16_t multiplier_port;
	efi_uint16_t lun;
} __attribute__ ((packed));
typedef struct efi_sata_device_path efi_sata_device_path_t;

#define EFI_VENDOR_MESSAGING_DEVICE_PATH_SUBTYPE	10

/* Media Device Path.  */
#define EFI_MEDIA_DEVICE_PATH_TYPE			4

#define EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE		1

struct efi_hard_drive_device_path {
	efi_device_path_t header;
	efi_uint32_t partition_number;
	efi_lba_t partition_start;
	efi_lba_t partition_size;
	efi_uint8_t partition_signature[16];
	efi_uint8_t partmap_type;
	efi_uint8_t signature_type;
} __attribute__ ((packed));
typedef struct efi_hard_drive_device_path
 efi_hard_drive_device_path_t;

#define EFI_CDROM_DEVICE_PATH_SUBTYPE		2

struct efi_cdrom_device_path {
	efi_device_path_t header;
	efi_uint32_t boot_entry;
	efi_lba_t partition_start;
	efi_lba_t partition_size;
} __attribute__ ((packed));
typedef struct efi_cdrom_device_path efi_cdrom_device_path_t;

#define EFI_VENDOR_MEDIA_DEVICE_PATH_SUBTYPE	3

struct efi_vendor_media_device_path {
	efi_device_path_t header;
	efi_packed_guid_t vendor_guid;
	efi_uint8_t vendor_defined_data[0];
} __attribute__ ((packed));
typedef struct efi_vendor_media_device_path
 efi_vendor_media_device_path_t;

#define EFI_FILE_PATH_DEVICE_PATH_SUBTYPE		4

struct efi_file_path_device_path {
	efi_device_path_t header;
	efi_char16_t path_name[0];
} __attribute__ ((packed));
typedef struct efi_file_path_device_path efi_file_path_device_path_t;

#define EFI_PROTOCOL_DEVICE_PATH_SUBTYPE		5

struct efi_protocol_device_path {
	efi_device_path_t header;
	efi_packed_guid_t guid;
} __attribute__ ((packed));
typedef struct efi_protocol_device_path efi_protocol_device_path_t;

#define EFI_PIWG_DEVICE_PATH_SUBTYPE		6

struct efi_piwg_device_path {
	efi_device_path_t header;
	efi_packed_guid_t guid;
} __attribute__ ((packed));
typedef struct efi_piwg_device_path efi_piwg_device_path_t;

/* BIOS Boot Specification Device Path.  */
#define EFI_BIOS_DEVICE_PATH_TYPE			5

#define EFI_BIOS_DEVICE_PATH_SUBTYPE		1

struct efi_bios_device_path {
	efi_device_path_t header;
	efi_uint16_t device_type;
	efi_uint16_t status_flags;
	char description[0];
} __attribute__ ((packed));
typedef struct efi_bios_device_path efi_bios_device_path_t;

struct efi_open_protocol_information_entry {
	efi_handle_t agent_handle;
	efi_handle_t controller_handle;
	efi_uint32_t attributes;
	efi_uint32_t open_count;
};
typedef struct efi_open_protocol_information_entry
 efi_open_protocol_information_entry_t;

struct efi_time {
	efi_uint16_t year;
	efi_uint8_t month;
	efi_uint8_t day;
	efi_uint8_t hour;
	efi_uint8_t minute;
	efi_uint8_t second;
	efi_uint8_t pad1;
	efi_uint32_t nanosecond;
	efi_int16_t time_zone;
	efi_uint8_t daylight;
	efi_uint8_t pad2;
} __attribute__ ((packed));
typedef struct efi_time efi_time_t;

struct efi_time_capabilities {
	efi_uint32_t resolution;
	efi_uint32_t accuracy;
	efi_boolean_t sets_to_zero;
};
typedef struct efi_time_capabilities efi_time_capabilities_t;

struct efi_input_key {
	efi_uint16_t scan_code;
	efi_char16_t unicode_char;
};
typedef struct efi_input_key efi_input_key_t;

struct efi_simple_text_output_mode {
	efi_int32_t max_mode;
	efi_int32_t mode;
	efi_int32_t attribute;
	efi_int32_t cursor_column;
	efi_int32_t cursor_row;
	efi_boolean_t cursor_visible;
};
typedef struct efi_simple_text_output_mode
 efi_simple_text_output_mode_t;

/* Tables.  */
struct efi_table_header {
	efi_uint64_t signature;
	efi_uint32_t revision;
	efi_uint32_t header_size;
	efi_uint32_t crc32;
	efi_uint32_t reserved;
};
typedef struct efi_table_header efi_table_header_t;

struct efi_boot_services {
	efi_table_header_t hdr;

	 efi_tpl_t(*raise_tpl) (efi_tpl_t new_tpl);

	void
	 (*restore_tpl) (efi_tpl_t old_tpl);

	 efi_status_t
	    (*allocate_pages) (efi_allocate_type_t type,
			       efi_memory_type_t memory_type,
			       efi_uintn_t pages,
			       efi_physical_address_t * memory);

	 efi_status_t
	    (*free_pages) (efi_physical_address_t memory, efi_uintn_t pages);

	 efi_status_t
	    (*get_memory_map) (efi_uintn_t * memory_map_size,
			       efi_memory_descriptor_t * memory_map,
			       efi_uintn_t * map_key,
			       efi_uintn_t * descriptor_size,
			       efi_uint32_t * descriptor_version);

	 efi_status_t
	    (*allocate_pool) (efi_memory_type_t pool_type,
			      efi_uintn_t size, void **buffer);

	 efi_status_t(*free_pool) (void *buffer);

	 efi_status_t
	    (*create_event) (efi_uint32_t type,
			     efi_tpl_t notify_tpl,
			     void (*notify_function) (efi_event_t event,
						      void *context),
			     void *notify_context, efi_event_t * event);

	 efi_status_t
	    (*set_timer) (efi_event_t event,
			  efi_timer_delay_t type, efi_uint64_t trigger_time);

	 efi_status_t
	    (*wait_for_event) (efi_uintn_t num_events,
			       efi_event_t * event, efi_uintn_t * index);

	 efi_status_t(*signal_event) (efi_event_t event);

	 efi_status_t(*close_event) (efi_event_t event);

	 efi_status_t(*check_event) (efi_event_t event);

	 efi_status_t
	    (*install_protocol_interface) (efi_handle_t * handle,
					   efi_guid_t * protocol,
					   efi_interface_type_t
					   protocol_interface_type,
					   void *protocol_interface);

	 efi_status_t
	    (*reinstall_protocol_interface) (efi_handle_t handle,
					     efi_guid_t * protocol,
					     void *old_interface,
					     void *new_interface);

	 efi_status_t
	    (*uninstall_protocol_interface) (efi_handle_t handle,
					     efi_guid_t * protocol,
					     void *protocol_interface);

	 efi_status_t
	    (*handle_protocol) (efi_handle_t handle,
				efi_guid_t * protocol,
				void **protocol_interface);

	void *reserved;

	 efi_status_t
	    (*register_protocol_notify) (efi_guid_t * protocol,
					 efi_event_t event,
					 void **registration);

	 efi_status_t
	    (*locate_handle) (efi_locate_search_type_t search_type,
			      efi_guid_t * protocol,
			      void *search_key,
			      efi_uintn_t * buffer_size, efi_handle_t * buffer);

	 efi_status_t
	    (*locate_device_path) (efi_guid_t * protocol,
				   efi_device_path_t ** device_path,
				   efi_handle_t * device);

	 efi_status_t
	    (*install_configuration_table) (efi_guid_t * guid, void *table);

	 efi_status_t
	    (*load_image) (efi_boolean_t boot_policy,
			   efi_handle_t parent_image_handle,
			   efi_device_path_t * file_path,
			   void *source_buffer,
			   efi_uintn_t source_size,
			   efi_handle_t * image_handle);

	 efi_status_t
	    (*start_image) (efi_handle_t image_handle,
			    efi_uintn_t * exit_data_size,
			    efi_char16_t ** exit_data);

	 efi_status_t
	    (*exit) (efi_handle_t image_handle,
		     efi_status_t exit_status,
		     efi_uintn_t exit_data_size,
		     efi_char16_t * exit_data) __attribute__ ((noreturn));

	 efi_status_t(*unload_image) (efi_handle_t image_handle);

	 efi_status_t
	    (*exit_boot_services) (efi_handle_t image_handle,
				   efi_uintn_t map_key);

	 efi_status_t(*get_next_monotonic_count) (efi_uint64_t * count);

	 efi_status_t(*stall) (efi_uintn_t microseconds);

	 efi_status_t
	    (*set_watchdog_timer) (efi_uintn_t timeout,
				   efi_uint64_t watchdog_code,
				   efi_uintn_t data_size,
				   efi_char16_t * watchdog_data);

	 efi_status_t
	    (*connect_controller) (efi_handle_t controller_handle,
				   efi_handle_t * driver_image_handle,
				   efi_device_path_protocol_t *
				   remaining_device_path,
				   efi_boolean_t recursive);

	 efi_status_t
	    (*disconnect_controller) (efi_handle_t controller_handle,
				      efi_handle_t driver_image_handle,
				      efi_handle_t child_handle);

	 efi_status_t
	    (*open_protocol) (efi_handle_t handle,
			      efi_guid_t * protocol,
			      void **protocol_interface,
			      efi_handle_t agent_handle,
			      efi_handle_t controller_handle,
			      efi_uint32_t attributes);

	 efi_status_t
	    (*close_protocol) (efi_handle_t handle,
			       efi_guid_t * protocol,
			       efi_handle_t agent_handle,
			       efi_handle_t controller_handle);

	 efi_status_t
	    (*open_protocol_information) (efi_handle_t handle,
					  efi_guid_t * protocol,
					  efi_open_protocol_information_entry_t
					  ** entry_buffer,
					  efi_uintn_t * entry_count);

	 efi_status_t
	    (*protocols_per_handle) (efi_handle_t handle,
				     efi_packed_guid_t *** protocol_buffer,
				     efi_uintn_t * protocol_buffer_count);

	 efi_status_t
	    (*locate_handle_buffer) (efi_locate_search_type_t search_type,
				     efi_guid_t * protocol,
				     void *search_key,
				     efi_uintn_t * no_handles,
				     efi_handle_t ** buffer);

	 efi_status_t
	    (*locate_protocol) (efi_guid_t * protocol,
				void *registration, void **protocol_interface);

	 efi_status_t
	    (*install_multiple_protocol_interfaces) (efi_handle_t * handle,
						     ...);

	 efi_status_t
	    (*uninstall_multiple_protocol_interfaces) (efi_handle_t handle,
						       ...);

	 efi_status_t
	    (*calculate_crc32) (void *data,
				efi_uintn_t data_size, efi_uint32_t * crc32);

	void
	 (*copy_mem) (void *destination, void *source, efi_uintn_t length);

	void
	 (*set_mem) (void *buffer, efi_uintn_t size, efi_uint8_t value);
};
typedef struct efi_boot_services efi_boot_services_t;

struct efi_runtime_services {
	efi_table_header_t hdr;

	 efi_status_t
	    (*get_time) (efi_time_t * time,
			 efi_time_capabilities_t * capabilities);

	 efi_status_t(*set_time) (efi_time_t * time);

	 efi_status_t
	    (*get_wakeup_time) (efi_boolean_t * enabled,
				efi_boolean_t * pending, efi_time_t * time);

	 efi_status_t
	    (*set_wakeup_time) (efi_boolean_t enabled, efi_time_t * time);

	 efi_status_t
	    (*set_virtual_address_map) (efi_uintn_t memory_map_size,
					efi_uintn_t descriptor_size,
					efi_uint32_t descriptor_version,
					efi_memory_descriptor_t * virtual_map);

	 efi_status_t
	    (*convert_pointer) (efi_uintn_t debug_disposition, void **address);

#define EFI_GLOBAL_VARIABLE_GUID \
  { 0x8BE4DF61, 0x93CA, 0x11d2, { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B,0x8C }}

	 efi_status_t
	    (*get_variable) (efi_char16_t * variable_name,
			     const efi_guid_t * vendor_guid,
			     efi_uint32_t * attributes,
			     efi_uintn_t * data_size, void *data);

	 efi_status_t
	    (*get_next_variable_name) (efi_uintn_t * variable_name_size,
				       efi_char16_t * variable_name,
				       efi_guid_t * vendor_guid);

	 efi_status_t
	    (*set_variable) (efi_char16_t * variable_name,
			     const efi_guid_t * vendor_guid,
			     efi_uint32_t attributes,
			     efi_uintn_t data_size, void *data);

	 efi_status_t
	    (*get_next_high_monotonic_count) (efi_uint32_t * high_count);

	void
	 (*reset_system) (efi_reset_type_t reset_type,
			  efi_status_t reset_status,
			  efi_uintn_t data_size, efi_char16_t * reset_data);
};
typedef struct efi_runtime_services efi_runtime_services_t;

struct efi_configuration_table {
	efi_packed_guid_t vendor_guid;
	void *vendor_table;
} __attribute__ ((packed));
typedef struct efi_configuration_table efi_configuration_table_t;

#define EFI_SYSTEM_TABLE_SIGNATURE 0x5453595320494249LL
#define EFI_RUNTIME_SERVICES_SIGNATURE 0x56524553544e5552LL
#define EFI_BOOT_SERVICES_SIGNATURE 0x56524553544f4f42LL

struct efi_serial_io_interface {
	efi_uint32_t revision;
	void (*reset) (void);
	 efi_status_t(*set_attributes) (struct efi_serial_io_interface
					* this, efi_uint64_t speed,
					efi_uint32_t fifo_depth,
					efi_uint32_t timeout,
					efi_parity_type_t parity,
					uint8_t word_len,
					efi_stop_bits_t stop_bits);
	 efi_status_t(*set_control_bits) (struct
					  efi_serial_io_interface *
					  this, efi_uint32_t flags);
	void (*get_control_bits) (void);
	 efi_status_t(*write) (struct efi_serial_io_interface * this,
			       efi_uintn_t * buf_size, void *buffer);
	 efi_status_t(*read) (struct efi_serial_io_interface * this,
			      efi_uintn_t * buf_size, void *buffer);
};

struct efi_simple_input_interface {
	efi_status_t
	    (*reset) (struct efi_simple_input_interface * this,
		      efi_boolean_t extended_verification);

	efi_status_t
	    (*read_key_stroke) (struct efi_simple_input_interface * this,
				efi_input_key_t * key);

	efi_event_t wait_for_key;
};
typedef struct efi_simple_input_interface
 efi_simple_input_interface_t;

struct efi_simple_text_output_interface {
	efi_status_t
	    (*reset) (struct efi_simple_text_output_interface * this,
		      efi_boolean_t extended_verification);

	efi_status_t
	    (*output_string) (struct efi_simple_text_output_interface *
			      this, efi_char16_t * string);

	efi_status_t
	    (*test_string) (struct efi_simple_text_output_interface * this,
			    efi_char16_t * string);

	efi_status_t
	    (*query_mode) (struct efi_simple_text_output_interface * this,
			   efi_uintn_t mode_number,
			   efi_uintn_t * columns, efi_uintn_t * rows);

	efi_status_t
	    (*set_mode) (struct efi_simple_text_output_interface * this,
			 efi_uintn_t mode_number);

	efi_status_t
	    (*set_attributes) (struct efi_simple_text_output_interface *
			       this, efi_uintn_t attribute);

	efi_status_t
	    (*clear_screen) (struct efi_simple_text_output_interface * this);

	efi_status_t
	    (*set_cursor_position) (struct efi_simple_text_output_interface
				    * this, efi_uintn_t column,
				    efi_uintn_t row);

	efi_status_t
	    (*enable_cursor) (struct efi_simple_text_output_interface *
			      this, efi_boolean_t visible);

	efi_simple_text_output_mode_t *mode;
};
typedef struct efi_simple_text_output_interface
 efi_simple_text_output_interface_t;

typedef uint8_t efi_pxe_packet_t[1472];

typedef struct efi_pxe_mode {
	uint8_t unused[52];
	efi_pxe_packet_t dhcp_discover;
	efi_pxe_packet_t dhcp_ack;
	efi_pxe_packet_t proxy_offer;
	efi_pxe_packet_t pxe_discover;
	efi_pxe_packet_t pxe_reply;
} efi_pxe_mode_t;

typedef struct efi_pxe {
	uint64_t rev;
	void (*start) (void);
	void (*stop) (void);
	void (*dhcp) (void);
	void (*discover) (void);
	void (*mftp) (void);
	void (*udpwrite) (void);
	void (*udpread) (void);
	void (*setipfilter) (void);
	void (*arp) (void);
	void (*setparams) (void);
	void (*setstationip) (void);
	void (*setpackets) (void);
	struct efi_pxe_mode *mode;
} efi_pxe_t;

#define EFI_BLACK		0x00
#define EFI_BLUE		0x01
#define EFI_GREEN		0x02
#define EFI_CYAN		0x03
#define EFI_RED		0x04
#define EFI_MAGENTA	0x05
#define EFI_BROWN		0x06
#define EFI_LIGHTGRAY	0x07
#define EFI_BRIGHT		0x08
#define EFI_DARKGRAY	0x08
#define EFI_LIGHTBLUE	0x09
#define EFI_LIGHTGREEN	0x0A
#define EFI_LIGHTCYAN	0x0B
#define EFI_LIGHTRED	0x0C
#define EFI_LIGHTMAGENTA	0x0D
#define EFI_YELLOW		0x0E
#define EFI_WHITE		0x0F

#define EFI_BACKGROUND_BLACK	0x00
#define EFI_BACKGROUND_BLUE	0x10
#define EFI_BACKGROUND_GREEN	0x20
#define EFI_BACKGROUND_CYAN	0x30
#define EFI_BACKGROUND_RED		0x40
#define EFI_BACKGROUND_MAGENTA	0x50
#define EFI_BACKGROUND_BROWN	0x60
#define EFI_BACKGROUND_LIGHTGRAY	0x70

#define EFI_TEXT_ATTR(fg, bg)	((fg) | ((bg)))

struct efi_system_table {
	efi_table_header_t hdr;
	efi_char16_t *firmware_vendor;
	efi_uint32_t firmware_revision;
	efi_handle_t console_in_handler;
	efi_simple_input_interface_t *con_in;
	efi_handle_t console_out_handler;
	efi_simple_text_output_interface_t *con_out;
	efi_handle_t standard_error_handle;
	efi_simple_text_output_interface_t *std_err;
	efi_runtime_services_t *runtime_services;
	efi_boot_services_t *boot_services;
	efi_uintn_t num_table_entries;
	efi_configuration_table_t *configuration_table;
};
typedef struct efi_system_table efi_system_table_t;

struct efi_loaded_image {
	efi_uint32_t revision;
	efi_handle_t parent_handle;
	efi_system_table_t *system_table;

	efi_handle_t device_handle;
	efi_device_path_t *file_path;
	void *reserved;

	efi_uint32_t load_options_size;
	void *load_options;

	void *image_base;
	efi_uint64_t image_size;
	efi_memory_type_t image_code_type;
	efi_memory_type_t image_data_type;

	 efi_status_t(*unload) (efi_handle_t image_handle);
};
typedef struct efi_loaded_image efi_loaded_image_t;

struct efi_disk_io {
	efi_uint64_t revision;
	 efi_status_t(*read) (struct efi_disk_io * this,
			      efi_uint32_t media_id,
			      efi_uint64_t offset,
			      efi_uintn_t buffer_size, void *buffer);
	 efi_status_t(*write) (struct efi_disk_io * this,
			       efi_uint32_t media_id,
			       efi_uint64_t offset,
			       efi_uintn_t buffer_size, void *buffer);
};
typedef struct efi_disk_io efi_disk_io_t;

struct efi_block_io_media {
	efi_uint32_t media_id;
	efi_boolean_t removable_media;
	efi_boolean_t media_present;
	efi_boolean_t logical_partition;
	efi_boolean_t read_only;
	efi_boolean_t write_caching;
	efi_uint8_t pad[3];
	efi_uint32_t block_size;
	efi_uint32_t io_align;
	efi_uint8_t pad2[4];
	efi_lba_t last_block;
};
typedef struct efi_block_io_media efi_block_io_media_t;

typedef uint8_t efi_mac_t[32];

struct efi_simple_network_mode {
	uint32_t state;
	uint32_t hwaddr_size;
	uint32_t media_header_size;
	uint32_t max_packet_size;
	uint32_t nvram_size;
	uint32_t nvram_access_size;
	uint32_t receive_filter_mask;
	uint32_t receive_filter_setting;
	uint32_t max_mcast_filter_count;
	uint32_t mcast_filter_count;
	efi_mac_t mcast_filter[16];
	efi_mac_t current_address;
	efi_mac_t broadcast_address;
	efi_mac_t permanent_address;
	uint8_t if_type;
	uint8_t mac_changeable;
	uint8_t multitx_supported;
	uint8_t media_present_supported;
	uint8_t media_present;
};

enum {
	EFI_NETWORK_STOPPED,
	EFI_NETWORK_STARTED,
	EFI_NETWORK_INITIALIZED,
};

struct efi_simple_network {
	uint64_t revision;
	 efi_status_t(*start) (struct efi_simple_network * this);
	void (*stop) (void);
	 efi_status_t(*initialize) (struct efi_simple_network * this,
				    efi_uintn_t extra_rx, efi_uintn_t extra_tx);
	void (*reset) (void);
	void (*shutdown) (void);
	void (*receive_filters) (void);
	void (*station_address) (void);
	void (*statistics) (void);
	void (*mcastiptomac) (void);
	void (*nvdata) (void);
	 efi_status_t(*get_status) (struct efi_simple_network * this,
				    uint32_t * int_status, void **txbuf);
	 efi_status_t(*transmit) (struct efi_simple_network * this,
				  efi_uintn_t header_size,
				  efi_uintn_t buffer_size,
				  void *buffer,
				  efi_mac_t * src_addr,
				  efi_mac_t * dest_addr,
				  efi_uint16_t * protocol);
	 efi_status_t(*receive) (struct efi_simple_network * this,
				 efi_uintn_t * header_size,
				 efi_uintn_t * buffer_size,
				 void *buffer,
				 efi_mac_t * src_addr,
				 efi_mac_t * dest_addr, uint16_t * protocol);
	void (*waitforpacket) (void);
	struct efi_simple_network_mode *mode;
};
typedef struct efi_simple_network efi_simple_network_t;

struct efi_block_io {
	efi_uint64_t revision;
	efi_block_io_media_t *media;
	 efi_status_t(*reset) (struct efi_block_io * this,
			       efi_boolean_t extended_verification);
	 efi_status_t(*read_blocks) (struct efi_block_io * this,
				     efi_uint32_t media_id,
				     efi_lba_t lba,
				     efi_uintn_t buffer_size, void *buffer);
	 efi_status_t(*write_blocks) (struct efi_block_io * this,
				      efi_uint32_t media_id,
				      efi_lba_t lba,
				      efi_uintn_t buffer_size, void *buffer);
	 efi_status_t(*flush_blocks) (struct efi_block_io * this);
};
typedef struct efi_block_io efi_block_io_t;

#define EFI_STACK_SIZE 0x20000

extern efi_system_table_t efi_system_table;

efi_handle_t uefi_create_handle(void);
efi_status_t uefi_add_protocol_interface(efi_handle_t handle, efi_guid_t guid, void* interface);
void* uefi_create_device_path(size_t size);

#endif /* ! EFI_API_HEADER */
