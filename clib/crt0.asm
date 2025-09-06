global _start

extern main

_start:
	; prepare argc, argv and envp
	; push then
	; call main function
	; 
	; invoke exit syscall 
	jmp $