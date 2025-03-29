[ORG 0x7c00]
BITS 16

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

global start

start:
	cli
	mov ax, 0x00
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, 0x7c00
	sti

	mov si, msg_init
	call print_str

	mov si, msg_disk_init
	call print_str

	mov bx, 0x1000
	mov dh, 10
	call disk_load

	mov si, msg_disk_readed
	call print_str

	mov si, msg_topm
	call print_str

.load_pm:
	cli
	lgdt[gdt_descriptor]
	mov eax, cr0
	or al, 0x1
	mov cr0, eax

	jmp CODE_SEG:init_pm

; prints

disk_load:
	pusha
	mov ah, 0x02
	mov al, 10
	mov ch, 0
	mov cl, 2
	mov dh, 0
	int 0x13
	jc disk_error
	popa
	ret

disk_error:
	mov si, msg_disk_error
	call print_str
	hlt
	jmp $

; print

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

;print_str messages

msg_init: db "Initializing bootloader...", 0x0a, 0x0d, 0
msg_disk_init: db "Initializing disk...", 0x0a, 0x0d, 0
msg_disk_readed: db "Disk readed successfuly.", 0x0a, 0x0d, 0
msg_disk_error: db "Disk read error!", 0x0a, 0x0d, 0
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

	jmp 0x1000

	hlt
	jmp $

times 510-($ - $$) db 0
dw 0xAA55

