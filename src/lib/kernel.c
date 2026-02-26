#include "kernel.h"
#include "qpu.h"
#include "lib.h"

#include "debug.h"
void kernel_init(Kernel* kernel,
        uint32_t num_qpus, uint32_t num_unifs,
        uint32_t* code, uint32_t code_size) {
    uint32_t total_size = code_size +
        num_qpus * num_unifs * sizeof(uint32_t) + // code
        num_qpus * 2 * sizeof(uint32_t) +         // unifs
        num_qpus * sizeof(uint32_t) +             // unif counter
        48;                                       // padding

    arena_init(&kernel->arena, total_size);

    kernel->num_qpus = num_qpus;
    kernel->num_unifs = num_unifs;
    kernel->cur_unif = arena_alloc(&kernel->arena, num_qpus * sizeof(uint32_t));
    memset(kernel->cur_unif, 0, num_qpus * sizeof(uint32_t));

    kernel->code = arena_alloc_align(&kernel->arena, code_size, 8);
    kernel->unif = arena_alloc_align(&kernel->arena, num_qpus * num_unifs * sizeof(uint32_t), 16);
    kernel->mbox_msg = arena_alloc(&kernel->arena, num_qpus * 2 * sizeof(uint32_t));

    memcpy(kernel->code, code, code_size);

    for (uint32_t i = 0; i < num_qpus; i++) {
        kernel->mbox_msg[i * 2] = TO_BUS(&kernel->unif[i * num_unifs]);
        kernel->mbox_msg[i * 2 + 1] = TO_BUS(kernel->code);
    }
}

void kernel_execute(Kernel* kernel) {
    qpu_execute(kernel->num_qpus, kernel->mbox_msg);
}

void kernel_free(Kernel* kernel) {
    arena_free(&kernel->arena);
}

void kernel_reset_unifs(Kernel* kernel) {
    memset(kernel->cur_unif, 0, kernel->num_qpus * sizeof(uint32_t));
}

void kernel_load_unif_f(Kernel* kernel, uint32_t qpu, float val) {
    assert(qpu < kernel->num_qpus, "Load to invalid QPU");
    assert(kernel->cur_unif[qpu] < kernel->num_unifs, "Loading too many unifs");
    memcpy(&kernel->unif[qpu * kernel->num_unifs + kernel->cur_unif[qpu]++], &val, sizeof(float));
}

void kernel_load_unif_d(Kernel* kernel, uint32_t qpu, uint32_t val) {
    assert(qpu < kernel->num_qpus, "Load to invalid QPU");
    assert(kernel->cur_unif[qpu] < kernel->num_unifs, "Loading too many unifs");
    memcpy(&kernel->unif[qpu * kernel->num_unifs + kernel->cur_unif[qpu]++], &val, sizeof(float));
}

