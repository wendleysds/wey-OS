[BITS 32]

global _entry32

extern kmain

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

  jmp CODE_SEG:kmain

	cli
	hlt
	jmp $
