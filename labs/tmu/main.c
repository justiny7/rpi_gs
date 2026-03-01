#include "mailbox_interface.h"
#include "qpu.h"
#include "lib.h"
#include "uart.h"
#include "sys_timer.h"
#include "arena_allocator.h"
#include "kernel.h"
#include "debug.h"

#include "qpu_code.h"

#define NUM_QPUS 12
#define NUM_UNIFS 6
#define ITERATIONS 1000
#define SIMD_WIDTH 16
#define N (ITERATIONS * NUM_QPUS * SIMD_WIDTH)
#define ELEMS_PER_QPU (ITERATIONS * SIMD_WIDTH)

Arena data_arena;

static uint32_t rand_state = 12345;
static uint32_t rand_int() {
    rand_state = rand_state * 1103515245 + 12345;
    return rand_state;
}

void main() {
    const uint32_t MiB = 1024 * 1024;
    arena_init(&data_arena, 30 * MiB);

    Kernel k;
    kernel_init(&k, NUM_QPUS, NUM_UNIFS, qpu_code, sizeof(qpu_code));

    uint32_t* arr = arena_alloc_align(&data_arena, N * sizeof(uint32_t), 16);
    for (int i = 0; i < N; i++) {
        arr[i] = rand_int();
    }
    uint32_t* indices = arena_alloc_align(&data_arena, N * sizeof(uint32_t), 16);
    for (int i = 0; i < N; i++) {
        indices[i] = rand_int() % N;
    }

    uint32_t* output = arena_alloc_align(&data_arena, N * sizeof(uint32_t), 16);

    DEBUG_D(N);
    for (int q = 0; q < NUM_QPUS; q++) {
        kernel_load_unif(&k, q, NUM_QPUS * SIMD_WIDTH);
        kernel_load_unif(&k, q, N);
        kernel_load_unif(&k, q, q);

        kernel_load_unif(&k, q, TO_BUS(arr));
        kernel_load_unif(&k, q, TO_BUS(indices + q * SIMD_WIDTH));
        kernel_load_unif(&k, q, TO_BUS(output + q * SIMD_WIDTH));
    }

    uint32_t t = sys_timer_get_usec();
    kernel_execute(&k);
    uint32_t gpu_t = sys_timer_get_usec() - t;
    DEBUG_D(gpu_t);

    kernel_free(&k);

    /*
    for (int i = 0; i < N; i++) {
        uart_putd(output[i]);
        uart_puts(" ");
    }
    uart_puts("\n");
    */

    for (int i = 0; i < N; i++) {
        if (output[i] != arr[indices[i]]) {
            DEBUG_DM(i, "iteration");
            DEBUG_D(output[i]);
            DEBUG_D(indices[i]);
            DEBUG_D(arr[indices[i]]);
            rpi_reset();
        }
    }

    uart_puts("SUCESS!\n");

    rpi_reset();
}
