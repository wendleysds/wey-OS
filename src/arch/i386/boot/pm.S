[BITS 16]

#include <asm/cpuflags.h>
#define GDT_KERNEL_CODE  (1 << 3)
#define GDT_KERNEL_DATA  (2 << 3)

global go_to_protect_mode

; void regparm(2) noreturn go_to_protect_mode(u32 entry_point, u32 boot_header);
go_to_protect_mode:
	cli
	mov esi, edx

	xor ebx, ebx
	mov bx, cs
	shl ebx, 4

	; Adjust the pointer to the GDT
	add dword[ljmp_offset], ebx

	jmp .flush
.flush:

	mov edx, cr0
	or dl, X86_CR0_PE
	mov cr0, edx

	db 0x66 ; operand-size override
	db 0xEA ; ljmp
ljmp_offset:
	dd init_pm
	dw GDT_KERNEL_CODE

[BITS 32]
section .text32
init_pm:
	mov dx, GDT_KERNEL_DATA
	mov ds, dx
	mov es, dx
	mov fs, dx
	mov gs, dx
	mov ss, dx

	jmp eax ; entry_point

	hlt
	jmp $
