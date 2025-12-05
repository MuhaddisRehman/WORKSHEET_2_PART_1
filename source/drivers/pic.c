#include "pic.h"
#include "io.h"

static void io_wait(void) {
    outb(0x80, 0);
}

void pic_remap(s32int offset1, s32int offset2)
{
    outb(PIC_1_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();
    outb(PIC_2_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();

    outb(PIC_1_DATA, offset1);
    io_wait();
    outb(PIC_2_DATA, offset2);
    io_wait();

    outb(PIC_1_DATA, 4);
    io_wait();
    outb(PIC_2_DATA, 2);
    io_wait();

    outb(PIC_1_DATA, PIC_ICW4_8086);
    io_wait();
    outb(PIC_2_DATA, PIC_ICW4_8086);
    io_wait();

    outb(PIC_1_DATA, 0xFF);
    io_wait();
    outb(PIC_2_DATA, 0xFF);
    io_wait();
}

void pic_acknowledge(u32int interrupt)
{
    if (interrupt >= PIC_2_OFFSET) {
        outb(PIC_2_COMMAND, PIC_ACKNOWLEDGE);
    }
    outb(PIC_1_COMMAND, PIC_ACKNOWLEDGE);
}
