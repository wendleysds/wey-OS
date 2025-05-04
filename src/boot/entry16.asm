[BITS 16]

global _start

extern main

_start:
	call main
	hlt
	jmp $
