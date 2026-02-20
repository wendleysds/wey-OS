global _switch_to

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


; asmlinkage void _switch_to(struct registers* prev, struct registers* to);
_switch_to:
	; esp+0 = return addr
	; esp+4 = prev
	; esp+8 = to

	mov eax, [esp+4]
	mov ecx, [esp+8]

	cmp eax, 0
	jz .skip_save
	mov [eax+12], esp

.skip_save:
	mov esp, [ecx+12]

	ret
