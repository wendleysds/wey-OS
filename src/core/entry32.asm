section .text.boot
[BITS 32]

global _entry32

;extern kmain
extern main

CODE_SEG equ 0x08
DATA_SEG equ 0x10

_entry32:
	mov ax, DATA_SEG
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax
  mov ebp, 0x00200000
  mov esp, ebp

	jmp CODE_SEG:main

  ;jmp CODE_SEG:kmain

	cli
	hlt
	jmp $


global _ldir
global _epg

_ldir:
	push ebp
	mov ebp, esp
	mov eax, [ebp+8]
	mov cr3, eax
	pop ebp
	ret

_epg:
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	ret
