#include "gpio.h"
#include "uart.h"
#include "lib.h"
#include "sys_timer.h"
#include "mailbox_interface.h"
#include "gaussian.h"
#include "math.h"
#include "debug.h"
#include "kernel.h"

#include "project_points_cov2d_inv.h"
#include "spherical_harmonics.h"

// #define WIDTH 800
// #define HEIGHT 600
#define WIDTH 160
#define HEIGHT 120

#define NUM_QPUS 12
#define SIMD_WIDTH 16

// #define RENDER

Arena data_arena;

uint32_t make_color(uint32_t r, uint32_t g, uint32_t b) {
    return (r << 16) | (g << 8) | b;
}

static uint8_t clamp_color(float c) {
    if (c < 0.0f) c = 0.0f;
    if (c > 1.0f) c = 1.0f;
    return (uint8_t)(c * 255.0f);
}

static uint32_t rand_state = 12345;

static uint32_t rand_u32(void) {
    rand_state = rand_state * 1103515245 + 12345;
    return rand_state;
}

static float rand_float(float min, float max) {
    return min + (float)(rand_u32() & 0xFFFF) / 65535.0f * (max - min);
}

static void sort_by_depth(ProjectedGaussianSoA* pg, uint32_t n) {
    for (uint32_t i = 0; i < n - 1; i++) {
        for (uint32_t j = 0; j < n - i - 1; j++) {
            if (pg->depth[j] < pg->depth[j + 1]) {
                ProjectedGaussian tmp = {
                    pg->screen_x[j],
                    pg->screen_y[j],
                    pg->depth[j],
                    pg->cov2d_inv[j],
                    pg->color[j],
                    pg->opacity[j],
                };

                pg->screen_x[j] = pg->screen_x[j + 1];
                pg->screen_y[j] = pg->screen_y[j + 1];
                pg->depth[j] = pg->depth[j + 1];
                pg->cov2d_inv[j] = pg->cov2d_inv[j + 1];
                pg->color[j] = pg->color[j + 1];
                pg->opacity[j] = pg->opacity[j + 1];

                pg->screen_x[j + 1] = tmp.screen_x;
                pg->screen_y[j + 1] = tmp.screen_y;
                pg->depth[j + 1] = tmp.depth;
                pg->cov2d_inv[j + 1] = tmp.cov2d_inv;
                pg->color[j + 1] = tmp.color;
                pg->opacity[j + 1] = tmp.opacity;
            }
        }
    }
}
static void sort_by_depth_k(ProjectedGaussianK* pg, uint32_t n) {
    for (uint32_t i = 0; i < n - 1; i++) {
        for (uint32_t j = 0; j < n - i - 1; j++) {
            if (pg->depth[j] < pg->depth[j + 1]) {
                ProjectedGaussian tmp = {
                    pg->screen_x[j],
                    pg->screen_y[j],
                    pg->depth[j],
                    { { pg->cov2d_inv_x[j], pg->cov2d_inv_y[j], pg->cov2d_inv_z[j] } },
                    { { pg->color_r[j], pg->color_g[j], pg->color_b[j] } },
                    pg->opacity[j],
                };

                pg->screen_x[j] = pg->screen_x[j + 1];
                pg->screen_y[j] = pg->screen_y[j + 1];
                pg->depth[j] = pg->depth[j + 1];
                pg->cov2d_inv_x[j] = pg->cov2d_inv_x[j + 1];
                pg->cov2d_inv_y[j] = pg->cov2d_inv_y[j + 1];
                pg->cov2d_inv_z[j] = pg->cov2d_inv_z[j + 1];
                pg->color_r[j] = pg->color_r[j + 1];
                pg->color_g[j] = pg->color_g[j + 1];
                pg->color_b[j] = pg->color_b[j + 1];
                pg->opacity[j] = pg->opacity[j + 1];

                pg->screen_x[j + 1] = tmp.screen_x;
                pg->screen_y[j + 1] = tmp.screen_y;
                pg->depth[j + 1] = tmp.depth;
                pg->cov2d_inv_x[j + 1] = tmp.cov2d_inv.x;
                pg->cov2d_inv_y[j + 1] = tmp.cov2d_inv.y;
                pg->cov2d_inv_z[j + 1] = tmp.cov2d_inv.z;
                pg->color_r[j + 1] = tmp.color.x;
                pg->color_g[j + 1] = tmp.color.y;
                pg->color_b[j + 1] = tmp.color.z;
                pg->opacity[j + 1] = tmp.opacity;
            }
        }
    }
}

void precompute_gaussians_SoA(Camera* c, GaussianSoA* g, ProjectedGaussianSoA* pg, uint32_t num_gaussians) {
    for (uint32_t i = 0; i < num_gaussians; i++) {
        project_point(c, g->pos[i], &pg->depth[i], &pg->screen_x[i], &pg->screen_y[i]);

        Mat3 cov3d = compute_cov3d(g->scale[i], g->rot[i]);
        Vec3 cov2d = project_cov2d(g->pos[i], cov3d, c->w2c, c->fx, c->fy);

        cov2d.x += 0.3f;
        cov2d.z += 0.3f;

        pg->cov2d_inv[i] = compute_cov2d_inverse(cov2d);
        pg->color[i] = eval_sh(g->pos[i], g->sh[i], c->pos);
    }
}

void precompute_gaussians_qpu(Camera* c, GaussianK* g, ProjectedGaussianK* pg, uint32_t num_gaussians) {
    //////// PROJECT POINTS + COV2D INV

    Kernel project_points_k;
    const int project_points_num_unifs = 34;

    kernel_init(&project_points_k,
            NUM_QPUS, project_points_num_unifs,
            project_points_cov2d_inv, sizeof(project_points_cov2d_inv));

    float* cov3d[9];
    for (uint32_t i = 0; i < 9; i++) {
        cov3d[i] = arena_alloc_align(&data_arena, num_gaussians * sizeof(float), 16);
    }

    for (uint32_t i = 0; i < num_gaussians; i++) {
        Vec3 g_scale = { { g->scale_x[i], g->scale_y[i], g->scale_z[i] } };
        Vec4 g_rot = { { g->rot_x[i], g->rot_y[i], g->rot_z[i], g->rot_w[i] } };
        Mat3 cov3d_m = compute_cov3d(g_scale, g_rot);

        for (uint32_t j = 0; j < 9; j++) {
            cov3d[j][i] = cov3d_m.m[j];
        }
    }

    for (uint32_t q = 0; q < NUM_QPUS; q++) {
        kernel_load_unif(&project_points_k, q, NUM_QPUS * SIMD_WIDTH);
        kernel_load_unif(&project_points_k, q, num_gaussians);
        kernel_load_unif(&project_points_k, q, q);
        kernel_load_unif(&project_points_k, q, TO_BUS(g->pos_x + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(g->pos_y + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(g->pos_z + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(pg->depth + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(pg->screen_x + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(pg->screen_y + q * SIMD_WIDTH));

        for (uint32_t i = 0; i < 12; i++) {
            kernel_load_unif(&project_points_k, q, c->w2c.m[i]);
        }
        kernel_load_unif(&project_points_k, q, c->fx);
        kernel_load_unif(&project_points_k, q, c->cx);
        kernel_load_unif(&project_points_k, q, c->fy);
        kernel_load_unif(&project_points_k, q, c->cy);

        kernel_load_unif(&project_points_k, q, TO_BUS(cov3d[0] + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(cov3d[1] + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(cov3d[2] + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(cov3d[4] + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(cov3d[5] + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(cov3d[8] + q * SIMD_WIDTH));

        kernel_load_unif(&project_points_k, q, TO_BUS(pg->cov2d_inv_x + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(pg->cov2d_inv_y + q * SIMD_WIDTH));
        kernel_load_unif(&project_points_k, q, TO_BUS(pg->cov2d_inv_z + q * SIMD_WIDTH));
    }

    uint32_t t;

    t = sys_timer_get_usec();
    kernel_execute(&project_points_k);
    uint32_t project_points_kernel_t  = sys_timer_get_usec() - t;
    DEBUG_D(project_points_kernel_t);

    ////////// SPHERICAL HARMONICS
    
    Kernel sh_k;
    const int sh_num_unifs = 60;
    kernel_init(&sh_k, NUM_QPUS, sh_num_unifs,
            spherical_harmonics, sizeof(spherical_harmonics));

    uint32_t sh[3] = {
        (uint32_t) (&g->sh_x),
        (uint32_t) (&g->sh_y),
        (uint32_t) (&g->sh_z)
    };
    float* colors[3] = {
        (float*) (&pg->color_r),
        (float*) (&pg->color_g),
        (float*) (&pg->color_b)
    };
    for (uint32_t q = 0; q < NUM_QPUS; q++) {
        kernel_load_unif(&sh_k, q, NUM_QPUS * SIMD_WIDTH);
        kernel_load_unif(&sh_k, q, num_gaussians);
        kernel_load_unif(&sh_k, q, q);

        kernel_load_unif(&sh_k, q, TO_BUS(g->pos_x + q * SIMD_WIDTH));
        kernel_load_unif(&sh_k, q, TO_BUS(g->pos_y + q * SIMD_WIDTH));
        kernel_load_unif(&sh_k, q, TO_BUS(g->pos_z + q * SIMD_WIDTH));

        kernel_load_unif(&sh_k, q, c->pos.x);
        kernel_load_unif(&sh_k, q, c->pos.y);
        kernel_load_unif(&sh_k, q, c->pos.z);

        for (uint32_t c = 0; c < 3; c++) {
            kernel_load_unif(&sh_k, q, TO_BUS(colors[c] + q * SIMD_WIDTH));
            for (uint32_t i = 0; i < 16; i++) {
                kernel_load_unif(&sh_k, q,
                    TO_BUS(sh[c] +
                        i * (num_gaussians * sizeof(float)) + // &sh[c][i]
                        (q * SIMD_WIDTH * sizeof(float)))); // &sh[c][i][q]
            }
        }
    }

    t = sys_timer_get_usec();
    kernel_execute(&sh_k);
    uint32_t sh_kernel_t = sys_timer_get_usec() - t;
    DEBUG_D(sh_kernel_t);
}

void main() {
    const int num_gaussians = NUM_QPUS * SIMD_WIDTH * 1;
    const int MiB = 1024 * 1024;
    arena_init(&data_arena, MiB * 30);

    uint32_t t;

    Vec3 cam_pos = { { 0.0f, 0.0f, 5.0f } };
    Vec3 cam_target = { { 0.0f, 0.0f, 0.0f } };
    Vec3 cam_up = { { 0.0f, 1.0f, 0.0f } };

    Camera* c = arena_alloc_align(&data_arena, sizeof(Camera), 16);
    init_camera(c, cam_pos, cam_target, cam_up, WIDTH, HEIGHT);

    GaussianK* g = arena_alloc_align(&data_arena, sizeof(GaussianK), 16);
    ProjectedGaussianK* pg = arena_alloc_align(&data_arena, sizeof(ProjectedGaussianK), 16);

    GaussianSoA g_soa;
    ProjectedGaussianSoA pg_soa;

    for (int i = 0; i < num_gaussians; i++) {
        g_soa.pos[i] = (Vec3) { {
            rand_float(-2.0f, 2.0f),
            rand_float(-1.5f, 1.5f),
            rand_float(-2.0f, 2.0f)
        } };

        g->pos_x[i] = g_soa.pos[i].x;
        g->pos_y[i] = g_soa.pos[i].y;
        g->pos_z[i] = g_soa.pos[i].z;
        
        float scale = rand_float(-2.0f, -0.5f);
        g_soa.scale[i] = (Vec3) { { scale, scale, scale } };

        g->scale_x[i] = g_soa.scale[i].x;
        g->scale_y[i] = g_soa.scale[i].y;
        g->scale_z[i] = g_soa.scale[i].z;
        
        g_soa.rot[i] = (Vec4) { { 0.0f, 0.0f, 0.0f, 1.0f } };

        g->rot_x[i] = g_soa.rot[i].x;
        g->rot_y[i] = g_soa.rot[i].y;
        g->rot_z[i] = g_soa.rot[i].z;
        g->rot_w[i] = g_soa.rot[i].w;

        pg_soa.opacity[i] = rand_float(0.5f, 1.0f); // load opacity directly to projected Gaussian

        pg->opacity[i] = pg_soa.opacity[i];
        
        for (int j = 0; j < 16; j++) {
            g_soa.sh[i][j] = (Vec3) { { 0.0f, 0.0f, 0.0f } };

            g->sh_x[j][i] = g_soa.sh[i][j].x;
            g->sh_y[j][i] = g_soa.sh[i][j].y;
            g->sh_z[j][i] = g_soa.sh[i][j].z;
        }
        
        float r = rand_float(0.2f, 1.0f);
        float gr = rand_float(0.2f, 1.0f);
        float b = rand_float(0.2f, 1.0f);
        g_soa.sh[i][0] = (Vec3) { {
            (r - 0.5f) / SH_C0,
            (gr - 0.5f) / SH_C0,
            (b - 0.5f) / SH_C0
        } };

        g->sh_x[0][i] = g_soa.sh[i][0].x;
        g->sh_y[0][i] = g_soa.sh[i][0].y;
        g->sh_z[0][i] = g_soa.sh[i][0].z;
    }

    DEBUG_D(num_gaussians);

    uart_puts("DONE LOADING GAUSSIANS\n");

    t = sys_timer_get_usec();
    precompute_gaussians_SoA(c, &g_soa, &pg_soa, num_gaussians);
    uint32_t cpu_t = sys_timer_get_usec() - t;
    DEBUG_D(cpu_t);

    t = sys_timer_get_usec();
    precompute_gaussians_qpu(c, g, pg, num_gaussians);
    uint32_t qpu_t = sys_timer_get_usec() - t;
    DEBUG_D(qpu_t);


#ifdef RENDER
    /*
    for (uint32_t i = 0; i < 16; i++) {
        uart_putd(i);
        uart_puts("\n");
        DEBUG_F(pg_soa.screen_x[i]);
        DEBUG_F(pg_soa.screen_y[i]);
        DEBUG_F(pg_soa.depth[i]);
        DEBUG_F(pg_soa.cov2d_inv[i].x);
        DEBUG_F(pg_soa.cov2d_inv[i].y);
        DEBUG_F(pg_soa.cov2d_inv[i].z);

        uart_puts("--\n");

        DEBUG_F(pg->screen_x[i]);
        DEBUG_F(pg->screen_y[i]);
        DEBUG_F(pg->depth[i]);
        DEBUG_F(pg->cov2d_inv_x[i]);
        DEBUG_F(pg->cov2d_inv_y[i]);
        DEBUG_F(pg->cov2d_inv_z[i]);

        uart_puts("------\n");
    }
    */

    uart_puts("SORTING...\n");

    sort_by_depth(&pg_soa, num_gaussians);
    sort_by_depth_k(pg, num_gaussians);

    uart_puts("RENDERING\n");

    uart_puts("P3\n");
    uart_putd(WIDTH);
    uart_puts(" ");
    uart_putd(HEIGHT);
    uart_puts("\n255\n");

    // /*
    // CPU RENDER
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;

            float out_r = 20.0f / 255.0f;
            float out_g = 20.0f / 255.0f;
            float out_b = 40.0f / 255.0f;

            for (uint32_t i = 0; i < num_gaussians; i++) {
                if (pg_soa.depth[i] <= 0.0f) continue;

                float alpha = pg_soa.opacity[i] * eval_gaussian_2d(px, py, pg_soa.screen_x[i], pg_soa.screen_y[i], pg_soa.cov2d_inv[i]);
                if (alpha < 0.001f) continue;

                out_r = alpha * pg_soa.color[i].x + (1.0f - alpha) * out_r;
                out_g = alpha * pg_soa.color[i].y + (1.0f - alpha) * out_g;
                out_b = alpha * pg_soa.color[i].z + (1.0f - alpha) * out_b;
            }

            uart_putd(clamp_color(out_r));
            uart_puts(" ");
            uart_putd(clamp_color(out_g));
            uart_puts(" ");
            uart_putd(clamp_color(out_b));
            uart_puts("\n");
        }
    }
    // */
    /*
    // QPU RENDER
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;

            float out_r = 20.0f / 255.0f;
            float out_g = 20.0f / 255.0f;
            float out_b = 40.0f / 255.0f;

            for (uint32_t i = 0; i < num_gaussians; i++) {
                if (pg->depth[i] <= 0.0f) continue;

                Vec3 cov2d_inv = { { pg->cov2d_inv_x[i], pg->cov2d_inv_y[i], pg->cov2d_inv_z[i] } };
                float alpha = pg->opacity[i] * eval_gaussian_2d(px, py, pg->screen_x[i], pg->screen_y[i], cov2d_inv);
                if (alpha < 0.001f) continue;

                out_r = alpha * pg->color_r[i] + (1.0f - alpha) * out_r;
                out_g = alpha * pg->color_g[i] + (1.0f - alpha) * out_g;
                out_b = alpha * pg->color_b[i] + (1.0f - alpha) * out_b;
            }

            uart_putd(clamp_color(out_r));
            uart_puts(" ");
            uart_putd(clamp_color(out_g));
            uart_puts(" ");
            uart_putd(clamp_color(out_b));
            uart_puts("\n");
        }
    }
    */
#endif

    rpi_reset();
}
