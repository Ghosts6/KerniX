# KerniX

A small, educational x86 kernel written in C and Assembly to practice bare-metal
systems programming: boot, interrupts, memory management, scheduling and
synchronization. No GUI, no userland distribution, just the core.

## Features (planned)
- **Boot:** Multiboot kernel loaded directly by QEMU.
- **Interrupts:** GDT, IDT, exception handlers and PIT timer.
- **Memory:** Physical frame allocator, paging and a kernel heap.
- **Scheduling:** Kernel threads, context switching and pluggable policies (FCFS, SJF, Priority, RR).
- **Synchronization:** Spinlocks, mutexes and semaphores.

## Tech Stack
- **Languages:** C, x86 Assembly (NASM), Bash, C++ (later)
- **Target:** i386 first, x86_64 later
- **Toolchain:** i686-elf-gcc (or `gcc -m32` on Linux), NASM, GNU Make, QEMU, GDB

## Layout
```
arch/x86/     boot entry, linker script, CPU specific code
kernel/       kernel main and core init
mm/           memory management
sched/        tasks and scheduler
sync/         synchronization primitives
drivers/      serial, VGA, timer, keyboard
lib/          freestanding library code
include/      kernel headers
tests/        host unit tests and QEMU boot tests
scripts/      helper scripts
Resource/     course projects (EECS3221)
```

## Getting Started

Install the toolchain (macOS with Homebrew, or Debian/Ubuntu with apt):
```sh
./scripts/install-toolchain.sh
```

Build and run:
```sh
make          # build build/kernix.elf
make run      # boot in QEMU, serial on stdio
make debug    # boot in QEMU paused, GDB stub on :1234
make test     # host unit tests + Multiboot/QEMU smoke
make clean
```

Or build inside Docker, no local toolchain needed:
```sh
make docker-build   # build the image once
make docker         # run make inside the container
```

## License
MIT, see [LICENSE](LICENSE).
