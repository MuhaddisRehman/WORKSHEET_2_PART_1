#include "interrupts.h"
#include "pic.h"
#include "io.h"
#include "keyboard.h"
#include "fb.h"

#define INTERRUPTS_DESCRIPTOR_COUNT 256
#define INTERRUPTS_TIMER 32
#define INTERRUPTS_KEYBOARD 33

struct IDTDescriptor idt_descriptors[INTERRUPTS_DESCRIPTOR_COUNT];
struct IDT idt;


void interrupts_init_descriptor(s32int index, u32int address)
{
    idt_descriptors[index].offset_high = (address >> 16) & 0xFFFF;
    idt_descriptors[index].offset_low = (address & 0xFFFF);
    idt_descriptors[index].segment_selector = 0x08;
    idt_descriptors[index].reserved = 0x00;
    idt_descriptors[index].type_and_attr = (0x01 << 7) | (0x00 << 6) | (0x00 << 5) | 0xe;
}

void interrupts_install_idt()
{
    for (int i = 0; i < INTERRUPTS_DESCRIPTOR_COUNT; i++) {
        idt_descriptors[i].offset_low = 0;
        idt_descriptors[i].segment_selector = 0;
        idt_descriptors[i].reserved = 0;
        idt_descriptors[i].type_and_attr = 0;
        idt_descriptors[i].offset_high = 0;
    }

    interrupts_init_descriptor(INTERRUPTS_TIMER, (u32int)interrupt_handler_32);
    interrupts_init_descriptor(INTERRUPTS_KEYBOARD, (u32int)interrupt_handler_33);

    idt.address = (s32int)&idt_descriptors;
    idt.size = (sizeof(struct IDTDescriptor) * INTERRUPTS_DESCRIPTOR_COUNT) - 1;
    load_idt((s32int)&idt);

    pic_remap(PIC_1_OFFSET, PIC_2_OFFSET);
    outb(0x21, inb(0x21) & ~((1 << 0) | (1 << 1)));
}

void interrupt_handler(u32int interrupt, __attribute__((unused)) u32int error_code) {
    u8int input;
    u8int ascii;

    switch (interrupt) {
        case INTERRUPTS_TIMER:
            pic_acknowledge(interrupt);
            break;

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
