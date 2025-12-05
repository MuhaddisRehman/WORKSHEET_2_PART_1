extern interrupt_handler

%macro no_error_code_interrupt_handler 1
global interrupt_handler_%1
interrupt_handler_%1:
    cli                     ; disable nested interrupts

    push dword 0            ; dummy error code
    push dword %1           ; interrupt number

    pushad
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10            ; data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call interrupt_handler

    pop gs
    pop fs
    pop es
    pop ds
    popad

    add esp, 8              ; clean up stack (int + error code)
    sti                     ; enable interrupts
    iret
%endmacro

no_error_code_interrupt_handler 33
