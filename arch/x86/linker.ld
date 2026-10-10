/* Higher-half kernel: VMA at 0xC0000000 + phys, LMA at physical load address. */

ENTRY(start)

KERNEL_VIRT_BASE = 0xC0000000;

SECTIONS
{
    . = KERNEL_VIRT_BASE + 1M;

    kernel_start = .;

    .text BLOCK(4K) : AT(ADDR(.text) - KERNEL_VIRT_BASE)
    {
        KEEP(*(.multiboot))
        *(.text*)
    }

    .rodata BLOCK(4K) : AT(ADDR(.rodata) - KERNEL_VIRT_BASE)
    {
        *(.rodata*)
    }

    .data BLOCK(4K) : AT(ADDR(.data) - KERNEL_VIRT_BASE)
    {
        *(.data*)
    }

    .bss BLOCK(4K) : AT(ADDR(.bss) - KERNEL_VIRT_BASE)
    {
        *(COMMON)
        *(.bss*)
    }

    kernel_end = .;

    /* Multiboot/QEMU jumps to the physical address of the entry point. */
    start = _start - KERNEL_VIRT_BASE;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame*)
    }
}
