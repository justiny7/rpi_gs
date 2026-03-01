#include "gpio.h"
#include "uart.h"
#include "lib.h"
#include "sys_timer.h"
#include "mailbox_interface.h"
#include "gaussian.h"
#include "math.h"
#include "debug.h"

#define WIDTH 640
#define HEIGHT 480
// #define WIDTH 128
// #define HEIGHT 96
#define TILE_SIZE 16
#define NUM_GAUSSIANS 50
#define NUM_TILES (WIDTH * HEIGHT / (TILE_SIZE * TILE_SIZE))

// #define USE_PPM_OUTPUT

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

// is a < b?
int cmp_less_pg(ProjectedGaussian* a, ProjectedGaussian* b) {
    if (a->tile != b->tile) {
        return a->tile < b->tile;
    }
    return a->depth > b->depth;
}

void merge(ProjectedGaussian* arr, int l, int m, int h) {
    int left_size = m - l + 1;
    int right_size = h - m;

    ProjectedGaussian left_arr[left_size], right_arr[right_size];
    
    int i, j, k;

    for (i = 0; i < left_size; i++)
        left_arr[i] = arr[l + i];
    for (j = 0; j < right_size; j++)
        right_arr[j] = arr[m + 1 + j];

    i = 0;
    j = 0;
    k = l;

    while (i < left_size && j < right_size) {
        if (cmp_less_pg(&left_arr[i], &right_arr[j])) {
            arr[k++] = left_arr[i++];
        } else {
            arr[k++] = right_arr[j++];
        }
    }

    while (i < left_size) {
        arr[k++] = left_arr[i++];
    }

    while (j < right_size) {
        arr[k++] = right_arr[j++];
    }
}

void sort_gaussians(ProjectedGaussian* arr, int l, int h) {
    if (l >= h) {
        return;
    }

    int m = l + (h - l) / 2;

    sort_gaussians(arr, l, m);
    sort_gaussians(arr, m + 1, h);

    merge(arr, l, m, h);
}


int cnt = 0;
void count_tile_intersections(ProjectedGaussian* pg, int n,
        uint32_t* tiles_touched, uint32_t* gaussians_touched) {
    for (int i = 0; i < n; i++) {
        float rad = pg[i].radius;
        int32_t l = max(0, (int32_t) ((pg[i].screen_x - rad) / TILE_SIZE));
        int32_t r = min(WIDTH / TILE_SIZE - 1, (int32_t) ((pg[i].screen_x + rad) / TILE_SIZE));
        int32_t t = max(0, (int32_t) ((pg[i].screen_y - rad) / TILE_SIZE));
        int32_t b = min(HEIGHT / TILE_SIZE - 1, (int32_t) ((pg[i].screen_y + rad) / TILE_SIZE));

        assert(r >= l, "r < l");
        assert(b >= t, "b < t");
        assert(l >= 0, "l is neg");
        assert(t >= 0, "t is neg");

        tiles_touched[i + 1] = (r - l + 1) * (b - t + 1);
        for (int j = l; j <= r; j++) {
            for (int k = t; k <= b; k++) {
                int idx = j * (HEIGHT / TILE_SIZE) + k;
                if (idx >= NUM_TILES) {
                    DEBUG_D(WIDTH / TILE_SIZE);
                    DEBUG_D(j);
                    DEBUG_D(HEIGHT / TILE_SIZE);
                    DEBUG_D(k);
                    DEBUG_D(idx);
                    panic("noo");
                }
                gaussians_touched[j * (HEIGHT / TILE_SIZE) + k + 1]++;
                cnt++;
            }
        }

        tiles_touched[i + 1] += tiles_touched[i];

        pg[i].tile = 0;
    }

    for (int i = 0; i < NUM_TILES; i++) {
        gaussians_touched[i + 1] += gaussians_touched[i];
    }

    DEBUG_D(gaussians_touched[NUM_TILES + 1]);
}
void fill_all_gaussians(ProjectedGaussian* pg, int n,
        ProjectedGaussian* all_gaussians, 
        uint32_t* tiles_touched) {
    for (int i = 0; i < n; i++) {
        float rad = pg[i].radius;
        int32_t l = max(0, (int32_t) ((pg[i].screen_x - rad) / TILE_SIZE));
        int32_t r = min(WIDTH / TILE_SIZE - 1, (int32_t) ((pg[i].screen_x + rad) / TILE_SIZE));
        int32_t t = max(0, (int32_t) ((pg[i].screen_y - rad) / TILE_SIZE));
        int32_t b = min(HEIGHT / TILE_SIZE - 1, (int32_t) ((pg[i].screen_y + rad) / TILE_SIZE));

        for (int j = l; j <= r; j++) {
            for (int k = t; k <= b; k++) {
                int tile_idx = (j - l) * (b - t + 1) + (k - t);

                int idx = tiles_touched[i] + tile_idx;
                all_gaussians[idx] = pg[i];
                all_gaussians[idx].tile = j * (HEIGHT / TILE_SIZE) + k;
            }
        }
    }
}

