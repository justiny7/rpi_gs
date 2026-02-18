#include "gpio.h"
#include "uart.h"
#include "lib.h"
#include "sys_timer.h"
#include "mailbox_interface.h"


void main() {
    float r = 1.5;
    float x = 2.0;

    float z = r + x;
    if (z > 3) {
        uart_puts("yay\n");
    } else {
        uart_puts("aw\n");
    }

    rpi_reset();
}

