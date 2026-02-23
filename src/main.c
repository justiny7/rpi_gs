/*

Goal: Write to output array

Output is size N, ELEMS_PER_QPU = N / NUM_QPUS. We want

    output[i * ELEMS_PER_QPU : (i + 1) * ELEMS_PER_QPU] = i

for i = 0 ... NUM_QPUS - 1

QPU i is in charge of element i. It's given:
- Pointer to beginning index in output
- Number of 16-wide moves it has to do (ELEMS_PER_QPU / 16)
- i

*/

#include "mailbox_interface.h"
#include "qpu.h"
#include "lib.h"
#include "uart.h"
#include "sys_timer.h"
#include "arena_allocator.h"
#include "kernel.h"

#include "move_mem.h"

#define NUM_QPUS 12
#define NUM_UNIFS 3
#define ITERATIONS 10000
#define SIMD_WIDTH 16
#define N (ITERATIONS * NUM_QPUS * SIMD_WIDTH)
#define ELEMS_PER_QPU (ITERATIONS * SIMD_WIDTH)

void main() {
    ;
}
