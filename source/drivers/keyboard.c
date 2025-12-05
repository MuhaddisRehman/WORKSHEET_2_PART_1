#include "keyboard.h"
#include "io.h"
#include "fb.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_COMMAND_PORT 0x64
#define INPUT_BUFFER_SIZE 256

static u8int input_buffer[INPUT_BUFFER_SIZE];
static u32int buffer_read_pos = 0;
static u32int buffer_write_pos = 0;
static u32int buffer_count = 0;

void keyboard_init_buffer() {
    buffer_read_pos = 0;
    buffer_write_pos = 0;
    buffer_count = 0;
}

void keyboard_init() {
    while (inb(KEYBOARD_STATUS_PORT) & 0x01) {
        inb(KEYBOARD_DATA_PORT);
    }
    keyboard_init_buffer();
}

static void buffer_put(u8int ch) {
    if (buffer_count < INPUT_BUFFER_SIZE) {
        input_buffer[buffer_write_pos] = ch;
        buffer_write_pos = (buffer_write_pos + 1) % INPUT_BUFFER_SIZE;
        buffer_count++;
    }
}

u8int getc() {
    if (buffer_count == 0) {
        return 0;
    }

    u8int ch = input_buffer[buffer_read_pos];
    buffer_read_pos = (buffer_read_pos + 1) % INPUT_BUFFER_SIZE;
    buffer_count--;
    return ch;
}

void readline(char* buffer, u32int max_size) {
    u32int i = 0;
    u8int ch;

    while (1) {
        while (buffer_count == 0);
        ch = getc();

        if (ch == '\n') {
            buffer[i] = '\0';
            fb_putc('\n', FB_LIGHT_GREY, FB_BLACK);
            return;
        }

        if (ch == '\b') {
            if (i > 0) {
                i--;
                buffer[i] = '\0';
                fb_putc('\b', FB_LIGHT_GREY, FB_BLACK);
                fb_putc(' ', FB_LIGHT_GREY, FB_BLACK);
                fb_putc('\b', FB_LIGHT_GREY, FB_BLACK);
            }
            continue;
        }

        if (i < max_size - 1) {
            buffer[i++] = ch;
            fb_putc(ch, FB_LIGHT_GREY, FB_BLACK);
        }
    }
}

u8int keyboard_read_scan_code(void)
{
    return inb(KEYBOARD_DATA_PORT);
}

u8int keyboard_scan_code_to_ascii(u8int scan_code)
{
    if (scan_code & 0x80) {
        return 0;
    }

    switch(scan_code) {
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';
        case 0x0C: return '-';
        case 0x0D: return '=';
        case 0x0E: return '\b';
        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        case 0x13: return 'r';
        case 0x14: return 't';
        case 0x15: return 'y';
        case 0x16: return 'u';
        case 0x17: return 'i';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x1A: return '[';
        case 0x1B: return ']';
        case 0x1C: return '\n';
        case 0x1E: return 'a';
        case 0x1F: return 's';
        case 0x20: return 'd';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x27: return ';';
        case 0x28: return '\'';
        case 0x29: return '`';
        case 0x2B: return '\\';
        case 0x2C: return 'z';
        case 0x2D: return 'x';
        case 0x2E: return 'c';
        case 0x2F: return 'v';
        case 0x30: return 'b';
        case 0x31: return 'n';
        case 0x32: return 'm';
        case 0x33: return ',';
        case 0x34: return '.';
        case 0x35: return '/';
        case 0x39: return ' ';
        case 0x37: return '*';
        case 0x47: return '7';
        case 0x48: return '8';
        case 0x49: return '9';
        case 0x4A: return '-';
        case 0x4B: return '4';
        case 0x4C: return '5';
        case 0x4D: return '6';
        case 0x4E: return '+';
        case 0x4F: return '1';
        case 0x50: return '2';
        case 0x51: return '3';
        case 0x52: return '0';
        case 0x53: return '.';
        default: return 0;
    }
}

void keyboard_handle_input(u8int ascii) {
    buffer_put(ascii);
}
