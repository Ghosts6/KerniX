# KerniX build

KERNEL     := kernix
BUILD      := build
ELF        := $(BUILD)/$(KERNEL).elf
USER_HELLO := $(BUILD)/user/hello.bin
USER_FORK  := $(BUILD)/user/fork.bin
LDSCRIPT   := arch/x86/linker.ld
SRC_DIRS   := arch/x86 kernel mm sched sync drivers lib

# Toolchain: prefer the i686-elf cross compiler, fall back to host gcc -m32 on Linux
ifneq ($(shell command -v i686-elf-gcc 2>/dev/null),)
CC        := i686-elf-gcc
CXX       := i686-elf-g++
ARCHFLAGS :=
LDARCH    :=
else ifeq ($(shell uname -s),Linux)
CC        := gcc
CXX       := g++
ARCHFLAGS := -m32 -fno-pie -fno-stack-protector
LDARCH    := -m32 -no-pie -Wl,--build-id=none
else
TOOLCHAIN_ERROR := i686-elf-gcc not found. Run ./scripts/install-toolchain.sh or use 'make docker'
endif

AS        := nasm
QEMU      := qemu-system-i386
HOSTCC    := $(shell command -v cc || command -v gcc)

CFLAGS    := -std=c11 -ffreestanding -O2 -g -Wall -Wextra -Werror -Iinclude $(ARCHFLAGS)
CXXFLAGS  := -std=c++17 -ffreestanding -O2 -g -Wall -Wextra -Werror -fno-exceptions \
             -fno-rtti -fno-use-cxa-atexit -Iinclude $(ARCHFLAGS)
ASFLAGS   := -f elf32 -g -F dwarf
LDFLAGS   := -T $(LDSCRIPT) -ffreestanding -nostdlib -Wl,-z,noexecstack $(LDARCH)
LDLIBS    := -lgcc

QEMUDISPLAY ?= none
QEMUFLAGS := -kernel $(ELF) -initrd $(USER_HELLO) -serial stdio -display $(QEMUDISPLAY) \
             -no-reboot -device isa-debug-exit,iobase=0xf4,iosize=0x04

C_SRCS    := $(shell find $(SRC_DIRS) -name '*.c' 2>/dev/null)
CXX_SRCS  := $(shell find $(SRC_DIRS) -name '*.cpp' 2>/dev/null)
ASM_SRCS  := $(shell find $(SRC_DIRS) -name '*.asm' 2>/dev/null)
OBJS      := $(C_SRCS:%.c=$(BUILD)/%.o) $(CXX_SRCS:%.cpp=$(BUILD)/%.o) \
             $(ASM_SRCS:%.asm=$(BUILD)/%.asm.o) $(BUILD)/user/fork_image.o
DEPS      := $(OBJS:.o=.d)

HOST_ID        := $(shell uname -s)-$(shell uname -m)
HOST_TEST_SRCS := $(shell find tests/host -name '*.c' 2>/dev/null)
HOST_TEST_BINS := $(HOST_TEST_SRCS:tests/host/%.c=$(BUILD)/tests/host/$(HOST_ID)/%)
HOSTCFLAGS     := -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude

DOCKER_IMAGE := kernix-build

# Optional x86_64 long-mode skeleton (not part of default test).
X64_ELF      := $(BUILD)/kernix64.elf
X64_LDSCRIPT := arch/x86_64/linker.ld
X64_AS       := nasm
QEMU64       := qemu-system-x86_64
# Prefer a real ELF64 toolchain; Linux gcc -m64 also works. macOS needs x86_64-elf-*.
ifneq ($(shell command -v x86_64-elf-gcc 2>/dev/null),)
X64_HAVE     := 1
X64_CC       := x86_64-elf-gcc
X64_CFLAGS   := -ffreestanding -O2 -g -Wall -Wextra -Werror -mcmodel=large -fno-pic
X64_LD       := x86_64-elf-gcc
X64_LDFLAGS  := -T $(X64_LDSCRIPT) -ffreestanding -nostdlib -no-pie
else ifeq ($(shell uname -s),Linux)
X64_HAVE     := 1
X64_CC       := gcc
X64_CFLAGS   := -m64 -ffreestanding -O2 -g -Wall -Wextra -Werror -mcmodel=large -fno-pic \
                -fno-stack-protector -fno-pie
X64_LD       := gcc
X64_LDFLAGS  := -m64 -T $(X64_LDSCRIPT) -ffreestanding -nostdlib -no-pie
else
X64_HAVE     := 0
endif

