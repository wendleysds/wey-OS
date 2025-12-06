#include "kernelsecs.h"

global hdr

extern main
extern setup_sectors

section .pad

section .header
hdr:
	db setup_sectors - 1          ; setup_sectors
	dd (__kernel_phys_size / 16)  ; syssize
	dw 0x0                        ; video_mode
	db 'cAfE'                     ; sig
	dw 0x0                        ; root_device
	dw main                       ; code16_start
	dd 0x0100000                  ; code32_start
	dd 0x0                        ; ramdisk_ptr
	dd 0x0                        ; ramdisk_size
	db 0x0                        ; ext_loader_ver
	db 0x0                        ; ext_loader_type
	dd 0x0                        ; kernel_alignment
	db 0x0                        ; relocatable_kernel
	db 0x0                        ; min_alignment
	dq 0x0100000                  ; pref_address
