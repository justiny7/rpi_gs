#ifndef KERNEL_H
#define KERNEL_H

#include "arena_allocator.h"

typedef struct {
    Arena arena;
    uint32_t num_qpus;
    uint32_t num_unifs;
    uint32_t* cur_unif;

    uint32_t* code;
    uint32_t* unif;
    uint32_t* mbox_msg;
} Kernel;

void kernel_init(Kernel* kernel,
        uint32_t num_qpus, uint32_t num_unifs,
        uint32_t* code, uint32_t code_len);
void kernel_execute(Kernel* kernel);
void kernel_free(Kernel* kernel);

void kernel_load_unif_f(Kernel* kernel, uint32_t qpu, float val);
void kernel_load_unif_d(Kernel* kernel, uint32_t qpu, uint32_t val);
#define kernel_load_unif(kernel, qpu, val) _Generic((val), \
    float: kernel_load_unif_f, \
    double: kernel_load_unif_f, \
    default: kernel_load_unif_d \
)(kernel, qpu, val)

#endif
