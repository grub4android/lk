#ifndef _APP_MENU_PRIVATE_H_
#define _APP_MENU_PRIVATE_H_

struct menu_entry {
	const char* name;
	void (*execute)(void);
	void (*format)(char** buf);
};

struct menu_stack_entry {
	struct menu_stack_entry* prev;
	struct menu_entry* entries;
};

extern struct menu_entry entries_main[];
extern struct menu_entry entries_settings[];

void menu_enter(struct menu_entry* menu);
void menu_leave(void);

#endif
