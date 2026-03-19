#include "armv6_asm.h"
#include "armv6_cache_asm.h"

// from main.c
.extern scheduler
.extern current_thread
.extern next_thread

.global irq_handler
irq_handler:
    sub lr, lr, #4

// can skip directly here for cooperative threads
.global svc_handler
svc_handler:
    // store return state
    srsdb sp!, #SYSTEM_MODE

    // switch to system mode from IRQ mode
    cps #SYSTEM_MODE

    push {r0-r12, lr}

    // get next thread from scheduler
    bl scheduler

    ldr r0, =next_thread
    ldr r1, [r0]
    ldr r2, =current_thread
    ldr r3, [r2]
    cmp r1, r3

    // skip context switch if next thread is the same
    beq skip_cswitch

    str sp, [r3] // store current_thread sp to current_thread
    str r1, [r2] // store next_thread to current_thread
    ldr sp, [r1] // load next_thread sp to sp

skip_cswitch:
    pop {r0-r12, lr}

    rfeia sp!

.global start_thread
start_thread:
    DSB(r4)

    cps #SYSTEM_MODE
    mov sp, r0

    PREFETCH_FLUSH(r4)
    pop {r0-r12, lr}

    rfeia sp!

.global get_sp
get_sp:
    mov r0, sp
    bx lr
