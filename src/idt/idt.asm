section .asm

global load_idt

load_idt:
	push ebp
	mov ebp, esp
	mov eax, [ebp+8]
	lidt [eax]
	mov esp, ebp
	pop ebp
	ret

