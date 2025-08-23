[BITS 32]

global _start

_start:
	mov eax, 1
	mov ebx, 2
	mov ecx, 3
	mov edx, 4
	int 0x80 ; syscall to isr80h_handler

	jmp $