#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lk/init.h>

typedef struct cmdline_item {
	struct list_node node;
	char* name;
	char* value;
} cmdline_item_t;

static struct list_node cmdline_list;

void cmdline_add(const char* name, const char* value) {
	cmdline_item_t* item = malloc(sizeof(cmdline_item_t));
	item->name = strdup(name);
	item->value = value?strdup(value):NULL;

	list_add_tail(&cmdline_list, &item->node);
}

size_t cmdline_length(void) {
	size_t len = 0;

	cmdline_item_t *item;
	list_for_every_entry(&cmdline_list, item, cmdline_item_t, node) {
		if(len!=0) len++;
		len+=strlen(item->name);
		if(item->value)
			len+= 1 + strlen(item->value);
	}

	// 0 terminator
	len++;

	return len;
}

size_t cmdline_generate(char* buf, size_t bufsize) {
	size_t len = 0;

	cmdline_item_t *item;
	list_for_every_entry(&cmdline_list, item, cmdline_item_t, node) {
		if(len!=0) buf[len++] = ' ';
		len+=strlcpy(buf+len, item->name, bufsize-len);

		if(item->value) {
			buf[len++] = '=';
			len+=strlcpy(buf+len, item->value, bufsize-len);
		}
	}

	return len;
}

static void android_cmdline_init(uint level)
{
	list_initialize(&cmdline_list);
}

LK_INIT_HOOK(android_cmdline, &android_cmdline_init, LK_INIT_LEVEL_HEAP);
