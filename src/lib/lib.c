#include "lib.h"
#include "uart.h"

#define PM_RSTC 0x2010001C
#define PM_WDOG 0x20100024
#define PM_PASSWORD 0x5A000000
#define PM_RSTC_WRCFG_FULL_RESET 0x20

void rpi_reboot() {
    uart_flush_tx();

    PUT32(PM_WDOG, PM_PASSWORD | 1);
    PUT32(PM_RSTC, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);
}
void rpi_reset() {
    uart_putk("\r\nDONE!!!\n");
    rpi_reboot();
}

void assert(bool val, const char* msg) {
    if (!val) {
        uart_puts("\n[ERROR] Assertion failed: ");
        uart_puts(msg);
        rpi_reset();
    }
}

int errno;
int* __errno() {
    return &errno;
}
