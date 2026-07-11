#include "kernel/printk.h"
#include <kernel/input/keyboard.h>
#include <kernel/interrupt.h>
#include <kernel/init.h>
#include <kernel/input.h>
#include <lib/string.h>
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
		case 1: return KEY_ESC;
		case 2: return KEY_1;
		case 3: return KEY_2;
		case 4: return KEY_3;
		case 5: return KEY_4;
		case 6: return KEY_5;
		case 7: return KEY_6;
		case 8: return KEY_7;
		case 9: return KEY_8;
		case 10: return KEY_9;
		case 11: return KEY_0;
		case 14: return KEY_BACKSPACE;
		case 15: return KEY_TAB;
		case 16: return KEY_Q;
		case 17: return KEY_W;
		case 18: return KEY_E;
		case 19: return KEY_R;
		case 20: return KEY_T;
		case 21: return KEY_Y;
		case 22: return KEY_U;
		case 23: return KEY_I;
		case 24: return KEY_O;
		case 25: return KEY_P;
		case 28: return KEY_ENTER;
		case 29: return KEY_LEFT_CTRL;
		case 30: return KEY_A;
		case 31: return KEY_S;
		case 32: return KEY_D;
		case 33: return KEY_F;
		case 34: return KEY_G;
		case 35: return KEY_H;
		case 36: return KEY_J;
		case 37: return KEY_K;
		case 38: return KEY_L;
		case 42: return KEY_LEFT_SHIFT;
		case 44: return KEY_Z;
		case 45: return KEY_X;
		case 46: return KEY_C;
		case 47: return KEY_V;
		case 48: return KEY_B;
		case 49: return KEY_N;
		case 50: return KEY_M;
		case 54: return KEY_RIGHT_SHIFT;
		case 56: return KEY_LEFT_ALT;
		case 57: return KEY_SPACE;
		case 58: return KEY_CAPSLOCK;
		case 59: return KEY_F1;
		case 60: return KEY_F2;
		case 61: return KEY_F3;
		case 62: return KEY_F4;
		case 63: return KEY_F5;
		case 64: return KEY_F6;
		case 65: return KEY_F7;
		case 66: return KEY_F8;
		case 67: return KEY_F9;
		case 68: return KEY_F10;
		case 69: return KEY_NUMLOCK;
		case 70: return KEY_SCROLLLOCK;
		case 87: return KEY_F11;
		case 88: return KEY_F12;
		default: return KEY_UNKNOWN;
	}
}

static kb_state_t kb_state;

static inline uint32_t _keyboard_get_modifiers(void){
	uint32_t mods = 0;

	if(kb_state.shift)
		mods |= INPUT_MOD_SHIFT;
	if(kb_state.ctrl)
		mods |= INPUT_MOD_CTRL;
	if(kb_state.alt)
		mods |= INPUT_MOD_ALT;
	if(kb_state.capslock)
		mods |= INPUT_MOD_CAPSLOCK;
	if(kb_state.numlock)
		mods |= INPUT_MOD_NUMLOCK;
	if(kb_state.scrolllock)
		mods |= INPUT_MOD_SCROLLLOCK;

	return mods;
}

static inline void _keyboard_handle_scancode(uint8_t sc){
	uint8_t release = sc & _KEYBOARD_KEY_RELEASED;
	uint8_t code = sc & ~_KEYBOARD_KEY_RELEASED;

	enum input_keycode keycode = _scancode_to_keycode(code);

	switch (keycode)
	{
		case KEY_RIGHT_SHIFT:
		case KEY_LEFT_SHIFT:
			kb_state.shift = !release;
			break;
		case KEY_RIGHT_CTRL:
		case KEY_LEFT_CTRL:
			kb_state.ctrl = !release;
			break;
		case KEY_RIGHT_ALT:
		case KEY_LEFT_ALT:
			kb_state.alt = !release;
			break;
		case KEY_CAPSLOCK:
			if(!release)
				kb_state.capslock = !kb_state.capslock;
		case KEY_NUMLOCK:
			if(!release)
				kb_state.numlock = !kb_state.numlock;
		case KEY_SCROLLLOCK:
			if(!release)
				kb_state.scrolllock = !kb_state.scrolllock;
		default: break;
	}

	if (keycode != KEY_UNKNOWN && keycode != KEY_NONE) {
		struct input_event event = {0};
		event.type = INPUT_EVENT_KEY;
		event.key.keycode = keycode;
		event.key.state = release ? INPUT_KEY_RELEASED : INPUT_KEY_PRESSED;
		event.key.modifiers = _keyboard_get_modifiers();
		input_report(&event);
	}
}

static void _iqr_keyboard_handler(struct irq_info* unused){
	uint8_t scancode = inb(_PS2_INPUT_PORT);
	_keyboard_handle_scancode(scancode);
}

static int __init ps2_keyboard_init(){
	outb(_PS2_COMMAND_PORT, _PS2_ENABLE_FIRST_PORT);
	memset(&kb_state, 0, sizeof(kb_state_t));

	return irq_register(IRQ_KEYBOARD, _iqr_keyboard_handler, NULL);
}

device_initcall(ps2_keyboard_init);
