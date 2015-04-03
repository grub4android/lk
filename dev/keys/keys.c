/*
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#include <err.h>
#include <bits.h>
#include <list.h>
#include <debug.h>
#include <trace.h>
#include <string.h>
#include <printf.h>
#include <malloc.h>
#include <lk/init.h>
#include <dev/keys.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <lib/console.h>

#define LOCAL_TRACE 0

static unsigned long key_bitmap[BITMAP_NUM_WORDS(MAX_KEYS)];

typedef struct {
	struct list_node node;
	uint16_t code;
	uint16_t value;
} key_event_t;

static struct list_node event_queue;
static event_t key_posted;
static mutex_t queue_mutex, bitmap_mutex;

static struct list_node event_sources;
static mutex_t source_mutex;

static int key_poll_thread(void *arg)
{
	for(;;) {
		mutex_acquire(&source_mutex);
		key_event_source_t *source;
		list_for_every_entry(&event_sources, source, key_event_source_t, node) {
			// update time
			lk_time_t current = current_time();
			source->delta = current-source->last;
			source->last = current;

			source->poll(source);
		}
		mutex_release(&source_mutex);

		thread_yield();
	}
	return 0;
}

static void keys_init_threading(uint level)
{
	thread_detach_and_resume(thread_create("key poll thread", &key_poll_thread, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
}

static void keys_init_heap(uint level)
{
	memset(key_bitmap, 0, sizeof(key_bitmap));
	mutex_init(&bitmap_mutex);

	list_initialize(&event_queue);
	event_init(&key_posted, 0, EVENT_FLAG_AUTOUNSIGNAL);
	mutex_init(&queue_mutex);

	list_initialize(&event_sources);
	mutex_init(&source_mutex);
}

void keys_add_source(key_event_source_t* source) {
	source->last = INFINITE_TIME;

	mutex_acquire(&source_mutex);
	list_add_tail(&event_sources, &source->node);
	mutex_release(&source_mutex);
}

int keys_post_event(uint16_t code, int16_t value)
{
	if (code >= MAX_KEYS) {
		LTRACEF("Invalid keycode posted: %d\n", code);
		return ERR_INVALID_ARGS;
	}

	mutex_acquire(&bitmap_mutex);
	if (value)
		bitmap_set(key_bitmap, code);
	else
		bitmap_clear(key_bitmap, code);
	mutex_release(&bitmap_mutex);

	// I guess we shouldn't ignore these
	if(!value) return NO_ERROR;
	
	// create event
	key_event_t* event = malloc(sizeof(key_event_t));
	event->code = code;
	event->value = value;

	// add event
	mutex_acquire(&queue_mutex);
	list_add_tail(&event_queue, &event->node);
	mutex_init(&queue_mutex);

	// signal
	LTRACEF("key state change: %d %d\n", code, value);
	event_signal(&key_posted, 0);

	return NO_ERROR;
}

int keys_get_state(uint16_t code)
{
	if (code >= MAX_KEYS) {
		LTRACEF("Invalid keycode requested: %d\n", code);
		return ERR_INVALID_ARGS;
	}

	return bitmap_test(key_bitmap, code);
}

void keys_clear_all(void)
{
	mutex_acquire(&bitmap_mutex);
	memset(key_bitmap, 0, sizeof(key_bitmap));
	mutex_release(&bitmap_mutex);

	mutex_acquire(&queue_mutex);
	while(keys_has_next()) {
		key_event_t* event = list_remove_tail_type(&event_queue, key_event_t, node);
		free(event);
	}
	mutex_release(&queue_mutex);
}

int keys_get_next(uint16_t* code, uint16_t* value, bool wait)
{
	// wait for next event
	if(!keys_has_next())
		event_wait(&key_posted);

	// get event
	mutex_acquire(&queue_mutex);
	key_event_t* event = list_remove_tail_type(&event_queue, key_event_t, node);
	mutex_release(&queue_mutex);

	// set result code
	*code = event->code;
	*value = event->value;

	// free event
	free(event);

	return NO_ERROR;
}

int keys_has_next(void)
{
	return !list_is_empty(&event_queue);
}

static int cmd_keys(int argc, const cmd_args *argv)
{
	if (argc < 2) {
	notenoughargs:
		printf("not enough arguments\n");
	usage:
		printf("usage:\n");
		printf("%s post <code> <value>\n", argv[0].str);
		return ERR_GENERIC;
	}

	if (!strcmp(argv[1].str, "post")) {
		if (argc < 4) goto notenoughargs;
		keys_post_event(argv[2].u, argv[3].u);
	} else {
		printf("unknown command\n");
		goto usage;
	}

	return NO_ERROR;
}

STATIC_COMMAND_START
#if LK_DEBUGLEVEL > 0
STATIC_COMMAND("keys", "key commands", &cmd_keys)
#endif
STATIC_COMMAND_END(keys);

LK_INIT_HOOK(keys_threading, &keys_init_threading, LK_INIT_LEVEL_THREADING);
LK_INIT_HOOK(keys_heap, &keys_init_heap, LK_INIT_LEVEL_HEAP);
