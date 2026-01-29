#ifndef LIB_H
#define LIB_H

#include <stdint.h>

void PUT32(uint32_t addr, uint32_t val);
uint32_t GET32(uint32_t addr);
void OR32(uint32_t addr, uint32_t val);

void PUT8(uint32_t addr, uint32_t val); // stores lowest 8 bytes of val
uint32_t GET8(uint32_t addr);

void mem_barrier_dsb();
void mem_barrier_dmb();

void rpi_reboot();

#endif
