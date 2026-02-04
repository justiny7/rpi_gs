#include "gpio.h"
#include "lib.h"

void __attribute__((interrupt("UNDEF"))) undefined_instruction_vector() {
    ;
}

void __attribute__((interrupt("SWI"))) software_interrupt_vector() {
    ;
}

void __attribute__((interrupt("ABORT"))) prefetch_abort_vector() {
    ;
}

void __attribute__((interrupt("ABORT"))) data_abort_vector() {
    ;
}

/*
void __attribute__((interrupt("IRQ"))) interrupt_vector() {
    ;
}
*/

void __attribute__((interrupt("FIQ"))) fast_interrupt_vector() {
    ;
}
