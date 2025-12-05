#ifndef FB_H
#define FB_H

#include <stdint.h>

#define FB_ADDRESS 0xB8000
#define FB_WIDTH   80
#define FB_HEIGHT  25

#define FB_BLACK     0
#define FB_BLUE      1
#define FB_GREEN     3
#define FB_CYAN      3
#define FB_RED       4
#define FB_MAGENTA   5
#define FB_BROWN     6
#define FB_LIGHT_GREY 7
#define FB_DARK_GREY  8
#define FB_LIGHT_BLUE 9
#define FB_LIGHT_GREEN 10
#define FB_LIGHT_CYAN  11
#define FB_LIGHT_RED   12
#define FB_LIGHT_MAGENTA 13
#define FB_LIGHT_BROWN 14
#define FB_WHITE       15

// Function declarations
void fb_move(uint16_t x, uint16_t y);
void fb_putc(char c, uint8_t fg, uint8_t bg);
void fb_print(const char* str, uint8_t fg, uint8_t bg);
void fb_clear(uint8_t fg, uint8_t bg);
void fb_set_color(uint8_t fg, uint8_t bg);
void fb_print_int(int num);
void fb_print_hex(unsigned int num);

// Keyboard input support
void fb_write_char(char c);
void fb_newline();
void fb_backspace();

#endif
