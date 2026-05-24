global do_syscall

;  long syscall(long long no, long long arg1, long long arg2, long long arg3, long long arg4, long long arg5, long long arg6)
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
	pop ebp
	ret