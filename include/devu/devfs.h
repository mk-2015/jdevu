#ifndef DEVU_DEVFS_H
#define DEVU_DEVFS_H

#include <linux/device.h>

int setup_unlockedmem(struct class *cls, int major);
void unsetup_unlockedmem(struct class *cls, int major);
int setup_ktraces(void);
void unsetup_ktraces(void);
int setup_unlkport(void);
void unsetup_unlkport(void);
int setup_kpcidescv(void);
void unsetup_kpcidescv(void);
int setup_kusbdescv(void);
void unsetup_kusbdescv(void);

#endif
