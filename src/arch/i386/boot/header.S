#include "kernelsecs.h"

global hdr

extern main
extern setup_sectors

section .pad

section .header
hdr:
	db setup_sectors - 1          ; setup_sectors
	dd (__kernel_phys_size / 16)  ; syssize
	dw 0                          ; video_mode
	dw 0xCAFE                     ; sig
	dw 0x80                       ; root_device
	dw main                       ; code16_start
	dd 0x0100000                  ; code32_start
	dq 0x0100000                  ; pref_address
