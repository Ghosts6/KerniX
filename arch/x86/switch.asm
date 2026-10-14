; void context_switch(uint32_t *old_esp, uint32_t new_esp)
; Saves callee-saved registers on the current stack, stores ESP in *old_esp,
; then loads new_esp and restores the registers saved there.

section .text
global context_switch

context_switch:
    mov eax, [esp + 4]
    mov edx, [esp + 8]

    push ebp
    push ebx
    push esi
    push edi

    mov [eax], esp
    mov esp, edx

    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
