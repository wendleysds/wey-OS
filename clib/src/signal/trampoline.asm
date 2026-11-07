bits 32

global __sigreturn_trampoline

__sigreturn_trampoline:
    mov eax, 34
    int 0x80
    hlt
