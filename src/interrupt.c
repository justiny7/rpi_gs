#include "gpio.h"
#include "lib.h"
#include "uart.h"

void __attribute__((interrupt("UNDEF"))) undefined_instruction_vector() {
    uart_puts("Unimplemented undefined instruction vector\n");
    rpi_reset();
}

void __attribute__((interrupt("SWI"))) software_interrupt_vector() {
    uart_puts("Unimplemented SWI vector\n");
    rpi_reset();
}

void __attribute__((interrupt("ABORT"))) prefetch_abort_vector() {
    uart_puts("Unimplemented prefetch abort vector\n");
    rpi_reset();
}

void __attribute__((interrupt("ABORT"))) data_abort_vector() {
    uart_puts("Unimplemented data abort vector\n");
    rpi_reset();
}

void __attribute__((interrupt("IRQ"))) interrupt_vector() {
    uart_puts("Unimplemented interrupt vector\n");
    rpi_reset();
}

void __attribute__((interrupt("FIQ"))) fast_interrupt_vector() {
    uart_puts("Unimplemented fast interrupt vector\n");
    rpi_reset();
}
