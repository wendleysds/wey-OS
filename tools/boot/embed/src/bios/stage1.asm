[ORG 0x7C00]
[BITS 16]

jmp short _start
nop

stack equ 0x7C00

headCount equ (stack - 6)
secPerTrk equ (stack - 4)
drvNum equ (stack -2)

_start:
	cli
	mov ax, 0x00
	mov ds, ax
	mov ss, ax
	mov fs, ax
	mov es, ax
	mov sp, stack
	sti

	movzx ax, dl
	push ax

	; Get (C)HS geometry
	mov ah, 0x08
	mov dl, 0x80
	int 0x13

	and cx, 0x3F ; Sectors per Track
	push cx

	movzx ax, dh ; dh = max head
	inc ax	
	push ax

	sub sp, 2 ; 1 word for index
	mov si, sp

	mov word [si], 0
	mov bx, 0x1000
	mov dx, word [stage1SecPerLba]

.load_stage2:
	; lba = [stage1lba + word [si] * 2]
	mov di, word [si]
	shl di, 1 ; di = index * sizeof(word)

	add di, stage1lba
	mov ax, [di] ; ax = lba
	

	call disk_read
	jc .err_read

	inc word [si]

	push dx

	mov ax, 0x200
	mul dx

	add bx, ax

	pop dx

	cmp word [si], 6
	jl .load_stage2

.done:
	mov dx, [drvNum]
	jmp 0x0000:0x1000

.err_read:
	call error
	db "error reading disk!", 0x0A, 0x0D, 0


; ax: lba
; bx: off
; es: seg
; dx: seccount
disk_read:
	pusha
	push dx

	; LBA -> chs
	;
	; temp = LBA / sectors_per_track
	; sector = (LBA % sectors_per_track) + 1
	; head = temp % heads
	; cylinder = temp / heads

	xor dx, dx
	div word[secPerTrk]
	inc dl
	; dl = sector

	mov cl, dl

	xor dx, dx
	div word[headCount]
	; dl = head
	; al = cylinder

	mov ch, al
	mov dh, dl

	pop ax

	mov ah, 0x02
	mov dl, [drvNum]

	int 0x13

	popa
	ret

error:
	pop word si
	mov ah, 0x0E
.next_char:
	lodsb
	or al, al
	jz .done
	int 0x10
	jmp .next_char
.done:
	int 0x18
.die:
	hlt
	jmp .die

times 0x1B0 - ($ - $$) db 0

stage1lba: dw 2048, 2056, 0, 0, 0, 0
stage1SecPerLba: db 8

times 0x1BE-($ - $$) db 0

; ----

partition1_status db 0x80 ; bootable
partition1_start_chs db 0, 2, 0
partition1_type db 0x0C ; FAT32 LBA
partition1_end_chs db 0xFF, 0xFF, 0xFF
partition1_start_lba dd 2048
partition1_size_lba dd 0x0010000 ; 32 MiB

; ----

times 510-($ - $$) db 0
dw 0xAA55