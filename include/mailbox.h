#ifndef MAILBOX_H
#define MAILBOX_H

#include <stdint.h>

#define MBOX_FULL_BIT 0x80000000 // (1 << 31)
#define MBOX_EMPTY_BIT 0x40000000 // (1 << 30)

#define MBOX_BASE_ADDR 0x2000B880
#define GPU_L2_OFFSET 0x40000000

enum {
    MBOX_READ = MBOX_BASE_ADDR,
    MBOX_STATUS = MBOX_BASE_ADDR + 0x0018,
    MBOX_WRITE = MBOX_BASE_ADDR + 0x0020,
};

typedef enum {
    MB_POWER_MANAGEMENT = 0,
    MB_FRAMEBUFFER,
    MB_VIRTUAL_UART,
    MB_VCHIQ,
    MB_LEDS,
    MB_BUTTONS,
    MB_TOUCHSCREEN,
    MB_UNUSED,
    MB_TAGS_ARM_TO_VC,
    MB_TAGS_VC_TO_ARM,
} MboxChannel;

void mbox_write(MboxChannel channel, uint32_t data);
uint32_t mbox_read(MboxChannel channel);

#endif
