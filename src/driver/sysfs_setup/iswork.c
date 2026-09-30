#include <linux/device.h>
#include <devu/sys.h>

static ssize_t iswork_show(struct device *dev, struct device_attribute *attr, char *buf) {
    // sysfs_emit is the preferred way to write to the sysfs buffer in the kernel
    return sysfs_emit(buf, "ITS WORKING\n");
}

static DEVICE_ATTR_RO(iswork); // 0444 = Read-only

int setup_iswork(struct devu_driver *drv) {
    return add_sysfs_file(drv, "iswork", &dev_attr_iswork);
}
