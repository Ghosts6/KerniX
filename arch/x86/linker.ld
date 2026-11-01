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

    .init_array : AT(ADDR(.init_array) - KERNEL_VIRT_BASE)
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array EXCLUDE_FILE(*crtbegin.o *crtbegin?.o) .ctors))
        PROVIDE_HIDDEN(__init_array_end = .);
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

    start = _start - KERNEL_VIRT_BASE;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame*)
    }
}
