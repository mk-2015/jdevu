#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <devu/devfs.h>
#include <devu/ioctl.h> 

static long unlkport_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    size_t sz = _IOC_SIZE(cmd);

    switch (cmd) {
        case PORT_CMD_PORT_READ: {
            port_t op;
            if (sz != sizeof(port_t)) 
                return -EINVAL;
            
            if (copy_from_user(&op, (void __user *)arg, sizeof(op)))
                return -EFAULT;

            switch (op.size) {
                case PORTSZ_BYTE: op.value = inb(op.port); break;
                case PORTSZ_WORD: op.value = inw(op.port); break;
                case PORTSZ_LONG: op.value = inl(op.port); break;
                default: return -EINVAL;
            }

            if (copy_to_user((void __user *)arg, &op, sizeof(op)))
                return -EFAULT;
            break;
        }

        case PORT_CMD_PORT_WRITE: {
            port_t op;
            if (sz != sizeof(port_t)) 
                return -EINVAL;
            
            if (copy_from_user(&op, (void __user *)arg, sizeof(op)))
                return -EFAULT;

            switch (op.size) {
                case PORTSZ_BYTE: outb((uint8_t)op.value, op.port); break;
                case PORTSZ_WORD: outw((uint16_t)op.value, op.port); break;
                case PORTSZ_LONG: outl(op.value, op.port); break;
                default: return -EINVAL;
            }
            break;
        }

        case PORT_CMD_BLOCK_READ:
        case PORT_CMD_BLOCK_WRITE: {
            port_block_t block;
            void *kbuf;
            size_t total_size;
            long ret = 0;

            if (sz != sizeof(port_block_t)) 
                return -EINVAL;

            if (copy_from_user(&block, (void __user *)arg, sizeof(block)))
                return -EFAULT;

            total_size = (size_t)block.count * block.size;
            if (total_size == 0 || total_size > (1024 * 1024)) // 1MB safety ceiling
                return -EINVAL;

            kbuf = kzalloc(total_size, GFP_KERNEL);
            if (!kbuf)
                return -ENOMEM;

            if (cmd == PORT_CMD_BLOCK_WRITE) {
                if (copy_from_user(kbuf, (void __user *)(unsigned long)block.user_buffer, total_size)) {
                    kfree(kbuf);
                    return -EFAULT;
                }
            }

            switch (block.size) {
                case PORTSZ_BYTE:
                    if (cmd == PORT_CMD_BLOCK_READ) insb(block.port, kbuf, block.count);
                    else outsb(block.port, kbuf, block.count);
                    break;
                case PORTSZ_WORD:
                    if (cmd == PORT_CMD_BLOCK_READ) insw(block.port, kbuf, block.count);
                    else outsw(block.port, kbuf, block.count);
                    break;
                case PORTSZ_LONG:
                    if (cmd == PORT_CMD_BLOCK_READ) insl(block.port, kbuf, block.count);
                    else outsl(block.port, kbuf, block.count);
                    break;
                default:
                    kfree(kbuf);
                    return -EINVAL;
            }

            if (cmd == PORT_CMD_BLOCK_READ) {
                if (copy_to_user((void __user *)(unsigned long)block.user_buffer, kbuf, total_size)) {
                    ret = -EFAULT;
                }
            }

            kfree(kbuf);
            return ret;
        }

        default:
            return -EINVAL;
    }

    return 0;
}

static const struct file_operations unlkport_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = unlkport_ioctl,
};

static struct miscdevice unlkport_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "unlkport",
    .fops = &unlkport_fops,
    .mode = 0600,
};

int setup_unlkport(void) { return misc_register(&unlkport_misc); }
void unsetup_unlkport(void) { misc_deregister(&unlkport_misc); }
