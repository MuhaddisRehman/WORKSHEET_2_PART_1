;Generic Interrupt Handler
;
extern interrupt_handler

%macro no_error_code_interrupt_handler 1
global interrupt_handler_%1
interrupt_handler_%1:
    push dword 0                  ; push 0 as error code (2nd parameter)
    push dword %1                 ; push the interrupt number (1st parameter)
    jmp common_interrupt_handler  ; jump to the common handler
%endmacro

common_interrupt_handler:         ; the common parts of the generic interrupt handler
    ; save the registers
    pushad                        ; push all general purpose registers

    ; call the C function (parameters already on stack)
    call interrupt_handler

    ; restore the registers
    popad                         ; pop all general purpose registers

    ; restore the esp (remove error code and interrupt number)
    add esp, 8

    ; return to the code that got interrupted
    iret

no_error_code_interrupt_handler 33  ; create handler for interrupt 33 (keyboard)
