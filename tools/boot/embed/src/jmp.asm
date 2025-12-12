global jump_segment
global jump_pm


[BITS 16]

; void (seg: uint16, entry_point: uint16)
jump_segment:
	mov fs, ax
	mov gs, ax
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, 0xFFFF

	push ax
	push dx

	cli
	retf

; void (entry_point uint32)
jump_pm:
	ret
