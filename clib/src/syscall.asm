bits 32

global do_syscall

extern errno ; int

; long do_syscall(long no, ...)
do_syscall:
	push ebp
	push edi
	push esi
	push ebx

	mov eax, [esp+20]  ; syscall number
	mov ebx, [esp+24]  ; arg1
	mov ecx, [esp+28]  ; arg2
	mov edx, [esp+32]  ; arg3
	mov esi, [esp+36]  ; arg4
	mov edi, [esp+40]  ; arg5
	mov ebp, [esp+44]  ; arg6

	int 0x80

	pop ebx
	pop esi
	pop edi
	pop ebp

	cmp eax, 0
	jge .positive

	neg eax
	mov dword [errno], eax
	mov eax, -1

.positive:
	ret
