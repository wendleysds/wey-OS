section .asm

global pcb_return

;struct Registers {
;    uint32_t eax    ( 0), ebx ( 4), ecx ( 8), edx (12); 
;    uint32_t esi    (16), edi (20), ebp (24);
;    uint32_t eip    (28), esp (32);
;    uint32_t ss     (36), cs  (40);
;    uint32_t eflags (44);
;} __attribute__((packed));


; void _restore_register(struct Registers *regs)
; Expects: [esp+4] = pointer to Registers struct
_restore_registers:
    mov ebp, [esp+4]

    mov eax, [ebp]        ; eax
    mov ebx, [ebp+4]      ; ebx
    mov ecx, [ebp+8]      ; ecx
    mov edx, [ebp+12]     ; edx
    mov esi, [ebp+16]     ; esi
    mov edi, [ebp+20]     ; edi
    mov ebp, [ebp+24]     ; ebp

    ret

; void pcb_return(struct Registers *regs)
; Expects: [esp+4] = pointer to Registers struct
pcb_return:
    mov esi, [esp+4]

    ; Check if CS & 0x3 == 3 -> user mode
    mov eax, [esi+40]
    and eax, 0x3
    cmp eax, 0x3
    je .user_return

.kernel_return:
    mov eax, [esi+44]     ; EFLAGS
    or eax, 0x200         ; IF = 1
    push eax
    push dword [esi+40]   ; CS
    push dword [esi+28]   ; EIP

    ; Re-updated the segments
    mov ax, 0x10          ; Kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push dword [esi+4]
    call _restore_registers
    add esp, 4

    iretd

.user_return:
    push dword [esi+36]   ; Stack Selector
    push dword [esi+32]   ; Stack Pointer

    mov eax, [esi+44]     ; Flags
    or eax, 0x200         ; Enable Flag Set
    push eax

    push dword [esi+40]   ; Code Segment
    push dword [esi+28]   ; Instruct Pointer

    ; Segments
    mov ax, [esi+36]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push dword [esi+4]
    call _restore_registers
    add esp, 4

    iretd