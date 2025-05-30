[BITS 16]

global _entry16
extern main

_entry16:
	mov al, byte 'H'
	mov ah, 0x0E
	int 0x10

	hlt
	jmp $

	;jmp main
