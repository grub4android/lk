#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <dev/fbcon.h>
#include <lib/pf2font.h>

#if WITH_APP_MENU
#include <app/menu.h>
#endif

#include "pf2_private.h"
#include "fontformat.h"

#define ERROR(x...) dprintf(CRITICAL, x)
#define LOG_DEBUG(x...) dprintf(SPEW, x)
#define CHECK_BIT(var,pos) (!!((var << pos) & 0x80))

static struct pf2_font *font = NULL;

static uint8_t color_r = 0xff;
static uint8_t color_g = 0xff;
static uint8_t color_b = 0xff;

/*
 * Source: http://www.opensource.apple.com/source/gcc/gcc-5666.3/libiberty/strndup.c
 */
static char *strndup(const char *s, size_t n)
{
	char *result;
	size_t len = strlen(s);

	if (n < len)
		len = n;

	result = (char *)malloc(len + 1);
	if (!result)
		return 0;

	result[len] = '\0';
	return (char *)memcpy(result, s, len);
}

/* Return a pointer to the character index entry for the glyph corresponding to
   the codepoint CODE in the font FONT. If not found, return zero.  */
static struct pf2_chix_entry *find_glyph(const struct pf2_font *font,
					 uint32_t code)
{
	struct pf2_chix_entry *table;
	size_t lo;
	size_t hi;
	size_t mid;

	table = font->chix;

	/* Use BMP index if possible.  */
	if (code < 0x10000 && font->bmp_idx) {
		if (font->bmp_idx[code] == 0xffff)
			return NULL;
		return &table[font->bmp_idx[code]];
	}

	/* Do a binary search in `char_index', which is ordered by code point.  */
	lo = 0;
	hi = font->num_chars - 1;

	if (!table)
		return NULL;

	while (lo <= hi) {
		mid = lo + (hi - lo) / 2;
		if (code < table[mid].code)
			hi = mid - 1;
		else if (code > table[mid].code)
			lo = mid + 1;
		else
			return &table[mid];
	}

	return NULL;
}

static struct pf2_font_glyph *font_get_glyph(struct pf2_font *font,
					     uint32_t code)
{
	struct pf2_chix_entry *index_entry;

	// get glyph
	index_entry = find_glyph(font, code);
	if (!index_entry)
		return NULL;

	// Return cached glyph.
	if (index_entry->glyph)
		return index_entry->glyph;

	// No font data, can't load any glyphs.
	if (!font->raw_data)
		return NULL;

	// read glyph data
	struct pf2_raw_font_glyph *glyph_src =
	    (void *)&font->raw_data[index_entry->offset];
	uint16_t width = __bswap_16(glyph_src->width);
	uint16_t height = __bswap_16(glyph_src->height);
	int16_t xoff = __bswap_16(glyph_src->offset_x);
	int16_t yoff = __bswap_16(glyph_src->offset_y);
	int16_t dwidth = __bswap_16(glyph_src->device_width);
	int len = (width * height + 7) / 8;

	// allocate glyph
	struct pf2_font_glyph *glyph =
	    malloc(sizeof(struct pf2_font_glyph) + len);
	if (!glyph) {
		ERROR("Error allocating glyph!\n");
		return NULL;
	}
	// set glyph data
	glyph->font = font;
	glyph->width = width;
	glyph->height = height;
	glyph->offset_x = xoff;
	glyph->offset_y = yoff;
	glyph->device_width = dwidth;
	glyph->bitmap = (void *)&glyph_src->data_start;

	index_entry->glyph = glyph;

	return glyph;
}

