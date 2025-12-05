# WORKSHEET 2 PART 1 - Tiny OS Development

**UFCFWK-15-2 Operating Systems**
**Student:** Muhaddis
**Date:** December 5, 2025

## Project Overview

This project implements a minimal operating system that boots from scratch using GRUB Legacy, demonstrating fundamental OS concepts including bootloading, calling C from assembly, and implementing a framebuffer driver for text output.

## Table of Contents

- [Task 1: Basic Bootloader](#task-1-basic-bootloader-20)
- [Task 2: Calling C from Assembly](#task-2-calling-c-from-assembly-20)
- [Task 3: Framebuffer Driver](#task-3-framebuffer-driver-40)
- [Building and Running](#building-and-running)
- [Project Structure](#project-structure)

---

## Task 1: Basic Bootloader (20%)

### Objective

Create a minimal bootable OS that initializes the system and sets up the multiboot header for GRUB.

### Implementation

#### Multiboot Header (`source/loader.asm`)

The bootloader implements the multiboot specification which GRUB uses to identify and load our kernel:

```asm
MAGIC_NUMBER equ 0x1BADB002
FLAGS equ 0x0
CHECKSUM equ -MAGIC_NUMBER

section .text
align 4
dd MAGIC_NUMBER     ; Multiboot magic number
dd FLAGS            ; Multiboot flags
dd CHECKSUM         ; Checksum (must sum to 0 with magic + flags)
```

**Key Points:**

- The multiboot header MUST be in the first 8KB of the kernel
- Magic number `0x1BADB002` identifies this as a multiboot-compliant kernel
- Checksum ensures integrity: `MAGIC_NUMBER + FLAGS + CHECKSUM = 0`

#### Linker Script (`source/link.ld`)

The linker script ensures our code loads at the correct memory address:

```ld
ENTRY(loader)
SECTIONS {
    . = 0x00100000;     /* Load at 1 MB (above BIOS/GRUB reserved area) */
    .text ALIGN (0x1000) : { *(.text) }
    .rodata ALIGN (0x1000) : { *(.rodata*) }
    .data ALIGN (0x1000) : { *(.data) }
    .bss ALIGN (0x1000) : {
        *(COMMON)
        *(.bss)
    }
}
```

#### GRUB Configuration (`boot/grub/menu.lst`)

```
default=0
timeout=0
title os
kernel /boot/kernel.elf
```

### Build Process

```bash
# Assemble bootloader
nasm -f elf32 source/loader.asm -o source/loader.o

# Link to create kernel ELF
ld -m elf_i386 -T source/link.ld source/loader.o -o iso/kernel.elf

# Create bootable ISO with GRUB Legacy
genisoimage -R -b boot/grub/stage2_eltorito -no-emul-boot \
    -boot-load-size 4 -A os -input-charset utf-8 \
    -quiet -boot-info-table -o os.iso iso
```

### Verification

Running the ISO in QEMU and checking the CPU log (`logQ.txt`) confirms the kernel boots and executes.

---

## Task 2: Calling C from Assembly (20%)

### Objective

Set up a C runtime environment from assembly and call C functions.

### Implementation

#### Stack Setup (`source/loader.asm`)

Before calling C, we must set up a stack:

```asm
KERNEL_STACK_SIZE equ 4096

section .bss
align 4
kernel_stack:
    resb KERNEL_STACK_SIZE

section .text
loader:
    mov esp, kernel_stack + KERNEL_STACK_SIZE   ; Stack grows downward
    call kernel_main                             ; Jump to C code
.loop:
    jmp .loop                                    ; Infinite loop
```

**How it works:**

- `esp` (stack pointer) points to the TOP of the stack
- Stack grows downward (decreasing addresses)
- We allocate 4KB for the kernel stack in the `.bss` section

#### C Functions (`source/kernel.c`)

```c
int sum_of_three(int a, int b, int c) {
    return a + b + c;
}

int multiply(int a, int b) {
    return a * b;
}

int subtract(int a, int b) {
    return a - b;
}
```

#### Calling Convention

The x86 calling convention (cdecl) used:

1. Arguments pushed onto stack (right-to-left)
2. `call` instruction pushes return address
3. Callee uses `ebp` to access arguments
4. Return value placed in `eax`
5. Caller cleans up stack

### Makefile Integration

```makefile
# Compile C code (32-bit, freestanding, no stack protector)
gcc -m32 -ffreestanding -fno-stack-protector -c source/kernel.c -o source/kernel.o

# Link assembly and C together
ld -m elf_i386 -T source/link.ld source/loader.o source/kernel.o -o iso/kernel.elf
```

**Flags explained:**

- `-m32`: Compile for 32-bit architecture
- `-ffreestanding`: No standard library
- `-fno-stack-protector`: Disable stack canaries (we have no runtime support)

---

## Task 3: Framebuffer Driver (40%)

### Objective

Implement a complete 2D framebuffer API for text output with color support.

### Framebuffer Basics

The VGA text mode framebuffer is located at physical address `0xB8000` and provides:

- 80 columns × 25 rows = 2000 characters
- Each character is 2 bytes: `[BG:4bits][FG:4bits][ASCII:8bits]`

### API Design (`source/drivers/fb.h`)

```c
#define FB_ADDRESS 0xB8000
#define FB_WIDTH   80
#define FB_HEIGHT  25

// Core functions
void fb_move(uint16_t x, uint16_t y);
void fb_putc(char c, uint8_t fg, uint8_t bg);
void fb_print(const char* str, uint8_t fg, uint8_t bg);
void fb_clear(uint8_t fg, uint8_t bg);

// Enhanced API
void fb_set_color(uint8_t fg, uint8_t bg);
void fb_print_int(int num);
void fb_print_hex(unsigned int num);
```

### Implementation Highlights

#### Character Output (`source/drivers/fb.c`)

```c
void fb_putc(char c, uint8_t fg, uint8_t bg) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        return;
    }

    // Format: [background:4][foreground:4][character:8]
    fb[cursor_y * FB_WIDTH + cursor_x] =
        ((bg & 0x0F) << 12) | ((fg & 0x0F) << 8) | c;

    cursor_x++;
    if (cursor_x >= FB_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
}
```

**Bit manipulation breakdown:**

- `(bg & 0x0F) << 12`: Background color in bits 15-12
- `(fg & 0x0F) << 8`: Foreground color in bits 11-8
- `c`: ASCII character in bits 7-0

#### Integer Printing

```c
void fb_print_int(int num) {
    char buffer[12];
    int i = 0;

    if (num == 0) {
        fb_putc('0', current_fg, current_bg);
        return;
    }

    int is_negative = num < 0;
    if (is_negative) num = -num;

    // Extract digits (reverse order)
    while (num != 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    if (is_negative) fb_putc('-', current_fg, current_bg);

    // Print in correct order
    while (i > 0) {
        fb_putc(buffer[--i], current_fg, current_bg);
    }
}
```

#### Hexadecimal Printing

```c
void fb_print_hex(unsigned int num) {
    const char* hex_digits = "0123456789ABCDEF";
    char buffer[9];
    int i = 0;

    while (num != 0) {
        buffer[i++] = hex_digits[num & 0xF];  // Get lowest 4 bits
        num >>= 4;                             // Shift right by 4 bits
    }

    fb_print("0x", current_fg, current_bg);
    while (i > 0) {
        fb_putc(buffer[--i], current_fg, current_bg);
    }
}
```

### Color System

The VGA text mode supports 16 colors (4 bits):

| Value | Color      | Bright Value | Color         |
| ----- | ---------- | ------------ | ------------- |
| 0     | Black      | 8            | Dark Grey     |
| 1     | Blue       | 9            | Light Blue    |
| 2     | Green      | 10           | Light Green   |
| 3     | Cyan       | 11           | Light Cyan    |
| 4     | Red        | 12           | Light Red     |
| 5     | Magenta    | 13           | Light Magenta |
| 6     | Brown      | 14           | Yellow        |
| 7     | Light Grey | 15           | White         |

### Demonstration Kernel

The kernel demonstrates all API features:

```c
void kernel_main() {
    fb_clear(FB_GREEN, FB_BLACK);
    fb_set_color(FB_GREEN, FB_BLACK);

    // Test arithmetic functions
    fb_print("sum_of_three(1,2,3) = ", FB_GREEN, FB_BLACK);
    fb_print_int(sum_of_three(1, 2, 3));  // Outputs: 6
    fb_print("\n", FB_GREEN, FB_BLACK);

    // Test hex output
    fb_print("\nHex test: ", FB_GREEN, FB_BLACK);
    fb_print_hex(0xCAFEBABE);  // Outputs: 0xCAFEBABE

    // Demonstrate color changing
    fb_set_color(FB_LIGHT_CYAN, FB_BLACK);
    fb_print("\nColor changed to cyan!\n", FB_LIGHT_CYAN, FB_BLACK);

    fb_set_color(FB_WHITE, FB_BLUE);
    fb_print("White on blue background!\n", FB_WHITE, FB_BLUE);

    while(1);  // Infinite loop
}
```

### Expected Output

```
sum_of_three(1,2,3) = 6
multiply(2,4) = 8
subtract(10,3) = 7

Hex test: 0xCAFEBABE

Color changed to cyan!
Now red!
White on blue background!
```

---

## Building and Running

### Prerequisites

- `nasm` - Assembler
- `gcc` - C compiler with 32-bit support
- `ld` - Linker
- `genisoimage` - ISO creation tool
- `qemu-system-i386` - x86 emulator

### Build Commands

```bash
# Clean previous build
make clean

# Build kernel and create ISO
make all

# Run in QEMU (curses mode, as per assignment)
make run

# To quit curses mode:
# Open another terminal and run:
make quit

# Run in QEMU with GUI (for visual testing)
make run-gui
```

### Makefile Structure

```makefile
ASM     = nasm
CC      = gcc
LD      = ld
GENISO  = genisoimage
QEMU    = qemu-system-i386

ASM_SRC = source/loader.asm
C_SRC   = source/kernel.c
LD_SCRIPT = source/link.ld

# Build steps
all: os.iso

os.iso: loader.o kernel.o
    # Link objects
    ld -m elf_i386 -T $(LD_SCRIPT) loader.o kernel.o -o iso/kernel.elf
    # Copy to ISO structure
    cp iso/kernel.elf iso/boot/
    cp -r boot/grub/* iso/boot/grub/
    # Create bootable ISO
    genisoimage -R -b boot/grub/stage2_eltorito ...
```

---

## Project Structure

```
.
├── boot/
│   └── grub/
│       ├── menu.lst           # GRUB configuration
│       └── stage2_eltorito    # GRUB bootloader stage
├── source/
│   ├── drivers/
│   │   ├── fb.h               # Framebuffer header
│   │   └── fb.c               # Framebuffer implementation
│   ├── kernel.c               # Kernel C code
│   ├── loader.asm             # Assembly bootloader
│   └── link.ld                # Linker script
├── iso/                       # ISO staging directory (generated)
│   ├── boot/
│   │   ├── kernel.elf
│   │   └── grub/
│   └── ...
├── Makefile                   # Build automation
├── os.iso                     # Bootable ISO image (generated)
├── logQ.txt                   # QEMU CPU execution log
└── README.md                  # This file
```

---

## Key Concepts Learned

### 1. **Booting Process**

- BIOS loads GRUB from MBR
- GRUB reads `menu.lst` and loads `kernel.elf`
- GRUB validates multiboot header
- Control transfers to `loader` entry point

### 2. **Memory Layout**

```
0x00000000 - 0x000FFFFF  : Reserved (BIOS, GRUB, memory-mapped I/O)
0x00100000 - ...         : Our kernel code (.text section)
0x001xxxxx - ...         : Our kernel data (.rodata, .data, .bss)
0x000B8000 - 0x000B8FA0  : VGA text mode framebuffer
```

### 3. **Calling Conventions**

- Assembly sets up stack (`esp`)
- C functions follow cdecl convention
- No standard library available (freestanding)

### 4. **Memory-Mapped I/O**

- Framebuffer at fixed address `0xB8000`
- Writing to memory directly controls hardware
- No system calls or OS layer needed

---

## Challenges and Solutions

### Challenge 1: GRUB Error 13 "Invalid executable format"

**Problem:** GRUB couldn't recognize the multiboot header.

**Solution:** The issue was `section .text:` (with colon) in loader.asm, which created a separate section. Changed to `section .text` to place the multiboot header at the correct location (offset 0x1000 in the ELF file).

### Challenge 2: Stack Protection Errors

**Problem:** GCC added stack canary checks (`__stack_chk_fail_local`) which we can't support.

**Solution:** Added `-fno-stack-protector` flag to disable stack protection in our freestanding environment.

### Challenge 3: Framebuffer Text Not Appearing

**Problem:** Integer results weren't displaying.

**Solution:** Implemented `fb_print_int()` function to convert integers to ASCII strings for display.

---

## Testing and Verification

### Test 1: Boot Verification

```bash
make clean && make all && make run-gui
```

✅ Kernel boots successfully with GRUB

### Test 2: C Function Calls

✅ `sum_of_three(1,2,3)` returns 6
✅ `multiply(2,4)` returns 8
✅ `subtract(10,3)` returns 7

### Test 3: Framebuffer Output

✅ Text displays correctly
✅ Integer printing works
✅ Hex printing works (0xCAFEBABE)
✅ Color changes work
✅ Newlines and cursor positioning work

### Test 4: CPU Execution Log

```bash
grep "CAFEBABE" logQ.txt
```

✅ Hex value appears in execution trace, confirming `fb_print_hex()` works

---

## References

- [The Little Book of OS Development](https://littleosbook.github.io/)
- [Multiboot Specification](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html)
- [OSDev Wiki - VGA Text Mode](https://wiki.osdev.org/Text_UI)
- [Intel x86 Architecture Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)

---

## Conclusion

This project successfully implements a minimal bootable OS demonstrating:

- ✅ Multiboot compliance and bootloading
- ✅ Assembly-to-C integration with proper calling conventions
- ✅ Memory-mapped I/O via framebuffer driver
- ✅ Complete 2D text API with color support
- ✅ Automated build system with Makefile

---

# WORKSHEET 2 PART 2 - Keyboard Interrupts and Terminal

## Part 2 Overview

This extension builds upon Part 1 by implementing:

- **Task 1 (20%):** Keyboard input via hardware interrupts
- **Task 2 (20%):** Input buffer API with `getc()` and `readline()`
- **Task 3 (40%):** Interactive terminal shell with commands
- **Task 4 (20%):** Documentation (this README)

---

## Task 1: Keyboard Input via Interrupts (20%)

### Objective

Display keyboard input in real-time using interrupt-driven I/O instead of polling.

### Architecture Overview

```
Keyboard Press → IRQ1 → PIC → IDT → interrupt_handler_33 → keyboard_handle_input → Framebuffer
```

### Implementation Components

#### 1. I/O Port Communication (`source/drivers/io.h`, `io.s`)

Communication with hardware requires reading/writing I/O ports:

```asm
; Write byte to I/O port
global outb
outb:
    mov al, [esp + 8]    ; Get data byte
    mov dx, [esp + 4]    ; Get port number
    out dx, al           ; Write to port
    ret

; Read byte from I/O port
global inb
inb:
    mov dx, [esp + 4]    ; Get port number
    in al, dx            ; Read from port
    ret
```

**Usage:**

- Read keyboard scan code: `inb(0x60)`
- Configure PIC: `outb(0x20, 0x20)` to acknowledge interrupt

#### 2. PIC (Programmable Interrupt Controller) Setup (`source/drivers/pic.c`)

The PIC maps hardware interrupts (IRQs) to CPU interrupt numbers:

```c
void pic_remap(s32int offset1, s32int offset2) {
    // Save masks
    u8int mask1 = inb(PIC_1_DATA);
    u8int mask2 = inb(PIC_2_DATA);

    // Initialize PIC in cascade mode
    outb(PIC_1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC_2_COMMAND, ICW1_INIT | ICW1_ICW4);

    // Set vector offsets
    outb(PIC_1_DATA, offset1);  // Master PIC: IRQ0-7 → INT 32-39
    outb(PIC_2_DATA, offset2);  // Slave PIC:  IRQ8-15 → INT 40-47

    // Set cascade configuration
    outb(PIC_1_DATA, 4);  // Tell master about slave on IRQ2
    outb(PIC_2_DATA, 2);  // Tell slave its cascade identity

    // Set 8086 mode
    outb(PIC_1_DATA, ICW4_8086);
    outb(PIC_2_DATA, ICW4_8086);

    // Restore masks
    outb(PIC_1_DATA, mask1);
    outb(PIC_2_DATA, mask2);
}
```

**Why remap?**

- By default, IRQs map to interrupts 0-15
- These conflict with CPU exceptions (divide by zero, page fault, etc.)
- We remap to 32-47 to avoid conflicts

#### 3. IDT (Interrupt Descriptor Table) Setup (`source/drivers/interrupts.c`)

The IDT tells the CPU where to jump when an interrupt occurs:

```c
struct IDTDescriptor {
    u16int offset_low;       // Handler address bits 0-15
    u16int segment_selector; // Code segment (0x08)
    u8int reserved;          // Always 0
    u8int type_and_attr;     // Gate type and privilege level
    u16int offset_high;      // Handler address bits 16-31
} __attribute__((packed));

void interrupts_init_descriptor(u8int index, u32int address) {
    idt_descriptors[index].offset_low = address & 0xFFFF;
    idt_descriptors[index].segment_selector = 0x08;  // Kernel code segment
    idt_descriptors[index].reserved = 0;
    idt_descriptors[index].type_and_attr = 0x8E;     // Present, DPL=0, 32-bit interrupt gate
    idt_descriptors[index].offset_high = (address >> 16) & 0xFFFF;
}
```

**Keyboard interrupt setup:**

```c
#define INTERRUPTS_KEYBOARD 33  // IRQ1 remapped to interrupt 33

void interrupts_install_idt() {
    // Install keyboard handler
    interrupts_init_descriptor(INTERRUPTS_KEYBOARD, (u32int)interrupt_handler_33);

    // Load IDT
    idt.address = (s32int)&idt_descriptors;
    idt.size = sizeof(struct IDTDescriptor) * 256;
    load_idt((s32int)&idt);

    // Remap PIC
    pic_remap(PIC_1_OFFSET, PIC_2_OFFSET);

    // Unmask keyboard interrupt (IRQ1)
    outb(0x21, inb(0x21) & ~(1 << 1));
}
```

#### 4. Assembly Interrupt Handler (`source/drivers/interrupt_asm.s`)

When keyboard interrupt fires, CPU jumps to this handler:

```asm
%macro no_error_code_interrupt_handler 1
global interrupt_handler_%1
interrupt_handler_%1:
    push dword 0           ; Dummy error code
    push dword %1          ; Interrupt number
    jmp common_interrupt_handler
%endmacro

no_error_code_interrupt_handler 33  ; Generate handler for IRQ1

common_interrupt_handler:
    ; Save all registers
    pusha                  ; Push EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    push ds
    push es
    push fs
    push gs

    ; Call C handler
    call interrupt_handler

    ; Restore registers
    pop gs
    pop fs
    pop es
    pop ds
    popa

    add esp, 8             ; Clean up error code and interrupt number
    iret                   ; Return from interrupt
```

#### 5. C Interrupt Dispatcher (`source/drivers/interrupts.c`)

```c
void interrupt_handler(struct cpu_state cpu, u32int interrupt, u32int error_code) {
    if (interrupt == INTERRUPTS_KEYBOARD) {
        // Read scan code from keyboard
        u8int scan_code = keyboard_read_scan_code();

        // Convert to ASCII
        u8int ascii = keyboard_scan_code_to_ascii(scan_code);

        // Handle the input
        if (ascii != 0) {
            keyboard_handle_input(ascii);
        }

        // Acknowledge interrupt to PIC
        pic_acknowledge(interrupt);
    }
}
```

#### 6. Keyboard Driver (`source/drivers/keyboard.c`)

**Scan code to ASCII conversion:**

```c
u8int keyboard_scan_code_to_ascii(u8int scan_code) {
    // Ignore key releases (bit 7 set)
    if (scan_code & 0x80) {
        return 0;
    }

    switch(scan_code) {
        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        // ... (83 keys mapped)
        case 0x1C: return '\n';  // Enter
        case 0x0E: return '\b';  // Backspace
        case 0x39: return ' ';   // Space
        default: return 0;
    }
}
```

**Input handling:**

```c
void keyboard_handle_input(u8int ascii) {
    if (ascii == '\b') {
        fb_backspace();         // Visual feedback
    } else if (ascii == '\n') {
        fb_newline();           // Move to next line
        buffer_put(ascii);      // Store in buffer
    } else {
        fb_write_char(ascii);   // Echo to screen
        buffer_put(ascii);      // Store in buffer
    }
}
```

### Enabling Interrupts

After setup, enable hardware interrupts:

```asm
global enable_hardware_interrupts
enable_hardware_interrupts:
    sti                  ; Set Interrupt flag
    ret
```

Called from kernel:

```c
void kernel_main() {
    interrupts_install_idt();
    terminal_init();
    enable_hardware_interrupts();  // Start receiving interrupts
    terminal_run();
}
```

---

## Task 2: Input Buffer API (20%)

### Objective

Provide blocking input functions for reading keyboard input.

### Circular Buffer Implementation

```c
#define INPUT_BUFFER_SIZE 256

static u8int input_buffer[INPUT_BUFFER_SIZE];
static u32int buffer_read_pos = 0;
static u32int buffer_write_pos = 0;
static u32int buffer_count = 0;
```

**Visualization:**

```
[a][b][c][ ][ ][ ][ ][ ]
 ^       ^
 read    write

buffer_count = 3
```

#### Writing to Buffer (from interrupt handler)

```c
static void buffer_put(u8int ch) {
    if (buffer_count < INPUT_BUFFER_SIZE) {
        input_buffer[buffer_write_pos] = ch;
        buffer_write_pos = (buffer_write_pos + 1) % INPUT_BUFFER_SIZE;  // Wrap around
        buffer_count++;
    }
}
```

#### Reading from Buffer - `getc()`

```c
u8int getc() {
    if (buffer_count == 0) {
        return 0;  // Buffer empty
    }

    u8int ch = input_buffer[buffer_read_pos];
    buffer_read_pos = (buffer_read_pos + 1) % INPUT_BUFFER_SIZE;  // Wrap around
    buffer_count--;
    return ch;
}
```

**Usage:** `source/drivers/keyboard.c:30-39`

#### Line-Buffered Input - `readline()`

Reads characters until newline is encountered:

```c
void readline(char* buffer, u32int max_size) {
    u32int i = 0;
    u8int ch;

    while (i < max_size - 1) {
        // Wait for character (busy-wait)
        while (buffer_count == 0);

        ch = getc();

        if (ch == '\n') {
            buffer[i] = '\0';  // Null-terminate
            return;
        }

        buffer[i++] = ch;
    }

    buffer[max_size - 1] = '\0';  // Ensure null termination
}
```

**Usage:** `source/terminal.c:162`

**Important:** `readline()` blocks until Enter is pressed, making it suitable for interactive shells.

---

## Task 3: Terminal Shell (40%)

### Objective

Implement an interactive command-line interface with built-in commands.

### Architecture

```
Terminal Loop → readline() → parse_command() → execute_command() → Command Function
```

### String Utilities

Since we're in a freestanding environment (no libc), we implement our own:

```c
static int str_len(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

static int str_cmp(const char* str1, const char* str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            return str1[i] - str2[i];
        }
        i++;
    }
    return str1[i] - str2[i];
}

static void str_copy(char* dest, const char* src) {
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}
```

### Command Parsing

Splits input into command and arguments:

```c
static void parse_command(const char* input, char* cmd, char* args) {
    int i = 0, j = 0;

    // Skip leading spaces
    while (input[i] == ' ' && input[i] != '\0') i++;

    // Extract command (first word)
    while (input[i] != ' ' && input[i] != '\0') {
        cmd[j++] = input[i++];
    }
    cmd[j] = '\0';

    // Skip spaces between command and arguments
    while (input[i] == ' ' && input[i] != '\0') i++;

    // Extract arguments (rest of line)
    j = 0;
    while (input[i] != '\0') {
        args[j++] = input[i++];
    }
    args[j] = '\0';
}
```

**Example:**

- Input: `"echo hello world"`
- Command: `"echo"`
- Arguments: `"hello world"`

### Command Table

Commands are stored in a dispatch table:

```c
struct command {
    const char* name;
    void (*function)(char* args);
};

static struct command commands[] = {
    {"echo", cmd_echo},
    {"clear", cmd_clear},
    {"help", cmd_help},
    {"version", cmd_version},
    {0, 0}  // Sentinel
};
```

### Command Implementations

#### 1. Echo Command

```c
static void cmd_echo(char* args) {
    fb_set_color(FB_LIGHT_GREY, FB_BLACK);
    fb_print(args, FB_LIGHT_GREY, FB_BLACK);
    fb_print("\n", FB_LIGHT_GREY, FB_BLACK);
}
```

**Usage:**

```
myos> echo Hello, World!
Hello, World!
```

#### 2. Clear Command

```c
static void cmd_clear(char* args) {
    (void)args;  // Unused
    fb_clear(FB_LIGHT_GREY, FB_BLACK);
    fb_set_color(FB_LIGHT_GREY, FB_BLACK);
}
```

Clears the screen and resets cursor position.

#### 3. Help Command

```c
static void cmd_help(char* args) {
    (void)args;
    fb_set_color(FB_LIGHT_CYAN, FB_BLACK);
    fb_print("Available commands:\n", FB_LIGHT_CYAN, FB_BLACK);
    fb_print("  echo [text]  - Display the provided text\n", FB_LIGHT_GREY, FB_BLACK);
    fb_print("  clear        - Clear the screen\n", FB_LIGHT_GREY, FB_BLACK);
    fb_print("  help         - Show this help message\n", FB_LIGHT_GREY, FB_BLACK);
    fb_print("  version      - Display OS version\n", FB_LIGHT_GREY, FB_BLACK);
    fb_print("\n", FB_LIGHT_GREY, FB_BLACK);
}
```

#### 4. Version Command

```c
static void cmd_version(char* args) {
    (void)args;
    fb_set_color(FB_LIGHT_GREEN, FB_BLACK);
    fb_print("TinyOS v1.0 - Worksheet 2 Part 2\n", FB_LIGHT_GREEN, FB_BLACK);
    fb_print("Built with interrupts and keyboard support\n", FB_LIGHT_GREY, FB_BLACK);
}
```

### Command Execution

```c
static void execute_command(const char* input) {
    char cmd[MAX_COMMAND_LENGTH];
    char args[MAX_ARGS_LENGTH];

    parse_command(input, cmd, args);

    // Empty command - do nothing
    if (str_len(cmd) == 0) {
        return;
    }

    // Search command table
    int i = 0;
    while (commands[i].name != 0) {
        if (str_cmp(cmd, commands[i].name) == 0) {
            commands[i].function(args);  // Execute command
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
```

### Terminal Main Loop

```c
void terminal_run() {
    char input[MAX_COMMAND_LENGTH];

    while (1) {
        // Display prompt
        fb_set_color(FB_LIGHT_GREEN, FB_BLACK);
        fb_print("myos> ", FB_LIGHT_GREEN, FB_BLACK);
        fb_set_color(FB_LIGHT_GREY, FB_BLACK);

        // Read user input (blocks until Enter)
        readline(input, MAX_COMMAND_LENGTH);

        // Execute command
        execute_command(input);
    }
}
```

**Example session:**

```
TinyOS Terminal
Type 'help' for available commands

myos> help
Available commands:
  echo [text]  - Display the provided text
  clear        - Clear the screen
  help         - Show this help message
  version      - Display OS version

myos> version
TinyOS v1.0 - Worksheet 2 Part 2
Built with interrupts and keyboard support

myos> echo Testing the terminal!
Testing the terminal!

myos> unknown
Unknown command: unknown
Type 'help' for available commands.

myos>
```

---

## Updated Project Structure

```
.
├── boot/
│   └── grub/
│       ├── menu.lst
│       └── stage2_eltorito
├── source/
│   ├── drivers/
│   │   ├── fb.h                           # Framebuffer header
│   │   ├── fb.c                           # Framebuffer implementation (extended)
│   │   ├── io.h                           # I/O port operations header
│   │   ├── io.s                           # I/O port operations (assembly)
│   │   ├── type.h                         # Type definitions
│   │   ├── pic.h                          # PIC header
│   │   ├── pic.c                          # PIC implementation
│   │   ├── interrupts.h                   # Interrupt structures
│   │   ├── interrupts.c                   # IDT setup and dispatcher
│   │   ├── interrupt_handlers.s           # IDT loader
│   │   ├── interrupt_asm.s                # Interrupt handler wrapper
│   │   ├── hardware_interrupt_enabler.h   # STI/CLI wrappers
│   │   ├── hardware_interrupt_enabler.s   # Enable/disable interrupts
│   │   ├── keyboard.h                     # Keyboard driver header
│   │   └── keyboard.c                     # Keyboard driver + buffer API
│   ├── terminal.h                         # Terminal shell header
│   ├── terminal.c                         # Terminal shell implementation
│   ├── kernel.c                           # Kernel entry point
│   ├── loader.asm                         # Bootloader
│   └── link.ld                            # Linker script
├── iso/                                   # ISO staging (generated)
├── Makefile                               # Build automation (updated)
├── os.iso                                 # Bootable ISO (generated)
└── README.md                              # This file
```

---

## Updated Building and Running

### Build Process

The Makefile now compiles all components:

```makefile
OBJECTS = source/loader.o \
          source/kernel.o \
          source/drivers/io.o \
          source/drivers/fb.o \
          source/drivers/pic.o \
          source/drivers/interrupts.o \
          source/drivers/interrupt_handlers.o \
          source/drivers/interrupt_asm.o \
          source/drivers/hardware_interrupt_enabler.o \
          source/drivers/keyboard.o \
          source/terminal.o
```

### Commands

```bash
# Clean previous build
make clean

# Build everything
make all

# Run in GUI mode (recommended for Part 2)
make run-gui

# Run in curses mode
make run

# Quit QEMU
make quit
```

### Testing the Terminal

1. **Boot the OS:**

   ```bash
   make clean && make all && make run-gui
   ```

2. **Test keyboard input:**

   - Type characters - they should appear on screen
   - Press Backspace - should delete characters
   - Press Enter - should execute command

3. **Test commands:**
   ```
   myos> help
   myos> version
   myos> echo This is a test
   myos> clear
   ```

---

## Key Concepts Learned (Part 2)

### 1. Interrupt-Driven I/O

**Polling vs Interrupts:**

| Polling                      | Interrupts                         |
| ---------------------------- | ---------------------------------- |
| CPU constantly checks device | Device notifies CPU when ready     |
| Wastes CPU cycles            | CPU can do other work              |
| Simple to implement          | Complex setup (IDT, PIC, handlers) |
| Used in simple systems       | Used in modern systems             |

### 2. x86 Interrupt Handling

**Interrupt flow:**

1. Hardware device (keyboard) asserts IRQ line
2. PIC forwards interrupt to CPU
3. CPU saves current state (flags, CS, EIP)
4. CPU looks up handler address in IDT
5. CPU jumps to handler
6. Handler saves all registers
7. Handler calls C function
8. Handler acknowledges interrupt to PIC
9. Handler restores registers
10. CPU executes `iret` to resume previous task

### 3. Circular Buffers

**Advantages:**

- Fixed size (no dynamic allocation)
- Efficient wraparound (modulo arithmetic)
- Decouples producer (interrupt) from consumer (application)

**Use cases:**

- Keyboard input buffering
- Serial port communication
- Audio streaming
- Network packet queues

### 4. Command-Line Interfaces

**Design patterns:**

- **Dispatch table:** Fast command lookup
- **String parsing:** Separate command from arguments
- **Modular commands:** Each command is independent function
- **Error handling:** Graceful handling of unknown commands

---

## Challenges and Solutions (Part 2)

### Challenge 1: Interrupt Not Firing

**Problem:** Keyboard interrupts weren't being received.

**Solution:**

1. Verified PIC remapping to avoid conflicts with CPU exceptions
2. Ensured keyboard interrupt (IRQ1) was unmasked: `outb(0x21, inb(0x21) & ~(1 << 1))`
3. Called `sti` instruction to enable hardware interrupts

### Challenge 2: Backspace Not Working Properly

**Problem:** Backspace deleted characters but left visual artifacts.

**Solution:** Updated `fb_backspace()` to overwrite deleted character with space:

```c
void fb_backspace() {
    if (cursor_x > 0) {
        cursor_x--;
        fb[cursor_y * FB_WIDTH + cursor_x] =
            ((current_bg & 0x0F) << 12) | ((current_fg & 0x0F) << 8) | ' ';
    }
}
```

### Challenge 3: Readline() Blocking Forever

**Problem:** `readline()` would hang if no input was available.

**Solution:** This is actually **intended behavior**. The function uses busy-wait:

```c
while (buffer_count == 0);  // Wait for keyboard interrupt to fill buffer
```

This is acceptable in our single-tasking OS. In a multitasking OS, we'd use:

- Sleep/wake mechanisms
- Semaphores
- Blocking I/O with scheduler integration

---

## Testing and Verification (Part 2)

### Test 1: Interrupt Setup

✅ PIC remapped successfully (IRQ1 → INT 33)
✅ IDT loaded without errors
✅ Keyboard interrupt handler installed

### Test 2: Keyboard Input

✅ Key presses generate interrupts
✅ Scan codes converted to ASCII correctly
✅ Characters displayed on screen in real-time
✅ Backspace removes characters
✅ Enter moves to next line

### Test 3: Input Buffer

✅ `getc()` returns buffered characters
✅ `readline()` blocks until Enter pressed
✅ Buffer handles wraparound correctly
✅ No buffer overflow on rapid typing

### Test 4: Terminal Commands

✅ `help` displays command list
✅ `version` displays OS version
✅ `echo` prints arguments
✅ `clear` clears screen
✅ Unknown commands handled gracefully
✅ Empty input ignored

### Test 5: Interactive Session

✅ Prompt displays correctly
✅ Multi-word arguments parsed correctly (`echo hello world`)
✅ Command execution doesn't crash kernel
✅ Terminal loop continues after each command

---

## Technical Specifications

### Interrupt Configuration

- **PIC Remap:** IRQ0-7 → INT 32-39, IRQ8-15 → INT 40-47
- **Keyboard Interrupt:** IRQ1 → INT 33
- **IDT Size:** 256 entries
- **Interrupt Gate Type:** 0x8E (Present, DPL=0, 32-bit interrupt gate)

### Keyboard Specifications

- **Data Port:** 0x60
- **Scan Code Set:** Set 1 (US QWERTY)
- **Keys Mapped:** 83 keys (letters, numbers, symbols, Enter, Backspace, Space)
- **Shift/Ctrl/Alt:** Not implemented (future enhancement)

### Buffer Specifications

- **Type:** Circular buffer
- **Size:** 256 bytes
- **Overflow Handling:** Drops new characters if full
- **Underflow Handling:** `getc()` returns 0 if empty

### Terminal Specifications

- **Prompt:** `myos> `
- **Max Command Length:** 256 characters
- **Max Arguments Length:** 200 characters
- **Commands Implemented:** 4 (echo, clear, help, version)
- **Command Table:** Null-terminated array of function pointers

---

## Conclusion (Part 2)

This project successfully extends Part 1 with:

- ✅ **Complete interrupt infrastructure** with IDT and PIC setup
- ✅ **Keyboard driver** with scan code mapping for 83 keys
- ✅ **Input buffer API** with `getc()` and `readline()`
- ✅ **Interactive terminal** with command parsing and execution
- ✅ **4 built-in commands** demonstrating extensibility
- ✅ **Real-time keyboard input** via interrupt-driven I/O

The system demonstrates fundamental OS concepts:

- Hardware interrupt handling
- Device driver development
- Buffered I/O
- User interface implementation
- Modular software design

**Total Lines of Code Added:** ~800 lines across 15 new files

---

## Future Enhancements

Possible extensions to this project:

1. **Keyboard enhancements:**

   - Shift key support for uppercase and symbols
   - Ctrl key combinations (Ctrl+C, Ctrl+L)
   - Arrow keys for command history

2. **Terminal features:**

   - Command history (up/down arrows)
   - Tab completion
   - Multi-line commands
   - Pipes and redirection

3. **System features:**

   - Timer interrupt (IRQ0)
   - Preemptive multitasking
   - Memory management
   - File system

4. **New commands:**
   - `ls` - list files
   - `cat` - display file contents
   - `reboot` - restart system
   - `shutdown` - halt CPU

---

## References (Part 2)

- [OSDev Wiki - Interrupts](https://wiki.osdev.org/Interrupts)
- [OSDev Wiki - 8259 PIC](https://wiki.osdev.org/8259_PIC)
- [OSDev Wiki - PS/2 Keyboard](https://wiki.osdev.org/PS/2_Keyboard)
- [Intel 8259A PIC Datasheet](https://pdos.csail.mit.edu/6.828/2010/readings/hardware/8259A.pdf)
- [The Little Book of OS Development - Interrupts](https://littleosbook.github.io/#interrupts)
