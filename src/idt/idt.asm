section .asm

global load_idt
global enable_interrupts
global disable_interrupts 

extern interrupt_handler

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

%macro ISR 1
	global isr%1
	isr%1:
		pushad
		push esp
		call interrupt_handler
		add esp, 8
		popad
		iret
%endmacro

%assign i 0
%rep 32
	ISR i
%assign i i+1
%endrep

section .data
global isr_pointer_table

isr_pointer_table:
%assign i 0
%rep 32
  dd isr%+i
%assign i i+1
%endrep