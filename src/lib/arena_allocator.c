#include "arena_allocator.h"
#include "mailbox_interface.h"
#include "lib.h"

#include <stdalign.h>
#include <stddef.h>

void* align_ptr(void* ptr, uint32_t align) {
    uint32_t ptr_val = (uint32_t) ptr;
    uint32_t rem = ptr_val % align;
    return (rem == 0) ? ptr : ((void*) (ptr_val + align - rem));
}

void arena_init(Arena* arena, uint32_t num_bytes) {
    arena->buf = (volatile uint8_t*) TO_CPU(qpu_init(num_bytes));
    arena->size = 0;
    arena->capacity = num_bytes;
}

void* arena_alloc(Arena* arena, uint32_t num_bytes) {
    return arena_alloc_align(arena, num_bytes, alignof(max_align_t));
}
void* arena_alloc_align(Arena* arena, uint32_t num_bytes, uint32_t align) {
    void* aligned_ptr = align_ptr((void*) (arena->buf + arena->size), align);
    uint32_t aligned_offset = (uint32_t) aligned_ptr - (uint32_t) arena->buf;

    assert(aligned_offset + num_bytes <= arena->capacity,
        "Arena capacity exceeded");

    arena->size = aligned_offset + num_bytes;
    return aligned_ptr;
}

void arena_dealloc(Arena* arena, uint32_t num_bytes) {
    uint32_t pos = (arena->size < num_bytes) ? 0 : arena->size - num_bytes;
    arena_dealloc_to(arena, pos);
}
void arena_dealloc_to(Arena* arena, uint32_t pos) {
    assert(pos <= arena->size, "Invalid arena dealloc position");
    arena->size = pos;
}

void arena_reset(Arena* arena) {
    arena->size = 0;
}

void arena_free(Arena* arena) {
    qpu_free((uint32_t) arena->buf);
    arena->size = arena->capacity = 0;
}