static int
load_font_index(struct pf2_font *font, char *section, unsigned section_len)
{
	unsigned i;

	if ((section_len % FONT_CHAR_INDEX_ENTRY_SIZE) != 0) {
		ERROR("Invalid chix section len %d!\n", section_len);
		return -1;
	}
	// allocate
	font->num_chars = section_len / FONT_CHAR_INDEX_ENTRY_SIZE;
	font->chix = malloc(font->num_chars * sizeof(struct pf2_chix_entry));
	if (!font->chix)
		return -1;
	font->bmp_idx = malloc(0x10000 * sizeof(uint16_t));
	if (!font->bmp_idx)
		return -1;
	memset(font->bmp_idx, 0xff, 0x10000 * sizeof(uint16_t));

	uint32_t last_code = 0;
	for (i = 0; i < font->num_chars; i++) {
		struct pf2_chix_entry *entry = &font->chix[i];
		struct pf2_raw_chix_entry *entry_src =
		    (void *)(section + i * FONT_CHAR_INDEX_ENTRY_SIZE);

		entry->code = __bswap_32(entry_src->code);
		entry->offset = __bswap_32(entry_src->offset);
		entry->storage_flags = entry_src->storage_flags;

		if (entry->storage_flags != 0) {
			ERROR("Only uncompressed characters are supported!\n");
			return -1;
		}

		if (i != 0 && entry->code <= last_code) {
			ERROR
			    ("font characters not in ascending order: %u <= %u",
			     entry->code, last_code);
			return -1;
		}

		if (entry->code < 0x10000)
			font->bmp_idx[entry->code] = i;

		last_code = entry->code;

		entry->glyph = 0;
	}

	return 0;
}

static int pf2_blit_glyph(struct pf2_font_glyph *src, uint32_t dx, uint32_t dy, int force_print, int print)
{
	unsigned x, y;
	struct fbcon_config *config = fbcon_display();
	uint8_t *data = config->base;

	if(!force_print && dx+src->width>config->width) return 0;

	if(!print) return 1;

	for (y = 0; y < src->height; y++) {
		if(y + dy>=config->height) break;

		uint8_t *row =
		    &data[config->width * (y + dy) * config->bpp / 8];

		for (x = 0; x < src->width; x++) {
			if(x + dx>=config->width) break;

			int bitIndex = y * src->width + x;
			int byteIndex = bitIndex / 8;
			int bitPos = bitIndex % 8;

			if (CHECK_BIT(src->bitmap[byteIndex], bitPos)) {
				uint8_t *pixel =
				    &row[(x + dx) * config->bpp / 8];
				pixel[0] = color_b;
				pixel[1] = color_g;
				pixel[2] = color_r;
			}
		}
	}

	return 1;
}

int pf2font_init(char *font_data, unsigned long font_len)
{
	unsigned long i;

	font = malloc(sizeof(struct pf2_font));
	if (!font)
		return -1;
	memset(font, 0, sizeof(*font));

	font->leading = 1;
	font->raw_data = (void *)font_data;

	for (i = 0; i < font_len; i++) {
		struct pf2_raw_section_header *hdr = (void *)&font_data[i];
		unsigned sz = __bswap_32(hdr->size);
		char *data = (void *)&font_data[i + sizeof(*hdr)];
		int is_first = !i;
		i += sizeof(*hdr) + sz - 1;

		if (is_first) {
			if (PF2_SECTION_IS_FILE(hdr->type)
			    && memcmp(data, FONT_FORMAT_PFF2_MAGIC, 4) == 0) {
				continue;
			} else {
				ERROR("Invalid magic!\n");
				return -1;
			}
		}

		if (PF2_SECTION_IS_FONT_NAME(hdr->type)) {
			font->name = strndup(data, sz);
		} else if (PF2_SECTION_IS_FAMILY(hdr->type)) {
			font->family = strndup(data, sz);
		} else if (PF2_SECTION_IS_WEIGHT(hdr->type)) {
			if (!strcmp(data, "normal"))
				font->weight = FONT_WEIGHT_NORMAL;
			else if (!strcmp(data, "bold"))
				font->weight = FONT_WEIGHT_BOLD;
			else {
				ERROR("invalid font weight: %s\n", data);
				return -1;
			}
		} else if (PF2_SECTION_IS_SLAN(hdr->type)) {
			if (!strcmp(data, "normal"))
				font->slant = FONT_SLANT_NORMAL;
			else if (!strcmp(data, "italic"))
				font->slant = FONT_SLANT_ITALIC;
			else {
				ERROR("invalid font slan: %s\n", data);
				return -1;
			}
		} else if (PF2_SECTION_IS_POINT_SIZE(hdr->type)) {
			font->point_size = __bswap_16(*((short *)data));
		}

		else if (PF2_SECTION_IS_MAX_CHAR_WIDTH(hdr->type)) {
			font->max_char_width = __bswap_16(*((short *)data));
		} else if (PF2_SECTION_IS_MAX_CHAR_HEIGHT(hdr->type)) {
			font->max_char_height = __bswap_16(*((short *)data));
		}

		else if (PF2_SECTION_IS_ASCENT(hdr->type)) {
			font->ascent = __bswap_16(*((short *)data));
		} else if (PF2_SECTION_IS_DESCENT(hdr->type)) {
			font->descent = __bswap_16(*((short *)data));
		} else if (PF2_SECTION_IS_CHAR_INDEX(hdr->type)) {
			if (load_font_index(font, data, sz)) {
				ERROR("Couldn't load font index!\n");
				return -1;
			}
		}
		// stop here
		else if (PF2_SECTION_IS_DATA(hdr->type)) {
			if (sz != ~0u) {
				ERROR("Invalid size in data section!\n");
				return -1;
			}
			break;
		} else {
			ERROR("Invalid section: %4s\n", hdr->type);
			return -1;
		}

	}

	LOG_DEBUG
	    ("font:\n\tname=%s\n\tfamily=%s\n\tweight=%d\n\tslant=%d\n\tpt=%d\n\tmaxw=%d\n\tmaxh=%d\n\tascent=%d\n\tdescent=%d\n\tnum_chars=%d\n",
	     font->name, font->family, font->weight, font->slant,
	     font->point_size, font->max_char_width, font->max_char_height,
	     font->ascent, font->descent, font->num_chars);

	return 0;
}

