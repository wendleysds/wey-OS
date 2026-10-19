bits 32

global do_syscallsyscall

extern errno ; int

; long do_syscall(long no, ...)
do_syscall:
	push ebp
	mov ebp, esp
	mov eax, [ebp+8]   ; syscall number
	mov ebx, [ebp+12]  ; arg1
	mov ecx, [ebp+16]  ; arg2
	mov edx, [ebp+20]  ; arg3
	mov esi, [ebp+24]  ; arg4
	mov edi, [ebp+28]  ; arg5
	mov ebp, [ebp+32]  ; arg6
	int 0x80

	cmp eax, 0
	jge positive

	mov dword [errno], eax
	mov eax, -1
	
positive:
	pop ebp
	ret