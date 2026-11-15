#include <kernel/input/keyboard.h>
#include <kernel/printk.h>
#include <kernel/input.h>
#include <kernel/init.h>
#include <device/terminal.h>
#include <device/tty.h>
#include <lib/string.h>
#include <stdbool.h>

typedef struct {
	u8 normal;    // Ex: 'a', '1', '/'
	u8 shifted;   // Ex: 'A', '!', '?'
} keymap_entry_t;

static kb_state_t kb_state;

static const keymap_entry_t qwerty_us_map[NR_KEYS] = {
	[KEY_0] = {'0', ')'},
	[KEY_1] = {'1', '!'},
	[KEY_2] = {'2', '@'},
	[KEY_3] = {'3', '#'},
	[KEY_4] = {'4', '$'},
	[KEY_5] = {'5', '%'},
	[KEY_6] = {'6', '^'},
	[KEY_7] = {'7', '&'},
	[KEY_8] = {'8', '*'},
	[KEY_9] = {'9', '('},

	[KEY_A] = {'a', 'A'}, [KEY_B] = {'b', 'B'}, [KEY_C] = {'c', 'C'},
	[KEY_D] = {'d', 'D'}, [KEY_E] = {'e', 'E'}, [KEY_F] = {'f', 'F'},
	[KEY_G] = {'g', 'G'}, [KEY_H] = {'h', 'H'}, [KEY_I] = {'i', 'I'},
	[KEY_J] = {'j', 'J'}, [KEY_K] = {'k', 'K'}, [KEY_L] = {'l', 'L'},
	[KEY_M] = {'m', 'M'}, [KEY_N] = {'n', 'N'}, [KEY_O] = {'o', 'O'},
	[KEY_P] = {'p', 'P'}, [KEY_Q] = {'q', 'Q'}, [KEY_R] = {'r', 'R'},
	[KEY_S] = {'s', 'S'}, [KEY_T] = {'t', 'T'}, [KEY_U] = {'u', 'U'},
	[KEY_V] = {'v', 'V'}, [KEY_W] = {'w', 'W'}, [KEY_X] = {'x', 'X'},
	[KEY_Y] = {'y', 'Y'}, [KEY_Z] = {'z', 'Z'},

	[KEY_GRAVE]       = {'`', '~'},
	[KEY_MINUS]       = {'-', '_'},
	[KEY_EQUAL]       = {'=', '+'},
	[KEY_LEFT_BRACE]  = {'[', '{'},
	[KEY_RIGHT_BRACE] = {']', '}'},
	[KEY_BACK_SLASH]  = {'\\', '|'},
	[KEY_SEMI_COLON]  = {';', ':'},
	[KEY_APOSTROPHE]  = {'\'', '"'},
	[KEY_COMMA]       = {',', '<'},
	[KEY_DOT]         = {'.', '>'},
	[KEY_SLASH]       = {'/', '?'},

	[KEY_SPACE]     = {' ', ' '},
	[KEY_ENTER]     = {'\r', '\r'},
	[KEY_TAB]       = {'\t', '\t'},
	[KEY_BACKSPACE] = {0x7F, 0x7F},

	// Numpad (NumLock=true)
	[KEY_KP_0]        = {'0', '0'}, [KEY_KP_1] = {'1', '1'},
	[KEY_KP_2]        = {'2', '2'}, [KEY_KP_3] = {'3', '3'},
	[KEY_KP_4]        = {'4', '4'}, [KEY_KP_5] = {'5', '5'},
	[KEY_KP_6]        = {'6', '6'}, [KEY_KP_7] = {'7', '7'},
	[KEY_KP_8]        = {'8', '8'}, [KEY_KP_9] = {'9', '9'},
	[KEY_KP_DOT]      = {'.', '.'},
	[KEY_KP_SLASH]    = {'/', '/'},
	[KEY_KP_ASTERISK] = {'*', '*'},
	[KEY_KP_MINUS]    = {'-', '-'},
	[KEY_KP_PLUS]     = {'+', '+'},
	[KEY_KP_ENTER]    = {'\r', '\r'},
};

static bool is_combo(bool alt, bool crtl, bool shift){
	return kb_state.alt == alt &&
		   kb_state.ctrl == crtl &&
		   kb_state.shift == shift;
}

