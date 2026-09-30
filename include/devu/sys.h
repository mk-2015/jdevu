#ifndef DEVU_SYS_H
#define DEVU_SYS_H

#include <linux/device.h>
#include <linux/cdev.h>

struct devu_driver {
    struct class *class;
    struct device *device;
    struct cdev cdev;
    dev_t dev_num;
    int major;
};

struct class* new_class(const char *name);
struct devu_driver* new_driver(const char *name, struct class *class, const struct file_operations *fops);
int add_sysfs_file(struct devu_driver *drv, const char *name, struct device_attribute *attr);
void free_driver(struct devu_driver *drv);

#endif
