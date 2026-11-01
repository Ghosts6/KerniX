; Resume a forked child: esp points at an interrupt_frame_t.
global fork_return
fork_return:
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8              ; int_no, err_code
    iret

section .note.GNU-stack noalloc noexec nowrite progbits
