#ifndef DEVU_DEVFS_H
#define DEVU_DEVFS_H

#include <linux/device.h>

int setup_unlockedmem(struct class *cls, int major);
void unsetup_unlockedmem(struct class *cls, int major);
int setup_ktraces(void);
void unsetup_ktraces(void);

#endif
