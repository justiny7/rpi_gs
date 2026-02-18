#include "gpio.h"
#include "uart.h"
#include "lib.h"
#include "sys_timer.h"
#include "mailbox_interface.h"
#include "gaussian.h"

#include <math.h>

#define WIDTH 80
#define HEIGHT 60
#define NUM_GAUSSIANS 2

#define USE_PPM_OUTPUT 1

uint32_t make_color(uint32_t r, uint32_t g, uint32_t b) {
    return (r << 16) | (g << 8) | b;
}

static uint8_t clamp_color(float c) {
    if (c < 0.0f) c = 0.0f;
    if (c > 1.0f) c = 1.0f;
    return (uint8_t)(c * 255.0f);
}

void main() {
    Vec3 cam_pos = { { 0.0f, 0.0f, 4.0f } };
    Vec3 cam_target = { { 0.0f, 0.0f, 0.0f } };
    Vec3 cam_up = { { 0.0f, 1.0f, 0.0f } };

    Camera c;
    init_camera(&c, cam_pos, cam_target, cam_up, WIDTH, HEIGHT);

    Gaussian g[NUM_GAUSSIANS];
    g[1].pos = (Vec3) { { -0.5f, 0.0f, 0.5f } };
    g[1].scale = (Vec3) { { -1.5f, -1.5f, -1.5f } };
    g[1].rot = (Vec4) { { 0.0f, 0.0f, 0.0f, 1.0f } };
    g[1].opacity = 0.9f;
    for (int i = 0; i < 16; i++) g[1].sh[i] = (Vec3) { { 0.0f, 0.0f, 0.0f } };
    g[1].sh[0] =  (Vec3) { {  // Magenta
        (0.9f - 0.5f) / SH_C0,
        (0.2f - 0.5f) / SH_C0,
        (0.9f - 0.5f) / SH_C0
    } };
    g[0].pos = (Vec3) { { 0.5f, 0.0f, -0.5f } };
    g[0].scale = (Vec3) { { -0.5f, -0.5f, -0.5f } };
    g[0].rot = (Vec4) { { 0.0f, 0.0f, 0.0f, 1.0f } };
    g[0].opacity = 0.9f;
    for (int i = 0; i < 16; i++) g[0].sh[i] = (Vec3) { { 0.0f, 0.0f, 0.0f } };
    g[0].sh[0] =  (Vec3) { {  // Cyan
        (0.2f - 0.5f) / SH_C0,
        (0.9f - 0.5f) / SH_C0,
        (0.9f - 0.5f) / SH_C0
    } };

    ProjectedGaussian pg[NUM_GAUSSIANS];
    precompute_gaussians(&c, g, pg, NUM_GAUSSIANS);

#if USE_PPM_OUTPUT
    // Output PPM header
    uart_puts("P3\n");
    uart_putd(WIDTH);
    uart_puts(" ");
    uart_putd(HEIGHT);
    uart_puts("\n255\n");

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;

            // Start with background color
            float out_r = 20.0f / 255.0f;
            float out_g = 20.0f / 255.0f;
            float out_b = 40.0f / 255.0f;

            for (uint32_t i = 0; i < NUM_GAUSSIANS; i++) {
                ProjectedGaussian* p = &pg[i];
                if (p->depth <= 0.0f) continue;

                float alpha = p->opacity * eval_gaussian_2d(px, py, p->screen_x, p->screen_y, p->cov2d_inv);
                if (alpha < 0.001f) continue;

                out_r = alpha * p->color.x + (1.0f - alpha) * out_r;
                out_g = alpha * p->color.y + (1.0f - alpha) * out_g;
                out_b = alpha * p->color.z + (1.0f - alpha) * out_b;
            }

            uart_putd(clamp_color(out_r));
            uart_puts(" ");
            uart_putd(clamp_color(out_g));
            uart_puts(" ");
            uart_putd(clamp_color(out_b));
            uart_puts("\n");
        }
    }
#else
    // FRAMEBUFFER
    uint32_t *fb;
    uint32_t size, width, height, pitch;

    mbox_framebuffer_set_physical_width_height(WIDTH, HEIGHT);
    mbox_framebuffer_set_virtual_width_height(WIDTH, HEIGHT);
    mbox_framebuffer_set_pixel_order(PIXEL_ORDER_BGR);
    mbox_framebuffer_set_depth(32);

    mbox_allocate_framebuffer(16, &fb, &size);

    pitch = mbox_framebuffer_get_pitch();
    mbox_framebuffer_get_physical_width_height(&width, &height);

    assert(width == WIDTH && height == HEIGHT, "Width and height mismatch");

    uint32_t* pixels = (uint32_t*)((uintptr_t)fb & 0x3FFFFFFF); 
    for (uint32_t i = 0; i < height * width; i++) {
        pixels[i] = make_color(20, 20, 40);
    }

    for (uint32_t i = 0; i < NUM_GAUSSIANS; i++) {
        ProjectedGaussian* p = &pg[i];

        if (p->depth <= 0.0f) {
            continue;
        }

        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                float px = (float)x + 0.5f;
                float py = (float)y + 0.5f;

                float alpha = p->opacity * eval_gaussian_2d(px, py, p->screen_x, p->screen_y, p->cov2d_inv);

                if (alpha < 0.001f) continue;

                int idx = (y * WIDTH + x);
                float bg_r = (float)((pixels[idx] >> 16) & 0xFF) / 255.0f;
                float bg_g = (float)((pixels[idx] >> 8) & 0xFF) / 255.0f;
                float bg_b = (float)(pixels[idx] & 0xFF) / 255.0f;

                float out_r = alpha * p->color.x + (1.0f - alpha) * bg_r;
                float out_g = alpha * p->color.y + (1.0f - alpha) * bg_g;
                float out_b = alpha * p->color.z + (1.0f - alpha) * bg_b;

                pixels[idx] = make_color((uint8_t)(fminf(fmaxf(out_r, 0.0f), 1.0f) * 255.0f),
                        (uint8_t)(fminf(fmaxf(out_g, 0.0f), 1.0f) * 255.0f),
                        (uint8_t)(fminf(fmaxf(out_b, 0.0f), 1.0f) * 255.0f));
            }
        }
    }
#endif

    rpi_reset();
    // while (1);
}
