#include "gpio.h"
#include "uart.h"
#include "lib.h"
#include "sys_timer.h"

#define ITERATIONS 0x500000

void main() {
    /*
    uart_init();

    // const char* s = "Hello world\n";
    // uart_puts(s);
    uart_puts("This is justin!\n");
    uart_puts("some more text just to see what happens");

    char* s = "UUUUUUUUUUU\n";
    for (char* i = s; *i; i++) {
        _uart_putc(*i);
        sys_timer_delay_ms(250);
    }

    while (1) {
        // unsigned char c = uart_getc();
        // uart_putc(c);
        uart_putc(uart_getc());
    }
    */
    Pin p = { 27 };
    Pin pp = { 20 };
    gpio_select_output(p);
    gpio_select_output(pp);
    
    while (1) {
        gpio_set_high(p);
        // gpio_set_high(pp);
        sys_timer_delay_sec(1);
        gpio_set_low(p);
        // gpio_set_low(pp);
        sys_timer_delay_sec(1);
    }
}

