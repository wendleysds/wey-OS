[BITS 32]

global _start
global kernel_registers

extern kmain

_start:
	call kmain

	hlt
	jmp $

kernel_registers:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    ret
