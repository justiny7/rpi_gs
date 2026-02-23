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
#include "arena_allocator.h"
#include "kernel.h"
#include "camera.h"
#include "debug.h"

#include "project_points.h"

#define NUM_QPUS 16
#define NUM_UNIFS 25
#define SIMD_WIDTH 16

#define N NUM_QPUS * SIMD_WIDTH * 500

#define abs(x, y) (((x - y) > 0) ? (x - y) : (y - x))

// #define VERBOSE

void uart_putf(float x) {
    uint32_t x1000 = 1000.f * x;
    uart_putd(x1000 / 1000);
    uart_puts(".");
    uart_putd(x1000 - (x1000 / 1000) * 1000);
}


uint32_t rng = 1;
float rand_f() {
    union {
        uint32_t d;
        float f;
    } u;

    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;

    u.d = (rng >> 9) | 0x3f800000;
    return u.f - 1.0;
}

void main() {
    uint32_t t;
    const int MiB = 1024 * 1024;

    Arena arena;
    arena_init(&arena, 40 * MiB);

    float* pos_x = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* pos_y = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* pos_z = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* depth = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* screen_x = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* screen_y = arena_alloc_align(&arena, N * sizeof(float), 16);

    Camera* c = arena_alloc_align(&arena, sizeof(Camera), 16);

    for (uint32_t i = 0; i < N; i++) {
        pos_x[i] = rand_f();
        pos_y[i] = rand_f();
        pos_z[i] = rand_f();

#ifdef VERBOSE
        uart_putf(pos_x[i]);
        uart_puts("\t");
        uart_putf(pos_y[i]);
        uart_puts("\t");
        uart_putf(pos_z[i]);
        uart_puts("\n");
#endif

    }
    c->fx = rand_f();
    c->fy = rand_f();
    c->cx = rand_f();
    c->cy = rand_f();
    for (uint32_t i = 0; i < 16; i++) {
        c->w2c.m[i] = rand_f();

#ifdef VERBOSE
        uart_putd(i);
        uart_puts(": ");
        uart_putf(c->w2c.m[i]);
        uart_puts("\n");
#endif
    }

#ifdef VERBOSE
    uart_puts("fx: ");
    uart_putf(c->fx);
    uart_puts("\n");
    uart_puts("cx: ");
    uart_putf(c->cx);
    uart_puts("\n");
    uart_puts("fy: ");
    uart_putf(c->fy);
    uart_puts("\n");
    uart_puts("cy: ");
    uart_putf(c->cy);
    uart_puts("\n");
#endif

    Kernel project_points_k;

    kernel_init(&project_points_k,
            NUM_QPUS, NUM_UNIFS,
            project_points, sizeof(project_points));

    for (uint32_t q = 0; q < NUM_QPUS; q++) {
        kernel_load_unif(&project_points_k, q, NUM_QPUS * SIMD_WIDTH);
        kernel_load_unif(&project_points_k, q, N);
        kernel_load_unif(&project_points_k, q, q);
        kernel_load_unif(&project_points_k, q, TO_BUS(pos_x + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(pos_y + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(pos_z + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(depth + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(screen_x + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(screen_y + q * SIMD_WIDTH));

        for (uint32_t i = 0; i < 12; i++) {
            kernel_load_unif(&project_points_k, q, c->w2c.m[i]);
        }
        kernel_load_unif(&project_points_k, q, c->fx);
        kernel_load_unif(&project_points_k, q, c->cx);
        kernel_load_unif(&project_points_k, q, c->fy);
        kernel_load_unif(&project_points_k, q, c->cy);
    }

    DEBUG_D(N);

    t = sys_timer_get_usec();
    kernel_execute(&project_points_k);
    uint32_t qpu_time = sys_timer_get_usec() - t;
    DEBUG_D(qpu_time);

#ifdef VERBOSE
    for (uint32_t i = 0; i < N; i++) {
        uart_putd(i);
        uart_puts("\ndepth: ");
        uart_putf(depth[i]);
        uart_puts("\nscreen_y: ");
        uart_putf(screen_x[i]);
        uart_puts("\nscreen_z: ");
        uart_putf(screen_y[i]);
        uart_puts("\n\n");
    }
#endif

    float depth_res[N];
    float screen_x_res[N];
    float screen_y_res[N];

    t = sys_timer_get_usec();
    for (uint32_t i = 0; i < N; i++) {
        Vec3 p = { { pos_x[i], pos_y[i], pos_z[i] } };

        project_point(c, p, &depth_res[i], &screen_x_res[i], &screen_y_res[i]);
    }
    uint32_t cpu_time = sys_timer_get_usec() - t;
    DEBUG_D(cpu_time);

    for (uint32_t i = 0; i < N; i++) {
        if (abs(depth_res[i], depth[i]) > 0.001) {
            uart_puts("DEPTH MISMATCH: ");
            uart_putd(i);
            uart_puts("\nQPU: ");
            uart_putf(depth[i]);
            uart_puts("\nCPU: ");
            uart_putf(depth_res[i]);
            rpi_reset();
        }
        if (abs(screen_x_res[i], screen_x[i]) > 0.001) {
            uart_puts("SCREEN X MISMATCH: ");
            uart_putd(i);
            uart_puts("\nQPU: ");
            uart_putf(screen_x[i]);
            uart_puts("\nCPU: ");
            uart_putf(screen_x_res[i]);
            rpi_reset();
        }
        if (abs(screen_y_res[i], screen_y[i]) > 0.001) {
            uart_puts("SCREEN Y MISMATCH: ");
            uart_putd(i);
            uart_puts("\nQPU: ");
            uart_putf(screen_y[i]);
            uart_puts("\nCPU: ");
            uart_putf(screen_y_res[i]);
            rpi_reset();
        }
    }

    rpi_reset();
}
