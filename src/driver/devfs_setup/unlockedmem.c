#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <devu/ioctl.h>
#include <devu/devfs.h>

static struct cdev mem_cdev;

static int mem_open(struct inode *inode, struct file *file) {
    mem_t *region = kmalloc(sizeof(mem_t), GFP_KERNEL);
    if (!region) return -ENOMEM;
    
    region->lower_phys_addr = 0;
    region->upper_phys_addr = 0;
    region->mem_type = DEVU_MEM_NONCACHED; // Default to safe
    
    file->private_data = region;
    return 0;
}

static int mem_release(struct inode *inode, struct file *file) {
    kfree(file->private_data);
    return 0;
}

static long mem_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    mem_t *region = (mem_t *)file->private_data;
    mem_t __user *user_region = (mem_t __user *)arg;
    
    if (cmd == DEVU_MEM_RESIZE) {
        if (copy_from_user(region, user_region, sizeof(mem_t)))
            return -EFAULT;
        printk(KERN_INFO "devu: Memory region set to 0x%llx - 0x%llx, type: %d\n", 
               region->lower_phys_addr, region->upper_phys_addr, region->mem_type);
        return 0;
    }
    return -EINVAL;
}

static ssize_t mem_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    mem_t *region = (mem_t *)file->private_data;
    void __iomem *vaddr;
    char *kbuf;
    size_t size = region->upper_phys_addr - region->lower_phys_addr;
    
    if (*ppos >= size) return 0;
    if (count > size - *ppos) count = size - *ppos;

    vaddr = ioremap(region->lower_phys_addr + *ppos, count);
    if (!vaddr) return -ENOMEM;

    kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf) { iounmap(vaddr); return -ENOMEM; }

    memcpy_fromio(kbuf, vaddr, count);
    
    if (copy_to_user(buf, kbuf, count)) {
        kfree(kbuf); iounmap(vaddr); return -EFAULT;
    }

    kfree(kbuf);
    iounmap(vaddr);
    *ppos += count;
    return count;
}

static int mem_mmap(struct file *file, struct vm_area_struct *vma) {
    mem_t *region = (mem_t *)file->private_data;
    unsigned long size = vma->vm_end - vma->vm_start;
    unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

    if (offset + size > (region->upper_phys_addr - region->lower_phys_addr))
        return -EINVAL;

    // Apply caching policy
    switch (region->mem_type) {
        case DEVU_MEM_NONCACHED:
            vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
            break;
        case DEVU_MEM_WRITECOMBINE:
            vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
            break;
        case DEVU_MEM_CACHED:
        default:
            // Standard caching (do nothing)
            break;
    }

    if (remap_pfn_range(vma, vma->vm_start,
                        (region->lower_phys_addr + offset) >> PAGE_SHIFT,
                        size, vma->vm_page_prot))
        return -EAGAIN;

    return 0;
}

static const struct file_operations mem_fops = {
    .owner = THIS_MODULE,
    .open = mem_open,
    .release = mem_release,
    .unlocked_ioctl = mem_ioctl,
    .read = mem_read,
    .mmap = mem_mmap,
};

int setup_unlockedmem(struct class *cls, int major) {
    dev_t dev = MKDEV(major, 11);
    cdev_init(&mem_cdev, &mem_fops);
    cdev_add(&mem_cdev, dev, 1);
    device_create(cls, NULL, dev, NULL, "unlkmem");
    return 0;
}

void unsetup_unlockedmem(struct class *cls, int major) {
    device_destroy(cls, MKDEV(major, 11));
    cdev_del(&mem_cdev);
}
