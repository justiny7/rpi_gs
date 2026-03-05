#ifndef HEAP_ALLOCATOR_H
#define HEAP_ALLOCATOR_H

#include "qpu.h"

#define HEAP_SIZE (50 * 1024 * 1024)

/* Single global heap state; defined in heap_allocator.c */
extern volatile uint8_t* heap_buf;
extern uint32_t heap_size;
extern uint32_t heap_capacity;

void heap_init(void);

void* heap_alloc(uint32_t num_bytes);
void* heap_alloc_align(uint32_t num_bytes, uint32_t align);

void heap_dealloc(uint32_t num_bytes);
void heap_dealloc_to(uint32_t pos);

void heap_reset(void);

void heap_free(void);

#endif
