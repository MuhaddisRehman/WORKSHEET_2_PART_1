#include "drivers/hardware_interrupt_enabler.h"
#include "drivers/fb.h"
#include "terminal.h"
#include "drivers/interrupts.h"

void kernel_main() {
    // Initial debug output before enabling interrupts
    fb_clear(FB_LIGHT_GREY, FB_BLACK);
    // fb_print("Kernel started...\n", FB_LIGHT_GREEN, FB_BLACK);

    // fb_print("Setting up interrupts...\n", FB_LIGHT_CYAN, FB_BLACK);
    interrupts_install_idt();
    // fb_print("IDT installed!\n", FB_LIGHT_CYAN, FB_BLACK);

    // fb_print("Initializing terminal...\n", FB_LIGHT_CYAN, FB_BLACK);
    terminal_init();
    // fb_print("Terminal ready!\n", FB_LIGHT_CYAN, FB_BLACK);

    // Enable hardware interrupts **after framebuffer setup**
    enable_hardware_interrupts();
    // fb_print("Interrupts enabled!\n", FB_LIGHT_GREEN, FB_BLACK);

    // Start main terminal loop
    terminal_run();
}
