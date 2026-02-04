#include "drivers.h"
#include "lib.h"
#include "boot_codes.h"
#include "boot_crc32.h"

void write32(uint32_t val) {
    uart_putc(val & 0xFF);
    uart_putc((val >> 8) & 0xFF);
    uart_putc((val >> 16) & 0xFF);
    uart_putc((val >> 24) & 0xFF);
}
uint32_t read32() {
    uint32_t res = 0;
    res |= uart_getc();
    res |= uart_getc() << 8;
    res |= uart_getc() << 16;
    res |= uart_getc() << 24;
    return res;
}

void main() {
    extern uint32_t __code_start__[];

    uart_init();

    // repeatedly call for code
    uint32_t t = sys_timer_get_usec();
    while (1) {
        if (uart_can_getc()) {
            break;
        }

        if (sys_timer_get_usec() - t >= LOOP_DELAY_US) {
            write32(GET_INFO);
            t = sys_timer_get_usec();
        }
    }

    // receive code, validate checksum
    uint32_t get_info_code = read32();
    uint32_t base_addr = read32(); // should be 0x8000
    uint32_t code_len = read32();
    uint32_t crc32_rec = read32();

    if (get_info_code != PUT_INFO || base_addr + code_len >= (uint32_t) __code_start__) {
        write32(BOOT_ERROR);
        rpi_reboot();
    }

    write32(GET_CODE);
    write32(crc32_rec);

    uint32_t put_code_op = read32();
    if (put_code_op != PUT_CODE) {
        write32(BOOT_ERROR);
        rpi_reboot();
    }

    // copy code to base address
    for (uint32_t i = 0; i < code_len; i++) {
        *(uint8_t*) (base_addr + i) = uart_getc();
    }

    if (crc32((void*) base_addr, code_len) != crc32_rec) { // calculated crc32
        write32(BOOT_ERROR);
        rpi_reboot();
    }

    write32(BOOT_SUCCESS);

    t = sys_timer_get_usec();
    while (sys_timer_get_usec() - t < 500 * 1000);

    ((void (*)(void)) base_addr)();
}
