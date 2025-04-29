#include <drivers/keyboard.h>
#include <drivers/terminal.h>
#include <arch/i386/idt.h>
#include <io.h>
#include <stdint.h>

char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', // 0x00 - 0x09
    '9', '0', '-', '=', '\b', // Backspace
    '\t', // Tab
    'q', 'w', 'e', 'r', // 0x10 - 0x13
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', // Enter key
    0, // Controle esquerdo (Ctrl)
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', // 0x1E - 0x29
    0, // Shift esquerdo
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 
    0, // Shift direito
    '*',
    0,  // Alt esquerdo
    ' ', // Espaço
    0,  // CapsLock
    // Teclas de função (F1 - F10)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, // NumLock
    0, // Scroll Lock
    // Home, seta para cima, PageUp
    0, // Home
    0, // seta para cima
    0, // Page Up
    '-',
    0, // seta para esquerda
    0,
    0, // seta para direita
    '+',
    0, // End
    0, // seta para baixo
    0, // Page Down
    0, // Insert
    0, // Delete
    0, 0, 0,
    0, // F11
    0, // F12
    0, // Tudo além é 0 por enquanto
};


void _iqr_keyboard_handler(struct InterruptFrame* frame){
	uint8_t scanCode = inb(0x60);

	/*if(!(scanCode & 0x60)){
		terminal_write(TERMINAL_DEFAULT_COLOR, "Scancode: 0x%x\n", scanCode);
	}*/

	if(scanCode < 128){
		char c = scancode_to_ascii[scanCode];
		terminal_write(TERMINAL_DEFAULT_COLOR, "%c", c);
	}

	outb(0x20, 0x20);
}

void init_keyboard(){
	idt_register_callback(0x21, _iqr_keyboard_handler);
	outb(0x64, 0xAE);
}
