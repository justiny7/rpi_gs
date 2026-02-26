#include "gpio.h"
#include "uart.h"
#include "lib.h"
#include "sys_timer.h"
#include "mailbox_interface.h"
#include "gaussian.h"
#include "math.h"
#include "debug.h"
#include "kernel.h"

#define WIDTH 800
#define HEIGHT 600

#define NUM_QPUS 12

// #define USE_PPM_OUTPUT 1

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

void main() {
    uint32_t t;

    Vec3 cam_pos = { { 0.0f, 0.0f, 5.0f } };
    Vec3 cam_target = { { 0.0f, 0.0f, 0.0f } };
    Vec3 cam_up = { { 0.0f, 1.0f, 0.0f } };

    Camera c;
    init_camera(&c, cam_pos, cam_target, cam_up, WIDTH, HEIGHT);

    GaussianSoA g;
    ProjectedGaussianSoA pg;
    for (int i = 0; i < MAX_GAUSSIANS; i++) {
        g.pos[i] = (Vec3) { {
            rand_float(-2.0f, 2.0f),
            rand_float(-1.5f, 1.5f),
            rand_float(-2.0f, 2.0f)
        } };
        
        float scale = rand_float(-2.0f, -0.5f);
        g.scale[i] = (Vec3) { { scale, scale, scale } };
        
        g.rot[i] = (Vec4) { { 0.0f, 0.0f, 0.0f, 1.0f } };
        pg.opacity[i] = rand_float(0.5f, 1.0f); // load opacity directly to pg
        
        for (int j = 0; j < 16; j++) {
            g.sh[i][j] = (Vec3) { { 0.0f, 0.0f, 0.0f } };
        }
        
        float r = rand_float(0.2f, 1.0f);
        float gr = rand_float(0.2f, 1.0f);
        float b = rand_float(0.2f, 1.0f);
        g.sh[i][0] = (Vec3) { {
            (r - 0.5f) / SH_C0,
            (gr - 0.5f) / SH_C0,
            (b - 0.5f) / SH_C0
        } };
    }

    t = sys_timer_get_usec();
    precompute_gaussians_SoA(&c, &g, &pg, MAX_GAUSSIANS);
    uint32_t precompute_t = sys_timer_get_usec() - t;
    DEBUG_D(precompute_t);

    sort_by_depth(&pg, MAX_GAUSSIANS);

#if USE_PPM_OUTPUT
    uart_puts("P3\n");
    uart_putd(WIDTH);
    uart_puts(" ");
    uart_putd(HEIGHT);
    uart_puts("\n255\n");

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;

            float out_r = 20.0f / 255.0f;
            float out_g = 20.0f / 255.0f;
            float out_b = 40.0f / 255.0f;

            for (uint32_t i = 0; i < MAX_GAUSSIANS; i++) {
                if (pg.depth[i] <= 0.0f) continue;

                float alpha = pg.opacity[i] * eval_gaussian_2d(px, py, pg.screen_x[i], pg.screen_y[i], pg.cov2d_inv[i]);
                if (alpha < 0.001f) continue;

                out_r = alpha * pg.color[i].x + (1.0f - alpha) * out_r;
                out_g = alpha * pg.color[i].y + (1.0f - alpha) * out_g;
                out_b = alpha * pg.color[i].z + (1.0f - alpha) * out_b;
            }

            uart_putd(clamp_color(out_r));
            uart_puts(" ");
            uart_putd(clamp_color(out_g));
            uart_puts(" ");
            uart_putd(clamp_color(out_b));
            uart_puts("\n");
        }
    }

    rpi_reset();
#else
    uint32_t *fb;
    uint32_t size, pitch;

    uart_puts("Initializing framebuffer...\n");
    mbox_framebuffer_init(WIDTH, HEIGHT, 32, &fb, &size, &pitch);

    DEBUG_X(fb);
    DEBUG_D(size);
    DEBUG_D(pitch);

    if (pitch != WIDTH * 4) {
        uart_puts("[ERROR] Framebuffer init failed!\n");
        while(1);
    }

    uint32_t* pixels = (uint32_t*)((uintptr_t)fb & 0x3FFFFFFF); 
    for (uint32_t i = 0; i < HEIGHT * WIDTH; i++) {
        pixels[i] = make_color(20, 20, 40);
    }

    t = sys_timer_get_usec();
    for (uint32_t i = 0; i < MAX_GAUSSIANS; i++) {
        if (pg.depth[i] <= 0.0f) continue;

        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                float px = (float)x + 0.5f;
                float py = (float)y + 0.5f;

                float alpha = pg.opacity[i] * eval_gaussian_2d(px, py, pg.screen_x[i], pg.screen_y[i], pg.cov2d_inv[i]);
                if (alpha < 0.001f) continue;

                int idx = (y * WIDTH + x);
                float bg_r = (float)((pixels[idx] >> 16) & 0xFF) / 255.0f;
                float bg_g = (float)((pixels[idx] >> 8) & 0xFF) / 255.0f;
                float bg_b = (float)(pixels[idx] & 0xFF) / 255.0f;

                float out_r = alpha * pg.color[i].x + (1.0f - alpha) * bg_r;
                float out_g = alpha * pg.color[i].y + (1.0f - alpha) * bg_g;
                float out_b = alpha * pg.color[i].z + (1.0f - alpha) * bg_b;

                pixels[idx] = make_color((uint8_t)(min(max(out_r, 0.0f), 1.0f) * 255.0f),
                        (uint8_t)(min(max(out_g, 0.0f), 1.0f) * 255.0f),
                        (uint8_t)(min(max(out_b, 0.0f), 1.0f) * 255.0f));
            }
        }
    }
    uint32_t render_t = sys_timer_get_usec() - t;
    DEBUG_D(render_t);

    while (1);
#endif
}
