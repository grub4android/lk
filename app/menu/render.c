#include <app.h>
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <kernel/event.h>
#include <kernel/thread.h>
#include <arch/ops.h>
#include <dev/udc.h>
#include <dev/fbcon.h>
#include <dev/keys.h>
#include <lib/pf2font.h>
#include <app/aboot.h>
#include <platform.h>
#include <kernel/mutex.h>
#include <app/display_server.h>
#include <app/menu.h>

#if WITH_DEV_PMIC_PM8921
#include <dev/pm8921_rtc.h>
#endif

#include "menu_private.h"

#include LKFONT_HEADER

static int block_user = 0;
static unsigned selection = 0;
static struct menu_stack_entry* menu_stack;

static uint8_t color_r = 0xff;
static uint8_t color_g = 0xff;
static uint8_t color_b = 0xff;

static int menu_entry_handler(void* p) {
	void (*fn)(void) = p;

	fn();
	block_user = 0;
	display_server_refresh();

	return 0;
}

static void menu_execute_entry(void (*fn)(void)) {
	thread_t *thr;

	block_user = 1;
	thr = thread_create("grub_sideload", menu_entry_handler, fn, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
	if (!thr)
	{
		return;
	}

	thread_resume(thr);
}

// logbuf pointer and size
static char** logbuf = NULL;
static unsigned logbuf_rows = 0;
static unsigned logbuf_cols = 0;
// logbuf position
unsigned logbuf_row = 0;
unsigned logbuf_col = 0;
unsigned logbuf_posx = 0;
static mutex_t logbuf_mutex;
static bool is_initialized = false;

void menu_putc(char c) {
	struct fbcon_config *config = fbcon_display();
	static int in_putc = 0;

	if(in_putc) return;
	if(!logbuf) return;

	// lock
	if(is_initialized && !in_critical_section())
		mutex_acquire(&logbuf_mutex);
	else enter_critical_section();

	in_putc = 1;

	// automatic line break
	int cwidth = pf2font_get_cwidth(c);
	if(logbuf_posx+cwidth>config->width) {
		logbuf_row++;
		logbuf_col = 0;
		logbuf_posx = 0;
		display_server_refresh();
	}

	// scroll down
	while(logbuf_row>logbuf_rows-1) {
		unsigned i;
		for(i=1; i<logbuf_rows; i++) {
			memcpy(logbuf[i-1], logbuf[i], logbuf_cols);
		}
		logbuf_row--;
	}

	// write char
	logbuf[logbuf_row][logbuf_col++] = c=='\n'?'\0':c;

	// expression start
	static int in_expr = 0;
	if(!in_expr && c=='\e') in_expr = 1;

	if(!in_expr) {
		logbuf_posx+=cwidth;
	}

	// expression end
	else if(in_expr && c=='m') in_expr = 0;

	// line break
	if(logbuf_col==logbuf_cols || c=='\n') {
		logbuf_row++;
		logbuf_col = 0;
		logbuf_posx = 0;
		display_server_refresh();
	}

	// unlock
	in_putc = 0;
	if(is_initialized && !in_critical_section())
		mutex_release(&logbuf_mutex);
	else exit_critical_section();
}

void menu_set_color(uint8_t r, uint8_t g, uint8_t b)
{
	color_r = r;
	color_g = g;
	color_b = b;

	pf2font_set_color(r, g, b);
}

static void menu_draw_divider(int dy, int height) {
	struct fbcon_config *config = fbcon_display();
	uint8_t* base = config->base;
	unsigned x;

	uint8_t *row = &base[config->width * dy * config->bpp / 8];
	for (x = 0; x < config->width*height; x++) {
		uint8_t *pixel = &row[x * config->bpp / 8];
		pixel[0] = color_b;
		pixel[1] = color_g;
		pixel[2] = color_r;
	}
}

static void menu_draw_item(int dy, const char* str, int selected) {
	if(selected) {
		menu_set_color(MENU_TEXT_COLOR);
		menu_draw_divider(dy-pf2font_get_ascent(), pf2font_get_fontheight());
	}

	if(selected)
		menu_set_color(NORMAL_TEXT_COLOR);
	else
		menu_set_color(MENU_TEXT_COLOR);
	pf2font_printf(0, dy, str);
}

void menu_enter(struct menu_entry* menu) {
	struct menu_stack_entry* prev = menu_stack;

	menu_stack = calloc(sizeof(struct menu_stack_entry), 1);
	menu_stack->entries = menu;
	menu_stack->prev = prev;

	selection = 0;
	display_server_refresh();
}

void menu_leave(void) {
	struct menu_stack_entry* prev = menu_stack->prev;

	if(!prev) return;

	free(menu_stack);
	menu_stack = prev;

	selection = 0;
	display_server_refresh();
}

static void menu_renderer(int keycode) {
	int y = 1;
	unsigned i;
	int fh = pf2font_get_fontheight();
	struct fbcon_config *config = fbcon_display();

	// input handling
	if(!block_user) {
		// handle keypress
		if(keycode==KEY_RIGHT && menu_stack->entries[selection].execute) {
			menu_execute_entry(menu_stack->entries[selection].execute);
			return;
		}
		if(keycode==KEY_DOWN) {
			int first_visible = -1;
			int next_visible = -1;

			for(i=0; menu_stack->entries[i].name; i++) {
				if(!menu_stack->entries[i].hide || !menu_stack->entries[i].hide()) {
					if(first_visible==-1) first_visible = i;
					if(next_visible==-1 && i>selection) next_visible = i;
				}
			}

			if(next_visible!=-1) selection = next_visible;
			else selection=first_visible;
		}
		if(keycode==KEY_UP) {
			int last_visible = -1;
			int previous_visible = -1;

			for(i=0; menu_stack->entries[i].name; i++) {
				if(!menu_stack->entries[i].hide || !menu_stack->entries[i].hide()) {
					last_visible = i;
					if(i<selection) previous_visible = i;
				}
			}

			if(previous_visible!=-1) selection = previous_visible;
			else selection=last_visible;
		}
	}

	// clear
	fbcon_clear();

	// title
	menu_set_color(NORMAL_TEXT_COLOR);
	pf2font_printf(0, fh*y++, "Fastboot Flash Mode (%s)", ABOOT_VERSION);

	// USB status
	if(usb_is_connected())
		pf2font_printf(0, fh*y++, "Transfer Mode: USB Connected");
	else
		pf2font_printf(0, fh*y++, "Connect USB Data Cable");

	// device info
	char sn_buf[13];
	target_serialno((unsigned char*)sn_buf);
	pf2font_printf(0, fh*y++, "CPU: %s Serial: %s", TARGET, sn_buf);

#if WITH_DEV_PMIC_PM8921
	// time
	unsigned long time;
	struct rtc_time tm;

	pm8xxx_rtc_read_time(&time);
	rtc_time_to_tm(time, &tm);

	pf2font_printf(0, fh*y++, "Time: %d-%02d-%02d %02d:%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min);
#endif

	// divider 1
	menu_set_color(DIVIDER_COLOR);
	menu_draw_divider(fh*y++ - pf2font_get_ascent()/2, 3);

	// draw interactive UI
	if(!block_user) {
		// menu header
		menu_set_color(NORMAL_TEXT_COLOR);
		pf2font_printf(0, fh*y++, "Boot Mode Selection Menu");
		pf2font_printf(0, fh*y++, "  Power Selects, Vol Up/Down Scrolls");

		// menu entries
		for(i=0; menu_stack->entries[i].name; i++) {
			if(menu_stack->entries[i].hide && menu_stack->entries[i].hide()) continue;

			char* buf = NULL;
			if(menu_stack->entries[i].format)
				menu_stack->entries[i].format(&buf);
			else buf = strdup(menu_stack->entries[i].name);
			menu_draw_item(fh*y++, buf, selection==i);

			if(buf)
				free(buf);
		}

		// divider 2
		menu_set_color(DIVIDER_COLOR);
		menu_draw_divider(fh*y++ - pf2font_get_ascent()/2, 3);
	}

	// draw log
	menu_set_color(LOG_COLOR_NORMAL);
	mutex_acquire(&logbuf_mutex);
	int log_top = y;
	int log_bottom = config->height/fh;
	int log_size = log_bottom-log_top;
	int start = (logbuf_row-log_size);
	for(i=(start>=0?start:0); i<=logbuf_row; i++) {
		pf2font_putns(0, fh*y++, logbuf[i], i==logbuf_row?(int32_t)logbuf_col:-1);
	}
	mutex_release(&logbuf_mutex);

	// flush
	fbcon_flush();
};

static void menu_init(const struct app_descriptor *app)
{
	dprintf(INFO, "Init menu.\n");
	pf2font_init((char*)lkfont_pf2, lkfont_pf2_len);
	mutex_init(&logbuf_mutex);

	struct fbcon_config* config = fbcon_display();
	logbuf_cols = config->width / pf2font_get_cwidth('!');
	logbuf_rows = config->height / pf2font_get_fontheight();

	unsigned i;
	logbuf = malloc(sizeof(char*)*logbuf_rows);
	for(i=0; i<logbuf_rows; i++)
		logbuf[i] = malloc(sizeof(char)*logbuf_cols);

	is_initialized = true;

	menu_stack = calloc(sizeof(struct menu_stack_entry), 1);
	menu_stack->entries = entries_main;
	menu_stack->prev = NULL;
	display_server_set_renderer(menu_renderer);
}

APP_START(menu)
	.init = menu_init,
APP_END
