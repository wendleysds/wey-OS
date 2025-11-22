[BITS 16]

global _start

extern main

section .start

_start:
	mov sp, 0xFFFF
	mov bp, sp

	call enter_unreal

	jmp main

enter_unreal:
	cli
	push ds

	lgdt [gdt_descriptor]

	mov eax, cr0
	or al, 1
	mov cr0, eax
	jmp 0x08:n1

n1:
	mov bx, 0x10
	mov ds, bx

	and al, 0xFE
	mov cr0, eax
	jmp 0x00:n2

n2:
	pop ds
	sti

	ret

gdt_descriptor:
   dw gdt_end - gdt - 1
   dd gdt

gdt:        dd 0,0
codedesc:   db 0xff, 0xff, 0, 0, 0, 10011010b, 00000000b, 0
flatdesc:   db 0xff, 0xff, 0, 0, 0, 10010010b, 11001111b, 0
gdt_end:
