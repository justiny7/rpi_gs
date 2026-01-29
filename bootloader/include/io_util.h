#ifndef IO_UTIL_H
#define IO_UTIL_H

#include <stdint.h>

void* read_file(uint32_t* size, const char* name);

uint8_t get8(int fd);
uint32_t get32(int fd);

void put8(int fd, uint8_t val);
void put32(int fd, uint32_t val);

#endif
