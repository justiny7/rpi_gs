#include "thread.h"
#include "lib.h"

void thread_init(volatile Thread* t, void (*fn)(void*)) {
    volatile uint8_t* stack_top = t->memory + STACK_MEM_SIZE;

    volatile StackFrame* frame = (StackFrame*) (stack_top - sizeof(StackFrame));
    memset((void*) frame, 0, sizeof(frame));

    frame->pc = (uint32_t) fn;
    frame->cpsr = SYSTEM_MODE; // system mode w/ interrupts enbl
    t->sp = (uint32_t*) frame;
}
void thread_yield() {
    // trigger software interrupt
    asm volatile ("svc #0" ::: "memory");
}
