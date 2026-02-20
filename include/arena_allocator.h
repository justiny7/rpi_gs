#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H

#include "qpu.h"

typedef struct {
    volatile uint8_t* buf;
    uint32_t size;
    uint32_t capacity;
} Arena;

void arena_init(Arena* arena, uint32_t num_bytes);

void* arena_alloc(Arena* arena, uint32_t num_bytes);
void* arena_alloc_align(Arena* arena, uint32_t num_bytes, uint32_t align);

void arena_dealloc(Arena* arena, uint32_t num_bytes);
void arena_dealloc_to(Arena* arena, uint32_t pos);

void arena_reset(Arena* arena);

void arena_free(Arena* arena);

#endif
