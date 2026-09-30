# --- Configuration ---
KERNEL_VERSION ?= $(shell uname -r)
KDIR           ?= /lib/modules/$(KERNEL_VERSION)/build
PWD            := $(shell pwd)

PREFIX         ?= /usr/local

ccflags-y      := -I$(PWD)/include

DRIVER_SRCS    := $(shell find src/driver -name '*.c')
DRIVER_OBJS    := $(DRIVER_SRCS:.c=.o)

obj-m          += devu.o
devu-objs      := $(DRIVER_OBJS)

CC             := gcc
CFLAGS         := -Wall -Wextra -O2 -Iinclude

.PHONY: all clean install

all: $(TOOL_EXE)
	@echo "=== Building Kernel Module ==="
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@echo "=== Moving Module to Destination ==="
	@mkdir -p dri
	@mv devu.ko dri/devu 2>/dev/null || mv src/driver/devu.ko dri/devu 2>/dev/null || true

install: all
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a
	@echo "Installation complete. Run 'modprobe devu' to load the driver."

clean:
	@echo "=== Cleaning Build Artifacts ==="
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf dri/
	find src/ -name '*.o' -o -name '*.ko' -o -name '.*.cmd' -o -name '*.mod.c' -o -name '*.mod' -exec rm -f {} +