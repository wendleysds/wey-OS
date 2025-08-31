section .text
    global _start

msg db 'Hello, World!', 0xA
    len equ $ - msg

_start:
    mov eax, 1
    mov ebx, msg
    mov ecx, len
    int 0x80
	
	jmp $
