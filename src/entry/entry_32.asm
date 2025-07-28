section .asm

global _entry_isr80h_32

extern isr80h_handler

_entry_isr80h_32:
    pushad
    push esp
    call isr80h_handler
    add esp, 4
    mov [esp+28], eax
    popad
    iretd