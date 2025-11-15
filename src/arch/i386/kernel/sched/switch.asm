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
;	unsigned short cs, _csh (42:44);
;	unsigned long flags (48), sp (52);
;	unsigned short ss, _ssh (54:56);
;} __packed;


; void _switch_to(struct registers* prev, struct registers* to);
_switch_to:
	; esp+0 = return addr
	; esp+4 = prev
	; esp+8 = to

	mov eax, [esp+4]
	mov ecx, [esp+8]

	mov [eax+12], esp
	mov esp, [ecx+12]

	ret
