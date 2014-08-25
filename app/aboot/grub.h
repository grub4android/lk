#ifndef _GRUB_H_
#define _GRUB_H_

int grub_boot(void);
void grub_get_bootdev(char **value);
int grub_has_tar(void);
int grub_load_from_sideload(void* data);

#endif
