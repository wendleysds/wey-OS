section .asm

extern isr_handler
extern irq_handler

isr_common_stub:
	pushad

	xor eax, eax
	mov ax, ds
	push eax

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	push esp
	call isr_handler
	add esp, 8

	pop ebx
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	popad
	add esp, 8
	iret

irq_common_stub:
  pushad

	xor eax, eax
	mov ax, ds
	push eax

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	push esp
	call irq_handler
	add esp, 8

	pop ebx
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	popad
	add esp, 8
	iret

%macro ISR_NOERRCODE 1
	global isr%1
	isr%1:
		push 0
		push %1
		jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
	global isr%1
	isr%1:
		push %1
		jmp isr_common_stub
%endmacro

%macro IRQ 2
	global irq%1
	irq%1:
		push 0
		push %2
		jmp irq_common_stub
		ret
%endmacro
  
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7

ISR_ERRCODE 8
ISR_NOERRCODE 9 
ISR_ERRCODE 10
ISR_ERRCODE 11
ISR_ERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

IRQ 0,  32
IRQ 1,  33
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

section .data
global isr_pointer_table
global irq_pointer_table

isr_pointer_table:
	dd isr0
	dd isr1
	dd isr2
	dd isr3
	dd isr4
	dd isr5
	dd isr6
	dd isr7
	dd isr8
	dd isr9
	dd isr10
	dd isr11
	dd isr12
	dd isr13
	dd isr14
	dd isr15
	dd isr16
	dd isr17
	dd isr18
	dd isr19
	dd isr20
	dd isr21
	dd isr22
	dd isr23
	dd isr24
	dd isr25
	dd isr26
	dd isr27
	dd isr28
	dd isr29
	dd isr30
	dd isr31

irq_pointer_table:
  dd irq0
  dd irq1
  dd irq2
  dd irq3
  dd irq4
  dd irq5
  dd irq6
  dd irq7
  dd irq8
  dd irq9
  dd irq10
  dd irq11
  dd irq12
  dd irq13
  dd irq14
  dd irq15
