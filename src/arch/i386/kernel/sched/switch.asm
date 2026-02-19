global _switch_to
global _ret_from_interrupt

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

; asmlinkage __no_return void _ret_from_interrupt(struct registers* regs);
_ret_from_interrupt:
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


; asmlinkage void _switch_to(struct registers* prev, struct registers* to);
_switch_to:
	; esp+0 = return addr
	; esp+4 = prev
	; esp+8 = to

	mov eax, [esp+4]
	mov ecx, [esp+8]

	pushad
	mov [eax+12], esp
	mov esp, [ecx+12]
	popad

	ret
