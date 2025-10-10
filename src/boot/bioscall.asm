[BITS 16]

global intcall

; void intcall(uint8_t int_no, const struct biosregs *ireg, struct biosregs *oreg);
intcall:
	mov al, [esp +4]  ; int_no
	mov dx, [esp +8]  ; ireg
	mov cx, [esp +12] ; oreg

	cmp al, byte [no]
	je .do
	mov [no], al
	jmp .do

.do:
	pushfd
	push word fs
	push word gs
	pushad

	; Copy ireg to stack frame
	sub sp, 44
	mov si, dx
	mov di, sp
	mov cx, 11
	rep movsd

	; Set registers poping from stack frame
	popad
	pop word gs
	pop word fs
	pop word es
	pop word ds
	popfd

	db 0xCD
no: db 0x0

	; Save cpu current state on stack
	pushfd
	push word ds
	push word es
	push word fs
	push word gs
	pushad

	; Re-establish C environment invariants
	cld
	movzx esp, sp
	mov ax, cs
	mov ds, ax
	mov es, ax

	; Get 3rd argument
	mov di, [esp+68]
	and di, di ; test if oreg is null
	jz .out

	; Copy cpu state from stack to frame
	mov si, sp
	mov cx, 11
	rep movsd

.out:
	; clear buffer
	add sp, 44

	; restore
	popad
	pop word gs
	pop word fs
	popfd

	ret
