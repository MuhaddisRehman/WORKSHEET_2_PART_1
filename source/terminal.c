#include "terminal.h"
#include "drivers/fb.h"
#include "drivers/keyboard.h"
#include "drivers/type.h"

#define MAX_COMMAND_LENGTH 256
#define MAX_ARGS_LENGTH 200

// String utility functions
static int str_len(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

static int str_cmp(const char* s1, const char* s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if ((unsigned char)s1[i] != (unsigned char)s2[i]) return (unsigned char)s1[i] - (unsigned char)s2[i];
        i++;
    }
    return (unsigned char)s1[i] - (unsigned char)s2[i];
}
static void str_copy(char* dest, const char* src, int dest_size) {
    int i = 0;
    if (dest_size <= 0) return;
    while (i < dest_size - 1 && src[i] != '\0') {
        dest[i] = src[i++];
    }
    dest[i] = '\0';
}

// Split command and arguments
static void parse_command(const char* input, char* cmd, int cmd_size, char* args, int args_size) {
    int i = 0, j = 0;

    /* skip leading whitespace */
    while (input[i] != '\0' && input[i] == ' ') i++;

    /* extract command */
    j = 0;
    while (input[i] != '\0' && input[i] != ' ' && j < cmd_size - 1) {
        cmd[j++] = input[i++];
    }
    cmd[j] = '\0';

    /* skip spaces */
    while (input[i] != '\0' && input[i] == ' ') i++;

    /* copy remainder into args */
    j = 0;
    while (input[i] != '\0' && j < args_size - 1) {
        args[j++] = input[i++];
    }
    args[j] = '\0';
}

// Command implementations
static void cmd_echo(char* args) {
    fb_set_color(FB_LIGHT_GREY, FB_BLACK);
    fb_print(args, FB_LIGHT_GREY, FB_BLACK);
    fb_print("\n", FB_LIGHT_GREY, FB_BLACK);
}

static void cmd_clear(char* args) {
    (void)args; // Unused parameter
    fb_clear(FB_LIGHT_GREY, FB_BLACK);
    fb_set_color(FB_LIGHT_GREY, FB_BLACK);
}

static void cmd_help(char* args) {
    (void)args; // Unused parameter
    fb_set_color(FB_LIGHT_CYAN, FB_BLACK);
    fb_print("Available commands:\n", FB_LIGHT_CYAN, FB_BLACK);
    fb_print("  echo [text]  - Display the provided text\n", FB_LIGHT_GREY, FB_BLACK);
    fb_print("  clear        - Clear the screen\n", FB_LIGHT_GREY, FB_BLACK);
    fb_print("  help         - Show this help message\n", FB_LIGHT_GREY, FB_BLACK);
    fb_print("  version      - Display OS version\n", FB_LIGHT_GREY, FB_BLACK);
    fb_print("\n", FB_LIGHT_GREY, FB_BLACK);
}

static void cmd_version(char* args) {
    (void)args; // Unused parameter
    fb_set_color(FB_LIGHT_GREEN, FB_BLACK);
    fb_print("TinyOS v1.0 - Worksheet 2 Part 2\n", FB_LIGHT_GREEN, FB_BLACK);
    fb_print("Built with interrupts and keyboard support\n", FB_LIGHT_GREY, FB_BLACK);
}

// Command structure
struct command {
    const char* name;
    void (*function)(char* args);
};

// Command table
static struct command commands[] = {
    {"echo", cmd_echo},
    {"clear", cmd_clear},
    {"help", cmd_help},
    {"version", cmd_version},
    {0, 0} // Sentinel
};

// Execute command
static void execute_command(const char* input) {
    char cmd[MAX_COMMAND_LENGTH];
    char args[MAX_ARGS_LENGTH];
    parse_command(input, cmd, MAX_COMMAND_LENGTH, args, MAX_ARGS_LENGTH);

    // Empty command
    if (str_len(cmd) == 0) {
        return;
    }

    // Find and execute command
    int i = 0;
    while (commands[i].name != 0) {
        if (str_cmp(cmd, commands[i].name) == 0) {
            commands[i].function(args);
            return;
        }
        i++;
    }

    // Unknown command
    fb_set_color(FB_LIGHT_RED, FB_BLACK);
    fb_print("Unknown command: ", FB_LIGHT_RED, FB_BLACK);
    fb_print(cmd, FB_LIGHT_RED, FB_BLACK);
    fb_print("\nType 'help' for available commands.\n", FB_LIGHT_GREY, FB_BLACK);
}

// Terminal initialization
void terminal_init() {
    fb_clear(FB_LIGHT_GREY, FB_BLACK);
    fb_set_color(FB_LIGHT_CYAN, FB_BLACK);
    fb_print("TinyOS Terminal\n", FB_LIGHT_CYAN, FB_BLACK);
    fb_print("Type 'help' for available commands\n\n", FB_LIGHT_GREY, FB_BLACK);
    keyboard_init_buffer();
}

// Terminal main loop
void terminal_run() {
    char input[MAX_COMMAND_LENGTH];

    while (1) {
        // Display prompt
        fb_set_color(FB_LIGHT_GREEN, FB_BLACK);
        fb_print("myos> ", FB_LIGHT_GREEN, FB_BLACK);
        fb_set_color(FB_LIGHT_GREY, FB_BLACK);

        // Read input
        readline(input, MAX_COMMAND_LENGTH);

        // Execute command
        execute_command(input);
    }
}
