/*

Goal: Write to output array

Output is size N, ELEMS_PER_QPU = N / NUM_QPUS. We want

    output[i * ELEMS_PER_QPU : (i + 1) * ELEMS_PER_QPU] = i

for i = 0 ... NUM_QPUS - 1

QPU i is in charge of element i. It's given:
- Pointer to beginning index in output
- Number of 16-wide moves it has to do (ELEMS_PER_QPU / 16)
- i

*/

#include "mailbox_interface.h"
#include "qpu.h"
#include "lib.h"
#include "uart.h"
#include "sys_timer.h"

#include "qpu_code.h"
#include "debug.h"

#define NUM_QPUS 16
#define ITERATIONS 10000
#define SIMD_WIDTH 16
#define N (ITERATIONS * NUM_QPUS * SIMD_WIDTH)
#define ELEMS_PER_QPU (ITERATIONS * SIMD_WIDTH)

typedef struct {
    uint32_t output[N] __attribute__((aligned(16)));

    uint32_t unif[NUM_QPUS][3];
    uint32_t code[sizeof(qpu_code) / sizeof(qpu_code[0])] __attribute__((aligned(8)));

    uint32_t mbox_msg[NUM_QPUS * 2];
} GPU;

void main() {
    uint32_t t;

    DEBUG_D(sizeof(GPU));
    uint32_t gpu_ptr = qpu_init(sizeof(GPU));
    volatile GPU* ptr = (volatile GPU*) TO_CPU(gpu_ptr);

    memcpy((void*) ptr->code, qpu_code, sizeof(ptr->code));

    /*
    for (int i = 0; i < N; i++) {
        uart_putx(ptr->output[i]);
        uart_puts(" ");

        if (i % ELEMS_PER_QPU == ELEMS_PER_QPU - 1) {
            uart_puts("\n");
        }
    }

    uart_puts("\n----------------\n\n");
    */

    for (uint32_t i = 0; i < NUM_QPUS; i++) {
        ptr->unif[i][0] = TO_BUS(ptr->output) + i * ELEMS_PER_QPU * sizeof(uint32_t);
        ptr->unif[i][1] = ITERATIONS / 4;
        ptr->unif[i][2] = i;
        DEBUG_X(ptr->unif[i][0]);
        DEBUG_D(ptr->unif[i][1]);
        DEBUG_D(ptr->unif[i][2]);

        ptr->mbox_msg[i * 2] = TO_BUS(ptr->unif + i);
        ptr->mbox_msg[i * 2 + 1] = TO_BUS(ptr->code);
        DEBUG_X(ptr->mbox_msg[i * 2]);
        DEBUG_X(ptr->mbox_msg[i * 2 + 1]);
    }

    DEBUG_X(ptr->output);
    DEBUG_X(ptr->unif);
    DEBUG_X(ptr->mbox_msg);
    DEBUG_X(ptr->code);


    mem_barrier_dsb();

    t = sys_timer_get_usec();
    qpu_execute(NUM_QPUS, (uint32_t*) ptr->mbox_msg);
    uint32_t gpu_us = sys_timer_get_usec() - t;

    uart_puts("GPU time: ");
    uart_putd(gpu_us);
    uart_puts("us\n");

    uint32_t arr[N];

    t = sys_timer_get_usec();
    for (uint32_t i = 0, c = 0; i < NUM_QPUS; i++) {
        for (int j = 0; j < ELEMS_PER_QPU; j++) {
            arr[c++] = i;
        }
    }
    uint32_t cpu_us = sys_timer_get_usec() - t;

    uart_puts("CPU time: ");
    uart_putd(cpu_us);
    uart_puts("us\n");

    uint32_t speedup_1000x = 1000.f * cpu_us / gpu_us;
    uart_puts("Speedup: ");
    uart_putd(speedup_1000x / 1000);
    uart_puts(".");
    uart_putd(speedup_1000x - (speedup_1000x / 1000) * 1000);
    uart_puts("x\n");

    /*
    for (int i = 0; i < N; i++) {
        uart_putd(ptr->output[i]);
        uart_puts(" ");

        if (i % ELEMS_PER_QPU == ELEMS_PER_QPU - 1) {
            uart_puts("\n");
        }
    }
    */

    for (int i = 0; i < N; i++) {
        assert(arr[i] == ptr->output[i], "mismatch");
    }


    rpi_reset();
}
