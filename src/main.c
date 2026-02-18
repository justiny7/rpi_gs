#include "gpio.h"
#include "uart.h"
#include "lib.h"
#include "sys_timer.h"
#include "mailbox_interface.h"
#include <math.h>

void main() {
    float f = 3.f;
    float s = sqrtf(f);

    int x = s * 100;
    uart_putd(x);
    uart_puts("\n");

    int y = expf(2.5) * 100000;
    uart_putd(y);

    rpi_reset();
}

