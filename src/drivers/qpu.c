#include "qpu.h"
#include "lib.h"

#include "uart.h"

// void qpu_execute(uint32_t num_qpus, uint32_t* unifs, uint32_t* code) {
void qpu_execute(uint32_t num_qpus, uint32_t* mail) {
    PUT32(V3D_DBCFG, 0); // Disallow IRQ
    PUT32(V3D_DBQITE, 0); // Disable IRQ
    PUT32(V3D_DBQITC, -1); // Resets IRQ flags

    PUT32(V3D_L2CACTL, 1 << 2); // Clear L2 cache
    PUT32(V3D_SLCACTL, -1); // Clear other caches

    PUT32(V3D_SRQCS, (1 << 7) | (1 << 8) | (1 << 16)); // Reset error bit and counts

    for (uint32_t q = 0; q < num_qpus; q++) {
        /*
        PUT32(V3D_SRQUA, (uint32_t) unifs[q]);
        PUT32(V3D_SRQPC, (uint32_t) code);
        */
        PUT32(V3D_SRQUA, mail[q * 2]);
        PUT32(V3D_SRQPC, mail[q * 2 + 1]);
    }

    // Busy wait polling
    while (((GET32(V3D_SRQCS) >> 16) & 0xFF) != num_qpus);

}
