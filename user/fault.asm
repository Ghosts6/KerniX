; User program that page-faults on a null deref (killed by the kernel).
BITS 32
ORG 0x08048000

_start:
    xor eax, eax
    mov dword [eax], 1
    ; should never reach here
    jmp $
