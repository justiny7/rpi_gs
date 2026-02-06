#include "gpio.h"
#include "uart.h"
#include "lib.h"
#include "sys_timer.h"
#include "mailbox_interface.h"

#define MSG_SIZE 8

uint32_t get_serial_number() {
    uint32_t msg[MSG_SIZE] __attribute__((aligned(16))) = {
        MSG_SIZE * sizeof(uint32_t),
        MBOX_REQUEST,
        MBOX_TAG_GET_BOARD_SERIAL,
        8,
        MBOX_REQUEST,
        0,
        0,
        0
    };

    mem_barrier_dsb();
    int res = mbox_get_property(msg);
    mem_barrier_dsb();

    return msg[5];
}

uint32_t get_revision() {
    uint32_t msg[MSG_SIZE] __attribute__((aligned(16))) = {
        MSG_SIZE * sizeof(uint32_t),
        MBOX_REQUEST,
        MBOX_TAG_GET_BOARD_REVISION,
        4,
        MBOX_REQUEST,
        0,
        0,
        0
    };

    mem_barrier_dsb();
    int res = mbox_get_property(msg);
    mem_barrier_dsb();

    return msg[5];
}

uint64_t get_memory() {
    uint32_t msg[MSG_SIZE] __attribute__((aligned(16))) = {
        MSG_SIZE * sizeof(uint32_t),
        MBOX_REQUEST,
        MBOX_TAG_GET_VC_MEMORY,
        8,
        MBOX_REQUEST,
        0,
        0,
        0
    };

    mem_barrier_dsb();
    int res = mbox_get_property(msg);
    mem_barrier_dsb();

    return msg[6];
    // return (((uint64_t) msg[6]) << 32) | msg[5];
}

uint32_t get_temp() {
    uint32_t msg[MSG_SIZE] __attribute__((aligned(16))) = {
        MSG_SIZE * sizeof(uint32_t),
        MBOX_REQUEST,
        MBOX_TAG_GET_TEMP,
        8,
        MBOX_REQUEST,
        0,
        0,
        0
    };

    mem_barrier_dsb();
    int res = mbox_get_property(msg);
    mem_barrier_dsb();

    return msg[6];
}

void main() {

    uart_puts("Board serial #: ");
    uart_putx(get_serial_number());
    uart_puts("\n");

    uart_puts("Board revision: ");
    uart_putx(get_revision());
    uart_puts("\n");

    uart_puts("Board mem: ");
    uart_putd(get_memory());
    uart_puts("\n");

    /*
    uint64_t mem_result = get_memory();
    uart_puts("Base mem address: ");
    uart_putx(mem_result >> 32);
    uart_puts("\nMem size (bytes): ");
    uart_putd(mem_result & ((1LL << 32) - 1));
    */

    uart_puts("\nBoard temp: ");
    uart_putd(get_temp());
    uart_puts("\n");

    uart_putk("\r\nDONE!!!\n");
    rpi_reboot();
}

