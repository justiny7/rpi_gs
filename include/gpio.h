#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#define MAX_PIN_NUM 53

#define GPIO_BASE 0x20200000
#define REG_SIZE_BYTES 4
#define REG_SIZE_BITS 32

#define PINMODE_BITS 3

enum {
    GPFSEL_BASE = GPIO_BASE,
    GPSET_BASE = GPIO_BASE + 0x001C,
    GPCLR_BASE = GPIO_BASE + 0x0028,
    GPLEV_BASE = GPIO_BASE + 0x0034,
    GPPUD_BASE = GPIO_BASE + 0x0094,
    GPPUDCLK_BASE = GPIO_BASE + 0x0098,
};

typedef struct Pin_t {
    uint32_t p_num;
} Pin;

typedef enum PinMode_t {
    GPIO_INPUT = 0,
    GPIO_OUTPUT = 1,
    GPIO_ALT_FUNC_0 = 4,
    GPIO_ALT_FUNC_1 = 5,
    GPIO_ALT_FUNC_2 = 6,
    GPIO_ALT_FUNC_3 = 7,
    GPIO_ALT_FUNC_4 = 3,
    GPIO_ALT_FUNC_5 = 2,
} PinMode;

typedef enum PinOutput_t {
    LOW,
    HIGH,
} PinOutput;

typedef enum GpioPull_t {
    GPIO_PULL_OFF,
    GPIO_PULL_DOWN,
    GPIO_PULL_UP,
} GpioPull;

void gpio_select(Pin pin, PinMode mode);
void gpio_set(Pin pin, PinOutput value);
PinOutput gpio_read(Pin pin);

void gpio_select_input(Pin pin);
void gpio_select_output(Pin pin);

void gpio_set_low(Pin pin);
void gpio_set_high(Pin pin);

void gpio_set_pull(Pin pin, GpioPull pull);

#endif



