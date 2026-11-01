# KerniX

[![CI](https://github.com/Ghosts6/KerniX/actions/workflows/ci.yml/badge.svg)](https://github.com/Ghosts6/KerniX/actions/workflows/ci.yml)

KerniX is a small, educational 32-bit x86 kernel written in C and NASM
assembly. It boots in QEMU and implements the core of an operating system from
scratch: interrupts, physical and virtual memory, preemptive scheduling,
synchronization, and a minimal ring-3 user mode with system calls. There is no
GUI or full userland distribution; the goal is a kernel that is small enough to
read end to end.

It is a good fit if you want to see how these pieces work on real (emulated)
hardware, step through them in GDB, or compare scheduling algorithms on live
threads. The scheduler and memory code build on the EECS3221 course projects
in `Resource/EECS3221` (CPU scheduling and MMU).

## Demo

Coming soon: a video walkthrough of building, booting and testing KerniX.

## Features

- **Boot:** Multiboot 1 kernel loaded directly by `qemu-system-i386 -kernel`.
  Output goes to both the COM1 serial port and VGA text mode.
- **CPU and interrupts:** GDT, IDT, exception handlers that dump registers
  (including decoded page faults), remapped PIC and a 100 Hz PIT timer.
- **Memory:** bitmap physical frame allocator (with frame refcounts for COW),
  4 KiB paging with the kernel mapped at `0xC0000000`, a free-list `kmalloc` /
  `kfree` heap, and a slab allocator for fixed-size objects.
- **Scheduling:** kernel threads with create, exit, yield and sleep, an
  assembly context switch, an idle task and timer preemption. Policies plug
  into one scheduler core: FCFS, SJF, Priority, RR and Priority-RR, ported from
  the course project. Each run prints course-style waiting, turnaround and
  response times.
- **Synchronization:** spinlocks, wait queues, mutexes and semaphores; the
  heap is IRQ-safe under preemption.
- **User mode:** TSS and ring-3 segments, per-process page directories, `int
  0x80` syscalls (`write`, `exit`, `yield`, `getpid`, `fork` with copy-on-write).
  A flat Multiboot module (`-initrd build/user/hello.bin`) prints via `write`
  and exits; user faults kill the process instead of panicking the kernel.
- **C++ / SMP / x86_64:** freestanding C++ (`new`/`delete`, global constructors),
  APIC detect in UP mode, and an optional long-mode skeleton (`make x86_64`).

## Getting Started

Install the toolchain (Homebrew on macOS, apt on Debian/Ubuntu):

```sh
./scripts/install-toolchain.sh
```

Build, run and test:

```sh
make          # build build/kernix.elf
make run      # boot in QEMU, serial on stdio
make debug    # boot paused with a GDB stub on :1234
make test     # host unit tests, then QEMU boot tests
make clean
```

Or use Docker, with no local cross compiler needed:

```sh
make docker-build   # build the toolchain image once
make docker         # run make inside the container
```

### Boot options

Options are passed on the kernel command line, for example
`qemu-system-i386 -kernel build/kernix.elf -serial stdio -append "sched=sjf"`.

| Option           | Effect                                                           |
|------------------|------------------------------------------------------------------|
| `sched=<policy>` | `fcfs`, `sjf`, `priority`, `rr` (default) or `priority_rr`       |
| `fault=de`       | Trigger a divide error to show exception reporting               |
| `fault=bp`       | Trigger a breakpoint                                             |
| `fault=pf`       | Write to an unmapped address to show the page-fault report      |

Example output with the default policy (trimmed):

```
KerniX boot OK
kernel at c0100000-c0151000 (higher half)
paging: 4K tables active, mapped low 16 MiB + higher half
heap: 256 KiB ready
sched: policy=rr quantum=2 ticks
Running task = [T1] [4] [4] for 2 units.
...
Average waiting time = 12.50
Average turnaround time = 16.00
Average response time = 5.00
sched demo ok policy=rr slices=12
```

### Debugging

`make debug` starts QEMU paused with a GDB stub. In a second terminal:

```sh
gdb build/kernix.elf -ex 'target remote localhost:1234'
(gdb) break kmain
(gdb) continue
```

On macOS, use `i686-elf-gdb` if your system `gdb` cannot read 32-bit ELF files.

## Testing

- **Host tests** (`tests/host/`): the frame allocator, the heap and the
  scheduling policies are compiled as normal programs and checked against
  known results.
- **QEMU tests** (`scripts/test-qemu.sh`): boot the kernel, check serial
  output and the exit code from QEMU's `isa-debug-exit` device. They cover
  paging, the heap, each fault type and every scheduling policy. For each
  policy, real kernel threads must run in the same order as the simulator
  predicts and produce the same statistics.

CI runs both suites in the Docker image on every push and pull request.

## Layout

```
arch/x86/     boot entry, linker script, GDT/IDT, ISRs, context switch
kernel/       kmain and panic
mm/           frame allocator, paging, kernel heap
sched/        tasks, scheduler core, policies, simulator, self-test
drivers/      serial, VGA, PIT, QEMU exit
lib/          freestanding string and kprintf
include/      kernel headers
tests/host/   host unit tests
scripts/      toolchain install and QEMU test scripts
Resource/     course projects (EECS3221)
```

## License

MIT, see [LICENSE](LICENSE).
