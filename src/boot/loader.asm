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

	call find_file
	jc .err
	
	call load_file

	jmp 0x0000:0x1000

.err:
	mov si, msg_ERROR
	call print_str

.halt:
	hlt
	jmp $


; Seach a file with the name iquals [entry_file_name] and get wis desired cluster
;
; [entry_file_name]
;
; returns:
;   store result in 'Cluster' variable and the file size in 'FileSize'.

find_file:
	pusha
	mov ax, word[BPB_RootClus]
	call cluster_to_lba
	call lba_to_chs

	mov bx, 0x8000 ; Root Sector Buffer
	mov al, byte[BPB_SecPerClus]
	call disk_read
	jc .not_found

	mov si, 0x8000

.next_entry:
    cmp byte [si], 0x00
	je .not_found
	cmp byte [si], 0xE5
	je .skip_entry

	mov di, entry_file_name
    mov cx, 11
    push si
    repe cmpsb
    pop si
    je .found

.skip_entry:
    add si, 32
    cmp si, 0x8000 + 512
	jb .next_entry

.not_found:
	popa
    stc
	ret

.found:
	mov dx, word [si+20]
    shl edx, 16
    mov ax, word [si+26]
    or dx, ax
    mov word [Cluster], dx

    popa
	clc
	ret

; Load the firsts [BPB_BytsPerSec * BPB_SecPerClus] bytes to 0x0000:0x10000
;
; [Cluster]
;
; returns:
;   none
load_file:
	pusha
	mov bx, 0x1000

	mov ax, word [Cluster]
    call cluster_to_lba
    call lba_to_chs

	mov al, byte [BPB_SecPerClus]
    call disk_read
	
	popa
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

msg_ERROR: db "NO", 0x0a, 0x0d, 0

entry_file_name: db "STEP1   BIN"

; For LBA
Head dw 0x0
Cylinder dw 0x0
Sector dw 0x0

DataSector dw 0x0
Cluster dw 0x0

times 510-($ - $$) db 0
dw 0xAA55
