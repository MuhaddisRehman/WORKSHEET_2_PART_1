#ifndef INCLUDE_IO_H
#define INCLUDE_IO_H

// Write a byte to an I/O port
void outb(unsigned short port, unsigned char data);

// Read a byte from an I/O port
unsigned char inb(unsigned short port);

#endif /* INCLUDE_IO_H */
