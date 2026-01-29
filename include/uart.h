#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

#define UART_BASE 0x20215000

#define UART_TX_PIN 14
#define UART_RX_PIN 15

#define UART_ENABLE_BIT 1 // (1 << 0)
#define UART_TX_CNTL_BIT 2 // (1 << 1)
#define UART_RX_CNTL_BIT 1 // (1 << 0)

#define UART_TX_READY_BIT 32 // (1 << 5)
#define UART_RX_READY_BIT 1 // (1 << 0)

// baudrate = system_clock_freq / (8 * (baud_rate_reg + 1))
// so 250MHz / (8 * 115200 BR)) - 1 = ~270
#define UART_BAUDRATE_PRESCALE 270
// #define UART_BAUDRATE_PRESCALE 3254 // for 9600 baudrate

enum {
    AUX_IRQ = UART_BASE,
    AUX_ENABLES = UART_BASE + 0x0004,
    AUX_MU_IO_REG = UART_BASE + 0x0040,
    AUX_MU_IER_REG = UART_BASE + 0x0044,
    AUX_MU_IIR_REG = UART_BASE + 0x0048,
    AUX_MU_LCR_REG = UART_BASE + 0x004C,
    AUX_MU_MCR_REG = UART_BASE + 0x0050,
    AUX_MU_LSR_REG = UART_BASE + 0x0054,
    AUX_MU_MSR_REG = UART_BASE + 0x0058,
    AUX_MU_SCRATCH = UART_BASE + 0x005C,
    AUX_MU_CNTL_REG = UART_BASE + 0x0060,
    AUX_MU_STAT_REG = UART_BASE + 0x0064,
    AUX_MU_BAUD_REG = UART_BASE + 0x0068,
};

void uart_init();
void uart_set_baudrate(uint32_t baudrate_reg);

void uart_enable();
void uart_rx_enable();
void uart_tx_enable();
void uart_disable();

void uart_disable_interrupts();

bool _uart_can_getc();
bool _uart_can_putc();
unsigned char _uart_getc();
void _uart_putc(unsigned char c);

unsigned char uart_getc();
void uart_putc(unsigned char c);
void uart_puts(const char* s);

bool uart_can_getc();
bool uart_can_putc();

void uart_flush_tx();

#endif
