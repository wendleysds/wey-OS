global _entry_isr80h_32

extern isr80h_handler

_entry_isr80h_32:
	pushad
	push dword 0    ; err code
	push dword 0x80 ; int num
	push esp
	call isr80h_handler
	add esp, 4
	add esp, 8
	popad
	iretd