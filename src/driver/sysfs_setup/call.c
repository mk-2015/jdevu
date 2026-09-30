#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kmod.h>
#include <devu/sys.h>

static ssize_t call_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    int cmd;
    char msg[128];
    int parsed;

    parsed = sscanf(buf, "%d %127[^\n]", &cmd, msg);

    switch (cmd) {
        case 1: // Call panic
            if (parsed >= 2) panic("%s", msg);
            break;
        case 2: // Call printk
            if (parsed >= 2) printk(KERN_INFO "devu: %s\n", msg);
            break;
        case 3: // Call sync
            char *argv[] = { "/bin/sync", NULL };
            char *envp[] = { "HOME=/", "TERM=linux", "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };
            call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
            
            printk(KERN_INFO "devu: System sync executed\n");
            break;
        default:
            printk(KERN_WARNING "devu: Unknown command ID %d\n", cmd);
            break;
    }
    return count;
}
static DEVICE_ATTR(call, 0200, NULL, call_store);

int setup_call(struct devu_driver *drv) {
    return add_sysfs_file(drv, "call", &dev_attr_call);
}
