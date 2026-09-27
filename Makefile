# --- Configuration ---
KERNEL_VERSION ?= $(shell uname -r)
KDIR           ?= /lib/modules/$(KERNEL_VERSION)/build
PWD            := $(shell pwd)

PREFIX         ?= /usr/local
BINDIR         := $(PREFIX)/bin

DRIVER_SRCS    := $(shell find src/driver -name '*.c')
DRIVER_OBJS    := $(DRIVER_SRCS:.c=.o)

obj-m          += devu.o
devu-objs      := $(patsubst src/driver/%,src/driver/%,$(DRIVER_OBJS))

TOOL_SRCS      := $(shell find src/tool -name '*.c')
TOOL_EXE       := bin/devuctl
CC             := gcc
CFLAGS         := -Wall -Wextra -O2

.PHONY: all clean install

all: $(TOOL_EXE)
	@echo "=== Building Kernel Module ==="
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@echo "=== Moving Module to Destination ==="
	@mkdir -p dri
	@mv devu.ko dri/devu 2>/dev/null || mv src/driver/devu.ko dri/devu 2>/dev/null || true

$(TOOL_EXE): $(TOOL_SRCS)
	@echo "=== Building User Space Tool ==="
	@mkdir -p bin
	$(CC) $(CFLAGS) $(TOOL_SRCS) -o $@

install: all
	install -m 755 $(TOOL_EXE) $(BINDIR)/devuctl
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a
	@echo "Installation complete. Run 'modprobe devu' to load the driver."

clean:
	@echo "=== Cleaning Build Artifacts ==="
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf bin/ dri/
	find src/ -name '*.o' -o -name '*.ko' -o -name '.*.cmd' -o -name '*.mod.c' -o -name '*.mod' -exec rm -f {} +