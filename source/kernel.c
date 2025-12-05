#include "drivers/hardware_interrupt_enabler.h"
#include "drivers/fb.h"
#include "terminal.h"
#include "drivers/interrupts.h"
#include "drivers/type.h"

void kernel_main() {
    fb_clear(FB_LIGHT_GREY, FB_BLACK);
    interrupts_install_idt();
    terminal_init();
    enable_hardware_interrupts();

    // Start main terminal loop
    terminal_run();
}
