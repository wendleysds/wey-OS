section .asm

global pcb_return

pcb_restore:
    ret

pcb_return:
    iretd

_restore_gp_regs:
    ret

_restore_se_regs:
    ret