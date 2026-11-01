; Flat user binary: fork, parent and child each write a line, then exit.
BITS 32
ORG 0x08048000

SYS_EXIT  equ 0
SYS_WRITE equ 1
SYS_FORK  equ 4

_start:
    mov eax, SYS_FORK
    int 0x80
    test eax, eax
    jz .child

    mov ecx, parent_msg
    mov edx, parent_len
    jmp .write

.child:
    mov ecx, child_msg
    mov edx, child_len

.write:
    mov eax, SYS_WRITE
    mov ebx, 1
    int 0x80

    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

parent_msg:
    db "fork parent", 10
parent_len equ $ - parent_msg

child_msg:
    db "fork child", 10
child_len equ $ - child_msg
