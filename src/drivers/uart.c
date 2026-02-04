#include "lib.h"
#include "gpio.h"
#include "uart.h"

void uart_init() {
    mem_barrier_dsb();

    Pin tx = { UART_TX_PIN };
    Pin rx = { UART_RX_PIN };
    gpio_select(tx, GPIO_ALT_FUNC_5);
    gpio_select(rx, GPIO_ALT_FUNC_5);

    // Disable pulls
    gpio_set_pull(tx, GPIO_PULL_OFF);
    gpio_set_pull(rx, GPIO_PULL_OFF);

    mem_barrier_dsb();

    OR32(AUX_ENABLES, UART_ENABLE_BIT);

    mem_barrier_dsb();

    uart_disable();
    uart_disable_interrupts();

    // set 8-bit mode (write 0b11)
    PUT32(AUX_MU_LCR_REG, 3);

    // default high
    PUT32(AUX_MU_MCR_REG, 0);

    // clear FIFOs
    PUT32(AUX_MU_IIR_REG, 6);

    uart_set_baudrate(UART_BAUDRATE_PRESCALE);
    uart_enable();

    mem_barrier_dsb();
}

void uart_set_baudrate(uint32_t baudrate_reg) {
    PUT32(AUX_MU_BAUD_REG, baudrate_reg);
}

void uart_enable() {
    PUT32(AUX_MU_CNTL_REG, UART_RX_CNTL_BIT | UART_TX_CNTL_BIT);
}
void uart_rx_enable() {
    PUT32(AUX_MU_CNTL_REG, UART_RX_CNTL_BIT);
}
void uart_tx_enable() {
    PUT32(AUX_MU_CNTL_REG, UART_TX_CNTL_BIT);
}
void uart_disable() {
    PUT32(AUX_MU_CNTL_REG, 0);
}

void uart_disable_interrupts() {
    PUT32(AUX_MU_IER_REG, 0);
}

// helpers w/ no barriers
bool _uart_can_getc() {
    return GET32(AUX_MU_LSR_REG) & UART_RX_READY_BIT;
}
bool _uart_can_putc() {
    return GET32(AUX_MU_LSR_REG) & UART_TX_READY_BIT;
}
bool _uart_tx_is_empty() {
    return GET32(AUX_MU_LSR_REG) & UART_TX_EMPTY_BIT;
}
uint8_t _uart_getc() {
    while (!_uart_can_getc());
    return GET32(AUX_MU_IO_REG) & 0xFF;
}
void _uart_putc(uint8_t c) {
    while (!_uart_can_putc());
    PUT32(AUX_MU_IO_REG, (uint32_t) c);
}

uint8_t uart_getc() {
    mem_barrier_dsb();
    uint8_t res = _uart_getc();
    mem_barrier_dsb();

    return res;
}
void uart_putc(uint8_t c) {
    mem_barrier_dsb();
    _uart_putc(c);
    mem_barrier_dsb();
}
void uart_puts(const char* s) {
    mem_barrier_dsb();
    while (*s) {
        if (*s == '\n') {
            _uart_putc('\r');
        }
        _uart_putc(*s++);
    }
    mem_barrier_dsb();
}
void uart_putk(const char* s) {
    mem_barrier_dsb();
    while (*s) {
        _uart_putc(*s++);
    }
    mem_barrier_dsb();
}

bool uart_can_getc() {
    mem_barrier_dsb();
    bool res = _uart_can_getc();
    mem_barrier_dsb();
    return res;
}
bool uart_can_putc() {
    mem_barrier_dsb();
    bool res = _uart_can_putc();
    mem_barrier_dsb();
    return res;
}

bool uart_tx_is_empty() {
    mem_barrier_dsb();
    bool res = _uart_tx_is_empty();
    mem_barrier_dsb();
    return res;
}
void uart_flush_tx() {
    mem_barrier_dsb();
    while (!_uart_tx_is_empty());
    mem_barrier_dsb();
}
