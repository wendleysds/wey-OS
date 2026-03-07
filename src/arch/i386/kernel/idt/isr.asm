global interrupt_pointer_table
global ret_from_fork
global ret_from_registers

extern interrupt_handler
extern interrupt_eoi
extern schedule
extern need_resched

;struct registers {
;	// pushad regs order
;	unsigned long di (00), si (04), bp (08), ksp (12);
;	unsigned long bx (16), dx (20), cx (24), ax  (28);
;
;	// err_code pushed by the processor if
;	// theres one
;	unsigned long int_no (32), err_code (36);
;
;	// Automatically pushed by the processor
;	// he dont push SS and SP if the privelege level inst change
;	unsigned long ip (40);
;	unsigned long cs (44);
;	unsigned long flags (48), sp (52);
;	unsigned long ss (56);
;} __packed;

; asmlinkage __no_return void ret_from_registers(struct registers* regs);
ret_from_registers:
	mov ebp, esp

	xor eax, eax
	mov ax, cs
	cmp eax, [ebp+44]
	je .same_priv

	mov ax, word [ebp+44]

	mov ds, ax
	mov es, ax
	mov gs, ax
	mov fs, ax

	push dword [ebp+56]
	push dword [ebp+52]

.same_priv:

	push dword [ebp+48]
	push dword [ebp+44]
	push dword [ebp+40]

	mov edi, [ebp+0]
	mov esi, [ebp+4]
	mov ebx, [ebp+16]
	mov edx, [ebp+20]
	mov ecx, [ebp+24]
	mov eax, [ebp+28]
	mov ebp, [ebp+8]

	iret

_ret_from_intr:
	mov ebp, esp

	mov eax, [ebp+32]
	cmp eax, 32
	jl .no_eoi

	push eax
	call interrupt_eoi
	add esp, 4

.no_eoi:
	mov eax, [need_resched]
	test eax, eax
	jz .restore

	mov dword [need_resched], 0
	call schedule

	mov ebp, esp

.restore:
	popad       ; restore all registers
	add esp, 8  ; clear err_code and int_no
	iret        ; kachow

ret_from_fork:
	popad
	add esp, 8
	iret

_isr_common:
	pushad
	push esp
	call interrupt_handler
	add esp, 4

	jmp _ret_from_intr

%macro ISR_NERRC 1
	global int%1:
	int%1:
		push 0  ; Dummy error code
		push %1
		jmp _isr_common
%endmacro

%macro ISR_ERRC 1
	global int%1:
	int%1:
		push %1
		jmp _isr_common
%endmacro

;ISR_NERRC 0
;ISR_NERRC 1
;ISR_NERRC 2
;ISR_NERRC 3
;ISR_NERRC 4
;ISR_NERRC 5
;ISR_NERRC 6
;ISR_NERRC 7
;ISR_ERRC 8
;ISR_NERRC 9
;ISR_ERRC 10
;ISR_ERRC 11
;ISR_ERRC 12
;ISR_ERRC 13
;ISR_ERRC 14
;ISR_NERRC 15
;ISR_NERRC 16
;ISR_ERRC 17
;ISR_NERRC 18
;ISR_NERRC 19
;ISR_NERRC 20
;ISR_ERRC 21
;ISR_NERRC 22
; ...
;ISR_NERRC 255

%assign i 0
%rep 256
	%if (i == 8) || (i == 10) || (i == 11) || (i == 12) || (i == 13) || (i == 14) || (i == 17) || (i == 21)
		ISR_ERRC i
	%else
		ISR_NERRC i
	%endif
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
