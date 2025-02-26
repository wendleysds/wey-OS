[ORG 0x7C00]

BITS 16

start:
  cli                                      ; Disable interrupts
  xor ax, ax      
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov sp, 0x7C00                           ; Define stack

  mov si, msg_loading
  call print_string

  mov bx, 0x1000
  mov dh, 0                                ; Drive 0 (boot)
  call disk_load

  call enter_protected_mode

  hlt
  jmp $

print_string:
  mov ah, 0x0E                             ; Print char func from BIOS
.loop:
  lodsb
  or al, al
  jz .done
  int 0x10
  jmp .loop
.done:
	ret

msg_loading db "Loading kernel...", 0

disk_load:
  pusha
  mov ah, 0x02                             ; disk reader service
  mov al, 10
  mov ch, 0
  mov cl, 2
  mov dh, 0
  int 0x13
  jc disk_error
  popa
  ret

disk_error:
  mov si, msg_disk_error
  call print_string
  hlt
  jmp $

msg_disk_error db "Disk read error!", 0

enter_protected_mode:
  cli
  lgdt [gdt_descriptor]                    ; Load GDT

  mov eax, cr0
  or eax, 0x1                              ; Active PE (Protection Enable)
  mov cr0, eax

  jmp CODE_SEG:init_pm                     ; Jump to protected mode

;  --Protected Mode--
[BITS 32]
init_pm:
	mov ax, DATA_SEG                         ; Config data segments
  mov ds, ax
	mov es, ax
  mov fs, ax
	mov gs, ax
  mov ss, ax
  mov esp, 0x90000                         ; Define stack in the top of RAM

  ; Call Kernel!!!!!!!!!!!
  call 0x1000
	mov si, msg_kernel_ok

	hlt
  jmp $

msg_kernel_ok db "Kernel loaded!", 0

; --GDT--
gdt:
  dq 0x0000000000000000                    ; Null segment
gdt_code:
  dq 0x00CF9A000000FFFF                    ; Code (Executável, Readable)
gdt_data:
  dq 0x00CF92000000FFFF                    ; Data (Read/Write)

gdt_descriptor:
  dw gdt_descriptor - gdt - 1
  dd gdt

CODE_SEG equ gdt_code - gdt
DATA_SEG equ gdt_data - gdt

;  --Boot assignature--
times 510-($-$$) db 0
dw 0xAA55

