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
typedef int32_t __s32;
typedef uint32_t __u32;
typedef int16_t __s16;
typedef uint16_t __u16;
typedef int8_t __s8;
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

#define PORTSZ_BYTE 1
#define PORTSZ_WORD 2
#define PORTSZ_LONG 4
#define PORTSZ_DEFU PORTSZ_WORD

typedef struct {
    uint16_t port;
    uint8_t  size;
    uint32_t value;
} __attribute__((packed)) port_t;

typedef struct {
    uint16_t port;
    uint8_t  size;
    uint32_t count;
    uint64_t user_buffer;
} __attribute__((packed)) port_block_t;

#define MAX_PCIDEVC_NAME_LEN 256

typedef struct {
    char name[MAX_PCIDEVC_NAME_LEN];
    __u64 phys_base_addrs[6];
} __attribute__((packed)) pcidevc_device;

typedef struct {
    __u64 num_devices;
    pcidevc_device *devices;
} __attribute__((packed)) pcidevc_devices_t;

#define MAX_USBDEVC_NAME_LEN 128

typedef struct {
    char name[MAX_USBDEVC_NAME_LEN];
    __u16 vendor_id;
    __u16 product_id;
    __u8 bus_loc;
    __u8 dev_addr;
} __attribute__((packed)) usbdevc_device;

typedef struct {
    __u64 num_devices;
    usbdevc_device *devices;
} __attribute__((packed)) usbdevc_devices_t;

#define DEVU_IOC_MAGIC 'd'
#define DEVU_MEM_RESIZE      _IOW(DEVU_IOC_MAGIC, 3, mem_t)
#define KTRACE_FUNC_FILTER   _IOW(DEVU_IOC_MAGIC, 4, struct ktrace_struct)
#define KTRACE_GET_COUNT     _IOR(DEVU_IOC_MAGIC, 5, __u64)
#define KTRACE_GET_DATA      _IOW(DEVU_IOC_MAGIC, 6, struct ktrace_fetch_struct)
#define IPCHUB_IOC_REGISTER  _IOW(DEVU_IOC_MAGIC, 7, struct ipchub_reg)
#define PORT_CMD_PORT_READ   _IOR(DEVU_IOC_MAGIC, 8, port_t)
#define PORT_CMD_PORT_WRITE  _IOW(DEVU_IOC_MAGIC, 9, port_t)
#define PORT_CMD_BLOCK_READ  _IOR(DEVU_IOC_MAGIC, 10, port_block_t)
#define PORT_CMD_BLOCK_WRITE _IOW(DEVU_IOC_MAGIC, 11, port_block_t)
#define PCIDEVC_GET_SIZE     _IOR(DEVU_IOC_MAGIC, 12, __u64)
#define PCIDEVC_GET_DEVICES  _IOW(DEVU_IOC_MAGIC, 13, pcidevc_devices_t)
#define USBDEVC_GET_SIZE     _IOR(DEVU_IOC_MAGIC, 14, __u64)
#define USBDEVC_GET_DEVICES  _IOW(DEVU_IOC_MAGIC, 15, usbdevc_devices_t)


#endif