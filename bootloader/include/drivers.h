// standalone drivers

#ifndef DRIVERS_H
#define DRIVERS_H

#include <stdint.h>
#include <stdbool.h>

/******* UART *******/
#define GPIO_BASE 0x20200000

enum {
    GPFSEL1 = GPIO_BASE + 0x0004,
    GPPUD_BASE = GPIO_BASE + 0x0094,
    GPPUDCLK_BASE = GPIO_BASE + 0x0098,
};

void gpio_select(uint32_t pin, uint32_t func);
void gpio_set_pull_off(uint32_t pin);

/******* UART *******/
#define UART_BASE 0x20215000

#define UART_TX_PIN 14
#define UART_RX_PIN 15

#define UART_ENABLE_BIT 1 // (1 << 0)
#define UART_TX_CNTL_BIT 2 // (1 << 1)
#define UART_RX_CNTL_BIT 1 // (1 << 0)

#define UART_TX_READY_BIT 32 // (1 << 5)
#define UART_TX_EMPTY_BIT 64 // (1 << 6)
#define UART_RX_READY_BIT 1 // (1 << 0)

#define UART_BAUDRATE_PRESCALE 270 // 115200 baud

enum {
    AUX_ENABLES = UART_BASE + 0x0004,
    AUX_MU_IO_REG = UART_BASE + 0x0040,
    AUX_MU_IER_REG = UART_BASE + 0x0044,
    AUX_MU_IIR_REG = UART_BASE + 0x0048,
    AUX_MU_LCR_REG = UART_BASE + 0x004C,
    AUX_MU_MCR_REG = UART_BASE + 0x0050,
    AUX_MU_LSR_REG = UART_BASE + 0x0054,
    AUX_MU_CNTL_REG = UART_BASE + 0x0060,
    AUX_MU_BAUD_REG = UART_BASE + 0x0068,
};

void uart_init();

uint8_t uart_getc();
void uart_putc(uint8_t c);

bool uart_can_getc();
bool uart_can_putc();

void uart_flush_tx();

/******* SYSTEM TIMER *******/
#define SYS_TIMER_CLO 0x20003004

uint32_t sys_timer_get_usec();

#endif
