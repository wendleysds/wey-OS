section .asm

global pcb_return
global pcb_save_context

;struct Registers {
;    uint32_t eax    ( 0), ebx ( 4), ecx ( 8), edx (12); 
;    uint32_t esi    (16), edi (20), ebp (24);
;    uint32_t eip    (28), esp (32);
;    uint32_t ss     (36), cs  (40);
;    uint32_t eflags (44);
;} __attribute__((packed));


; int pcb_save_context(struct Registers* regs)
pcb_save_context:
    pusha
    mov eax, [esp + 36]  ; Pointer to struct Registers (esp+36: 8 regs * 4 + return addr)

    mov ebx, [esp + 28]  ; edi
    mov [eax + 20], ebx
    mov ebx, [esp + 24]  ; esi
    mov [eax + 16], ebx
    mov ebx, [esp + 20]  ; ebp
    mov [eax + 24], ebx
    mov ebx, [esp + 16]  ; esp (original value before pusha)
    mov [eax + 32], ebx
    mov ebx, [esp + 12]  ; ebx
    mov [eax + 4], ebx
    mov ebx, [esp + 8]   ; edx
    mov [eax + 12], ebx
    mov ebx, [esp + 4]   ; ecx
    mov [eax + 8], ebx
    mov ebx, [esp]       ; eax
    mov [eax + 0], ebx

    pushfd
    pop ebx
    mov [eax + 44], ebx  ; eflags

    mov bx, cs
    mov [eax + 40], ebx  ; cs

    mov bx, ss
    mov [eax + 36], ebx  ; ss

    call .get_eip
	mov eax, 0
	ret

.get_eip:
    pop ebx
    mov [eax + 28], ebx  ; eip

    lea ebx, [esp + 36]  ; esp after pusha, pushfd, call
    mov [eax + 32], ebx  ; save esp

    popa
    mov eax, 1 
    ret

; void _restore_register(struct Registers *regs)
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

    push dword esi
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

    push dword esi
    call _restore_registers
    add esp, 4

    iretd
