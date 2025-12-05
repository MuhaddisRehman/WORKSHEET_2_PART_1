#ifndef INCLUDE_KEYBOARD_H
#define INCLUDE_KEYBOARD_H

#define KEYBOARD_MAX_ASCII 83

#include "type.h"

u8int keyboard_read_scan_code(void);
u8int keyboard_scan_code_to_ascii(u8int scan_code);
void keyboard_handle_input(u8int ascii);

// Input buffer API
void keyboard_init_buffer();
u8int getc();
void readline(char* buffer, u32int max_size);

#endif /* INCLUDE_KEYBOARD_H */
