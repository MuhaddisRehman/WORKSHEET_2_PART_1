#include "pic.h"
#include "io.h"

void pic_remap(s32int offset1, s32int offset2)
{
    unsigned char a1, a2;

    // Save masks
    a1 = inb(PIC_1_DATA);
    a2 = inb(PIC_2_DATA);

    // Start initialization sequence (in cascade mode)
    outb(PIC_1_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    outb(PIC_2_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);

    // Set vector offsets
    outb(PIC_1_DATA, offset1);
    outb(PIC_2_DATA, offset2);

    // Tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    outb(PIC_1_DATA, 4);
    // Tell Slave PIC its cascade identity (0000 0010)
    outb(PIC_2_DATA, 2);

    // Set mode
    outb(PIC_1_DATA, PIC_ICW4_8086);
    outb(PIC_2_DATA, PIC_ICW4_8086);

    // Restore saved masks
    outb(PIC_1_DATA, a1);
    outb(PIC_2_DATA, a2);
}

void pic_acknowledge(u32int interrupt)
{
    if (interrupt >= PIC_2_OFFSET) {
        outb(PIC_2_COMMAND, PIC_ACKNOWLEDGE);
    }
    outb(PIC_1_COMMAND, PIC_ACKNOWLEDGE);
}
