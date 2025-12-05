#include "interrupts.h"
#include "pic.h"
#include "io.h"
#include "keyboard.h"

#define INTERRUPTS_DESCRIPTOR_COUNT 256
#define INTERRUPTS_KEYBOARD 33

struct IDTDescriptor idt_descriptors[INTERRUPTS_DESCRIPTOR_COUNT];
struct IDT idt;

void interrupts_init_descriptor(s32int index, u32int address)
{
    idt_descriptors[index].offset_high = (address >> 16) & 0xFFFF;
    idt_descriptors[index].offset_low = (address & 0xFFFF);
    idt_descriptors[index].segment_selector = 0x08; // The second (code) segment selector in GDT
    idt_descriptors[index].reserved = 0x00;

  
    idt_descriptors[index].type_and_attr = (0x01 << 7) |  // P
                                           (0x00 << 6) | (0x00 << 5) |  // DPL
                                           0xe;  // 0b1110=0xE 32-bit interrupt gate
}

void interrupts_install_idt()
{
    interrupts_init_descriptor(INTERRUPTS_KEYBOARD, (u32int)interrupt_handler_33);

    idt.address = (s32int)&idt_descriptors;
    idt.size = sizeof(struct IDTDescriptor) * INTERRUPTS_DESCRIPTOR_COUNT;
    load_idt((s32int)&idt);

    pic_remap(PIC_1_OFFSET, PIC_2_OFFSET);

    // Unmask keyboard interrupt (IRQ1)
    outb(0x21, inb(0x21) & ~(1 << 1));
}

/* Interrupt handlers */
void interrupt_handler(u32int interrupt, __attribute__((unused)) u32int error_code) {
    u8int input;
    u8int ascii;

    switch (interrupt) {
        case INTERRUPTS_KEYBOARD:
            input = keyboard_read_scan_code();
            ascii = keyboard_scan_code_to_ascii(input);

            if (ascii != 0) {
                keyboard_handle_input(ascii);
            }

            pic_acknowledge(interrupt);
            break;

        default:
            break;
    }
}
