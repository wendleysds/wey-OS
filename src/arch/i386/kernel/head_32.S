#include <def/config.h>
#include <asm/gdt.h>

#undef PAGE_OFFSET
#define PAGE_OFFSET 0xC0000000
#define pa(X) ((X) - PAGE_OFFSET)

global __start_32

CODE_SEG equ early_gdt_code - early_gdt_start
DATA_SEG equ early_gdt_data - early_gdt_start

; methods
extern i386_start_kernel
extern mk_early_pgtbl_32

; variables
extern initial_pgdir

section .entry.boot
__start_32:
	lgdt [pa(early_gdt_descriptor)]
	mov ax, DATA_SEG
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	; zero bss
	cld
	xor eax, eax

	mov edi, pa(__kernel_bss_start)
	mov ecx, pa(__kernel_bss_end)
	sub ecx, edi
	shr ecx, 2
	rep stosd

	mov esp, EARY_STACK_ADDR
	mov ebp, esp

	mov [pa(boot_hdr_ptr)], esi

	call mk_early_pgtbl_32

enable_paging:
	mov eax, pa(initial_pgdir)
	mov cr3, eax

	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

	mov eax, cr3
	mov cr3, eax

	jmp CODE_SEG:l1
l1:
	add esp, PAGE_OFFSET

	mov eax, [pa(boot_hdr_ptr)]
	call i386_start_kernel

	jmp $

section .data

boot_hdr_ptr: dd 0x0

align 32

early_gdt_start:
early_gdt_null:
	dd 0x0
	dd 0x0

early_gdt_code:
	dw 0xffff
	dw 0
	db 0
	db 0x9a
	db 11001111b
	db 0

early_gdt_data:
	dw 0xffff
	dw 0
	db 0
	db 0x92
	db 11001111b
	db 0

early_gdt_end:

early_gdt_descriptor:
	dw early_gdt_end - early_gdt_start - 1
	dd early_gdt_start - PAGE_OFFSET