; Multiboot entry: enable paging with a higher-half map, then call kmain.

MB_MAGIC    equ 0x1BADB002
MB_ALIGN    equ 1 << 0                  ; align modules on page boundaries
MB_MEMINFO  equ 1 << 1                  ; ask for a memory map
MB_FLAGS    equ MB_ALIGN | MB_MEMINFO
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

KERNEL_VIRT_BASE   equ 0xC0000000
STACK_SIZE         equ 16384
EARLY_MAP_MB       equ 16
PAGE_PRESENT_RW_4M equ 0x83             ; present | writable | 4 MiB page

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

align 4096
boot_page_directory:
    resb 4096

section .text
global _start
extern kmain

; Physical entry (Multiboot sets EIP to start = _start - KERNEL_VIRT_BASE).
; eax = multiboot magic, ebx = multiboot info physical address.
_start:
    cli

    ; Preserve Multiboot registers across page-directory setup.
    mov esi, eax
    mov edi, ebx

    mov ebx, (boot_page_directory - KERNEL_VIRT_BASE)
    xor eax, eax
    mov ecx, 1024
.clear_pd:
    mov [ebx], eax
    add ebx, 4
    loop .clear_pd

    mov ebx, (boot_page_directory - KERNEL_VIRT_BASE)

    ; Identity-map and higher-half-map the first EARLY_MAP_MB of RAM (4 MiB pages).
    xor edx, edx                        ; physical base
    xor ecx, ecx                        ; identity PDE index
    mov ebp, (KERNEL_VIRT_BASE >> 22)   ; higher-half PDE index
    mov eax, (EARLY_MAP_MB / 4)
.fill_pde:
    mov dword [ebx + ecx * 4], edx
    or dword [ebx + ecx * 4], PAGE_PRESENT_RW_4M
    mov dword [ebx + ebp * 4], edx
    or dword [ebx + ebp * 4], PAGE_PRESENT_RW_4M
    add edx, 0x400000
    inc ecx
    inc ebp
    dec eax
    jnz .fill_pde

    mov eax, (boot_page_directory - KERNEL_VIRT_BASE)
    mov cr3, eax

    mov eax, cr4
    or eax, 0x00000010                  ; CR4.PSE
    mov cr4, eax

    mov eax, cr0
    or eax, 0x80000000                  ; CR0.PG
    mov cr0, eax

    lea ecx, [.higher_half]
    jmp ecx

.higher_half:
    mov esp, stack_top
    xor ebp, ebp
    push edi                            ; multiboot info (phys)
    push esi                            ; multiboot magic
    call kmain

    cli
.hang:
    hlt
    jmp .hang

; Mark stack non-executable for modern linkers (gcc -m32).
section .note.GNU-stack noalloc noexec nowrite progbits
