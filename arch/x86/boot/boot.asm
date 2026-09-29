; Multiboot entry point: set up a stack and jump to kmain.

MB_MAGIC    equ 0x1BADB002
MB_ALIGN    equ 1 << 0                  ; align modules on page boundaries
MB_MEMINFO  equ 1 << 1                  ; ask for a memory map
MB_FLAGS    equ MB_ALIGN | MB_MEMINFO
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

STACK_SIZE  equ 16384

section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

section .bss
align 16
stack_bottom:
    resb STACK_SIZE
stack_top:

section .text
global _start
extern kmain

_start:
    cli
    mov esp, stack_top
    xor ebp, ebp
    push ebx                            ; multiboot info pointer
    push eax                            ; multiboot magic
    call kmain

    cli
.hang:
    hlt
    jmp .hang

; Mark stack non-executable for modern linkers (gcc -m32).
section .note.GNU-stack noalloc noexec nowrite progbits
