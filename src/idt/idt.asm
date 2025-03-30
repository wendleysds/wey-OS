section .asm

global load_idt
global enable_interrupts
global disable_interrupts

load_idt:
	push ebp
	mov ebp, esp

	mov ebx, [ebp+8]
	lidt [ebx]
	pop ebp
	ret

enable_interrupts:
	sti
	ret

disable_interrupts:
	cli
	ret

