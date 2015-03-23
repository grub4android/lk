#include <debug.h>
#include <stdlib.h>
#include <assert.h>
#include <uefi/api.h>
#include <uefi/api_simpletext.h>

#define MODE0_COLUMN_COUNT        144
#define MODE0_ROW_COUNT           39

#define BACKSPACE                 8
#define ESC                       27
#define CSI                       0x9B
#define DEL                       127
#define BRIGHT_CONTROL_OFFSET     2
#define FOREGROUND_CONTROL_OFFSET 6
#define BACKGROUND_CONTROL_OFFSET 11
#define ROW_OFFSET                2
#define COLUMN_OFFSET             5

efi_char16_t mSetModeString[]            = { ESC, '[', '=', '3', 'h', 0 };
efi_char16_t mSetAttributeString[]       = { ESC, '[', '0', 'm', ESC, '[', '4', '0', 'm', ESC, '[', '4', '0', 'm', 0 };
efi_char16_t mClearScreenString[]        = { ESC, '[', '2', 'J', 0 };
efi_char16_t mSetCursorPositionString[]  = { ESC, '[', '0', '0', ';', '0', '0', 'H', 0 };
static bool output_escape_char = false;

static efi_simple_text_output_mode_t efi_simpletext_out_mode = {
  .max_mode = 1,
  .mode = 0,
  .attribute = EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK),
  .cursor_column = 0,
  .cursor_row = 0,
  .cursor_visible = true
};

static bool TextOutIsValidAscii(efi_char16_t c) {
	//
	// valid ASCII code lies in the extent of 0x20 - 0x7F
	//
	if ((c >= 0x20) && (c <= 0x7F))
		return true;

	return false;
}

static bool TextOutIsValidEfiCntlChar(efi_char16_t c) {
	//
	// only support four control characters.
	//
	if (c == CHAR_NULL ||
		c == CHAR_BACKSPACE ||
		c == CHAR_LINEFEED ||
		c == CHAR_CARRIAGE_RETURN ||
		c == CHAR_TAB ) {
		return true;
	}

	return false;
}

efi_uintn_t strlen16(efi_char16_t const * s) {
	size_t i;

	i=0;
	while(s[i]) {
		i+= 1;
	}

	return i;
}

efi_uintn_t strsize16(efi_char16_t const * s) {
	size_t i;

	i=0;
	while(s[i]) {
		i+=sizeof(*s);
	}

	return i;
}

efi_uintn_t strsize(efi_char8_t const * s) {
	size_t i;

	i=0;
	while(s[i]) {
		i+=sizeof(*s);
	}

	return i;
}

efi_char8_t* SafeUnicodeStrToAsciiStr(const efi_char16_t* source, efi_char8_t* destination) {
	efi_char8_t* ret;

	ASSERT(destination != NULL);

	//
	// ASSERT if Source is long than PcdMaximumUnicodeStringLength.
	// Length tests are performed inside StrLen().
	//
	ASSERT(strsize16(source)!=0);

	//
	// Source and Destination should not overlap
	//
	ASSERT((efi_uintn_t)((efi_char16_t *)destination-source) > strlen16(source));
	ASSERT((efi_uintn_t)((efi_char8_t *)source-destination) > strlen16(source));


	ret = destination;
	while(*source != '\0') {
		//
		// If any non-ascii characters in Source then replace it with '?'.
		//
		if(*source < 0x80)
			*destination = (efi_char8_t)*source;
		else {
			*destination = '?';

			// Surrogate pair check.
			if((*source >= 0xD800) && (*source <= 0xDFFF))
				source++;
		}

		destination++;
		source++;
	}

	*destination = '\0';

	//
	// ASSERT Original Destination is less long than PcdMaximumAsciiStringLength.
	// Length tests are performed inside AsciiStrLen().
	//
	ASSERT(strsize(ret)!=0);

	return ret;
}

/*
 * SimpleText OUTPUT
 */

static efi_status_t efi_simpletext_reset(struct efi_simple_text_output_interface* this, efi_boolean_t extended_verification) {
	this->set_attributes(this, EFI_TEXT_ATTR(this->mode->attribute & 0x0F, EFI_BACKGROUND_BLACK));

	return this->set_mode(this, 0);
}

static efi_status_t debug_serial_write(efi_uint8_t* str, efi_uintn_t size) {
	efi_uintn_t i;
	for(i=0; i<size; i++)
		platform_dputc(str[i]);

	return EFI_SUCCESS;
}