void pf2font_set_color(uint8_t r, uint8_t g, uint8_t b)
{
	color_r = r;
	color_g = g;
	color_b = b;
}

static int pf2font_puts_internal(uint32_t x, uint32_t y, const char *str, int print)
{
	uint32_t dx = x;
	int written = 0, rc=0;

	if (!font)
		return -1;

	while (*str != 0) {
		char c = *str++;

		// ESC
		if(c=='\e') {
			c = *str++;

			// COLOR
			if(c=='[') {
				char id[10];
				int len = 0;

				// read number
				while (c != 0) {
					c = *str++;
					if(c=='m') break;

					id[len++] = c;
				}
				id[len] = '\0';
				int iid = atoi(id);

				if(print) {
#if WITH_APP_MENU
					// set color
					if(iid==31) menu_set_color(LOG_COLOR_RED);
					if(iid==33) menu_set_color(LOG_COLOR_BLUE);
					else if(iid==0) menu_set_color(LOG_COLOR_NORMAL);
#endif
				}

				continue;
			}
		}

		// TAB
		int loop = 1, i;
		if(c=='\t') {
			c = ' ';
			loop=4;
		}

		for(i=0; i<loop; i++) {
			struct pf2_font_glyph *glyph = font_get_glyph(font, c);
			if (!glyph) {
				dx += font->max_char_width + font->leading;
				continue;
			}

			if (!glyph->width) {
				dx += glyph->device_width + font->leading;
				continue;
			}

			dx += glyph->offset_x + font->leading;

			rc=pf2_blit_glyph(glyph, dx,
					   y - (glyph->height + glyph->offset_y), 0, print);
			if(!rc) return written;
			written+=rc;

			dx += glyph->width;
		}
	}

	return dx-x;
}

int pf2font_puts(uint32_t x, uint32_t y, const char *str) {
	return pf2font_puts_internal(x, y, str, 1);
}

int pf2font_printf(uint32_t x, uint32_t y, const char *fmt, ...)
{
	char buf[256];
	int err;

	va_list ap;
	va_start(ap, fmt);
	err = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if(!err) return -1;

	return pf2font_puts(x, y, buf);
}

int pf2font_get_fontheight(void) {
	if (!font)
		return 0;

	return font->max_char_height;
}

int pf2font_get_strwidth(const char* fmt, ...) {
	char buf[256];
	int err;

	if (!font)
		return -1;

	va_list ap;
	va_start(ap, fmt);
	err = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if(!err) return -1;

	return pf2font_puts_internal(0, 0, buf, 0);
}

int pf2font_get_cwidth(char c)
{
	int ret = 0;

	if (!font)
		return 0;

	struct pf2_font_glyph *glyph = font_get_glyph(font, c);
	if (!glyph) {
		return font->max_char_width + font->leading;
	}

	if (!glyph->width) {
		return glyph->device_width + font->leading;;
	}

	ret += glyph->offset_x + font->leading;
	ret += glyph->width;

	// TAB
	if(c=='\t') ret*=4;

	return ret;
}

int pf2font_get_ascent(void) {
	if (!font)
		return 0;

	return font->ascent;
}
