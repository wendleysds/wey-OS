#include "kernelsecs.h"

global hdr

extern main
extern setup_sectors

section .header
hdr:
	dd 1                      ; tag.type
	dd 40                     ; tag.size

	db setup_sectors - 1      ; setup_sectors
	db 0, 0, 0                ; pad1

	dd (__kernel_size / 16)   ; syssize
	dd main                   ; code16_start
	dd 0x0100000              ; code32_start

	dd 0x0200000              ; kernel_alignment

	db 0                      ; relocatable_kernel
	db 8                      ; min_alignment
	dw 0xAA55                 ; boot_sig
	dq 0x0100000              ; pref_address
