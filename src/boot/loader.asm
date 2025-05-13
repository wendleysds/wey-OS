[ORG 0x7C00]
[BITS 16]

global start

start:
	cli
	mov ax, 0x00
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov es, ax

	mov sp, 0x7C00
	sti

	call disk_load
	mov si, msg_disk_readed
	call print_str

	jmp 0x0000:0x1000

	hlt
	jmp $

disk_load:
	mov si, msg_disk_init
	call print_str

	pusha
	mov ah, 0x02
	mov al, 10
	mov ch, 0
	mov cl, 2
	mov dh, 0

	xor bx, bx
	mov es, bx
	mov bx, 0x1000
	int 0x13

	jc disk_error
	popa
	ret
disk_error:
	mov si, msg_disk_error
	call print_str
	hlt
	jmp $

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

msg_disk_init: db "Initializing disk...", 0x0a, 0x0d, 0
msg_disk_readed: db "Disk readed successfuly.", 0x0a, 0x0d, 0
msg_disk_error: db "Disk read error!", 0x0a, 0x0d, 0

times 510-($ - $$) db 0
dw 0xAA55

