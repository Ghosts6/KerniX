; Flat user binary loaded at USER_CODE_BASE (0x08048000).
; Syscalls: eax=num, ebx/ecx/edx=args; int 0x80.
BITS 32
ORG 0x08048000

SYS_EXIT   equ 0
SYS_WRITE  equ 1
SYS_GETPID equ 3

_start:
    mov eax, SYS_GETPID
    int 0x80

    mov eax, SYS_WRITE
    mov ebx, 1
    mov ecx, msg
    mov edx, msg_len
    int 0x80

    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

msg:
    db "hello from user", 10
msg_len equ $ - msg
