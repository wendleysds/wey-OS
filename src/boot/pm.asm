[BITS 16]

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

global go_to_protect_mode

go_to_protect_mode:
	cli
	lgdt[gdt_descriptor]
	mov eax, cr0
	or eax, 0x1
	mov cr0, eax

	mov ebx, dword [0x8000] ; Backup VideoStruct
	
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
	
	mov dword [0x8000], ebx ; Restore VideoStruct and save in desired position

	mov cx, 0x08
	mov al, 0x02
	mov dx, 0x80
	mul dx
	add ax, 0x20
	mov word[DataSector], ax

	push eax
	mov eax, 0x02
	mov dword [Cluster], eax
	pop eax

	mov edx, entry_dir_name
	call find_file

	mov edx, entry_file_name
	call find_file
	jc .err

	call load_file

	jmp .ok

.err:
   	hlt
	jmp $

.ok:
	jmp CODE_SEG:0x0100000

;Seach a file with the name iquals [entry_file_name] and get wis desired cluster
;
; edx [entry_file_name]
;
; returns:
;   store result in 'Cluster'
find_file:
	pushad
	push edx

	mov eax, dword [Cluster]
	call cluster_to_lba

	mov ecx, 0x08
	mov edi, 0x6000
	call ata_lba_read

	mov si, 0x6000
	pop edx

.next_entry:
	cmp byte [si], 0x00
	je .not_found
	cmp byte [si], 0xE5
	je .skip_entry

	mov edi, edx
    mov ecx, 11
    push si
    repe cmpsb
    pop si
    je .found

.skip_entry:
    add si, 32
    cmp si, 0x6000 + 512 ; firsts 16 entries
	jb .next_entry

.not_found:
	popf
    stc
	ret

.found:	
	movzx edx, word [si + 0x14]
    shl edx, 16
    movzx eax, word [si + 0x1A]
    or edx, eax

    mov dword [Cluster], edx
	mov eax, edx

	popad
	clc
	ret

load_file:
	pushad
	mov edi, 0x0100000
	mov ecx, 0x8

.read_cluster:
	mov eax, dword [Cluster]
	cmp eax, 0x0FFFFFF8
	jae .done

	call cluster_to_lba
	call ata_lba_read

	add edi, 0x1000 ; 4096

	call get_next_cluster
	jmp .read_cluster 

.done:
	popad
	ret

; Get the next cluster
;
; [Cluster]
;
; returns;
;   store result in 'Cluster' variable
get_next_cluster:
	pushad
	mov eax, dword[Cluster]
	shl eax, 2 ; cluster * 4

	; sector = RsvdSecCnt + (offset / 512)
	mov ecx, 0x200
	xor edx, edx
	div ecx ; eax = offset / bytes_per_sector, edx = offset % bytes_per_sector

	mov ebx, 0x20
	add eax, ebx ; BPB_RsvdSecCnt + offset

	; Read the sector
	mov edi, 0x6000
	mov ecx, 1
	call ata_lba_read

	; Get fat entry
	mov esi, 0x6000
	add esi, edx

	; Get and store cluster
	mov eax, dword [esi]
	and eax, 0x0FFFFFFF
	
	mov dword [Cluster], eax

	popad
	ret

; Convert Cluster to LBA
;
; LBA = (([CLUSTER] – 2) * BPB_SecPerClus) + FirstDataSector;
;
; eax: [Cluster]
;
; returns: 
;   eax: [LBA]
cluster_to_lba:
	push ebx
	sub eax, 0x2
	mov ebx, 0x08
	mul ebx
	add eax, dword[DataSector]
	pop ebx
	ret

; Read the disk with ATA PIO
;
; eax: [LBA]
; ecx: [Sectors Amount]
; edi: [Buffer]
;
; returns:
;   none
ata_lba_read:
	pushad
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

	mov ecx, 256 ; 256 words == 512 bytes at time
	mov dx, 0x1F0
	rep insw
	pop ecx

	loop .next_sector
	popad
	ret

entry_dir_name: db "BOOT       ", 0
entry_file_name: db "KERNEL  BIN", 0

DataSector: dd 0x0
Cluster: dd 0x0