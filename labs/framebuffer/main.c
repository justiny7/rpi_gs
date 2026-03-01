#include "mailbox_interface.h"
#include "uart.h"
#include "lib.h"
#include "debug.h"

#define HEIGHT 1200
#define WIDTH 1200
#define K 8

void main() {
    uint32_t *fb;
    uint32_t size, width, height, pitch;

    mbox_framebuffer_init(WIDTH, HEIGHT, 32, &fb, &size, &pitch);
    DEBUG_D(mbox_framebuffer_get_pixel_order());
    DEBUG_D(mbox_framebuffer_get_alpha_mode());
    

    pitch = mbox_framebuffer_get_pitch();
    mbox_framebuffer_get_physical_width_height(&width, &height);

    assert(width == WIDTH && height == HEIGHT, "Width and height mismatch");
    uart_puts("buffer ptr: ");
    uart_putx((uint32_t) fb);
    uart_puts("\n");

    uart_puts("pitch: ");
    uart_putd(pitch);
    uart_puts("\n");

    uint32_t* pixel_ptr = (uint32_t*)((uintptr_t)fb & 0x3FFFFFFF); 
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            int idx = i * width + j;
            if ((i * K / WIDTH + j * K / WIDTH) & 1) {
                pixel_ptr[idx] = 0xFFBD4BE3; // QEMU ignored alpha channels, even when you turn on
            } else {
                pixel_ptr[idx] = 0xFFFFFFFF;
            }

            if (i + j % 100 == 0) {
                uart_putx(pixel_ptr[idx]);
                uart_puts("\n");
            }
        }
    }

    while(1); // when testing with QEMU
    // rpi_reset(); // when testing with pi
}
