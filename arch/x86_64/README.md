# x86_64 port (Stage 8 skeleton)

Build a separate long-mode kernel that is **not** the default CI target.

Needs an ELF64 toolchain (`x86_64-elf-gcc`, or Linux `gcc -m64`):

```sh
make x86_64
make run-x86_64
```

On macOS without `x86_64-elf-gcc`, use Docker: `make docker` does not build this by
default; run `docker … make x86_64` on an image with `gcc-multilib` / `qemu-system-x86`.

What this skeleton does:
- Multiboot1 entry in 32-bit protected mode
- Sets up 4-level paging (PML4 → PDPT → PD) with 2 MiB pages
- Enters long mode and prints a serial banner

What it does **not** yet share with the i386 kernel: scheduler, user mode, syscalls.
The default `make test` path stays `qemu-system-i386`.
