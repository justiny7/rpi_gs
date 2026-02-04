#include "gpio.h"
#include "uart.h"
#include "lib.h"
#include "sys_timer.h"

#define ITERATIONS 0x500000

#define PIN 20
#define N 30

Pin p = { PIN };

volatile uint32_t n_interrupt;
volatile uint32_t n_rising;
volatile uint32_t n_falling;

volatile uint32_t last_clk;
uint32_t times[N * 2];
uint32_t t_rising[N];
uint32_t t_falling[N];

void print_int(uint32_t x) {
    if (x == 0) {
        uart_putc('0');
        return;
    }

    uint8_t num[10] = {};
    int i = 0;
    for (; x > 0; x /= 10) {
        num[i++] = '0' + (x % 10);
    }

    for (i--; i >= 0; i--) {
        uart_putc(num[i]);
    }
}

void __attribute__((interrupt("IRQ"))) interrupt_vector() {
    if (gpio_has_interrupt()) {
        if (gpio_event_detected(p)) {
            uint32_t now = sys_timer_get_usec();
            times[n_interrupt++] = now - last_clk;
            last_clk = now;

            if (gpio_read(p) == HIGH) {
                n_rising++;
            } else {
                n_falling++;
            }

            gpio_event_clear(p);
        }
    }
}

void main() {
    gpio_enable_int_rising_edge(p);
    gpio_enable_int_falling_edge(p);

    gpio_select_output(p);
    last_clk = sys_timer_get_usec();

    for (int i = 0; i < N; i++) {
        gpio_set_high(p);
        sys_timer_delay_us(10000);
        gpio_set_low(p);
        sys_timer_delay_us(20000);
    }

    uart_puts("Total interrupts: ");
    print_int(n_interrupt);
    uart_puts("\nTotal rising: ");
    print_int(n_rising);
    uart_puts("\nTotal falling: ");
    print_int(n_falling);

    uart_puts("\n\nTimes between interrupts (us):\n");
    for (int i = 1; i < N; i++) {
        print_int(i);
        uart_puts(": ");
        print_int(times[i]);
        uart_putc('\n');
    }

    uart_putk("DONE!!!\n");
    rpi_reboot();
}


