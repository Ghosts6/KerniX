; Minimal Multiboot → long mode skeleton (identity-mapped low memory).
MB_MAGIC    equ 0x1BADB002
MB_FLAGS    equ 3
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

section .bss
align 4096
pml4:
    resb 4096
pdpt:
    resb 4096
pd:
    resb 4096
align 16
stack64:
    resb 16384
stack64_top:

section .data
align 16
gdt64:
    dq 0
    dq 0x00AF9A000000FFFF   ; 64-bit code
    dq 0x00AF92000000FFFF   ; 64-bit data
gdt64_end:
gdt64_ptr:
    dw gdt64_end - gdt64 - 1
    dd gdt64
    dd 0

section .text
bits 32
global _start
extern kmain64

_start:
    cli
    mov esp, stack64_top

    ; Clear paging structures.
    mov edi, pml4
    xor eax, eax
    mov ecx, (4096 * 3) / 4
    rep stosd

    mov eax, pdpt
    or eax, 0x03
    mov [pml4], eax

    mov eax, pd
    or eax, 0x03
    mov [pdpt], eax

    ; 512 x 2 MiB = 1 GiB identity map.
    mov edi, pd
    mov eax, 0x83
    mov ecx, 512
.fill_pd:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .fill_pd

    mov eax, pml4
    mov cr3, eax

    mov eax, cr4
    or eax, (1 << 5)            ; PAE
    mov cr4, eax

    mov ecx, 0xC0000080         ; IA32_EFER
    rdmsr
    or eax, (1 << 8)            ; LME
    wrmsr

    mov eax, cr0
    or eax, 0x80000001          ; PG | PE
    mov cr0, eax

    lgdt [gdt64_ptr]
    jmp 0x08:long_mode

bits 64
long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, stack64_top
    call kmain64
.hang:
    hlt
    jmp .hang

section .note.GNU-stack noalloc noexec nowrite progbits
