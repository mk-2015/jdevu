#ifndef DEVU_FUNCDECL_H
#define DEVU_FUNCDECL_H

#include <linux/device.h>
#include <devu/sys.h>

int setup_sys(struct class **cls, struct devu_driver **drv);
void unsetup_sys(struct class *cls, struct devu_driver *drv);
int setup_iswork(struct devu_driver *drv);
int setup_call(struct devu_driver *drv);
int setup_unlockedmem(struct class *cls, int major);
void unsetup_unlockedmem(struct class *cls, int major);
int setup_ktraces(void);
void unsetup_ktraces(void);
int setup_unlkport(void);
void unsetup_unlkport(void);

#endif
