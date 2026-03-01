#include "mailbox_interface.h"
#include "qpu.h"
#include "lib.h"
#include "uart.h"
#include "sys_timer.h"
#include "arena_allocator.h"
#include "kernel.h"
#include "camera.h"
#include "debug.h"
#include "gaussian.h"
#include "math.h"

#include "spherical_harmonics.h"

#define NUM_QPUS 12
#define NUM_UNIFS 60
#define SIMD_WIDTH 16
#define EPS 0.1

#define N (NUM_QPUS * SIMD_WIDTH * 300)
// #define N 96000

#define abs(x, y) (((x - y) > 0) ? (x - y) : (y - x))

// #define VERBOSE

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

uint32_t mul_eps(float a, float b, float eps) {
    a = (a < 0) ? -a : a;
    b = (b < 0) ? -b : b;
    float mn = min(a, b);
    float mx = max(a, b);

    uint32_t res = (mx - mn) < EPS;
    res |= (mn * (1.0 + eps) >= mx) && (mx * (1.0 - eps) <= mn);

#ifdef VERBOSE
    if (!res) {
        uart_puts("mn * (1.0 + eps): ");
        uart_putf(mn * (1.0 + eps));
        uart_puts("\n");
        uart_puts("mx: ");
        uart_putf(mx);
        uart_puts("\n\n");

        uart_puts("mx * (1.0 - eps): ");
        uart_putf(mx * (1.0 - eps));
        uart_puts("\n");
        uart_puts("mn: ");
        uart_putf(mn);
        uart_puts("\n\n");
    }
#endif
    return res;
}

Vec3 sh[N][16];
void calc_sh_cpu(Vec3 cam_pos,
        float* pos_x, float* pos_y, float* pos_z,
        float* color_r, float* color_g, float* color_b) {

    for (uint32_t i = 0; i < N; i++) {
        Vec3 pos = { { pos_x[i], pos_y[i], pos_z[i] } };
        Vec3 res = eval_sh(pos, sh[i], cam_pos); 
        color_r[i] = res.x;
        color_g[i] = res.y;
        color_b[i] = res.z;
    }
}

void calc_sh_qpu(Vec3 cam_pos,
        float* pos_x, float* pos_y, float* pos_z,
        float** sh_x, float** sh_y, float** sh_z,
        float* color_r, float* color_g, float* color_b) {

    Kernel kernel;
    kernel_init(&kernel,
            NUM_QPUS, NUM_UNIFS,
            spherical_harmonics, sizeof(spherical_harmonics));

    float** sh[3] = { sh_x, sh_y, sh_z };
    float* color[3] = { color_r, color_g, color_b };
    for (uint32_t q = 0; q < NUM_QPUS; q++) {
        kernel_load_unif(&kernel, q, NUM_QPUS * SIMD_WIDTH);
        kernel_load_unif(&kernel, q, N);
        kernel_load_unif(&kernel, q, q);

        kernel_load_unif(&kernel, q, TO_BUS(pos_x + q * SIMD_WIDTH));
        kernel_load_unif(&kernel, q, TO_BUS(pos_y + q * SIMD_WIDTH));
        kernel_load_unif(&kernel, q, TO_BUS(pos_z + q * SIMD_WIDTH));

        kernel_load_unif(&kernel, q, cam_pos.x);
        kernel_load_unif(&kernel, q, cam_pos.y);
        kernel_load_unif(&kernel, q, cam_pos.z);

        for (uint32_t c = 0; c < 3; c++) {
            kernel_load_unif(&kernel, q, TO_BUS(color[c] + q * SIMD_WIDTH));
            for (uint32_t i = 0; i < 16; i++) {
                kernel_load_unif(&kernel, q, TO_BUS(sh[c][i] + q * SIMD_WIDTH));
            }
        }
    }

    kernel_execute(&kernel);
}

