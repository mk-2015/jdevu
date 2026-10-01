#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <devu/ioctl.h>

struct ipchub_message {
    struct list_head list;
    ipchub_event_t event;
};

struct ipchub_topic {
    struct list_head list;
    char name[MAX_TOPIC_LEN];
    struct list_head message_queue;
    wait_queue_head_t wait_queue;
    struct mutex lock;
    int ref_count;
};

struct ipchub_client {
    struct ipchub_topic *topic;
    bool is_publisher;
};

static LIST_HEAD(topic_list);
static DEFINE_MUTEX(registry_lock);

/* Find or create a topic with reference counting */
static struct ipchub_topic *get_topic(const char *name) {
    struct ipchub_topic *t;
    
    mutex_lock(&registry_lock);
    list_for_each_entry(t, &topic_list, list) {
        if (strncmp(t->name, name, MAX_TOPIC_LEN) == 0) {
            t->ref_count++;
            mutex_unlock(&registry_lock);
            return t;
        }
    }

    t = kzalloc(sizeof(*t), GFP_KERNEL);
    if (!t) {
        mutex_unlock(&registry_lock);
        return NULL;
    }

    strscpy(t->name, name, MAX_TOPIC_LEN);
    INIT_LIST_HEAD(&t->message_queue);
    init_waitqueue_head(&t->wait_queue);
    mutex_init(&t->lock);
    t->ref_count = 1;
    list_add(&t->list, &topic_list);
    mutex_unlock(&registry_lock);

    return t;
}

/* Decrement topic reference and clean up if empty */
static void put_topic(struct ipchub_topic *t) {
    struct ipchub_message *msg, *tmp;

    if (!t)
        return;

    mutex_lock(&registry_lock);
    t->ref_count--;
    if (t->ref_count <= 0) {
        list_del(&t->list);
        mutex_unlock(&registry_lock);

        mutex_lock(&t->lock);
        list_for_each_entry_safe(msg, tmp, &t->message_queue, list) {
            list_del(&msg->list);
            kfree(msg);
        }
        mutex_unlock(&t->lock);

        mutex_destroy(&t->lock);
        kfree(t);
        return;
    }
    mutex_unlock(&registry_lock);
}

static int ipchub_open(struct inode *inode, struct file *file) {
    struct ipchub_client *client = kzalloc(sizeof(*client), GFP_KERNEL);
    if (!client)
        return -ENOMEM;

    file->private_data = client;
    return 0;
}

static int ipchub_release(struct inode *inode, struct file *file) {
    struct ipchub_client *client = file->private_data;

    if (client) {
        if (client->topic) {
            put_topic(client->topic);
        }
        kfree(client);
    }
    return 0;
}

static long ipchub_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct ipchub_client *client = file->private_data;
    struct ipchub_reg reg;
    struct ipchub_topic *new_topic;

    if (!client)
        return -EINVAL;

    switch (cmd) {
        case IPCHUB_IOC_REGISTER:
            if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)))
                return -EFAULT;

            if (reg.type != IPCHUB_TYPE_PUB && reg.type != IPCHUB_TYPE_SUB)
                return -EINVAL;

            reg.topic[MAX_TOPIC_LEN - 1] = '\0';
            new_topic = get_topic(reg.topic);
            if (!new_topic)
                return -ENOMEM;

            if (client->topic) {
                put_topic(client->topic);
            }

            client->topic = new_topic;
            client->is_publisher = (reg.type == IPCHUB_TYPE_PUB);
            break;

        default:
            return -ENOTTY;
    }
    return 0;
}

static ssize_t ipchub_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    struct ipchub_client *client = file->private_data;
    struct ipchub_message *msg;

    if (!client || !client->topic || !client->is_publisher)
        return -EPERM;

    if (count > MAX_MSG_LEN)
        count = MAX_MSG_LEN;

    msg = kzalloc(sizeof(*msg), GFP_KERNEL);
    if (!msg)
        return -ENOMEM;

    msg->event.pid = task_pid_vnr(current);
    msg->event.uid = from_kuid(&init_user_ns, current_uid());
    msg->event.gid = from_kgid(&init_user_ns, current_gid());
    msg->event.len = count;

    if (copy_from_user(msg->event.payload, buf, count)) {
        kfree(msg);
        return -EFAULT;
    }

    mutex_lock(&client->topic->lock);
    list_add_tail(&msg->list, &client->topic->message_queue);
    mutex_unlock(&client->topic->lock);

    wake_up_interruptible(&client->topic->wait_queue);
    return count;
}

static ssize_t ipchub_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    struct ipchub_client *client = file->private_data;
    struct ipchub_message *msg;
    int ret;

    if (!client || !client->topic || client->is_publisher)
        return -EPERM;

    mutex_lock(&client->topic->lock);

    while (list_empty(&client->topic->message_queue)) {
        mutex_unlock(&client->topic->lock);

        if (file->f_flags & O_NONBLOCK)
            return -EAGAIN;

        ret = wait_event_interruptible(client->topic->wait_queue,
                                       !list_empty(&client->topic->message_queue));
        if (ret)
            return ret;

        mutex_lock(&client->topic->lock);
    }

    msg = list_first_entry(&client->topic->message_queue, struct ipchub_message, list);
    list_del(&msg->list);
    mutex_unlock(&client->topic->lock);

    if (count < sizeof(ipchub_event_t)) {
        kfree(msg);
        return -EINVAL;
    }

    if (copy_to_user(buf, &msg->event, sizeof(ipchub_event_t))) {
        kfree(msg);
        return -EFAULT;
    }

    kfree(msg);
    return sizeof(ipchub_event_t);
}

static unsigned int ipchub_poll(struct file *file, struct poll_table_struct *wait) {
    struct ipchub_client *client = file->private_data;
    unsigned int mask = 0;

    if (!client || !client->topic)
        return 0;

    poll_wait(file, &client->topic->wait_queue, wait);

    mutex_lock(&client->topic->lock);
    if (client->is_publisher) {
        mask |= EPOLLOUT | EPOLLWRNORM;
    } else {
        if (!list_empty(&client->topic->message_queue)) {
            mask |= EPOLLIN | EPOLLRDNORM;
        }
    }
    mutex_unlock(&client->topic->lock);

    return mask;
}

static const struct file_operations ipchub_fops = {
    .owner = THIS_MODULE,
    .open = ipchub_open,
    .release = ipchub_release,
    .write = ipchub_write,
    .read = ipchub_read,
    .poll = ipchub_poll,
    .unlocked_ioctl = ipchub_ioctl,
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