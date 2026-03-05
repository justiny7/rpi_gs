#include "heap_allocator.h"
#include "mailbox_interface.h"
#include "lib.h"

#include <stdalign.h>
#include <stddef.h>

volatile uint8_t* heap_buf;
uint32_t heap_size;
uint32_t heap_capacity;

extern uint32_t __heap_start__;

// align must be power of 2
static void* align_ptr(void* ptr, uint32_t align) {
    uint32_t ptr_val = (uint32_t) ptr;
    return (void*) ((ptr_val + align - 1) & ~(align - 1));
}

void heap_init(void) {
    heap_buf = (volatile uint8_t*) TO_CPU(__heap_start__);
    heap_size = 0;
    heap_capacity = HEAP_SIZE;
}

void* heap_alloc(uint32_t num_bytes) {
    return heap_alloc_align(num_bytes, alignof(max_align_t));
}

void* heap_alloc_align(uint32_t num_bytes, uint32_t align) {
    void* aligned_ptr = align_ptr((void*) (heap_buf + heap_size), align);
    uint32_t aligned_offset = (uint32_t) aligned_ptr - (uint32_t) heap_buf;

    assert(aligned_offset + num_bytes <= heap_capacity,
        "Heap capacity exceeded");

    heap_size = aligned_offset + num_bytes;
    return aligned_ptr;
}

void heap_dealloc(uint32_t num_bytes) {
    uint32_t pos = (heap_size < num_bytes) ? 0 : heap_size - num_bytes;
    heap_dealloc_to(pos);
}

void heap_dealloc_to(uint32_t pos) {
    assert(pos <= heap_size, "Invalid heap dealloc position");
    heap_size = pos;
}

void heap_reset(void) {
    heap_size = 0;
}

