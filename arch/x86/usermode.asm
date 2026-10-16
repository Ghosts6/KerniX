; Enter ring 3 via iret. Never returns.
; void enter_usermode(uint32_t entry, uint32_t user_stack);
global enter_usermode
enter_usermode:
    cli
    mov eax, [esp + 4]          ; entry (eip)
    mov ecx, [esp + 8]          ; user stack

    mov dx, 0x23                ; user data | RPL3
    mov ds, dx
    mov es, dx
    mov fs, dx
    mov gs, dx

    push dword 0x23             ; ss
    push ecx                    ; esp
    pushf
    pop edx
    or edx, 0x200               ; IF
    push edx                    ; eflags
    push dword 0x1B             ; cs (user code | RPL3)
    push eax                    ; eip
    iret

section .note.GNU-stack noalloc noexec nowrite progbits
