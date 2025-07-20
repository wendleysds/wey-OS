section .asm

global _set_resgisters_segments

_set_resgisters_segments:
    push ebp
    mov ebp, esp
    mov ax, word [ebp+8]
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    pop ebp
    ret
