global enable_hardware_interrupts
global disable_hardware_interrupts

enable_hardware_interrupts:
    sti
    ret
disable_hardware_interrupts:
    cli
    ret