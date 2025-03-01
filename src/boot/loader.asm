[ORG 0x7c00]
BITS 16

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

global start

start:
	mov si, msg_init
	call print_str

	cli
	mov ax, 0x00
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, 0x7c00
	sti

	mov si, msg_topm
	call print_str

.load_pm:
	cli
	lgdt[gdt_descriptor]
	mov eax, cr0
	or eax, 0x1
	mov cr0, eax

	jmp CODE_SEG:init_pm

; prints

print_str:
	mov ah, 0x0E
.print_loop:
	lodsb
	or al, al
	jz .print_done
	int 0x10
	jmp .print_loop
.print_done:
	ret
msg_init: db "Initializing bootloader...", 0x0a, 0x0d, 0
msg_topm: db "Entering protected mode...", 0x0a, 0x0d, 0

; GTD

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

gdt_descriptor:
	dw gdt_end - gdt_start-1
	dd gdt_start

[BITS 32]
init_pm:
	mov ax, DATA_SEG
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	in al, 0x92
	or al, 2
	out 0x92, al

	mov edi, 0xB8000
	mov esi, msg_hello
	mov ecx, 13
	mov ah, 0x07

.print_loop:
	lodsb
	stosw
	loop .print_loop

	jmp $ 

msg_hello db "Hello, World!", 0

	jmp $

times 510-($ - $$) db 0
dw 0xAA55

