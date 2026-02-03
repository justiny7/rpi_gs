#ifndef TTY_UTIL_H
#define TTY_UTIL_H

#include <stdint.h>
#include <stdbool.h>

#define OPEN_TTY_MAX_RETRIES 5

char* find_ttyusb();
int open_tty(const char* device);
int set_tty_to_8n1(int fd, uint32_t speed);
bool tty_disconnected(const char* device);

#endif
