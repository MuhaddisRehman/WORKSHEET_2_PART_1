extern interrupt_handler

%macro no_error_code_interrupt_handler 1
global interrupt_handler_%1
interrupt_handler_%1:
    cli
    pushad
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push dword 0
    push dword %1
    call interrupt_handler
    add esp, 8

    pop gs
    pop fs
    pop es
    pop ds
    popad

    sti
    iret
%endmacro

no_error_code_interrupt_handler 32
no_error_code_interrupt_handler 33
