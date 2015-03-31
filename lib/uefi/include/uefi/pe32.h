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

#ifndef EFI_PE32_HEADER
#define EFI_PE32_HEADER	1

#include <stdint.h>

/* The MSDOS compatibility stub. This was copied from the output of
   objcopy, and it is not necessary to care about what this means.  */
#define PE32_MSDOS_STUB \
  { \
    0x4d, 0x5a, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00, \
    0x04, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, \
    0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, \
    0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd, \
    0x21, 0xb8, 0x01, 0x4c, 0xcd, 0x21, 0x54, 0x68, \
    0x69, 0x73, 0x20, 0x70, 0x72, 0x6f, 0x67, 0x72, \
    0x61, 0x6d, 0x20, 0x63, 0x61, 0x6e, 0x6e, 0x6f, \
    0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6e, \
    0x20, 0x69, 0x6e, 0x20, 0x44, 0x4f, 0x53, 0x20, \
    0x6d, 0x6f, 0x64, 0x65, 0x2e, 0x0d, 0x0d, 0x0a, \
    0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  \
  }

#define PE32_MSDOS_STUB_SIZE	0x80

/* According to the spec, the minimal alignment is 512 bytes...
   But some examples (such as EFI drivers in the Intel
   Sample Implementation) use 32 bytes (0x20) instead, and it seems
   to be working. For now, GRUB uses 512 bytes for safety.  */
#define PE32_SECTION_ALIGNMENT	0x200
#define PE32_FILE_ALIGNMENT	PE32_SECTION_ALIGNMENT

struct pe32_coff_header {
	uint16_t machine;
	uint16_t num_sections;
	uint32_t time;
	uint32_t symtab_offset;
	uint32_t num_symbols;
	uint16_t optional_header_size;
	uint16_t characteristics;
};

#define PE32_MACHINE_I386			0x14c
#define PE32_MACHINE_IA64			0x200
#define PE32_MACHINE_X86_64		0x8664
#define PE32_MACHINE_ARMTHUMB_MIXED	0x01c2
#define PE32_MACHINE_ARM64			0xAA64

#define PE32_RELOCS_STRIPPED		0x0001
#define PE32_EXECUTABLE_IMAGE		0x0002
#define PE32_LINE_NUMS_STRIPPED		0x0004
#define PE32_LOCAL_SYMS_STRIPPED		0x0008
#define PE32_AGGRESSIVE_WS_TRIM		0x0010
#define PE32_LARGE_ADDRESS_AWARE		0x0020
#define PE32_16BIT_MACHINE			0x0040
#define PE32_BYTES_REVERSED_LO		0x0080
#define PE32_32BIT_MACHINE			0x0100
#define PE32_DEBUG_STRIPPED		0x0200
#define PE32_REMOVABLE_RUN_FROM_SWAP	0x0400
#define PE32_SYSTEM			0x1000
#define PE32_DLL				0x2000
#define PE32_UP_SYSTEM_ONLY		0x4000
#define PE32_BYTES_REVERSED_HI		0x8000

struct pe32_data_directory {
	uint32_t rva;
	uint32_t size;
};

struct pe32_optional_header {
	uint16_t magic;
	uint8_t major_linker_version;
	uint8_t minor_linker_version;
	uint32_t code_size;
	uint32_t data_size;
	uint32_t bss_size;
	uint32_t entry_addr;
	uint32_t code_base;

	uint32_t data_base;
	uint32_t image_base;

	uint32_t section_alignment;
	uint32_t file_alignment;
	uint16_t major_os_version;
	uint16_t minor_os_version;
	uint16_t major_image_version;
	uint16_t minor_image_version;
	uint16_t major_subsystem_version;
	uint16_t minor_subsystem_version;
	uint32_t reserved;
	uint32_t image_size;
	uint32_t header_size;
	uint32_t checksum;
	uint16_t subsystem;
	uint16_t dll_characteristics;

	uint32_t stack_reserve_size;
	uint32_t stack_commit_size;
	uint32_t heap_reserve_size;
	uint32_t heap_commit_size;

	uint32_t loader_flags;
	uint32_t num_data_directories;

	/* Data directories.  */
	struct pe32_data_directory export_table;
	struct pe32_data_directory import_table;
	struct pe32_data_directory resource_table;
	struct pe32_data_directory exception_table;
	struct pe32_data_directory certificate_table;
	struct pe32_data_directory base_relocation_table;
	struct pe32_data_directory debug;
	struct pe32_data_directory architecture;
	struct pe32_data_directory global_ptr;
	struct pe32_data_directory tls_table;
	struct pe32_data_directory load_config_table;
	struct pe32_data_directory bound_import;
	struct pe32_data_directory iat;
	struct pe32_data_directory delay_import_descriptor;
	struct pe32_data_directory com_runtime_header;
	struct pe32_data_directory reserved_entry;
};

struct pe64_optional_header {
	uint16_t magic;
	uint8_t major_linker_version;
	uint8_t minor_linker_version;
	uint32_t code_size;
	uint32_t data_size;
	uint32_t bss_size;
	uint32_t entry_addr;
	uint32_t code_base;

	uint64_t image_base;

