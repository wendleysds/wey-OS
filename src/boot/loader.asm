ORG 0x7c00
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
	mov fs, ax
	mov es, ax

	mov sp, 0x7c00
	sti

.load_pm:
	cli
	lgdt[gdt_descriptor]
	mov eax, cr0
	or eax, 0x1
	mov cr0, eax

	jmp CODE_SEG:init_pm

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
	dw gdt_end - gdt_start - 1
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

	; ATA LBA args
	mov eax, 1
	mov ecx, 100
	mov edi, 0x0100000
	call ata_lba_read

	jmp CODE_SEG:0x0100000

ata_lba_read:
  mov ebx, eax, ; Backup the LBA
	
  ; Send the highest 8 bits of the lba to hard disk controller
  shr eax, 24
  or eax, 0xE0 ; Select the master drive
  mov dx, 0x1F6
  out dx, al

  ; Send the total sectors to read
  mov eax, ecx
  mov dx, 0x1F2
  out dx, al

  ; Send bits of the LBA
  mov eax, ebx ; Restore the backup LBA
  mov dx, 0x1F3
  out dx, al

  ; Send more bits of the LBA
  mov dx, 0x1F4
  mov eax, ebx
  shr eax, 8
  out dx, al

  ; Send upper 16 bits of the LBA
  mov dx, 0x1F5
  mov eax, ebx
  shr eax, 16
  out dx, al

  mov dx, 0x1f7
  mov al, 0x20
  out dx, al

  ; Read all sectors into memory
.next_sector:
  push ecx

.try_again:
  mov dx, 0x1f7
  in al, dx
  test al, 8
  jz .try_again

  mov ecx, 256 ; 256 words at time
  mov dx, 0x1F0
  rep insw
  pop ecx
  loop .next_sector
  ret

times 510-($ - $$) db 0
dw 0xAA55
