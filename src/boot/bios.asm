[BITS 16]

global bios_putchar
global intcall

bios_putchar:
	push bp
	mov bp, sp

	mov al, [bp+6]
	mov ah, 0x0E
	int 0x10

	pop bp
	ret

; void intcall(uint8_t int_no, const struct biosregs *ireg, struct biosregs *oreg);
intcall:
	mov bp, sp
	mov al, [bp+4]  ; int_no
	mov dx, [bp+8]  ; ireg
	mov cx, [bp+12] ; oreg

	cmp al, byte [no]
	je .do
	mov [no], al
	jmp .do

.do:
	pushfd
	push fs
	push gs
	pushad

	; Copy ireg to stack frame
	sub sp, 44
	mov si, dx
	mov di, sp
	mov cx, 11
	rep movsd

	; set registers poping from stack frame
	popad
	pop gs
	pop fs
	pop es
	pop ds
	popfd

	db 0xCD
no: db 0x0

	ret
