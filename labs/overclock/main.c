#include "mailbox_interface.h"
#include "uart.h"
#include "lib.h"
#include "gpio.h"
#include "sys_timer.h"

void test_clock(uint32_t clock) {
    uart_puts("----- CLOCK: ");
    uart_putx(clock);
    uart_puts(" -----\n");

    uart_puts("Set clock rate (hz): ");
    uart_putd(mbox_get_clock_rate(clock));
    uart_puts("\n");
    uart_puts("Measured clock rate (hz): ");
    uart_putd(mbox_get_measured_clock_rate(clock));
    uart_puts("\n");
    uart_puts("Max clock rate (hz): ");
    uint32_t max_rate = mbox_get_max_clock_rate(clock);
    uart_putd(max_rate);
    uart_puts("\n");
    uint32_t min_rate = mbox_get_min_clock_rate(clock);
    uart_puts("Min clock rate (hz): ");
    uart_putd(min_rate);
    uart_puts("\n\n");

    uart_puts("Setting clock rate to max...");

    mbox_set_clock_rate(clock, max_rate);
    uart_set_baudrate_to_core_clock();

    uart_puts("\n");

    uart_puts("Set clock rate (hz): ");
    uart_putd(mbox_get_clock_rate(clock));
    uart_puts("\n");
    uart_puts("Measured clock rate (hz): ");
    uart_putd(mbox_get_measured_clock_rate(clock));
    uart_puts("\n\n");

    uart_puts("Setting clock rate to min...");
    uart_flush_tx();

    mbox_set_clock_rate(clock, min_rate);
    uart_set_baudrate_to_core_clock();

    uart_puts("\n");

    uart_puts("Set clock rate (hz): ");
    uart_putd(mbox_get_clock_rate(clock));
    uart_puts("\n");
    uart_puts("Measured clock rate (hz): ");
    uart_putd(mbox_get_measured_clock_rate(clock));
    uart_puts("\n\n");
    uart_flush_tx();
}

void gpio_test() {
    Pin p = { 20 };
    gpio_select_output(p);

    uint32_t t = sys_timer_get_usec();
    const int N = 100000;
    for (int i = 0; i < N; i++) {
        gpio_set_high(p);
        gpio_set_low(p);
    }
    uint32_t time = sys_timer_get_usec() - t;
    
    uart_puts("Elapsed time (us): ");
    uart_putd(time);
    uart_puts("\n\n");
}

void gpio_test_clock(uint32_t clock) {
    mbox_set_clock_rate(clock, mbox_get_max_clock_rate(clock));
    uart_set_baudrate_to_core_clock();

    gpio_test();
}

void main() {
    test_clock(MBOX_CLK_ARM);
    test_clock(MBOX_CLK_CORE);
    test_clock(MBOX_CLK_V3D);

    uart_puts("\n======= TIMING GPIO ========\n\n");

    uart_puts("Original...\n");
    gpio_test();

    uart_puts("Setting CORE to max...\n");
    gpio_test_clock(MBOX_CLK_CORE);
    uart_puts("Setting ARM to max...\n");
    gpio_test_clock(MBOX_CLK_ARM);
    uart_puts("Setting UART to max...\n");
    gpio_test_clock(MBOX_CLK_UART);

    rpi_reset();
}
