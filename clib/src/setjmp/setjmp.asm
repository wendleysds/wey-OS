; setjmp.asm – i386 implementation of setjmp() and longjmp()
;
; jmp_buf layout (array of 6 x uint32_t):
;   offset  0  (+0)  : ebx
;   offset  4  (+4)  : esi
;   offset  8  (+8)  : edi
;   offset 12  (+12) : ebp
;   offset 16  (+16) : esp  (caller's esp *after* call, i.e. after return addr is pushed)
;   offset 20  (+20) : eip  (return address on the caller's stack)

bits 32

global setjmp
global longjmp

; ─────────────────────────────────────────────────────────────────────────────
; int setjmp(jmp_buf env)
;
; Calling convention (cdecl):
;   [esp+0]  = return address
;   [esp+4]  = env  (pointer to jmp_buf)
;
; Returns 0 on direct call.
; ─────────────────────────────────────────────────────────────────────────────
setjmp:
    mov  eax, [esp+4]       ; eax = env (jmp_buf *)

    ; Save callee-saved integer registers
    mov  [eax+0],  ebx
    mov  [eax+4],  esi
    mov  [eax+8],  edi
    mov  [eax+12], ebp

    ; Save the caller's stack pointer.
    ; At entry esp points to the return address, so esp+4 is where the
    ; caller's stack was before the call instruction.
    lea  ecx, [esp+4]
    mov  [eax+16], ecx

    ; Save return address (= where longjmp should jump to)
    mov  ecx, [esp]
    mov  [eax+20], ecx

    ; Return 0
    xor  eax, eax
    ret

; ─────────────────────────────────────────────────────────────────────────────
; void longjmp(jmp_buf env, int val)  [noreturn]
;
; Calling convention (cdecl):
;   [esp+0]  = return address (unused – we never return normally)
;   [esp+4]  = env  (pointer to jmp_buf)
;   [esp+8]  = val
; ─────────────────────────────────────────────────────────────────────────────
longjmp:
    mov  edx, [esp+4]       ; edx = env (jmp_buf *)
    mov  eax, [esp+8]       ; eax = val

    ; If val == 0, force it to 1 (setjmp must never appear to return 0)
    test eax, eax
    jnz  .nonzero
    mov  eax, 1
.nonzero:

    ; Restore callee-saved registers
    mov  ebx, [edx+0]
    mov  esi, [edx+4]
    mov  edi, [edx+8]
    mov  ebp, [edx+12]

    ; Restore esp and eip.
    ; We put the saved return address on the restored stack so that a plain
    ; 'ret' at the end jumps to the right place without clobbering eax.
    mov  esp, [edx+16]      ; restore caller's esp
    mov  ecx, [edx+20]      ; ecx = saved eip (return address)
    push ecx                ; place it on the (now-restored) stack
    ret                     ; jump to saved eip, with eax = val
