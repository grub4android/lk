#include <app.h>
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <kernel/event.h>
#include <kernel/thread.h>
#include <dev/udc.h>
#include <dev/fbcon.h>
#include <app/display_server.h>
#include <kernel/mutex.h>
#include <lk/init.h>
#include <dev/keys.h>

static event_t e_frame_finished;
static event_t e_start_server;
static event_t e_continue;
static bool is_running = 0;
static bool request_stop = 0;
static bool request_refresh = 0;
static renderer_t renderer = NULL;

static int keymap[MAX_KEYS];
#define CHECK_AND_REPORT_KEY(code, value) \
	if(value) { \
		if(!keymap[code]){ \
			keymap[code] = 1; \
			return code; \
		} \
	} else{keymap[code]=0;}

static int getkey(void)
{
	// small delay to prevent unwanted keypresses
	spin(1000);

	CHECK_AND_REPORT_KEY(KEY_UP, target_volume_up());
	CHECK_AND_REPORT_KEY(KEY_DOWN, target_volume_down());
	CHECK_AND_REPORT_KEY(KEY_RIGHT, target_power_key());

	return 0;
}

void display_server_start(void) {
	if(is_running) {
		dprintf(INFO, "display server already running!\n");
		return;
	}

	// start server and wait for it
	dprintf(INFO, "starting display server...\n");
	event_signal(&e_start_server, true);
	while(!is_running) {
		thread_yield();
	}
	dprintf(INFO, "Done.\n");

	display_server_unpause();
}

void display_server_stop(void) {
	if(!is_running) {
		dprintf(INFO, "display server is not running!\n");
		return;
	}

	// we can't stop the server if the loop is blocked
	display_server_unpause();

	// stop server and wait for it
	dprintf(INFO, "stopping display server...\n");
	request_stop = 1;
	while(is_running) {
		thread_yield();
	}
	dprintf(INFO, "Done.\n");

	// WORKAROUND: render frame from this thread
	if(renderer) renderer(0);
}

void display_server_set_renderer(renderer_t r) {
	renderer = r;
	display_server_refresh();
}

void display_server_refresh(void) {
	request_refresh = 1;
}

void display_server_pause(void) {
	event_unsignal(&e_continue);
	display_server_refresh();
}

void display_server_unpause(void) {
	event_signal(&e_continue, true);
}

static int display_server_thread(void *args)
{
	for (;;) {
		// wait for start event
		dprintf(INFO, "%s: IDLE\n", __func__);
		if (event_wait(&e_start_server) < 0) {
			dprintf(INFO, "%p: event_wait() returned error\n", get_current_thread());
			return -1;
		}

		// main worker loop
		dprintf(INFO, "%s: START\n", __func__);
		is_running = 1;

		// ignore first key to prevent unwanted interactions
		getkey();

		int keycode = 0;
		for(;;) {
			// render frame
			if(renderer) renderer(keycode);

			// signal refresh
			event_signal(&e_frame_finished, true);

			// poll key
			while(!(keycode=getkey()) && !request_stop && !request_refresh) {
				thread_yield();
			}

			// stop request
			if(request_stop) {
				request_stop = 0;
				break;
			}

			// refresh request
			if(request_refresh) {
				request_refresh = 0;
			}

			event_wait(&e_continue);
		}

		dprintf(INFO, "%s: EXIT\n", __func__);
		is_running = 0;
	}

	return 0;
}

static void server_init(uint x)
{
	dprintf(INFO, "Init display server.\n");

	event_init(&e_frame_finished, false, EVENT_FLAG_AUTOUNSIGNAL);
	event_init(&e_start_server, false, EVENT_FLAG_AUTOUNSIGNAL);
	event_init(&e_continue, false, 0);

	thread_resume(thread_create("display_server", &display_server_thread, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
}

LK_INIT_HOOK(app_display_server, &server_init, LK_INIT_LEVEL_THREADING);
