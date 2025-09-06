global do_syscall

;  long syscall(long no, long arg1, long arg2, long arg3, long arg4)
do_syscall:
	push ebp
	mov ebp, esp
	mov eax, [ebp+8]   ; syscall number
	mov ebx, [ebp+12]  ; arg1
	mov ecx, [ebp+16]  ; arg2
	mov edx, [ebp+20]  ; arg3
	mov esi, [ebp+24]  ; arg4
	int 0x80
	pop ebp
	ret