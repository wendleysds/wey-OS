global _start

extern main

_start:
	; prepare argc, argv and envp
	; push then
	call main
	; 
	; invoke exit syscall 
	jmp $