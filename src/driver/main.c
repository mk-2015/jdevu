#include <linux/module.h>
#include <linux/kernel.h>
#include <devu/sys.h>
#include <devu/funcdecl.h>

static struct class *devu_class;
static struct devu_driver *devu_drv;

static int __init my_module_init(void) {
    int ret = setup_sys(&devu_class, &devu_drv);
    if (ret) return ret;

    printk(KERN_INFO "devu: Module loaded with crash API\n");
    return 0;
}

static void __exit my_module_exit(void) {
    unsetup_sys(devu_class, devu_drv);
    printk(KERN_INFO "devu: Module unloaded\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Devu Kernel Utilities Driver");
