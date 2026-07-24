#include <kernel/interrupt.h>
#include <kernel/init.h>
#include <kernel/input.h>
#include <io/ports.h>

/*
 * Simple PS/2 keyboard driver
 */

#define _PS2_COMMAND_PORT 0x64
#define _PS2_INPUT_PORT 0x60
#define _PS2_ENABLE_FIRST_PORT 0xAE

#define _IQR_KEYBOARD_INTERRUPT 0x21
#define _KEYBOARD_KEY_RELEASED 0x80

static enum input_keycode _scancode_to_keycode(uint8_t scancode){
	switch (scancode) {
		case 0x01: return KEY_ESC;
		case 0x02: return KEY_1;
		case 0x03: return KEY_2;
		case 0x04: return KEY_3;
		case 0x05: return KEY_4;
		case 0x06: return KEY_5;
		case 0x07: return KEY_6;
		case 0x08: return KEY_7;
		case 0x09: return KEY_8;
		case 0x0A: return KEY_9;
		case 0x0B: return KEY_0;
		case 0x0C: return KEY_MINUS;
		case 0x0D: return KEY_EQUAL;
		case 0x0E: return KEY_BACKSPACE;
		case 0x0F: return KEY_TAB;
		case 0x10: return KEY_Q;
		case 0x11: return KEY_W;
		case 0x12: return KEY_E;
		case 0x13: return KEY_R;
		case 0x14: return KEY_T;
		case 0x15: return KEY_Y;
		case 0x16: return KEY_U;
		case 0x17: return KEY_I;
		case 0x18: return KEY_O;
		case 0x19: return KEY_P;
		case 0x1A: return KEY_LEFT_BRACE;
		case 0x1B: return KEY_RIGHT_BRACE;
		case 0x1C: return KEY_ENTER;
		case 0x1D: return KEY_LEFT_CTRL;
		case 0x1E: return KEY_A;
		case 0x1F: return KEY_S;
		case 0x20: return KEY_D;
		case 0x21: return KEY_F;
		case 0x22: return KEY_G;
		case 0x23: return KEY_H;
		case 0x24: return KEY_J;
		case 0x25: return KEY_K;
		case 0x26: return KEY_L;
		case 0x27: return KEY_SEMI_COLON;
		case 0x28: return KEY_QUOTE;
		case 0x29: return KEY_GRAVE;
		case 0x2A: return KEY_LEFT_SHIFT;
		case 0x2B: return KEY_BACK_SLASH;
		case 0x2C: return KEY_Z;
		case 0x2D: return KEY_X;
		case 0x2E: return KEY_C;
		case 0x2F: return KEY_V;
		case 0x30: return KEY_B;
		case 0x31: return KEY_N;
		case 0x32: return KEY_M;
		case 0x33: return KEY_COMMA;
		case 0x34: return KEY_DOT;
		case 0x35: return KEY_SLASH;
		case 0x36: return KEY_RIGHT_SHIFT;
		case 0x37: return KEY_KP_ASTERISK;
		case 0x38: return KEY_LEFT_ALT;
		case 0x39: return KEY_SPACE;
		case 0x3A: return KEY_CAPSLOCK;
		case 0x3B: return KEY_F1;
		case 0x3C: return KEY_F2;
		case 0x3D: return KEY_F3;
		case 0x3E: return KEY_F4;
		case 0x3F: return KEY_F5;
		case 0x40: return KEY_F6;
		case 0x41: return KEY_F7;
		case 0x42: return KEY_F8;
		case 0x43: return KEY_F9;
		case 0x44: return KEY_F10;
		case 0x45: return KEY_NUMLOCK;
		case 0x46: return KEY_SCROLLLOCK;
		case 0x47: return KEY_KP_7;
		case 0x48: return KEY_KP_8;
		case 0x49: return KEY_KP_9;
		case 0x4A: return KEY_KP_MINUS;
		case 0x4B: return KEY_KP_4;
		case 0x4C: return KEY_KP_5;
		case 0x4D: return KEY_KP_6;
		case 0x4E: return KEY_KP_PLUS;
		case 0x4F: return KEY_KP_1;
		case 0x50: return KEY_KP_2;
		case 0x51: return KEY_KP_3;
		case 0x52: return KEY_KP_0;
		case 0x53: return KEY_KP_DOT;
		case 0x57: return KEY_F11;
		case 0x58: return KEY_F12;
		default: return KEY_UNKNOWN;
	}
}

static inline void _keyboard_handle_scancode(uint8_t sc){
	uint8_t release = sc & _KEYBOARD_KEY_RELEASED;
	uint8_t code = sc & ~_KEYBOARD_KEY_RELEASED;

	enum input_keycode keycode = _scancode_to_keycode(code);
	if (keycode != KEY_UNKNOWN && keycode != KEY_NONE) {
		struct input_event event = {0};
		event.type = INPUT_EVENT_KEY;
		event.key.keycode = keycode;
		event.key.state = release ? INPUT_KEY_RELEASED : INPUT_KEY_PRESSED;
		input_report(&event);
	}
}

static void _iqr_keyboard_handler(struct irq_info* unused){
	uint8_t scancode = inb(_PS2_INPUT_PORT);
	_keyboard_handle_scancode(scancode);
}

static int __init ps2_keyboard_init(){
	outb(_PS2_COMMAND_PORT, _PS2_ENABLE_FIRST_PORT);
	return irq_register(IRQ_KEYBOARD, _iqr_keyboard_handler, NULL);
}

device_initcall(ps2_keyboard_init);
