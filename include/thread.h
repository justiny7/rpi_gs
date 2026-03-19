#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>

#define STACK_MEM_SIZE 1 * 1024 * 1024  // 1 MiB

typedef struct {
    uint32_t* sp;
    uint8_t memory[STACK_MEM_SIZE];
} Thread;

typedef struct {
    uint32_t reg[13];
    uint32_t lr;
    uint32_t pc;
    uint32_t cpsr;
} StackFrame;

void thread_init(volatile Thread* t, void (*fn)(void*));
void thread_yield();

#endif

