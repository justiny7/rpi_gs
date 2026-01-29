#include "sys_timer.h"
#include "lib.h"

uint32_t sys_timer_get_usec() {
    mem_barrier_dsb();
    uint32_t res = GET32(SYS_TIMER_CLO);
    mem_barrier_dsb();
    return res;
}

void sys_timer_delay_us(uint32_t us) {
    uint32_t st = sys_timer_get_usec();
    while (sys_timer_get_usec() - st < us);
}
void sys_timer_delay_ms(uint32_t ms) {
    sys_timer_delay_us(ms * 1000);
}
void sys_timer_delay_sec(uint32_t sec) {
    sys_timer_delay_us(sec * 1000000);
}
