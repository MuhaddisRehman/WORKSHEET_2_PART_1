#include "fb.h"

static uint16_t* fb = (uint16_t*)FB_ADDRESS;
static uint16_t cursor_x = 0;
static uint16_t cursor_y = 0;
static uint8_t current_fg = FB_LIGHT_GREY;
static uint8_t current_bg = FB_BLACK;

void fb_move(uint16_t x, uint16_t y) {
    cursor_x = x;
    cursor_y = y;
}

void fb_putc(char c, uint8_t fg, uint8_t bg) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        return;
    }
    fb[cursor_y * FB_WIDTH + cursor_x] = ((bg & 0x0F) << 12) | ((fg & 0x0F) << 8) | c;
    cursor_x++;
    if (cursor_x >= FB_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
}

void fb_print(const char* str, uint8_t fg, uint8_t bg) {
    while (*str) {
        fb_putc(*str++, fg, bg);
    }
}

void fb_clear(uint8_t fg, uint8_t bg) {
    for (int y = 0; y < FB_HEIGHT; y++) {
        for (int x = 0; x < FB_WIDTH; x++) {
            fb[y * FB_WIDTH + x] = ((bg & 0x0F) << 12) | ((fg & 0x0F) << 8) | ' ';
        }
    }
    fb_move(0, 0);
}

void fb_set_color(uint8_t fg, uint8_t bg) {
    current_fg = fg;
    current_bg = bg;
}

void fb_print_int(int num) {
    char buffer[12];
    int i = 0;
    int is_negative = 0;

    if (num == 0) {
        fb_putc('0', current_fg, current_bg);
        return;
    }

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    while (num != 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    if (is_negative) {
        fb_putc('-', current_fg, current_bg);
    }

    while (i > 0) {
        fb_putc(buffer[--i], current_fg, current_bg);
    }
}

void fb_print_hex(unsigned int num) {
    char buffer[9];
    const char* hex_digits = "0123456789ABCDEF";
    int i = 0;

    if (num == 0) {
        fb_print("0x0", current_fg, current_bg);
        return;
    }

    while (num != 0) {
        buffer[i++] = hex_digits[num & 0xF];
        num >>= 4;
    }

    fb_print("0x", current_fg, current_bg);
    while (i > 0) {
        fb_putc(buffer[--i], current_fg, current_bg);
    }
}

void fb_write_char(char c) {
    fb_putc(c, current_fg, current_bg);
}

void fb_newline() {
    cursor_x = 0;
    cursor_y++;
    if (cursor_y >= FB_HEIGHT) {
        cursor_y = FB_HEIGHT - 1;
    }
}

void fb_backspace() {
    if (cursor_x > 0) {
        cursor_x--;
        fb[cursor_y * FB_WIDTH + cursor_x] = ((current_bg & 0x0F) << 12) | ((current_fg & 0x0F) << 8) | ' ';
    } else if (cursor_y > 0) {
        cursor_y--;
        cursor_x = FB_WIDTH - 1;
        fb[cursor_y * FB_WIDTH + cursor_x] = ((current_bg & 0x0F) << 12) | ((current_fg & 0x0F) << 8) | ' ';
    }
}