static efi_status_t efi_simpletext_output_string(struct efi_simple_text_output_interface* this, efi_char16_t* string) {
	efi_uintn_t size;
	efi_char8_t* output_string;
	efi_status_t status;
	efi_simple_text_output_mode_t* mode;
	efi_uintn_t max_column;
	efi_uintn_t max_row;

	size = strlen16(string) + 1;
	output_string = malloc(size);

	//If there is any non-ascii characters in String buffer then replace it with '?'
	//Eventually, UnicodeStrToAsciiStr API should be fixed.
	SafeUnicodeStrToAsciiStr(string, output_string);
	debug_serial_write((efi_uint8_t*)output_string, size - 1);

	//
	// Parse each character of the string to output
	// to update the cursor position information
	//
	mode = this->mode;

	status = this->query_mode(this, mode->mode, &max_column, &max_row);
	if(status!=EFI_SUCCESS)
		return status;

	for (; *string!=CHAR_NULL; string++) {

		switch (*string) {
			case CHAR_BACKSPACE:
				if (mode->cursor_column > 0)
					mode->cursor_column--;
				break;

			case CHAR_LINEFEED:
				if (mode->cursor_row < (efi_int32_t) (max_row - 1))
					mode->cursor_row++;
				break;

			case CHAR_CARRIAGE_RETURN:
				mode->cursor_column = 0;
				break;

			default:
				if(mode->cursor_column < (efi_int32_t) (max_column - 1)) {
					mode->cursor_column++;
				} else {
					mode->cursor_column = 0;
					if (mode->cursor_row < (efi_int32_t) (max_row - 1)) {
						mode->cursor_row++;
					}
				}
				break;
		}
	}

	free(output_string);

	return EFI_SUCCESS;
}

static efi_status_t efi_simpletext_test_string(struct efi_simple_text_output_interface* this, efi_char16_t* string) {
	efi_char8_t c;

	for (; *string!=CHAR_NULL; string++) {
		c = (efi_char8_t)*string;
		if (!(TextOutIsValidAscii(c) || TextOutIsValidEfiCntlChar(c)))
			return EFI_UNSUPPORTED;
	}

	return EFI_SUCCESS;
}

static efi_status_t efi_simpletext_query_mode(struct efi_simple_text_output_interface* this, efi_uintn_t mode_number, efi_uintn_t* columns, efi_uintn_t* rows) {
	if (this->mode->max_mode > 1)
		return EFI_DEVICE_ERROR;

	if (mode_number == 0) {
		*columns  = MODE0_COLUMN_COUNT;
		*rows     = MODE0_ROW_COUNT;
		return EFI_SUCCESS;
	}

	return EFI_UNSUPPORTED;
}


static efi_status_t efi_simpletext_set_mode(struct efi_simple_text_output_interface* this, efi_uintn_t mode_number) {
	efi_status_t status;

	// check mode number
	if(mode_number >= (efi_uintn_t) this->mode->max_mode)
		return EFI_UNSUPPORTED;

	//
	// Set the current mode
	//
	this->mode->mode = (efi_int32_t)mode_number;

	// clear screen
	this->clear_screen(this);

	// print mode
	output_escape_char = true;
	status = this->output_string(this, mSetModeString);
	output_escape_char = false;

	if(status!=EFI_SUCCESS)
		return EFI_DEVICE_ERROR;

	// clear screen
	status = this->clear_screen(this);
	if(status!=EFI_SUCCESS)
		return EFI_DEVICE_ERROR;

	return EFI_SUCCESS;
}

