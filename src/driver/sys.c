#include <devu/sys.h>
#include <linux/slab.h>
#include <linux/module.h>

struct class* new_class(const char *name) {
    return class_create(name);
}

struct devu_driver* new_driver(const char *name, struct class *class, const struct file_operations *fops) {
    struct devu_driver *drv = kmalloc(sizeof(struct devu_driver), GFP_KERNEL);
    if (!drv) return NULL;

    drv->class = class;
    
    // Allocate char device region
    if (alloc_chrdev_region(&drv->dev_num, 0, 1, name) < 0) {
        kfree(drv);
        return NULL;
    }
    drv->major = MAJOR(drv->dev_num);

    // Initialize cdev
    cdev_init(&drv->cdev, fops);
    if (cdev_add(&drv->cdev, drv->dev_num, 1) < 0) {
        unregister_chrdev_region(drv->dev_num, 1);
        kfree(drv);
        return NULL;
    }

    // Create device node
    drv->device = device_create(class, NULL, drv->dev_num, NULL, name);
    if (IS_ERR(drv->device)) {
        cdev_del(&drv->cdev);
        unregister_chrdev_region(drv->dev_num, 1);
        kfree(drv);
        return NULL;
    }

    return drv;
}

int add_sysfs_file(struct devu_driver *drv, const char *name, struct device_attribute *attr) {
    if (!drv || !drv->device) return -EINVAL;
    // Note: The 'name' parameter here is ignored in this simple wrapper
    // because it relies on the attribute's internal name defined via DEVICE_ATTR
    return device_create_file(drv->device, attr);
}

void free_driver(struct devu_driver *drv) {
    if (!drv) return;
    device_destroy(drv->class, drv->dev_num);
    cdev_del(&drv->cdev);
    unregister_chrdev_region(drv->dev_num, 1);
    kfree(drv);
}
