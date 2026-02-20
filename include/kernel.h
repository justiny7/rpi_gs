#ifndef KERNEL_H
#define KERNEL_H

#include "arena_allocator.h"

typedef struct {
    Arena arena;
    uint32_t num_qpus;
    uint32_t num_unifs;

    uint32_t* code;
    uint32_t* unif;
    uint32_t* mbox_msg;
} Kernel;

void kernel_init(Kernel* kernel,
        uint32_t num_qpus, uint32_t num_unifs,
        uint32_t* code, uint32_t code_len);
void kernel_execute(Kernel* kernel);
void kernel_free(Kernel* kernel);

void kernel_load_unif_float(Kernel* kernel, uint32_t qpu, uint32_t unif_idx, float val);
void kernel_load_unif_u32(Kernel* kernel, uint32_t qpu, uint32_t unif_idx, uint32_t val);
#define kernel_load_unif(kernel, qpu, unif_idx, val) _Generic((val), \
    float: kernel_load_unif_float, \
    double: kernel_load_unif_float, \
    default: kernel_load_unif_u32 \
)(kernel, qpu, unif_idx, val)

#endif