static efi_status_t efi_simpletext_set_attributes(struct efi_simple_text_output_interface* this, efi_uintn_t attribute) {
	efi_uint8_t foreground_control;
	efi_uint8_t background_control;
	efi_uint8_t bright_control;
	efi_uint32_t saved_column = 0;
	efi_uint32_t saved_row = 0;
	efi_status_t status;

	//
	//  only the bit0..6 of the Attribute is valid
	//
	if((attribute | 0x7f) != 0x7f)
		return EFI_UNSUPPORTED;

	//
	// Skip outputting the command string for the same attribute
	// It improves the terminal performance significantly
	//
	if(this->mode->attribute == (efi_int32_t)attribute)
		return EFI_SUCCESS;

	//
	//  convert Attribute value to terminal emulator
	//  understandable foreground color
	//
	switch(attribute & 0x07) {
		case EFI_BLACK:
			foreground_control = 30;
			break;

		case EFI_BLUE:
			foreground_control = 34;
			break;

		case EFI_GREEN:
			foreground_control = 32;
			break;

		case EFI_CYAN:
			foreground_control = 36;
			break;

		case EFI_RED:
			foreground_control = 31;
			break;

		case EFI_MAGENTA:
			foreground_control = 35;
			break;

		case EFI_BROWN:
			foreground_control = 33;
			break;

		default:
		case EFI_LIGHTGRAY:
			foreground_control = 37;
			break;
	}

	//
	//  bit4 of the Attribute indicates bright control
	//  of terminal emulator.
	//
	bright_control = (efi_uint8_t)((attribute >> 3) & 1);

	//
	//  convert Attribute value to terminal emulator
	//  understandable background color.
	//
	switch ((attribute >> 4) & 0x07) {
		case EFI_BLACK:
			background_control = 40;
			break;

		case EFI_BLUE:
			background_control = 44;
			break;

		case EFI_GREEN:
			background_control = 42;
			break;

		case EFI_CYAN:
			background_control = 46;
			break;

		case EFI_RED:
			background_control = 41;
			break;

		case EFI_MAGENTA:
			background_control = 45;
			break;

		case EFI_BROWN:
			background_control = 43;
			break;

		default:
		case EFI_LIGHTGRAY:
			background_control = 47;
			break;
	}

	//
	// terminal emulator's control sequence to set attributes
	//
	mSetAttributeString[BRIGHT_CONTROL_OFFSET]          = (efi_char16_t) ('0' + bright_control);
	mSetAttributeString[FOREGROUND_CONTROL_OFFSET + 0]  = (efi_char16_t) ('0' + (foreground_control / 10));
	mSetAttributeString[FOREGROUND_CONTROL_OFFSET + 1]  = (efi_char16_t) ('0' + (foreground_control % 10));
	mSetAttributeString[BACKGROUND_CONTROL_OFFSET + 0]  = (efi_char16_t) ('0' + (background_control / 10));
	mSetAttributeString[BACKGROUND_CONTROL_OFFSET + 1]  = (efi_char16_t) ('0' + (background_control % 10));

	//
	// save current column and row
	// for future scrolling back use.
	//
	saved_column = this->mode->cursor_column;
	saved_row = this->mode->cursor_row;

	output_escape_char = true;
	status = this->output_string (this, mSetAttributeString);
	output_escape_char = false;

	if(status!=EFI_SUCCESS)
		return EFI_DEVICE_ERROR;
	//
	//  scroll back to saved cursor position.
	//
	this->mode->cursor_column = saved_column;
	this->mode->cursor_row = saved_row;

	this->mode->attribute = (efi_int32_t)attribute;
	return EFI_SUCCESS;
}

static efi_status_t efi_simpletext_clear_screen(struct efi_simple_text_output_interface* this) {
	efi_status_t status;

	output_escape_char = true;
	status = this->output_string (this, mClearScreenString);
	output_escape_char = false;

	if(status!=EFI_SUCCESS)
		return EFI_DEVICE_ERROR;

	return this->set_cursor_position(this, 0, 0);
}

static efi_status_t efi_simpletext_set_cursor_position(struct efi_simple_text_output_interface* this, efi_uintn_t column, efi_uintn_t row) {
	efi_simple_text_output_mode_t *mode;
	efi_status_t status;
	efi_uintn_t max_column;
	efi_uintn_t max_row;

	mode = this->mode;

	status = this->query_mode(this, mode->mode, &max_column, &max_row);
	if (status!=EFI_SUCCESS)
		return EFI_UNSUPPORTED;

	if ((column >= max_column) || (row >= max_row))
		return EFI_UNSUPPORTED;

	//
	// control sequence to move the cursor
	//
	mSetCursorPositionString[ROW_OFFSET + 0]    = (efi_char16_t) ('0' + ((row + 1) / 10));
	mSetCursorPositionString[ROW_OFFSET + 1]    = (efi_char16_t) ('0' + ((row + 1) % 10));
	mSetCursorPositionString[COLUMN_OFFSET + 0] = (efi_char16_t) ('0' + ((column + 1) / 10));
	mSetCursorPositionString[COLUMN_OFFSET + 1] = (efi_char16_t) ('0' + ((column + 1) % 10));

	output_escape_char = true;
	status = this->output_string (this, mSetCursorPositionString);
	output_escape_char = false;

	if(status!=EFI_SUCCESS)
		return EFI_DEVICE_ERROR;

	mode->cursor_column = (efi_int32_t)column;
	mode->cursor_row = (efi_int32_t)row;

	return EFI_SUCCESS;
}

static efi_status_t efi_simpletext_enable_cursor(struct efi_simple_text_output_interface* this, efi_boolean_t visible) {
	if (!visible)
		return EFI_UNSUPPORTED;

	return EFI_SUCCESS;
}

