#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

#define __PF2_SECTION_IS_X(s, x) (memcmp(s, FONT_FORMAT_SECTION_NAMES_##x, 4)==0)
#define PF2_SECTION_IS_FILE(s) (__PF2_SECTION_IS_X(s, FILE))
#define PF2_SECTION_IS_FONT_NAME(s) (__PF2_SECTION_IS_X(s, FONT_NAME))
#define PF2_SECTION_IS_POINT_SIZE(s) (__PF2_SECTION_IS_X(s, POINT_SIZE))
#define PF2_SECTION_IS_WEIGHT(s) (__PF2_SECTION_IS_X(s, WEIGHT))
#define PF2_SECTION_IS_MAX_CHAR_WIDTH(s) (__PF2_SECTION_IS_X(s, MAX_CHAR_WIDTH))
#define PF2_SECTION_IS_MAX_CHAR_HEIGHT(s) (__PF2_SECTION_IS_X(s, MAX_CHAR_HEIGHT))
#define PF2_SECTION_IS_ASCENT(s) (__PF2_SECTION_IS_X(s, ASCENT))
#define PF2_SECTION_IS_DESCENT(s) (__PF2_SECTION_IS_X(s, DESCENT))
#define PF2_SECTION_IS_CHAR_INDEX(s) (__PF2_SECTION_IS_X(s, CHAR_INDEX))
#define PF2_SECTION_IS_DATA(s) (__PF2_SECTION_IS_X(s, DATA))
#define PF2_SECTION_IS_FAMILY(s) (__PF2_SECTION_IS_X(s, FAMILY))
#define PF2_SECTION_IS_SLAN(s) (__PF2_SECTION_IS_X(s, SLAN))

#define FONT_WEIGHT_NORMAL 100
#define FONT_WEIGHT_BOLD 200

#define FONT_SLANT_NORMAL 0
#define FONT_SLANT_ITALIC 1

#define FONT_CHAR_INDEX_ENTRY_SIZE (4 + 1 + 4)

#define EXTRACT_BYTE(n)	((unsigned long long)((uint8_t *)&x)[n])
static inline uint16_t __bswap_16(uint16_t x)
{
	return (EXTRACT_BYTE(0) << 8) | EXTRACT_BYTE(1);
}

static inline uint32_t __bswap_32(uint32_t x)
{
	return (EXTRACT_BYTE(0) << 24) | (EXTRACT_BYTE(1) << 16) |
	    (EXTRACT_BYTE(2) << 8) | EXTRACT_BYTE(3);
}

static inline uint64_t __bswap_64(uint64_t x)
{
	return (EXTRACT_BYTE(0) << 56) | (EXTRACT_BYTE(1) << 48) |
	    (EXTRACT_BYTE(2) << 40) | (EXTRACT_BYTE(3) << 32) | (EXTRACT_BYTE(4)
								 << 24) |
	    (EXTRACT_BYTE(5) << 16) | (EXTRACT_BYTE(6) << 8) | EXTRACT_BYTE(7);
}

#undef EXTRACT_BYTE

/*
 * RAW FILE STRUCTS
 */

struct pf2_raw_section_header {
	char type[4];
	unsigned size;
} __attribute__ ((__packed__));

struct pf2_raw_font_glyph {
	uint16_t width;
	uint16_t height;
	int16_t offset_x;
	int16_t offset_y;
	uint16_t device_width;
	uint8_t data_start;
} __attribute__ ((__packed__));

struct pf2_raw_chix_entry {
	uint32_t code;
	uint8_t storage_flags;
	uint32_t offset;
} __attribute__ ((__packed__));

/*
 * PARSED DATA STRUCTS
 */

struct pf2_font {
	char *raw_data;
	char *name;
	char *family;
	short point_size;
	short weight;
	short slant;
	short max_char_width;
	short max_char_height;
	short ascent;
	short descent;
	short leading;
	uint32_t num_chars;
	struct pf2_chix_entry *chix;
	uint16_t *bmp_idx;
};

struct pf2_chix_entry {
	uint32_t code;
	uint8_t storage_flags;
	uint32_t offset;

	struct pf2_font_glyph *glyph;
};

struct pf2_font_glyph {
	struct pf2_font *font;
	uint16_t width;
	uint16_t height;
	int16_t offset_x;
	int16_t offset_y;
	uint16_t device_width;
	uint8_t data_start;
	uint8_t *bitmap;
};

#endif // MAIN_H