void main() {
    Vec3 cam_pos = { { 0.0f, 0.0f, 5.0f } };
    Vec3 cam_target = { { 0.0f, 0.0f, 0.0f } };
    Vec3 cam_up = { { 0.0f, 1.0f, 0.0f } };

    Camera c;
    init_camera(&c, cam_pos, cam_target, cam_up, WIDTH, HEIGHT);

    Gaussian g[NUM_GAUSSIANS];
    for (int i = 0; i < NUM_GAUSSIANS; i++) {
        g[i].pos = (Vec3) { {
            rand_float(-2.0f, 2.0f),
            rand_float(-1.5f, 1.5f),
            rand_float(-2.0f, 2.0f)
        } };
        
        float scale = rand_float(-2.0f, -0.5f);
        g[i].scale = (Vec3) { { scale, scale, scale } };
        
        g[i].rot = (Vec4) { { 0.0f, 0.0f, 0.0f, 1.0f } };
        g[i].opacity = rand_float(0.5f, 1.0f);
        
        for (int j = 0; j < 16; j++) {
            g[i].sh[j] = (Vec3) { { 0.0f, 0.0f, 0.0f } };
        }
        
        float r = rand_float(0.2f, 1.0f);
        float gr = rand_float(0.2f, 1.0f);
        float b = rand_float(0.2f, 1.0f);
        g[i].sh[0] = (Vec3) { {
            (r - 0.5f) / SH_C0,
            (gr - 0.5f) / SH_C0,
            (b - 0.5f) / SH_C0
        } };
    }

    ProjectedGaussian pg[NUM_GAUSSIANS];
    uint32_t tiles_touched[NUM_GAUSSIANS + 1];
    uint32_t gaussians_touched[NUM_TILES + 1];
    memset(tiles_touched, 0, sizeof(tiles_touched));
    memset(gaussians_touched, 0, sizeof(gaussians_touched));

    uart_puts("precompute\n");
    precompute_gaussians(&c, g, pg, NUM_GAUSSIANS);
    uart_puts("intersections\n");
    count_tile_intersections(pg, NUM_GAUSSIANS, tiles_touched, gaussians_touched);

    DEBUG_D(tiles_touched[NUM_GAUSSIANS]);
    DEBUG_D(gaussians_touched[NUM_TILES]);
    DEBUG_D(cnt);
    assert(tiles_touched[NUM_GAUSSIANS] == gaussians_touched[NUM_TILES], "touched mismatch");

    uint32_t total_intersections = tiles_touched[NUM_GAUSSIANS];
    assert(total_intersections < MAX_GAUSSIANS, "too many intersections");
    ProjectedGaussian all_gaussians[MAX_GAUSSIANS];
    fill_all_gaussians(pg, NUM_GAUSSIANS, all_gaussians, tiles_touched);

    sort_gaussians(all_gaussians, 0, total_intersections - 1);
    sort_gaussians(pg, 0, NUM_GAUSSIANS - 1);

    // rpi_reset();

#if USE_PPM_OUTPUT
    uart_puts("P3\n");
    uart_putd(WIDTH);
    uart_puts(" ");
    uart_putd(HEIGHT);
    uart_puts("\n255\n");

    // /*
    for (uint32_t y = 0; y < HEIGHT; y++) {
        for (uint32_t x = 0; x < WIDTH; x++) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;

            float out_r = 20.0f / 255.0f;
            float out_g = 20.0f / 255.0f;
            float out_b = 40.0f / 255.0f;

            uint32_t tile_idx = (x / TILE_SIZE) * (HEIGHT / TILE_SIZE) + (y / TILE_SIZE);
            for (uint32_t i = gaussians_touched[tile_idx]; i < gaussians_touched[tile_idx + 1]; i++) {
                ProjectedGaussian* p = &all_gaussians[i];
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
    // */
    /*
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;

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
    */


    rpi_reset();
#else
    uint32_t *fb;
    uint32_t size, pitch;

    uart_puts("Initializing framebuffer...\n");
    mbox_framebuffer_init(WIDTH, HEIGHT, 32, &fb, &size, &pitch);

    uart_puts("buffer ptr: "); uart_putx((uint32_t)fb); uart_puts("\n");
    uart_puts("size: "); uart_putd(size); uart_puts("\n");
    uart_puts("pitch: "); uart_putd(pitch); uart_puts(" (expected "); uart_putd(WIDTH * 4); uart_puts(")\n");

    if (pitch != WIDTH * 4) {
        uart_puts("[ERROR] Framebuffer init failed!\n");
        while(1);
    }

    uint32_t* pixels = (uint32_t*)((uintptr_t)fb & 0x3FFFFFFF); 
    /*
    for (uint32_t i = 0; i < HEIGHT * WIDTH; i++) {
        pixels[i] = make_color(20, 20, 40);
    }
    */

    /*
    for (uint32_t i = 0; i < NUM_GAUSSIANS; i++) {
        ProjectedGaussian* p = &pg[i];
        if (p->depth <= 0.0f) continue;

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

                pixels[idx] = make_color((uint8_t)(min(max(out_r, 0.0f), 1.0f) * 255.0f),
                        (uint8_t)(min(max(out_g, 0.0f), 1.0f) * 255.0f),
                        (uint8_t)(min(max(out_b, 0.0f), 1.0f) * 255.0f));
            }
        }
    }
    */
    for (uint32_t y = 0; y < HEIGHT; y++) {
        for (uint32_t x = 0; x < WIDTH; x++) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;

            // float out_r = 20.0f / 255.0f;
            // float out_g = 20.0f / 255.0f;
            // float out_b = 40.0f / 255.0f;
            float out_r = 0.0f;
            float out_g = 0.0f;
            float out_b = 0.0f;

            uint32_t tile_idx = (x / TILE_SIZE) * (HEIGHT / TILE_SIZE) + (y / TILE_SIZE);
            for (uint32_t i = gaussians_touched[tile_idx]; i < gaussians_touched[tile_idx + 1]; i++) {
                ProjectedGaussian* p = &all_gaussians[i];
                if (p->depth <= 0.0f) continue;

                float alpha = p->opacity * eval_gaussian_2d(px, py, p->screen_x, p->screen_y, p->cov2d_inv);
                if (alpha < 0.001f) continue;

                out_r = alpha * p->color.x + (1.0f - alpha) * out_r;
                out_g = alpha * p->color.y + (1.0f - alpha) * out_g;
                out_b = alpha * p->color.z + (1.0f - alpha) * out_b;
            }

            pixels[y * WIDTH + x] = make_color(clamp_color(out_r), clamp_color(out_g), clamp_color(out_b));
            if (y == 0 && x == 0) {
                DEBUG_X(pixels[0]);
            }
        }
    }

    while (1);
#endif
}
