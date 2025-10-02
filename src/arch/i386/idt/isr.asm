global interrupt_pointer_table

extern interrupt_handler

_isr_common:
	pusha
	push esp
	call interrupt_handler
	add esp, 4
	popa

	add esp, 8
	iret

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
