#include <def/config.h>
#include <asm/gdt.h>

global __start_32

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

extern i386_start_kernel
extern mk_early_pgtbl_32
extern early_alloc_init_boot
extern early_alloc_boot

extern __kernel_bss_start
extern __kernel_bss_end

extern __kernel_phys_end

section .entry.boot
__start_32:
	lgdt[early_gdt_descriptor]
	mov ax, DATA_SEG
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	; zero bss
	cld
	xor eax, eax

	mov edi, __kernel_bss_start
	sub edi, PAGE_OFFSET
	add edi, KERNEL_PHYS_BASE

	mov ecx, __kernel_bss_end
	sub ecx, PAGE_OFFSET
	add ecx, KERNEL_PHYS_BASE

	sub ecx, edi
	shr ecx, 2
	rep stosw

	mov esp, EARY_STACK_ADDR
	mov ebp, esp

	jmp CODE_SEG:l1
l1:
	call early_alloc_init_boot
	
	mov dword [boot_hdr_ptr], esi
	call mk_early_pgtbl_32
	call load_directory

	mov eax, KiB(8)
	push eax
	call early_alloc_boot
	add eax, KiB(8)
	add eax, PAGE_OFFSET
	push eax

	call enable_paging

	pop esp
	mov ebp, esp

	mov dword eax, [boot_hdr_ptr]
	jmp i386_start_kernel

	hlt
	jmp $

section .text.boot

load_directory:
	mov cr3, eax
	ret

enable_paging:
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	jmp flush_all
flush_all:
	mov eax, cr3
	mov cr3, eax
	ret

section .data.boot
boot_hdr_ptr: dd 0x0

align 32

gdt_start:
gdt_null:
	dd 0x0
	dd 0x0

gdt_code:
	dw 0xffff
	dw 0
	db 0
	db 0x9a
	db 11001111b
	db 0

gdt_data:
	dw 0xffff
	dw 0
	db 0
	db 0x92
	db 11001111b
	db 0

gdt_end:

early_gdt_descriptor:
	dw gdt_end - gdt_start - 1
	dd gdt_start