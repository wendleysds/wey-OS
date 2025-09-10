#ifndef _KEYBOARD_H
#define _KEYBOARD_H

typedef void(*keyboard_callback_t)(char); 

void keyboard_init();
void keyboard_set_callback(keyboard_callback_t callback);

#endif
