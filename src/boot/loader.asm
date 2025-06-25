[ORG 0x7C00]
[BITS 16]

jmp short start
nop

; FAT32 BIOS Parameter Block
BS_OEMName      db "MSWIN4.1"
BPB_BytsPerSec  dw 0x200
BPB_SecPerClus  db 0x08
BPB_RsvdSecCnt  dw 0x20
BPB_NumFATs     db 0x02
BPB_RootEntCnt  dw 0x00
BPB_TotSec16    dw 0x00
BPB_Media       db 0xF8 ; Media descriptor (0xF8 = HD, 0xF0 = floppy)
BPB_FATSz16     dw 0x00
BPB_SecPerTrk   dw 0x3F
BPB_NumHeads    dw 0x10
BPB_HiddSec     dd 0x00
BPB_TotSec32    dd 32768 ; 16 MiB / 512 = 32768

; FAT32 Extended BPB
BPB_FATSz32     dd 0x80 ; (128 sectors × 512 = 64 KiB)
BPB_ExtFlags    dw 0x00
BPB_FSVer       dw 0x00
BPB_RootClus    dd 0x02
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

	; Get Data Sector: BPB_ResvdSecCnt + (BPB_NumFATs * FATSz)
	mov cx, word[BPB_SecPerClus]
	mov al, byte[BPB_NumFATs]
	mul word[BPB_FATSz32]
	add ax, word[BPB_RsvdSecCnt]
	mov word[DataSector], ax

	mov ax, word[BPB_RootClus]
	mov word[Cluster], ax

	mov bx, 0x8000
	mov dx, init_folder
	call find_file

	mov dx, init_file_name
	call find_file
	jc .notfound

	mov bx, 0x1000
	call load_file

	jmp 0x0000:0x1000

.notfound:
	mov si, msg_ERRFNOT
	jmp .halt

.err:
	mov si, msg_ERROR
	jmp .halt

.diskerr:
	mov si, msg_ERRDISK

.halt:
	call print_str
	hlt
	jmp $


; Seach a file with the name iquals [File Name] and get wis desired cluster
;
; [Cluster]
; dx: [File Name]
; bx: [Buffer]
;
; returns:
;   store result in 'Cluster' variable and the file size in 'FileSize'.
find_file:
	pusha
	push dx
	mov ax, word[Cluster]
	call cluster_to_lba
	call lba_to_chs

	mov al, byte[BPB_SecPerClus]
	call disk_read
	jc start.diskerr

	mov si, bx
	pop dx

.next_entry:
	cmp byte[si], 0x00
	je .not_found
	cmp byte[si], 0xE5
	je .skip_entry

	mov di, dx
	mov cx, 11
	push si
	repe cmpsb
	pop si
	je .found

.skip_entry:
	add si, 32
	jmp .next_entry

.not_found:
	popa
	stc
	ret

.found:
	mov dx, word[si+20]
	shl edx, 16
	mov ax, word[si+26]
	or dx, ax
	mov word[Cluster], dx

	mov dx, word[si+28]
	mov word[FileSize], dx

	popa
	clc
	ret

; Load the firsts [BPB_BytsPerSec * BPB_SecPerClus] bytes to bx: [Destination]
;
; [Cluster]
; [FileSize]
; bx: [Destination]
;
; returns:
;   none
load_file:
	pusha

	mov ax, word[Cluster]
	call cluster_to_lba
	call lba_to_chs

	mov al, byte[BPB_SecPerClus]
	call disk_read
	
	popa
	clc
	ret

; Print messages in the screen
; 
; si: [message]
;
; returns:
;   none
print_str:
	pusha
	mov ah, 0x0E
.next_char:
	lodsb
	or al, al
	jz .done
	int 0x10
	jmp .next_char
.done:
	popa
	ret

; Read sectors in CHS mode and store im memory
;
; bx: [Buffer]
; al: [Sectors Amount]
;
; Set the variables 'Sector', 'Cylinder' and 'Head' before using!
;
; returns: 
;   none
disk_read:
	pusha
	mov ah, 0x02
	mov ch, byte[Cylinder]
	mov dh, byte[Head]
	mov cl, byte[Sector]

	mov dl, byte[BS_DrvNum]
	int 0x13
	jc .disk_error

	popa
	clc
	ret
.disk_error:
	popa
	stc
	ret

; Convert Cluster to LBA
;
; LBA = (([CLUSTER] – 2) * BPB_SecPerClus) + FirstDataSector;
;
; ax: [Cluster]
; cx: [SecPerClus]
;
; returns: 
;   ax: [LBA]
cluster_to_lba:
	sub ax, 0x2
	xor cx, cx
	mov cl, byte[BPB_SecPerClus] ; convert byte to word
	mul cx
	add ax, word[DataSector]
	ret

; Convert LBA scheme to CHS scheme
;
; Sector = (LBA % (Sectors per Track)) + 1
; Head = LBA % (Number of Heads)
; Cylinder = LBA / (Number of Heads)
;
; ax: [LBA]
;
; returns:
;   store results in Cylinder, Head, Sector variables
lba_to_chs:
	xor dx, dx
	div word[BPB_SecPerTrk]
	inc dl
	mov byte[Sector], dl
	xor dx, dx
	div word[BPB_NumHeads]
	mov byte[Head], dl
	mov byte[Cylinder], al
	ret

msg_ERRFNOT: db "File not found!", 0
msg_ERRDISK: db "Disk error!", 0
msg_ERROR: db "Error", 0

init_folder: db "BOOT       ", 0
init_file_name: db "STEP1   BIN", 0

FileSize: dw 0x0

; For LBA
Head: dw 0x0
Cylinder: dw 0x0
Sector: dw 0x0

DataSector: dw 0x0
Cluster: dw 0x0

times 510-($ - $$) db 0
dw 0xAA55
