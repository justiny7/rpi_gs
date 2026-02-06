#include "lib.h"
#include "uart.h"
#include "mailbox_interface.h"

void main() {
    uart_puts("Board serial #: ");
    uart_putx(mbox_get_board_serial());
    uart_puts("\n");

    uart_puts("Board revision: ");
    uart_putx(mbox_get_board_revision());
    uart_puts("\n");

    uart_puts("ARM mem: ");
    uart_putd(mbox_get_arm_memory());
    uart_puts("\n");

    uart_puts("GPU mem: ");
    uart_putd(mbox_get_vc_memory());
    uart_puts("\n");

    uart_puts("Board temp: ");
    uart_putd(mbox_get_temp());
    uart_puts("\n");

    uart_puts("Board max temp: ");
    uart_putd(mbox_get_max_temp());
    uart_puts("\n");

    rpi_reset();
}

