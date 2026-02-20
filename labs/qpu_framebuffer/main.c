#include "mailbox_interface.h"
#include "qpu.h"
#include "lib.h"
#include "uart.h"
#include "sys_timer.h"

#include "qpu_code.h"

#define WIDTH 800
#define HEIGHT 600

#define NUM_QPUS 12
#define ITERATIONS (WIDTH * HEIGHT / (NUM_QPUS * SIMD_WIDTH))
#define SIMD_WIDTH 16
#define N (ITERATIONS * NUM_QPUS * SIMD_WIDTH)
#define ELEMS_PER_QPU (ITERATIONS * SIMD_WIDTH)

typedef struct {
    uint32_t unif[NUM_QPUS][4];
    uint32_t code[sizeof(qpu_code) / sizeof(qpu_code[0])] __attribute__((aligned(8)));

    uint32_t mbox_msg[NUM_QPUS * 2];
} GPU;

void main() {
    uint32_t *fb;
    uint32_t size, pitch;
    mbox_framebuffer_init(WIDTH, HEIGHT, 32, &fb, &size, &pitch);
    if (pitch != WIDTH * 4) {
        panic("Framebuffer init failed!\n");
    }

    uint32_t t;

    uint32_t gpu_ptr = qpu_init(sizeof(GPU));
    volatile GPU* ptr = (volatile GPU*) TO_CPU(gpu_ptr);

    memcpy((void*) ptr->code, qpu_code, sizeof(ptr->code));

    const uint32_t colors[] = {
        0x00000000,
        0x000000FF,
        0x0000FF00,
        0x0000FFFF,
        0x00FF0000,
        0x00FF00FF,
        0x00FFFF00,
        0x00FFFFFF,
        0x00000088,
        0x00008800,
        0x00880000,
        0x00888888
    };

    for (uint32_t i = 0; i < NUM_QPUS; i++) {
        ptr->unif[i][0] = (uint32_t) fb + i * ELEMS_PER_QPU * sizeof(uint32_t);
        ptr->unif[i][1] = ITERATIONS / 4;
        ptr->unif[i][2] = i;
        ptr->unif[i][3] = colors[i];

        ptr->mbox_msg[i * 2] = TO_BUS(&ptr->unif[i]);
        ptr->mbox_msg[i * 2 + 1] = TO_BUS(ptr->code);
    }

    mem_barrier_dsb();

    t = sys_timer_get_usec();
    qpu_execute(NUM_QPUS, (uint32_t*) ptr->mbox_msg);
    uint32_t gpu_us = sys_timer_get_usec() - t;

    uart_puts("GPU time: ");
    uart_putd(gpu_us);
    uart_puts("us\n");

    uint32_t* pixels = (uint32_t*) TO_CPU(fb);
    for (int i = 0; i < N; i += ELEMS_PER_QPU) {
        uart_putd(i);
        uart_puts(": ");
        uart_putx(pixels[i]);
        uart_puts("\n");
    }

    rpi_reset();
}
