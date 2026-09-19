#include <def/config.h>

global _entry_isr80h_32
extern isr80h_handler

_set_segments:
	mov ds, ax
	mov es, ax
	mov gs, ax
	mov fs, ax
	ret

_entry_isr80h_32:
	push dword 0    ; err code
	push dword 0x80 ; int num
	pushad
	push esp

	mov ax, GDT_KERNEL_DATA
	call _set_segments

	call isr80h_handler
	add esp, 4

	mov ax, GDT_USER_DATA
	call _set_segments

	popad
	add esp, 8 ; clear erros
	iretd