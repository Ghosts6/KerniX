# KerniX build

KERNEL    := kernix
BUILD     := build
ELF       := $(BUILD)/$(KERNEL).elf
LDSCRIPT  := arch/x86/linker.ld
SRC_DIRS  := arch kernel mm sched sync drivers lib

# Toolchain: prefer the i686-elf cross compiler, fall back to host gcc -m32 on Linux
ifneq ($(shell command -v i686-elf-gcc 2>/dev/null),)
CC        := i686-elf-gcc
ARCHFLAGS :=
LDARCH    :=
else ifeq ($(shell uname -s),Linux)
CC        := gcc
ARCHFLAGS := -m32 -fno-pie -fno-stack-protector
LDARCH    := -m32 -no-pie -Wl,--build-id=none
else
TOOLCHAIN_ERROR := i686-elf-gcc not found. Run ./scripts/install-toolchain.sh or use 'make docker'
endif

AS        := nasm
QEMU      := qemu-system-i386
HOSTCC    := $(shell command -v cc || command -v gcc)

CFLAGS    := -std=c11 -ffreestanding -O2 -g -Wall -Wextra -Werror -Iinclude $(ARCHFLAGS)
ASFLAGS   := -f elf32 -g -F dwarf
LDFLAGS   := -T $(LDSCRIPT) -ffreestanding -nostdlib -Wl,-z,noexecstack $(LDARCH)
LDLIBS    := -lgcc
# -display none so isa-debug-exit can terminate QEMU (macOS cocoa keeps the process alive).
# Override with: make run QEMUDISPLAY=cocoa
QEMUDISPLAY ?= none
QEMUFLAGS := -kernel $(ELF) -serial stdio -display $(QEMUDISPLAY) -no-reboot \
             -device isa-debug-exit,iobase=0xf4,iosize=0x04

C_SRCS    := $(shell find $(SRC_DIRS) -name '*.c' 2>/dev/null)
ASM_SRCS  := $(shell find $(SRC_DIRS) -name '*.asm' 2>/dev/null)
OBJS      := $(C_SRCS:%.c=$(BUILD)/%.o) $(ASM_SRCS:%.asm=$(BUILD)/%.asm.o)
DEPS      := $(OBJS:.o=.d)

# Host unit tests (normal hosted programs, not freestanding).
# Platform tag avoids Exec format errors when switching between host and Docker.
HOST_ID        := $(shell uname -s)-$(shell uname -m)
HOST_TEST_SRCS := $(shell find tests/host -name '*.c' 2>/dev/null)
HOST_TEST_BINS := $(HOST_TEST_SRCS:tests/host/%.c=$(BUILD)/tests/host/$(HOST_ID)/%)
HOSTCFLAGS     := -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude

DOCKER_IMAGE := kernix-build

.PHONY: all run debug clean test test-host test-qemu docker-build docker help toolchain

all: $(ELF)

toolchain:
	@if [ -n "$(TOOLCHAIN_ERROR)" ]; then echo "$(TOOLCHAIN_ERROR)" >&2; exit 1; fi

$(OBJS): | toolchain

$(ELF): $(OBJS) $(LDSCRIPT)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/%.asm.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -MD $(BUILD)/$*.asm.d $< -o $@

$(BUILD)/tests/host/$(HOST_ID)/%: tests/host/%.c
	@mkdir -p $(dir $@)
	$(HOSTCC) $(HOSTCFLAGS) $< -o $@

$(BUILD)/tests/host/$(HOST_ID)/pmm_test: tests/host/pmm_test.c mm/pmm.c lib/string.c arch/x86/multiboot.c
	@mkdir -p $(dir $@)
	# -fno-builtin: avoid GCC rewriting our memset/strlen into recursive calls.
	$(HOSTCC) $(HOSTCFLAGS) -fno-builtin -DKERNIX_HOST_TEST $^ -o $@

$(BUILD)/tests/host/$(HOST_ID)/heap_test: tests/host/heap_test.c mm/heap.c
	@mkdir -p $(dir $@)
	$(HOSTCC) $(HOSTCFLAGS) $^ -o $@

run: $(ELF)
	@$(QEMU) $(QEMUFLAGS); \
	ec=$$?; \
	if [ $$ec -eq 1 ]; then exit 0; fi; \
	exit $$ec

debug: $(ELF)
	@echo "QEMU paused, GDB stub on localhost:1234"
	@echo "Attach with: gdb $(ELF) -ex 'target remote localhost:1234'"
	$(QEMU) $(QEMUFLAGS) -s -S

test: test-host test-qemu

test-host: $(HOST_TEST_BINS)
	@if [ -z "$(HOST_TEST_BINS)" ]; then echo "no host tests found" >&2; exit 1; fi
	@status=0; \
	for t in $(HOST_TEST_BINS); do \
		echo "== $$t =="; \
		"$$t" || status=1; \
	done; \
	exit $$status

test-qemu: $(ELF)
	./scripts/test-qemu.sh $(ELF)

clean:
	rm -rf $(BUILD)

docker-build:
	docker build -t $(DOCKER_IMAGE) .

docker:
	docker run --rm -v "$(CURDIR)":/src -u "$$(id -u):$$(id -g)" $(DOCKER_IMAGE) make

help:
	@echo "all           build $(ELF)"
	@echo "run           boot the kernel in QEMU (serial banner; QEMUDISPLAY=cocoa for GUI)"
	@echo "debug         boot paused with GDB stub on :1234"
	@echo "test          run host and QEMU boot tests"
	@echo "test-host     run host unit tests"
	@echo "test-qemu     boot in QEMU, check serial banner and exit code"
	@echo "clean         remove $(BUILD)/"
	@echo "docker-build  build the toolchain image"
	@echo "docker        run make inside the toolchain image"

-include $(DEPS)