void main() {
    DEBUG_D(N);

    uint32_t t;
    const int MiB = 1024 * 1024;

    Arena arena;
    arena_init(&arena, 40 * MiB);
    
    // output
    float* color_r = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* color_g = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* color_b = arena_alloc_align(&arena, N * sizeof(float), 16);

    float* color_r_cpu = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* color_g_cpu = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* color_b_cpu = arena_alloc_align(&arena, N * sizeof(float), 16);

    // input
    Camera* c = arena_alloc_align(&arena, sizeof(Camera), 16);
    Vec3 cam_pos = { { 0.0f, 0.0f, 0.0f } };
    Vec3 cam_target = { { 0.0f, 0.0f, 5.0f } };
    Vec3 cam_up = { { 0.0f, 1.0f, 0.0f } };

    init_camera(c, cam_pos, cam_target, cam_up, 800, 600);

    float* pos_x = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* pos_y = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* pos_z = arena_alloc_align(&arena, N * sizeof(float), 16);

    float* sh_x[16];
    float* sh_y[16];
    float* sh_z[16];
    for (uint32_t i = 0; i < 16; i++) {
        sh_x[i] = arena_alloc_align(&arena, N * sizeof(float), 16);
        sh_y[i] = arena_alloc_align(&arena, N * sizeof(float), 16);
        sh_z[i] = arena_alloc_align(&arena, N * sizeof(float), 16);
    }

    for (uint32_t i = 0; i < N; i++) {
        pos_x[i] = rand_f() * 1000;
        pos_y[i] = rand_f() * 1000;
        pos_z[i] = rand_f() * 1000;

        for (uint32_t j = 0; j < 16; j++) {
            sh_x[j][i] = rand_f() * 100000;
            sh_y[j][i] = rand_f() * 100000;
            sh_z[j][i] = rand_f() * 100000;

            sh[i][j] = (Vec3) { { sh_x[j][i], sh_y[j][i], sh_z[j][i] } };
        }
    }

    t = sys_timer_get_usec();
    calc_sh_cpu(cam_pos,
        pos_x, pos_y, pos_z,
        color_r_cpu, color_g_cpu, color_b_cpu);
    uint32_t cpu_t = sys_timer_get_usec() - t;
    DEBUG_D(cpu_t);

    t = sys_timer_get_usec();
    calc_sh_qpu(cam_pos,
        pos_x, pos_y, pos_z,
        sh_x, sh_y, sh_z,
        color_r, color_g, color_b);
    uint32_t qpu_t = sys_timer_get_usec() - t;
    DEBUG_D(qpu_t);

    float speedup = 1.0 * cpu_t / qpu_t;
    DEBUG_F(speedup);

#ifdef VERBOSE
    for (int i = 0; i < N; i++) {
        DEBUG_D(i);
        DEBUG_F(color_r[i]);
        DEBUG_F(color_g[i]);
        DEBUG_F(color_b[i]);

        DEBUG_F(color_r_cpu[i]);
        DEBUG_F(color_g_cpu[i]);
        DEBUG_F(color_b_cpu[i]);

        uart_puts("\n");
    }

    uart_puts("\n");
#endif

    uint32_t mismatch = 0;
    for (int i = 0; i < N; i++) {
        if (!mul_eps(color_r[i], color_r_cpu[i], EPS) ||
            !mul_eps(color_r[i], color_r_cpu[i], EPS) ||
            !mul_eps(color_r[i], color_r_cpu[i], EPS)) {
#ifdef VERBOSE
            uart_puts("SH MISMATCH: ");
            uart_putd(i);
            uart_puts("\n");

            DEBUG_F(color_r[i]);
            DEBUG_F(color_g[i]);
            DEBUG_F(color_b[i]);

            DEBUG_F(color_r_cpu[i]);
            DEBUG_F(color_g_cpu[i]);
            DEBUG_F(color_b_cpu[i]);
            
            uart_puts("\n");
#endif

            mismatch++;
        }
    }

    // small FP errors
    DEBUG_D(mismatch);

    rpi_reset();
}
