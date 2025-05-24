[ORG 0x7C00]
[BITS 16]

jmp short start
nop

; FAT32 BIOS Parameter Block
BS_OEMName      db "MSWIN4.1"
BPB_BytsPerSec  dw 0x200
BPB_SecPerClus  db 0x80
BPB_RsvdSecCnt  dw 0x20
BPB_NumFATs     db 0x02
BPB_RootEntCnt  dw 0x00
BPB_TotSec16    dw 0x00
BPB_Media       db 0xF8 ; Media descriptor (0xF8 = HD, 0xF0 = floppy)
BPB_FATSz16     dw 0x00
BPB_SecPerTrk   dw 0x20
BPB_NumHeads    dw 0x40
BPB_HiddSec     dd 0x00
BPB_TotSec32    dd 32768 ; 16 MiB / 512 = 32768

; FAT32 Extended BPB
BPB_FATSz32     dd 0x80 ; (128 sectors × 512 = 64 KiB)
BPB_ExtFlags    dw 0x00
BPB_FSVer       dw 0x00
BPB_RootClus    dd 0x20
BPB_FSInfo      dw 0x01
BPB_BkBootSec   dw 0x06
BPB_Reserved    times 12 db 0

BS_DrvNum       db 0x80
BS_Reserved1    db 0x00
BS_BootSig      db 0x29
BS_VolID        dd 0xCAFE
BS_VolLab       db "NO NAME    " ; 11 bytes, volume label
BS_FilSysType   db "FAT32   "    ; 8 bytes not used for type detection

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
	mov al, 20
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

