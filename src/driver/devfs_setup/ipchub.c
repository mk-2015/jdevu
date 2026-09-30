#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <devu/ipchub.h>

static int ipchub_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "devu: ipchub opened\n");
    return 0;
}

static const struct file_operations ipchub_fops = {
    .owner = THIS_MODULE,
    .open = ipchub_open,
};

static struct miscdevice ipchub_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "ipchub",
    .fops = &ipchub_fops,
    .mode = 0666,
};

int setup_ipchub(void) {
    return misc_register(&ipchub_misc);
}

void unsetup_ipchub(void) {
    misc_deregister(&ipchub_misc);
}