	uint32_t section_alignment;
	uint32_t file_alignment;
	uint16_t major_os_version;
	uint16_t minor_os_version;
	uint16_t major_image_version;
	uint16_t minor_image_version;
	uint16_t major_subsystem_version;
	uint16_t minor_subsystem_version;
	uint32_t reserved;
	uint32_t image_size;
	uint32_t header_size;
	uint32_t checksum;
	uint16_t subsystem;
	uint16_t dll_characteristics;

	uint64_t stack_reserve_size;
	uint64_t stack_commit_size;
	uint64_t heap_reserve_size;
	uint64_t heap_commit_size;

	uint32_t loader_flags;
	uint32_t num_data_directories;

	/* Data directories.  */
	struct pe32_data_directory export_table;
	struct pe32_data_directory import_table;
	struct pe32_data_directory resource_table;
	struct pe32_data_directory exception_table;
	struct pe32_data_directory certificate_table;
	struct pe32_data_directory base_relocation_table;
	struct pe32_data_directory debug;
	struct pe32_data_directory architecture;
	struct pe32_data_directory global_ptr;
	struct pe32_data_directory tls_table;
	struct pe32_data_directory load_config_table;
	struct pe32_data_directory bound_import;
	struct pe32_data_directory iat;
	struct pe32_data_directory delay_import_descriptor;
	struct pe32_data_directory com_runtime_header;
	struct pe32_data_directory reserved_entry;
};

#define PE32_PE32_MAGIC	0x10b
#define PE32_PE64_MAGIC	0x20b

#define PE32_SUBSYSTEM_EFI_APPLICATION	10

#define PE32_NUM_DATA_DIRECTORIES	16

struct pe32_section_table {
	char name[8];
	uint32_t virtual_size;
	uint32_t virtual_address;
	uint32_t raw_data_size;
	uint32_t raw_data_offset;
	uint32_t relocations_offset;
	uint32_t line_numbers_offset;
	uint16_t num_relocations;
	uint16_t num_line_numbers;
	uint32_t characteristics;
};

#define PE32_SCN_CNT_CODE			0x00000020
#define PE32_SCN_CNT_INITIALIZED_DATA	0x00000040
#define PE32_SCN_MEM_DISCARDABLE		0x02000000
#define PE32_SCN_MEM_EXECUTE		0x20000000
#define PE32_SCN_MEM_READ			0x40000000
#define PE32_SCN_MEM_WRITE			0x80000000

#define PE32_SCN_ALIGN_1BYTES		0x00100000
#define PE32_SCN_ALIGN_2BYTES		0x00200000
#define PE32_SCN_ALIGN_4BYTES		0x00300000
#define PE32_SCN_ALIGN_8BYTES		0x00400000
#define PE32_SCN_ALIGN_16BYTES		0x00500000
#define PE32_SCN_ALIGN_32BYTES		0x00600000
#define PE32_SCN_ALIGN_64BYTES		0x00700000

#define PE32_SCN_ALIGN_SHIFT		20
#define PE32_SCN_ALIGN_MASK		7

#define PE32_SIGNATURE_SIZE 4

struct pe32_header {
	/* This should be filled in with PE32_MSDOS_STUB.  */
	uint8_t msdos_stub[PE32_MSDOS_STUB_SIZE];

	/* This is always PE\0\0.  */
	char signature[PE32_SIGNATURE_SIZE];

	/* The COFF file header.  */
	struct pe32_coff_header coff_header;

#if TARGET_SIZEOF_VOID_P == 8
	/* The Optional header.  */
	struct pe64_optional_header optional_header;
#else
	/* The Optional header.  */
	struct pe32_optional_header optional_header;
#endif
};

struct pe32_fixup_block {
	uint32_t page_rva;
	uint32_t block_size;
	uint16_t entries[0];
};

#define PE32_FIXUP_ENTRY(type, offset)	(((type) << 12) | (offset))

#define PE32_REL_BASED_ABSOLUTE	0
#define PE32_REL_BASED_HIGH	1
#define PE32_REL_BASED_LOW		2
#define PE32_REL_BASED_HIGHLOW	3
#define PE32_REL_BASED_HIGHADJ	4
#define PE32_REL_BASED_MIPS_JMPADDR 5
#define PE32_REL_BASED_ARM_MOV32A  5
#define PE32_REL_BASED_SECTION	6
#define PE32_REL_BASED_REL		7
#define PE32_REL_BASED_ARM_MOV32T  7
#define PE32_REL_BASED_IA64_IMM64	9
#define PE32_REL_BASED_DIR64	10
#define PE32_REL_BASED_HIGH3ADJ	11

struct pe32_symbol {
	union {
		char short_name[8];
		uint32_t long_name[2];
	};

	uint32_t value;
	uint16_t section;
	uint16_t type;
	uint8_t storage_class;
	uint8_t num_aux;
} __attribute__ ((packed));

#define PE32_SYM_CLASS_EXTERNAL	2
#define PE32_SYM_CLASS_STATIC	3
#define PE32_SYM_CLASS_FILE	0x67

#define PE32_DT_FUNCTION		0x20

struct pe32_reloc {
	uint32_t offset;
	uint32_t symtab_index;
	uint16_t type;
} __attribute__ ((packed));

#define PE32_REL_I386_DIR32	0x6
#define PE32_REL_I386_REL32	0x14

int peloader_load(void* data, size_t size, void* ramdisk, size_t ramdisk_size, const char* bootdev, const char* bootpath);

#endif /* ! EFI_PE32_HEADER */
