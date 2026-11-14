bits 32

global _start

extern main
extern exit

_start:
	xor ebp, ebp

	mov ecx, [esp]               ; ECX = argc
	lea edx, [esp + 4]           ; EDX = argv
	lea esi, [edx + ecx * 4 + 4] ; ESI = envp

	and esp, -16

	push esi
	push edx
	push ecx

	call main

	push eax
	call exit

	pop ebx
	mov eax, 1
	int 0x80
