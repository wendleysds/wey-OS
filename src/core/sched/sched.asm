section .asm

global pcb_return

pcb_restore:
    ; Restore registers and segments
    ret

pcb_return:
    ; TODO: Implement
    ;iretd
    ret
