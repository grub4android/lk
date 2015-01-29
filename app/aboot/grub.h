#ifndef _GRUB_H_
#define _GRUB_H_

int grub_boot(void);
void grub_get_bootdev(char **value);
void grub_get_bootpath(char **value);
int grub_load_from_sideload(void* data);

#endif
