section .asm
[BITS 16]

global bios_putchar

bios_putchar:
	push bp
	mov bp, sp

	mov al, [bp+6]
	mov ah, 0x0E
	int 0x10

	pop bp
	ret

%macro intcall 1
	global bios_int%1h
	bios_int%1h:
		push bp
		mov bp, sp

		mov si, [bp + 6]

		mov ax, [si]
		mov bx, [si+2]
		mov cx, [si+4]
		mov dx, [si+6]
		mov di, [si+8]
		mov es, [si+10]

		int 0x%1

		mov [si], ax
		mov [si+2], bx
		mov [si+4], cx
		mov [si+6], dx
		mov [si+8], di
		mov [si+10], es

		pop bp
		ret
%endmacro

intcall 10
	
