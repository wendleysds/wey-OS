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

int_common_stub:
	pushad

	xor eax, eax
	mov ax, ds
	push eax

	push esp
	call interrupt_handler
	add esp, 8

	pop eax
	popad
	iret

%macro interrupt 1
	global int%1
	int%1:
		push 0
		push %1
		jmp int_common_stub
		ret
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