struct efi_simple_text_output_interface efi_simpletext_con_out = {
	.reset = efi_simpletext_reset,
	.output_string = efi_simpletext_output_string,
	.test_string = efi_simpletext_test_string,
	.query_mode = efi_simpletext_query_mode,
	.set_mode = efi_simpletext_set_mode,
	.set_attributes = efi_simpletext_set_attributes,
	.clear_screen = efi_simpletext_clear_screen,
	.set_cursor_position = efi_simpletext_set_cursor_position,
	.enable_cursor = efi_simpletext_enable_cursor,
	.mode = &efi_simpletext_out_mode,
};

/*
 * SimpleText INPUT
 */

efi_status_t efi_simpletext_in_reset(struct efi_simple_input_interface* this, efi_boolean_t extended_verification) {
	return EFI_SUCCESS;
}

efi_status_t efi_simpletext_in_read_key_stroke(struct efi_simple_input_interface* this, efi_input_key_t * key) {
	efi_char8_t c;

	if(!platform_dtstc())
		return EFI_NOT_READY;

	platform_dgetc((char*)&c, 1);

	//
	// Check for ESC sequence. This code is not techincally correct VT100 code.
	// An illegal ESC sequence represents an ESC and the characters that follow.
	// This code will eat one or two chars after an escape. This is done to
	// prevent some complex FIFOing of the data. It is good enough to get
	// the arrow and delete keys working
	//
	key->unicode_char = 0;
	key->scan_code = SCAN_NULL;
	if (c == 0x1b) {
		platform_dgetc((char*)&c, 1);
		if (c == '[') {
			platform_dgetc((char*)&c, 1);
			switch (c) {
			case 'A':
				key->scan_code = SCAN_UP;
				break;
			case 'B':
				key->scan_code = SCAN_DOWN;
				break;
			case 'C':
				key->scan_code = SCAN_RIGHT;
				break;
			case 'D':
				key->scan_code = SCAN_LEFT;
				break;
			case 'H':
				key->scan_code = SCAN_HOME;
				break;
			case 'K':
			case 'F':	// PC ANSI
				key->scan_code = SCAN_END;
				break;
			case '@':
			case 'L':
				key->scan_code = SCAN_INSERT;
				break;
			case 'P':
			case 'X':	// PC ANSI
				key->scan_code = SCAN_DELETE;
				break;
			case 'U':
			case '/':
			case 'G':	// PC ANSI
				key->scan_code = SCAN_PAGE_DOWN;
				break;
			case 'V':
			case '?':
			case 'I':	// PC ANSI
				key->scan_code = SCAN_PAGE_UP;
				break;

				// PCANSI that does not conflict with VT100
			case 'M':
				key->scan_code = SCAN_F1;
				break;
			case 'N':
				key->scan_code = SCAN_F2;
				break;
			case 'O':
				key->scan_code = SCAN_F3;
				break;
			case 'Q':
				key->scan_code = SCAN_F5;
				break;
			case 'R':
				key->scan_code = SCAN_F6;
				break;
			case 'S':
				key->scan_code = SCAN_F7;
				break;
			case 'T':
				key->scan_code = SCAN_F8;
				break;

			default:
				key->unicode_char = c;
				break;
			}
		} else if (c == '0') {
			platform_dgetc((char*)&c, 1);
			switch (c) {
			case 'P':
				key->scan_code = SCAN_F1;
				break;
			case 'Q':
				key->scan_code = SCAN_F2;
				break;
			case 'w':
				key->scan_code = SCAN_F3;
				break;
			case 'x':
				key->scan_code = SCAN_F4;
				break;
			case 't':
				key->scan_code = SCAN_F5;
				break;
			case 'u':
				key->scan_code = SCAN_F6;
				break;
			case 'q':
				key->scan_code = SCAN_F7;
				break;
			case 'r':
				key->scan_code = SCAN_F8;
				break;
			case 'p':
				key->scan_code = SCAN_F9;
				break;
			case 'm':
				key->scan_code = SCAN_F10;
				break;
			default:
				break;
			}
		}
	} else if (c < ' ') {
		if ((c == CHAR_BACKSPACE) ||
			(c == CHAR_TAB) ||
			(c == CHAR_LINEFEED) || (c == CHAR_CARRIAGE_RETURN)) {
			// Only let through EFI required control characters
			key->unicode_char = (efi_char16_t) c;
		}
	} else if (c == 0x7f) {
		key->scan_code = SCAN_DELETE;
	} else {
		key->unicode_char = (efi_char16_t) c;
	}

	return EFI_SUCCESS;
}

struct efi_simple_input_interface efi_simpletext_con_in = {
	.reset = efi_simpletext_in_reset,
	.read_key_stroke = efi_simpletext_in_read_key_stroke,
	.wait_for_key = 0,
};
