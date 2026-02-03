#include "drivers.h"
#include "lib.h"

/******* GPIO *******/
void gpio_select(uint32_t pin, uint32_t func) {
    const uint32_t offset = 3 * (pin % 10);
    uint32_t val = GET32(GPFSEL1);
    val &= ~(0x7U << offset);
    val |= (func << offset);
    PUT32(GPFSEL1, val);
}
void gpio_set_pull_off(uint32_t pin) {
    PUT32(GPPUD_BASE, 0);
    for (volatile int i = 0; i < 150; i++);
    PUT32(GPPUDCLK_BASE, 1U << pin);
    for (volatile int i = 0; i < 150; i++);
    PUT32(GPPUD_BASE, 0);
    PUT32(GPPUDCLK_BASE, 0);
}

/******* UART *******/
void uart_init() {
    mem_barrier_dsb();

    gpio_select(UART_TX_PIN, 2); // Alt func 5
    gpio_select(UART_RX_PIN, 2);

    // Disable pulls
    gpio_set_pull_off(UART_TX_PIN);
    gpio_set_pull_off(UART_RX_PIN);

    mem_barrier_dsb();

    OR32(AUX_ENABLES, UART_ENABLE_BIT);

    mem_barrier_dsb();

    // disable UART
    PUT32(AUX_MU_CNTL_REG, 0);

    // disable interrupts
    PUT32(AUX_MU_IER_REG, 0);

    // set 8-bit mode (write 0b11)
    PUT32(AUX_MU_LCR_REG, 3);

    // default high
    PUT32(AUX_MU_MCR_REG, 0);

    // clear FIFOs
    PUT32(AUX_MU_IIR_REG, 6);

    // set baud rate
    PUT32(AUX_MU_BAUD_REG, UART_BAUDRATE_PRESCALE);

    // enable UART
    PUT32(AUX_MU_CNTL_REG, UART_RX_CNTL_BIT | UART_TX_CNTL_BIT);

    mem_barrier_dsb();
}

uint8_t uart_getc() {
    mem_barrier_dsb();
    while (!(GET32(AUX_MU_LSR_REG) & UART_RX_READY_BIT));
    uint8_t res = GET32(AUX_MU_IO_REG) & 0xFF;
    mem_barrier_dsb();
    return res;
}
void uart_putc(uint8_t c) {
    mem_barrier_dsb();
    while (!(GET32(AUX_MU_LSR_REG) & UART_TX_READY_BIT));
    PUT32(AUX_MU_IO_REG, (uint32_t) c);
    mem_barrier_dsb();
}

bool uart_can_getc() {
    mem_barrier_dsb();
    bool res = GET32(AUX_MU_LSR_REG) & UART_RX_READY_BIT;
    mem_barrier_dsb();
    return res;
}

void uart_flush_tx() {
    mem_barrier_dsb();
    while (!(GET32(AUX_MU_LSR_REG) & UART_TX_EMPTY_BIT));
    mem_barrier_dsb();
}

/******* SYSTEM TIMER *******/
uint32_t sys_timer_get_usec() {
    mem_barrier_dsb();
    uint32_t res = GET32(SYS_TIMER_CLO);
    mem_barrier_dsb();
    return res;
}
