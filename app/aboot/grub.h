#ifndef _GRUB_H_
#define _GRUB_H_

int grub_boot(void);
void grub_get_bootdev(char **value);
void grub_get_bootpath(char **value);
int grub_has_tar(void);
int grub_load_from_sideload(void* data);
struct tar_io* grub_tar_get_tio(void);

#endif