.PHONY: all run debug clean test test-host test-qemu docker-build docker help toolchain \
        x86_64 run-x86_64

all: $(ELF) $(USER_HELLO)

$(USER_HELLO): user/hello.asm
	@mkdir -p $(dir $@)
	$(AS) -f bin $< -o $@

$(USER_FORK): user/fork.asm
	@mkdir -p $(dir $@)
	$(AS) -f bin $< -o $@

$(BUILD)/user/fork_image.c: $(USER_FORK) scripts/embed-bin.py
	@mkdir -p $(dir $@)
	python3 scripts/embed-bin.py $(USER_FORK) fork_image $@

toolchain:
	@if [ -n "$(TOOLCHAIN_ERROR)" ]; then echo "$(TOOLCHAIN_ERROR)" >&2; exit 1; fi

$(OBJS): | toolchain

$(ELF): $(OBJS) $(LDSCRIPT)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/user/fork_image.o: $(BUILD)/user/fork_image.c
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
	$(HOSTCC) $(HOSTCFLAGS) -fno-builtin -DKERNIX_HOST_TEST $^ -o $@

$(BUILD)/tests/host/$(HOST_ID)/heap_test: tests/host/heap_test.c mm/heap.c
	@mkdir -p $(dir $@)
	$(HOSTCC) $(HOSTCFLAGS) -DKERNIX_HOST_TEST $^ -o $@

$(BUILD)/tests/host/$(HOST_ID)/sync_test: tests/host/sync_test.c
	@mkdir -p $(dir $@)
	$(HOSTCC) $(HOSTCFLAGS) -DKERNIX_HOST_TEST $^ -o $@

$(BUILD)/tests/host/$(HOST_ID)/sched_test: tests/host/sched_test.c sched/policy.c sched/sim.c
	@mkdir -p $(dir $@)
	$(HOSTCC) $(HOSTCFLAGS) $^ -o $@

$(BUILD)/tests/host/$(HOST_ID)/slab_test: tests/host/slab_test.c mm/slab.c
	@mkdir -p $(dir $@)
	$(HOSTCC) $(HOSTCFLAGS) -DKERNIX_HOST_TEST $^ -o $@

run: $(ELF) $(USER_HELLO)
	@$(QEMU) $(QEMUFLAGS); \
	ec=$$?; \
	if [ $$ec -eq 1 ]; then exit 0; fi; \
	exit $$ec

debug: $(ELF) $(USER_HELLO)
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

test-qemu: $(ELF) $(USER_HELLO)
	./scripts/test-qemu.sh $(ELF) $(USER_HELLO)

x86_64:
ifeq ($(X64_HAVE),1)
	@$(MAKE) $(X64_ELF)
else
	@echo "x86_64: install x86_64-elf-gcc (or build on Linux with gcc -m64)." >&2
	@echo "        i386 'make test' does not require this target." >&2
	@exit 1
endif

$(X64_ELF): arch/x86_64/boot.asm arch/x86_64/kmain64.c $(X64_LDSCRIPT)
	@mkdir -p $(BUILD)/arch/x86_64
	$(X64_AS) -f elf64 arch/x86_64/boot.asm -o $(BUILD)/arch/x86_64/boot.asm.o
	$(X64_CC) $(X64_CFLAGS) -c arch/x86_64/kmain64.c -o $(BUILD)/arch/x86_64/kmain64.o
	$(X64_LD) $(X64_LDFLAGS) -o $@ $(BUILD)/arch/x86_64/boot.asm.o \
		$(BUILD)/arch/x86_64/kmain64.o

run-x86_64: x86_64
	@$(QEMU64) -kernel $(X64_ELF) -serial stdio -display none -no-reboot \
		-device isa-debug-exit,iobase=0xf4,iosize=0x04; \
	ec=$$?; \
	if [ $$ec -eq 1 ]; then exit 0; fi; \
	exit $$ec

clean:
	rm -rf $(BUILD)

docker-build:
	docker build -t $(DOCKER_IMAGE) .

docker:
	docker run --rm -v "$(CURDIR)":/src -u "$$(id -u):$$(id -g)" $(DOCKER_IMAGE) make

help:
	@echo "all           build $(ELF) and user hello"
	@echo "run           boot the kernel in QEMU"
	@echo "debug         boot paused with GDB stub on :1234"
	@echo "test          run host and QEMU boot tests"
	@echo "x86_64        build long-mode skeleton $(X64_ELF)"
	@echo "run-x86_64    boot the x86_64 skeleton in qemu-system-x86_64"
	@echo "clean         remove $(BUILD)/"

-include $(DEPS)
