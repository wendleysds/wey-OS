global page_fault_entry

extern page_fault_handler

page_fault_entry:
	push dword 14 ; int num
	pushad
	push esp
	call page_fault_handler
	add esp, 4
	popad
	add esp, 8 ; clear codes
	iretd