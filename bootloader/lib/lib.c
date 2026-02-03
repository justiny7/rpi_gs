#include "lib.h"
#include "drivers.h"

#define PM_RSTC 0x2010001C
#define PM_WDOG 0x20100024
#define PM_PASSWORD 0x5A000000
#define PM_RSTC_WRCFG_FULL_RESET 0x20

void rpi_reboot() {
    uart_flush_tx();

    PUT32(PM_WDOG, PM_PASSWORD | 1);
    PUT32(PM_RSTC, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);
}
