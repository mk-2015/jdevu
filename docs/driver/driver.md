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

### I/O Port Interface (`/dev/unlkport`)
Raw CPU I/O port access.
- **Type**: `miscdevice`
- **IOCTL**:
  - `PORT_CMD_PORT_READ`/`WRITE`: Read/Write a byte/word/long from a port.
  - `PORT_CMD_BLOCK_READ`/`WRITE`: Read/Write blocks of data using `ins`/`outs` instructions.
- **Purpose**: Provides root-level access to CPU I/O ports for hardware debugging.

### PCI Descriptor Interface (`/dev/kpcidescv`)
PCI device inspection.
- **Type**: `miscdevice`
- **IOCTL**:
  - `PCIDEVC_GET_SIZE`: Get number of PCI devices.
  - `PCIDEVC_GET_DEVICES`: Fetch array of device names and base addresses.
- **Purpose**: Inspects system PCI device topology and resources.

### USB Descriptor Interface (`/dev/kusbdescv`)
USB device inspection.
- **Type**: `miscdevice`
- **IOCTL**:
  - `USBDEVC_GET_SIZE`: Get number of USB devices.
  - `USBDEVC_GET_DEVICES`: Fetch array of device IDs and locations.
- **Purpose**: Inspects system USB device topology.
