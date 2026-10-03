#include <devu/sys.h>
#include <devu/funcdecl.h>
#include <devu/devfs.h>

static struct class *cmem_cls;

int setup_sys(struct class **cls, struct devu_driver **drv) {
    *cls = new_class("devu");
    if (!*cls) return -ENOMEM;

    cmem_cls = new_class("cmem");
    if (!cmem_cls) {
        class_destroy(*cls);
        return -ENOMEM;
    }

    *drv = new_driver("devu", *cls, NULL);
    if (!*drv) {
        class_destroy(*cls);
        class_destroy(cmem_cls);
        return -ENOMEM;
    }

    setup_iswork(*drv);
    setup_call(*drv);
    setup_unlockedmem(cmem_cls, (*drv)->major);
    setup_ktraces();
    setup_unlkport();
    setup_kpcidescv();
    setup_kusbdescv();
    return 0;
}

void unsetup_sys(struct class *cls, struct devu_driver *drv) {
    if (drv) {
        unsetup_kusbdescv();
        unsetup_kpcidescv();
        unsetup_unlkport();
        unsetup_ktraces();
        unsetup_unlockedmem(cmem_cls, drv->major);
        free_driver(drv);
    }
    if (cmem_cls) class_destroy(cmem_cls);
    if (cls) class_destroy(cls);
}
