#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/pci.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <devu/devfs.h>
#include <devu/ioctl.h>

static int kpcidescv_open(struct inode *inode, struct file *file) {
    pr_info("devu: kpcidescv opened\n");
    return 0;
}

static long kpcidescv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct pci_dev *pdev = NULL;

    switch (cmd) {
    case PCIDEVC_GET_SIZE: {
        __u64 count = 0;

        while ((pdev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, pdev)) != NULL) {
            count++;
        }

        if (copy_to_user((void __user *)arg, &count, sizeof(count)))
            return -EFAULT;

        break;
    }

    case PCIDEVC_GET_DEVICES: {
        pcidevc_devices_t user_devices;
        pcidevc_device *kernel_buf;
        __u64 count = 0;
        __u64 i = 0;

        if (copy_from_user(&user_devices, (void __user *)arg, sizeof(user_devices)))
            return -EFAULT;

        while ((pdev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, pdev)) != NULL) {
            count++;
        }

        if (count == 0) {
            user_devices.num_devices = 0;
            if (copy_to_user((void __user *)arg, &user_devices, sizeof(user_devices)))
                return -EFAULT;
            return 0;
        }

        if (user_devices.num_devices < count)
            count = user_devices.num_devices;

        kernel_buf = kmalloc_array(count, sizeof(pcidevc_device), GFP_KERNEL);
        if (!kernel_buf)
            return -ENOMEM;

        pdev = NULL;
        while ((pdev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, pdev)) != NULL && i < count) {
            int b;
            snprintf(kernel_buf[i].name, MAX_PCIDEVC_NAME_LEN, "pci_%04x:%04x", pdev->vendor, pdev->device);
            
            for (b = 0; b < 6; b++) {
                kernel_buf[i].phys_base_addrs[b] = (__u64)pci_resource_start(pdev, b);
            }
            i++;
        }

        if (pdev)
            pci_dev_put(pdev);

        if (copy_to_user(user_devices.devices, kernel_buf, i * sizeof(pcidevc_device))) {
            kfree(kernel_buf);
            return -EFAULT;
        }

        user_devices.num_devices = i;
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

static const struct file_operations kpcidescv_fops = {
    .owner = THIS_MODULE,
    .open = kpcidescv_open,
    .unlocked_ioctl = kpcidescv_ioctl,
};

static struct miscdevice kpcidescv_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "kpcidescv",
    .fops = &kpcidescv_fops,
    .mode = 0644,
};

int setup_kpcidescv(void) { 
    return misc_register(&kpcidescv_misc); 
}

void unsetup_kpcidescv(void) { 
    misc_deregister(&kpcidescv_misc); 
}