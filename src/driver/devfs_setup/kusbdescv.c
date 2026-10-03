#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/usb.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <devu/devfs.h>
#include <devu/ioctl.h>

static int count_usb_devices(struct usb_device *udev, void *data) {
    __u64 *count = (__u64 *)data;
    (*count)++;
    return 0;
}

struct usb_populate_ctx {
    usbdevc_device *buf;
    __u64 max_count;
    __u64 index;
};

static int populate_usb_devices(struct usb_device *udev, void *data) {
    struct usb_populate_ctx *ctx = (struct usb_populate_ctx *)data;
    if (ctx->index >= ctx->max_count)
        return 0;

    ctx->buf[ctx->index].vendor_id = le16_to_cpu(udev->descriptor.idVendor);
    ctx->buf[ctx->index].product_id = le16_to_cpu(udev->descriptor.idProduct);
    ctx->buf[ctx->index].bus_loc = udev->bus->busnum;
    ctx->buf[ctx->index].dev_addr = udev->devnum;

    snprintf(ctx->buf[ctx->index].name, MAX_USBDEVC_NAME_LEN, "usb_%04x:%04x",
             ctx->buf[ctx->index].vendor_id,
             ctx->buf[ctx->index].product_id);

    ctx->index++;
    return 0;
}

static long kusbdescv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case USBDEVC_GET_SIZE: {
        __u64 count = 0;
        usb_for_each_dev(&count, count_usb_devices);

        if (copy_to_user((void __user *)arg, &count, sizeof(count)))
            return -EFAULT;
        break;
    }

    case USBDEVC_GET_DEVICES: {
        usbdevc_devices_t user_devices;
        usbdevc_device *kernel_buf;
        __u64 count = 0;
        struct usb_populate_ctx ctx;

        if (copy_from_user(&user_devices, (void __user *)arg, sizeof(user_devices)))
            return -EFAULT;

        usb_for_each_dev(&count, count_usb_devices);

        if (count == 0) {
            user_devices.num_devices = 0;
            if (copy_to_user((void __user *)arg, &user_devices, sizeof(user_devices)))
                return -EFAULT;
            return 0;
        }

        if (user_devices.num_devices < count)
            count = user_devices.num_devices;

        kernel_buf = kmalloc_array(count, sizeof(usbdevc_device), GFP_KERNEL);
        if (!kernel_buf)
            return -ENOMEM;

        ctx.buf = kernel_buf;
        ctx.max_count = count;
        ctx.index = 0;

        usb_for_each_dev(&ctx, populate_usb_devices);

        if (copy_to_user(user_devices.devices, kernel_buf, ctx.index * sizeof(usbdevc_device))) {
            kfree(kernel_buf);
            return -EFAULT;
        }

        user_devices.num_devices = ctx.index;
        if (copy_to_user((void __user *)arg, &user_devices, sizeof(user_devices))) {
            kfree(kernel_buf);
            return -EFAULT;
        }

        kfree(kernel_buf);
        break;
    }

    default:
        return -ENOTTY;
    }

    return 0;
}

static const struct file_operations kusbdescv_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = kusbdescv_ioctl,
};

static struct miscdevice kusbdescv_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "kusbdescv",
    .fops = &kusbdescv_fops,
    .mode = 0644,
};

int setup_kusbdescv(void) { return misc_register(&kusbdescv_misc); }
void unsetup_kusbdescv(void) { misc_deregister(&kusbdescv_misc); }