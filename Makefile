# Makefile for WORKSHEET_2_PART_1 & PART_2
# Builds loader (assembly) and kernel (C), creates ISO, runs in QEMU

# ----------------------
# Tools
# ----------------------
ASM     = nasm                  # assembler
CC      = gcc                   # C compiler
LD      = ld                    # linker
GENISO  = genisoimage            # ISO creator
QEMU    = qemu-system-i386       # emulator

# ----------------------
# Flags
# ----------------------
ASMFLAGS = -f elf32
CFLAGS   = -m32 -ffreestanding -fno-stack-protector -c
LDFLAGS  = -m elf_i386

# ----------------------
# Source / Objects
# ----------------------
LD_SCRIPT = source/link.ld
ELF     = iso/kernel.elf
ISO     = os.iso
LOG     = logQ.txt

# Object files
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

# ----------------------
# Build rules
# ----------------------

# Default target
all: $(ISO)

# Assembly files (.asm -> .o)
%.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

# Assembly files (.s -> .o)
%.o: %.s
	$(ASM) $(ASMFLAGS) $< -o $@

# C files (.c -> .o)
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

# Link all objects into ELF kernel
$(ELF): $(OBJECTS) $(LD_SCRIPT)
	mkdir -p iso
	$(LD) $(LDFLAGS) -T $(LD_SCRIPT) $(OBJECTS) -o $(ELF)

# Create bootable ISO
$(ISO): $(ELF)
	mkdir -p iso/boot/grub
	cp $(ELF) iso/boot/
	cp -r boot/grub/* iso/boot/grub/
	$(GENISO) -R \
	-b boot/grub/stage2_eltorito \
	-no-emul-boot \
	-boot-load-size 4 \
	-A os \
	-input-charset utf-8 \
	-quiet \
	-boot-info-table \
	-o $(ISO) iso

# Clean all generated files
clean:
	rm -rf $(OBJECTS) $(ELF) $(ISO) iso $(LOG)

# Run in QEMU with curses mode (as per assignment page 7)
# To quit: In another terminal, run: make quit
run: $(ISO)
	$(QEMU) -display curses -monitor telnet::45454,server,nowait -serial mon:stdio -boot d -cdrom $(ISO) -m 32 -d cpu -D $(LOG)

# Run in QEMU with GUI (for visual testing)
run-gui: $(ISO)
	$(QEMU) -boot d -cdrom $(ISO) -m 32 -d cpu -D $(LOG)

# Quit running QEMU instances
quit:
	@echo "Stopping QEMU..."
	@pkill -f "qemu-system-i386.*os.iso" || echo "No QEMU instances running"
