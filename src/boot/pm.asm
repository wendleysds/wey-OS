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

extern test

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
	mov dword[ADDR], 0xB8000

	mov cx, 0x08
	mov al, 0x02
	mov dx, 0x80
	mul dx
	add ax, 0x20
	mov word[DataSector], ax

	call clear_screen

	call find_file
	jc .err

	call load_file

	jmp .ok

.err:
	mov eax, 1
   	jmp .halt

.ok:
	mov eax, 0

.halt:
	call print_eax_decimal
	hlt
	jmp $

	;jmp CODE_SEG:0x0100000

;Seach a file with the name iquals [entry_file_name] and get wis desired cluster
;
; [entry_file_name]
;
; returns:
;   store result in 'Cluster'
find_file:
	pushad
	mov eax, 0x02
	call cluster_to_lba

	mov ecx, 0x08
	mov edi, 0x6000
	call ata_lba_read

	mov si, 0x6000

.next_entry:
	cmp byte [si], 0x00
	je .not_found
	cmp byte [si], 0xE5
	je .skip_entry

	mov edi, entry_file_name
    mov ecx, 11
    push si
    repe cmpsb
    pop si
    je .found

.skip_entry:
    add si, 32
    cmp si, 0x6000 + 512
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

.read_cluster:
	mov eax, dword [Cluster]
	call cluster_to_lba

	mov ecx, 0x8
	call ata_lba_read

	add edi, 0x1000

	call get_next_cluster

	mov eax, dword [Cluster]
	cmp eax, 0x0FFFFFF8
	jae .done

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
	push edx
	mov edi, 0x6000
	mov ecx, 1
	call ata_lba_read
	pop edx

	; Get fat entry
	mov esi, 0x6000
	add esi, edx

	; Get and store cluster
	mov eax, dword [esi]
	and eax, 0x0FFFFFFF
	
	mov dword [Cluster], eax

	popad
	ret

; Clears the screen
clear_screen:
	push eax
	push ebx
	mov eax, 80
	mov ebx, 25
	mul ebx

	mov ebx, 0xB8000
.loop:
	mov [ebx], byte 0x0
	inc ebx
	dec eax

	cmp eax, 0
	je .done
	jmp .loop

.done:
	pop ebx
	pop eax
	ret

print_eax_decimal:
	pushad

	mov ecx, 0             ; Contador de dígitos
	mov ebx, 10            ; Divisor

	mov esi, [ADDR]
	cmp eax, 0
	jne .convert_loop
	mov byte [esi], '0'
	mov byte [esi+1], 0x0F ; Atributo de cor
	add esi, 2
	jmp .done

.convert_loop:
	xor edx, edx
	div ebx                ; EAX=quociente, EDX=resto
	push edx               ; Salva dígito
	inc ecx
	cmp eax, 0
	jne .convert_loop

.print_digits:
	pop edx
	add dl, '0'            ; Converte para ASCII
	mov [esi], dl          ; Escreve caractere
	mov byte [esi+1], 0x0F ; Cor cinza claro
	add esi, 2
	loop .print_digits
	add esi, 2
	mov [ADDR], esi

.done:
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
	ret

entry_file_name: db "KERNEL  BIN"

DataSector: dd 0x0
Cluster: dd 0x0
ADDR: dd 0x0