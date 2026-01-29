#ifndef SYS_TIMER_H
#define SYS_TIMER_H

#include <stdint.h>

#define SYS_TIMER_BASE 0x20003000

enum {
    SYS_TIMER_CS = SYS_TIMER_BASE,
    SYS_TIMER_CLO = SYS_TIMER_BASE + 0x0004,
    SYS_TIMER_CHI = SYS_TIMER_BASE + 0x0008,
    SYS_TIMER_C0 = SYS_TIMER_BASE + 0x000c,
    SYS_TIMER_C1 = SYS_TIMER_BASE + 0x0010,
    SYS_TIMER_C2 = SYS_TIMER_BASE + 0x0014,
    SYS_TIMER_C3 = SYS_TIMER_BASE + 0x0018,
};

// bottom 32 bits of system timer
uint32_t sys_timer_get_usec();

void sys_timer_delay_us(uint32_t us);
void sys_timer_delay_ms(uint32_t ms);
void sys_timer_delay_sec(uint32_t sec);

#endif
