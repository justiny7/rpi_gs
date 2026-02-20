#include "qpu.h"
#include "lib.h"
#include "mailbox_interface.h"

#include "uart.h"

uint32_t qpu_init(uint32_t num_bytes) {
    assert(!mbox_set_enable_qpu(1), "Failed QPU enable");

    uint32_t handle = mbox_allocate_memory(num_bytes, PAGE_SIZE, MEM_FLAG_L1_NONALLOCATING);
    if (!handle) {
        assert(!mbox_set_enable_qpu(0), "Failed QPU disable");
        panic("Can't allocate memory");
    }

    return mbox_lock_memory(handle);
}

void qpu_free(uint32_t handle) {
    mbox_release_memory(handle);
    mbox_unlock_memory(handle);
    assert(!mbox_set_enable_qpu(0), "Failed QPU disable");
}

void qpu_execute(uint32_t num_qpus, uint32_t* mail) {
    PUT32(V3D_DBCFG, 0); // Disallow IRQ
    PUT32(V3D_DBQITE, 0); // Disable IRQ
    PUT32(V3D_DBQITC, -1); // Resets IRQ flags

    PUT32(V3D_L2CACTL, 1 << 2); // Clear L2 cache
    PUT32(V3D_SLCACTL, -1); // Clear other caches

    PUT32(V3D_SRQCS, (1 << 7) | (1 << 8) | (1 << 16)); // Reset error bit and counts

    for (uint32_t q = 0; q < num_qpus; q++) {
        PUT32(V3D_SRQUA, mail[q * 2]);
        PUT32(V3D_SRQPC, mail[q * 2 + 1]);
    }

    // Busy wait polling
    while (((GET32(V3D_SRQCS) >> 16) & 0xFF) != num_qpus);

}
