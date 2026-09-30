# Devu Kernel Utilities (devu)

## Overview
`devu` is a monolithic Linux kernel driver designed for system-level experimentation and low-level debugging. It provides a modular framework to interact directly with the kernel's memory, function execution flow, and system state.

## Architecture
The driver is structured into three primary planes:
1.  **Control Plane (`/sys/class/devu/`)**: Exposes sysfs attributes to manipulate system-wide behavior (e.g., triggering panics, syncing filesystems).
2.  **Data Plane (`/dev/`)**: Exposes character device nodes for high-speed I/O operations and memory-mapped access.
3.  **Core API**: A custom abstraction layer (`include/devu/sys.h`) providing simplified registration for classes and device nodes.

## Interfaces

### Control Interface (`/sys/class/devu/devu/call`)
Multiplexed interface for executing kernel-side actions.
Format: `ID [MESSAGE]`
- `1`: Panic system with message.
- `2`: Log message via `printk`.
- `3`: Sync filesystems.

### Memory Interface (`/dev/unlkmem`)
Provides physical memory access.
- **IOCTL**: `DEVU_MEM_RESIZE` configures the physical memory range (`mem_t`).
- **Read/Write/Mmap**: Access the configured physical address range.

### Tracing Interface (`/dev/ktraces`)
Runtime kernel instrumentation using kprobes.
- **IOCTL**: `KTRACE_FUNC_FILTER` attaches dynamic probes to kernel functions.

### IPC Interface (`/dev/ipchub`)
Inter-Process Communication Hub.
- **Type**: `miscdevice`
- **Purpose**: Provides a high-speed, kernel-resident communication channel between processes.
