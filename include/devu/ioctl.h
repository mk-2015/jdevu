#ifndef DEVU_IOCTL_H
#define DEVU_IOCTL_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
typedef int64_t __s64;
typedef uint64_t __u64;
typedef uint32_t __u32;
typedef uint8_t __u8;
#endif

typedef struct {
    __u64 lower_phys_addr;
    __u64 upper_phys_addr;
} __attribute__((packed)) mem_t;

typedef struct {
    __u64 pid;
    __s64 uid;
    __s64 gid;
    char pname[64];
} __attribute__((packed)) ktraces_hit_info;

struct ktrace_struct {
    char func_name[64];
};

struct ktrace_fetch_struct {
    __u64 num_hits;
    ktraces_hit_info *hits;
};

#define MAX_TOPIC_LEN 64
#define MAX_MSG_LEN   256

#define IPCHUB_TYPE_PUB 1
#define IPCHUB_TYPE_SUB 2

struct ipchub_reg {
    __u32 type;
    char topic[MAX_TOPIC_LEN];
} __attribute__((packed));

typedef struct {
    __u64 pid;
    __s64 uid;
    __s64 gid;
    char payload[MAX_MSG_LEN];
    __u64 len;
} __attribute__((packed)) ipchub_event_t;

#define DEVU_IOC_MAGIC 'd'
#define DEVU_MEM_RESIZE     _IOW(DEVU_IOC_MAGIC, 3, mem_t)
#define KTRACE_FUNC_FILTER  _IOW(DEVU_IOC_MAGIC, 4, struct ktrace_struct)
#define KTRACE_GET_COUNT    _IOR(DEVU_IOC_MAGIC, 5, __u64)
#define KTRACE_GET_DATA     _IOW(DEVU_IOC_MAGIC, 6, struct ktrace_fetch_struct)
#define IPCHUB_IOC_REGISTER _IOW(DEVU_IOC_MAGIC, 10, struct ipchub_reg)

#endif