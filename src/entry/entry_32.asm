section .asm

global _entry_isr80h_32

extern _isr80h_handler

_entry_isr80h_32:
    pushad
    iretd

section .data
sys_status: db 0