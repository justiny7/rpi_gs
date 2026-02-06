#include "mailbox.h"
#include "lib.h"

void mbox_write(MboxChannel channel, uint32_t data) {
    mem_barrier_dsb();
    while (GET32(MBOX_STATUS) & MBOX_FULL_BIT);
    PUT32(MBOX_WRITE, (data & ~0xF) | (channel & 0xF) | GPU_L2_OFFSET);
    mem_barrier_dsb();
}
uint32_t mbox_read(MboxChannel channel) {
    while (1) {
        mem_barrier_dsb();
        while (GET32(MBOX_STATUS) & MBOX_EMPTY_BIT); 
        uint32_t res = GET32(MBOX_READ);
        mem_barrier_dsb();
        
        if ((res & 0xF) == channel) {
            return res & ~0xF;
        }
    }
}
