global _entry_isr80h_32

extern isr80h_handler

_entry_isr80h_32:
	push dword 0    ; err code
	push dword 0x80 ; int num
	pushad
	push esp
	call isr80h_handler
	add esp, 4
	popad
	add esp, 8 ; clear erros
	iretd