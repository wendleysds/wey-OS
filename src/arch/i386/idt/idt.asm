section .asm

global load_idt
global enable_interrupts
global disable_interrupts 
global interrupt_pointer_table

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


%macro interrupt 1
	global int%1
	int%1:
		pushad
		push esp
		push dword %1
		call interrupt_handler
		add esp, 8
		popad
		iret
%endmacro

%assign i 0
%rep 256
	interrupt i
%assign i i+1
%endrep

%macro create_int 1
	dd int%1
%endmacro

interrupt_pointer_table:
%assign i 0
%rep 256
	create_int i
%assign i i+1
%endrep

