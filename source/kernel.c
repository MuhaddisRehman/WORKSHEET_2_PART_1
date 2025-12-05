#include "drivers/hardware_interrupt_enabler.h"
#include "drivers/fb.h"
#include "terminal.h"
#include "drivers/interrupts.h"

void kernel_main() {
    fb_clear(FB_LIGHT_GREY, FB_BLACK);
    interrupts_install_idt();
    terminal_init();
    // enable_hardware_interrupts(); Uncomment this line to enable hardware interrupts
    
    // Start main terminal loop
    terminal_run();
}
