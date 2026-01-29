#include "lib.h"
#include "gpio.h"
#include "sys_timer.h"

void gpio_select(Pin pin, PinMode mode) {
    const uint32_t p_num = pin.p_num;
    if (p_num > MAX_PIN_NUM) {
        return;
        // gpio_panic("illegal pin=%d\n", p_num);
    }
    if((mode & 0b111) != mode) {
        return;
        // gpio_panic("illegal func=%x\n", mode);
    }
    
    const uint32_t register_offset = REG_SIZE_BYTES * (p_num / 10);
    const uint32_t register_base = GPFSEL_BASE + register_offset;

    const uint32_t bit_offset = PINMODE_BITS * (p_num % 10);

    uint32_t value = GET32(register_base);
    value &= ~(0x7U << bit_offset);
    value |= (mode << bit_offset);
    PUT32(register_base, value);
}

void gpio_set(Pin pin, PinOutput value) {
    const uint32_t p_num = pin.p_num;
    if (p_num > MAX_PIN_NUM) {
        return;
        // gpio_panic("illegal pin=%d\n", p_num);
    }

    const uint32_t register_base = (value == LOW ? GPCLR_BASE : GPSET_BASE);
    const uint32_t register_offset = (p_num >> 5) * REG_SIZE_BYTES;
    const uint32_t val_bit = 1U << (p_num & 31);

    PUT32(register_base + register_offset, val_bit);
}

PinOutput gpio_read(Pin pin) {
    const uint32_t p_num = pin.p_num;
    if (p_num > MAX_PIN_NUM) {
        // gpio_panic("illegal pin=%d\n", p_num);
    }

    const uint32_t offset = (p_num >> 5) * REG_SIZE_BYTES;
    const uint32_t shift = p_num & 31;

    return (PinOutput) ((GET32(GPLEV_BASE + offset) >> shift) & 1);
}

void gpio_select_input(Pin pin) {
    gpio_select(pin, PM_INPUT);
}
void gpio_select_output(Pin pin) {
    gpio_select(pin, PM_OUTPUT);
}

void gpio_set_low(Pin pin) {
    gpio_set(pin, LOW);
}
void gpio_set_high(Pin pin) {
    gpio_set(pin, HIGH);
}

void gpio_set_pull(Pin pin, GpioPull pull) {
    const uint32_t p_num = pin.p_num;
    if (p_num > MAX_PIN_NUM) {
        return;
    }

    const uint32_t offset = REG_SIZE_BYTES * (p_num >> 5);
    const uint32_t shift = p_num & 31;

    PUT32(GPPUD_BASE, pull);
    sys_timer_delay_us(150);
    PUT32(GPPUDCLK_BASE + offset, 1U << shift);
    sys_timer_delay_us(150);
    PUT32(GPPUD_BASE, 0);
    PUT32(GPPUDCLK_BASE + offset, 0);
}

