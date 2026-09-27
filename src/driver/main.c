#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MK-2015");

static int __init devu_init(void) {
    printk("devu: Template module loaded successfully\n");
    return 0;
}

static void __exit devu_exit(void) {
    printk(KERN_INFO "devu: Template module unloaded\n");
}

module_init(devu_init);
module_exit(devu_exit);