static u8 keycode_to_ascii(enum input_keycode code) {
    if (code >= NR_KEYS) return '\0';

    keymap_entry_t entry = qwerty_us_map[code];
	
	// fallback to unknow mapped keys
    if (entry.normal == '\0') return '\0';

    bool is_alpha = (code >= KEY_A && code <= KEY_Z);
    
    bool use_uppercase = false;
    if (is_alpha) {

		if(is_combo(false, true, false)){
			return entry.normal & 0x1F;
		}

        use_uppercase = kb_state.shift ^ kb_state.capslock;
    } else {
        use_uppercase = kb_state.shift;
    }

    return use_uppercase ? entry.shifted : entry.normal;
}

static bool update_kb_state(enum input_keycode keycode, bool release){
	switch (keycode) {
		case KEY_RIGHT_SHIFT:
		case KEY_LEFT_SHIFT:
			kb_state.shift = !release;
			return true;
		case KEY_RIGHT_CTRL:
		case KEY_LEFT_CTRL:
			kb_state.ctrl = !release;
			return true;
		case KEY_RIGHT_ALT:
		case KEY_LEFT_ALT:
			kb_state.alt = !release;
			return true;
		case KEY_CAPSLOCK:
			if(!release)
				kb_state.capslock = !kb_state.capslock;
			return true;
		case KEY_NUMLOCK:
			if(!release)
				kb_state.numlock = !kb_state.numlock;
			return true;
		case KEY_SCROLLLOCK:
			if(!release)
				kb_state.scrolllock = !kb_state.scrolllock;
			return true;
		default: return release;
	}
}

static bool is_fkey(enum input_keycode keycode){
	return keycode >= KEY_F1 && keycode <= KEY_F24;
}

static const char* keycode_to_escape_seq(enum input_keycode code) {
	switch (code) {
		case KEY_UP:        return "\033[A";
		case KEY_DOWN:      return "\033[B";
		case KEY_RIGHT:     return "\033[C";
		case KEY_LEFT:      return "\033[D";
		case KEY_HOME:      return "\033[H";
		case KEY_END:       return "\033[F";
		case KEY_PAGE_UP:   return "\033[5~";
		case KEY_PAGE_DOWN: return "\033[6~";
		case KEY_INSERT:    return "\033[2~";
		case KEY_DELETE:    return "\033[3~";
		default:            return NULL;
	}
}

static enum input_keycode handle_numpad(enum input_keycode code) {
	if (kb_state.numlock)
		return code;

	switch (code) {
		case KEY_KP_8: return KEY_UP;
		case KEY_KP_2: return KEY_DOWN;
		case KEY_KP_4: return KEY_LEFT;
		case KEY_KP_6: return KEY_RIGHT;
		case KEY_KP_7: return KEY_HOME;
		case KEY_KP_1: return KEY_END;
		case KEY_KP_9: return KEY_PAGE_UP;
		case KEY_KP_3: return KEY_PAGE_DOWN;
		case KEY_KP_0: return KEY_INSERT;
		case KEY_KP_DOT: return KEY_DELETE;
		default: return code;
	}
}

static void key_event(struct input_handler *self, const struct input_event *event){
	if(event->type != INPUT_EVENT_KEY) return;

	enum input_keycode keycode = event->key.keycode;
	bool release = event->key.state == INPUT_KEY_RELEASED;

	if(update_kb_state(keycode, release)) return;

	if(is_combo(true, false, false) && is_fkey(keycode)){
		terminal_switch(keycode - KEY_F1);
		return;
	}

	keycode = handle_numpad(keycode);

	struct tty_struct *tty = terminal_get_current()->tty;
	if (!tty) return;

	const char *seq = keycode_to_escape_seq(keycode);
	if (seq) {
		tty_receive_buf(tty, (const u8*)seq, strlen(seq));
		return;
	}

	u8 ch = keycode_to_ascii(keycode);
	if (ch != '\0') {
		tty_receive_buf(tty, &ch, 1);
	}
}

static struct input_handler keyboard_event_listener =  {
	.event = key_event
};

static int __init keyboard_init(void){
	memset(&kb_state, 0x0, sizeof(kb_state_t));
	kb_state.numlock = true;
	return input_register_handler(&keyboard_event_listener);
}

device_initcall(keyboard_init);
