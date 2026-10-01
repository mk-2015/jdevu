#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/cred.h>
#include <linux/uidgid.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/miscdevice.h>
#include <devu/ioctl.h>

#define MAX_HITS 1024
static atomic_t device_open_count = ATOMIC_INIT(0);

struct ktrace_session {
    struct kprobe kp;
    atomic64_t hit_count;
    char func_name[64];
    bool active;
    struct mutex setup_mutex;
    ktraces_hit_info *hit_info;
};

// High-performance, completely lockless pre-handler for the hot path
static int notrace devu_generic_kprobe_handler(struct kprobe *p, struct pt_regs *regs)
{
    struct ktrace_session *session = container_of(p, struct ktrace_session, kp);
    __u64 index;
    ktraces_hit_info *hit;
    struct task_struct *task = current;

    index = atomic64_inc_return(&session->hit_count) - 1;
    hit = &session->hit_info[index % MAX_HITS];

    hit->pid = task_tgid_nr(task);
    hit->uid = __kuid_val(task_uid(task));
    hit->gid = __kgid_val(task_cred_xxx(task, gid));
    strscpy(hit->pname, task->comm, sizeof(hit->pname));

    return 0;
}

static int ktrace_open(struct inode *inode, struct file *file)
{
    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    struct ktrace_session *session;

    // Ensure only one active file handle at a time
    if (atomic_inc_return(&device_open_count) > 1) {
        atomic_dec(&device_open_count);
        return -EBUSY;
    }

    session = kzalloc(sizeof(*session), GFP_KERNEL);
    if (!session) {
        atomic_dec(&device_open_count);
        return -ENOMEM;
    }

    session->hit_info = kzalloc(sizeof(ktraces_hit_info) * MAX_HITS, GFP_KERNEL);
    if (!session->hit_info) {
        kfree(session);
        atomic_dec(&device_open_count);
        return -ENOMEM;
    }

    mutex_init(&session->setup_mutex);
    atomic64_set(&session->hit_count, 0);
    session->active = false;

    file->private_data = session;
    return 0;
}

static int ktrace_release(struct inode *inode, struct file *file)
{
    struct ktrace_session *session = file->private_data;

    mutex_lock(&session->setup_mutex);
    if (session->active) {
        unregister_kprobe(&session->kp);
        kfree(session->kp.symbol_name);
    }
    mutex_unlock(&session->setup_mutex);

    kfree(session->hit_info);
    kfree(session);
    atomic_dec(&device_open_count);
    return 0;
}

static long ktrace_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct ktrace_session *session = file->private_data;
    void __user *argp = (void __user *)arg;
    int ret = 0;

    switch (cmd) {
    case KTRACE_FUNC_FILTER: {
        struct ktrace_struct kts;
        char *new_sym;

        if (copy_from_user(&kts, argp, sizeof(kts)))
            return -EFAULT;
        kts.func_name[sizeof(kts.func_name) - 1] = '\0';

        new_sym = kstrdup(kts.func_name, GFP_KERNEL);
        if (!new_sym)
            return -ENOMEM;

        mutex_lock(&session->setup_mutex);
        if (session->active) {
            unregister_kprobe(&session->kp);
            kfree(session->kp.symbol_name);
            session->active = false;
        }
        memset(&session->kp, 0, sizeof(session->kp));
        session->kp.symbol_name = new_sym;
        session->kp.pre_handler = devu_generic_kprobe_handler;
        ret = register_kprobe(&session->kp);
        if (ret < 0) {
            kfree(new_sym);
            session->kp.symbol_name = NULL;
        } else {
            session->active = true;
            atomic64_set(&session->hit_count, 0);
            strscpy(session->func_name, kts.func_name, sizeof(session->func_name));
        }
        mutex_unlock(&session->setup_mutex);
        break;
    }
    case KTRACE_GET_COUNT: {
        __u64 count = atomic64_read(&session->hit_count);
        if (copy_to_user(argp, &count, sizeof(count)))
            ret = -EFAULT;
        break;
    }
    case KTRACE_GET_DATA: {
        struct ktrace_fetch_struct fetch;
        ktraces_hit_info *temp_hits;
        __u64 count;

        if (copy_from_user(&fetch, argp, sizeof(fetch)))
            return -EFAULT;
        if (fetch.num_hits == 0 || !fetch.hits)
            return -EINVAL;

        temp_hits = kmalloc_array(MAX_HITS, sizeof(ktraces_hit_info), GFP_KERNEL);
        if (!temp_hits)
            return -ENOMEM;

        count = atomic64_read(&session->hit_count);
        if (count > fetch.num_hits)
            count = fetch.num_hits;
        if (count > MAX_HITS)
            count = MAX_HITS;

        memcpy(temp_hits, session->hit_info, sizeof(ktraces_hit_info) * count);

    count = min_t(__u64, atomic64_read(&session->hit_count), MAX_HITS);
    count = min_t(__u64, count, fetch.num_hits);
    if (copy_to_user(fetch.hits, session->hit_info, count * sizeof(*session->hit_info))) {
        kfree(temp_hits);
        return -EFAULT;
    }

        kfree(temp_hits);
        break;
    }
    default:
        ret = -EINVAL;
    }

    return ret;
}

static const struct file_operations ktrace_fops = {
    .owner = THIS_MODULE,
    .open = ktrace_open,
    .release = ktrace_release,
    .unlocked_ioctl = ktrace_ioctl,
};

static struct miscdevice ktrace_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "ktraces",
    .fops = &ktrace_fops,
    .mode = 0600
};

NOKPROBE_SYMBOL(devu_generic_kprobe_handler);

int setup_ktraces(void)
{
    return misc_register(&ktrace_misc);
}

void unsetup_ktraces(void)
{
    misc_deregister(&ktrace_misc);
}