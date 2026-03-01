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

#include "project_points_cov2d_inv.h"

#define NUM_QPUS 12
// #define NUM_QPUS 1
#define NUM_UNIFS 35
#define SIMD_WIDTH 16
#define EPS 0.1

#define N NUM_QPUS * SIMD_WIDTH * 300
// #define N NUM_QPUS * SIMD_WIDTH * 2

// #define VERBOSE

uint32_t rng = 2948095;
// uint32_t rng = 1;
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

float abs(float x, float y) {
    if (x - y > EPS) {
        return x - y;
    } else {
        return y - x;
    }
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


void main() {
    uint32_t t;
    const int MiB = 1024 * 1024;

    Arena arena;
    arena_init(&arena, 30 * MiB);

    // input
    float* pos_x = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* pos_y = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* pos_z = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* cov3d[9];
    for (uint32_t i = 0; i < 9; i++) {
        cov3d[i] = arena_alloc_align(&arena, N * sizeof(float), 16);
    }

    // output
    float* depth = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* radius = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* screen_x = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* screen_y = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* cov2d_inv_x = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* cov2d_inv_y = arena_alloc_align(&arena, N * sizeof(float), 16);
    float* cov2d_inv_z = arena_alloc_align(&arena, N * sizeof(float), 16);

    Camera* c = arena_alloc_align(&arena, sizeof(Camera), 16);
    Vec3 cam_pos = { { 0.0f, 0.0f, 0.0f } };
    Vec3 cam_target = { { 0.0f, 0.0f, 5.0f } };
    Vec3 cam_up = { { 0.0f, 1.0f, 0.0f } };

    init_camera(c, cam_pos, cam_target, cam_up, 640, 480);

    for (uint32_t i = 0; i < N; i++) {
        pos_x[i] = rand_f() * 100;
        pos_y[i] = rand_f() * 100;
        pos_z[i] = rand_f() * 100;

        Vec3 scale;
        scale.x = rand_f() * 2;
        scale.y = rand_f() * 2;
        scale.z = rand_f() * 2;

        Vec4 rot;
        rot.x = rand_f();
        rot.y = rand_f();
        rot.z = rand_f();
        rot.w = rand_f();
        vec4_sdiv(rot, vec4_len(rot));

        Mat3 cov3d_m = compute_cov3d(scale, rot);
        for (int j = 0; j < 9; j++) {
            cov3d[j][i] = cov3d_m.m[j];
        }

#ifdef VERBOSE
        uart_putf(pos_x[i]);
        uart_puts("\t");
        uart_putf(pos_y[i]);
        uart_puts("\t");
        uart_putf(pos_z[i]);
        uart_puts("\n");
#endif

    }

#ifdef VERBOSE
    for (uint32_t i = 0; i < 16; i++) {
        uart_putd(i);
        uart_puts(": ");
        uart_putf(c->w2c.m[i]);
        uart_puts("\n");
    }

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

    Kernel project_points_cov2d_inv_k;

    kernel_init(&project_points_cov2d_inv_k,
            NUM_QPUS, NUM_UNIFS,
            project_points_cov2d_inv, sizeof(project_points_cov2d_inv));

    for (uint32_t q = 0; q < NUM_QPUS; q++) {
        kernel_load_unif(&project_points_cov2d_inv_k, q, NUM_QPUS * SIMD_WIDTH);
        kernel_load_unif(&project_points_cov2d_inv_k, q, N);
        kernel_load_unif(&project_points_cov2d_inv_k, q, q);
        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(pos_x + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(pos_y + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(pos_z + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(depth + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(screen_x + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(screen_y + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(radius + q * SIMD_WIDTH));

        for (uint32_t i = 0; i < 12; i++) {
            kernel_load_unif(&project_points_cov2d_inv_k, q, c->w2c.m[i]);
        }
        kernel_load_unif(&project_points_cov2d_inv_k, q, c->fx);
        kernel_load_unif(&project_points_cov2d_inv_k, q, c->cx);
        kernel_load_unif(&project_points_cov2d_inv_k, q, c->fy);
        kernel_load_unif(&project_points_cov2d_inv_k, q, c->cy);

        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(cov3d[0] + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(cov3d[1] + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(cov3d[2] + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(cov3d[4] + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(cov3d[5] + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(cov3d[8] + q * SIMD_WIDTH));

        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(cov2d_inv_x + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(cov2d_inv_y + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_cov2d_inv_k, q, TO_BUS(cov2d_inv_z + q * SIMD_WIDTH));
    }

    DEBUG_D(N);

    t = sys_timer_get_usec();
    kernel_execute(&project_points_cov2d_inv_k);
    uint32_t qpu_time = sys_timer_get_usec() - t;
    DEBUG_D(qpu_time);

#ifdef VERBOSE
    for (uint32_t i = 0; i < N; i++) {
        DEBUG_DM(i, "Iteration");
        DEBUG_F(depth[i]);
        DEBUG_F(screen_x[i]);
        DEBUG_F(screen_y[i]);
        DEBUG_F(cov2d_inv_x[i]);
        DEBUG_F(cov2d_inv_y[i]);
        DEBUG_F(cov2d_inv_z[i]);
        DEBUG_F(radius[i]);
        uart_puts("\n");
    }
#endif

    float depth_res[N];
    float screen_x_res[N];
    float screen_y_res[N];
    float radius_res[N];
    Vec3 cov2d_inv_res[N];

    t = sys_timer_get_usec();
    for (uint32_t i = 0; i < N; i++) {
        Vec3 p = { { pos_x[i], pos_y[i], pos_z[i] } };
        Mat3 cov3d_m;
        for (uint32_t j = 0; j < 9; j++) {
            cov3d_m.m[j] = cov3d[j][i];
        }

        project_point(c, p, &depth_res[i], &screen_x_res[i], &screen_y_res[i]);

        // don't care about Gaussians not in frustum
        if (depth_res[i] < CAMERA_ZNEAR || depth_res[i] > CAMERA_ZFAR) {
            continue;
        }
        Vec3 cov2d = project_cov2d(p, cov3d_m, c->w2c, c->fx, c->fy);

        cov2d.x += 0.3f;
        cov2d.z += 0.3f;

        cov2d_inv_res[i] = compute_cov2d_inverse(cov2d);

        float mid = 0.5f * (cov2d.x + cov2d.z);
        float det = max(cov2d.x * cov2d.z - cov2d.y * cov2d.y, 1e-6f);
        float lambda1 = mid + sqrtf(max(0.1f, mid * mid - det));
        float lambda2 = mid - sqrtf(max(0.1f, mid * mid - det));
        float r = 3.5f * sqrtf(max(lambda1, lambda2));

        /*
        DEBUG_DM(i, "iteration");
        DEBUG_F(screen_x_res[i]);
        DEBUG_F(screen_y_res[i]);
        DEBUG_F(depth_res[i]);
        DEBUG_F(mid);
        DEBUG_F(det);
        DEBUG_F(lambda1);
        DEBUG_F(lambda2);
        DEBUG_F(radius[i]);
        DEBUG_F(r);
        uart_puts("\n");
        */

        radius_res[i] = r;

        /*
        int r_int = (int) r;
        if (r > r_int) {
            radius_res[i] = r + 1;
        } else {
            radius_res[i] = r;
        }
        */
    }

    uint32_t cpu_time = sys_timer_get_usec() - t;
    DEBUG_D(cpu_time);

    float speedup = 1.0 * cpu_time / qpu_time;
    DEBUG_F(speedup);

    uint32_t depth_screen_mismatch = 0, cov2d_mismatch = 0, rad_mismatch = 0;
    for (uint32_t i = 0; i < N; i++) {
        // don't care about Gaussians not in frustum, though QPU will still calculate
        if (depth_res[i] < CAMERA_ZNEAR || depth_res[i] > CAMERA_ZFAR) {
            continue;
        }

        if (!mul_eps(screen_x_res[i], screen_x[i], EPS) ||
                !mul_eps(screen_y_res[i], screen_y[i], EPS) ||
                !mul_eps(depth_res[i], depth[i], EPS)) {
#ifdef VERBOSE
            uart_puts("SCREEN OR DEPTH MISMATCH: ");
            uart_putd(i);
            uart_puts("\nQPU:\n");
            DEBUG_F(depth[i]);
            DEBUG_F(screen_x[i]);
            DEBUG_F(screen_y[i]);
            DEBUG_F(cov2d_inv_x[i]);
            DEBUG_F(cov2d_inv_y[i]);
            DEBUG_F(cov2d_inv_z[i]);

            uart_puts("\nCPU:\n");
            DEBUG_F(depth_res[i]);
            DEBUG_F(screen_x_res[i]);
            DEBUG_F(screen_y_res[i]);
            DEBUG_F(cov2d_inv_res[i].x);
            DEBUG_F(cov2d_inv_res[i].y);
            DEBUG_F(cov2d_inv_res[i].z);

            uart_puts("\n");
#endif

            depth_screen_mismatch++;
        }
        if (!mul_eps(cov2d_inv_res[i].x, cov2d_inv_x[i], EPS) ||
                !mul_eps(cov2d_inv_res[i].y, cov2d_inv_y[i], EPS) ||
                !mul_eps(cov2d_inv_res[i].z, cov2d_inv_z[i], EPS)) {
// #ifdef VERBOSE
            uart_puts("COV2D MISMATCH: ");
            uart_putd(i);
            uart_puts("\nQPU:\n");
            DEBUG_F(depth[i]);
            DEBUG_F(screen_x[i]);
            DEBUG_F(screen_y[i]);
            DEBUG_F(cov2d_inv_x[i]);
            DEBUG_F(cov2d_inv_y[i]);
            DEBUG_F(cov2d_inv_z[i]);

            uart_puts("\nCPU:\n");
            DEBUG_F(depth_res[i]);
            DEBUG_F(screen_x_res[i]);
            DEBUG_F(screen_y_res[i]);
            DEBUG_F(cov2d_inv_res[i].x);
            DEBUG_F(cov2d_inv_res[i].y);
            DEBUG_F(cov2d_inv_res[i].z);

            uart_puts("\n");

            {
                Vec3 p = { { pos_x[i], pos_y[i], pos_z[i] } };
                Mat3 cov3d_m;
                for (uint32_t j = 0; j < 9; j++) {
                    cov3d_m.m[j] = cov3d[j][i];
                }

                project_point(c, p, &depth_res[i], &screen_x_res[i], &screen_y_res[i]);

                // don't care about Gaussians not in frustum
                if (depth_res[i] < CAMERA_ZNEAR || depth_res[i] > CAMERA_ZFAR) {
                    panic("shouldn't be here");
                }
                Vec3 cov2d = project_cov2d(p, cov3d_m, c->w2c, c->fx, c->fy);

                cov2d.x += 0.3f;
                cov2d.z += 0.3f;

                DEBUG_F(cov2d.x);
                DEBUG_F(cov2d.y);
                DEBUG_F(cov2d.z);

                cov2d_inv_res[i] = compute_cov2d_inverse(cov2d);
            }

            uart_puts("\n\n");

            cov2d_mismatch++;
        }
        if (!mul_eps(radius_res[i], radius[i], EPS)) {
            uart_puts("RAD MISMATCH: ");
            uart_putd(i);

            uart_puts("\nQPU:\n");
            DEBUG_F(radius[i]);

            uart_puts("\nCPU:\n");
            DEBUG_F(radius_res[i]);

            rad_mismatch++;

        }
// #endif

    }

    // mismatches in cov2D_inv usually bc cov2D has minor fp error that's blown up by inverse
    DEBUG_D(depth_screen_mismatch);
    DEBUG_D(cov2d_mismatch);
    DEBUG_D(rad_mismatch);

    rpi_reset();
}
