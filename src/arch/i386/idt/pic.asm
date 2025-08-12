[BITS 32]
section .asm

global pic_remap

pic_remap:
    mov al, 0x11
    out 0x20, al ; Tell master PIC
    out 0xA0, al

    mov al, 0x20 ; Interrupt 0x20 is where master ISR should start
    out 0x21, al

    mov al, 0x28
    out 0xA1, al

    mov al, 0x04 ; ICW3
    out 0x21, al

    mov al, 0x02
    out 0xA1, al

    mov al, 0x01
    out 0x21, al
    out 0xA1, al
    ret