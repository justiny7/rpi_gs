#include "io_util.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>

void* read_file(uint32_t* size, const char* name) {
    int fd;
    if ((fd = open(name, O_RDONLY)) < 0) {
        printf("ERROR: read_file - can't open file\n");
    }

    struct stat file_stat;
    if (stat(name, &file_stat) < 0) {
        printf("ERROR: read_file - can't stat file\n");
    }

    *size = file_stat.st_size;
    void* buf = calloc((*size + 3) & ~0x3, 1);

    if (*size) {
        int res = read(fd, buf, *size);
        if (res != *size) {
            printf("ERROR: read_file - can't read file %s\n", name);
            exit(1);
        }
    }

    close(fd);

    return buf;
}

uint8_t get8(int fd) {
    uint8_t res;
    ssize_t n = read(fd, &res, 1);

    if (n == 1) {
        return res;
    }
    if (n == 0) {
        printf("---------- DISCONNECTED ----------\n");
        exit(0);
    }

    printf("ERROR: get8 - can't get char\n");
    exit(1);
}
uint32_t get32(int fd) {
    uint32_t res = 0;
    res |= get8(fd);
    res |= get8(fd) << 8;
    res |= get8(fd) << 16;
    res |= get8(fd) << 24;
    return res;
}

void put8(int fd, uint8_t val) {
    if (write(fd, &val, 1) != 1) {
        printf("ERROR: put8 - can't write char\n");
        exit(1);
    }
}
void put32(int fd, uint32_t val) {
    if (write(fd, &val, 4) != 4) {
        printf("ERROR: put32 - can't write uint32\n");
        exit(1);
    }
}

/*
uint8_t get8(int fd) {
    uint8_t res;
    scanf("%c", &res);
    return res;
}
uint32_t get32(int fd) {
    uint32_t res = 0;
    scanf("%x", &res);
    return res;
}

void put8(int fd, uint8_t val) {
    printf("%c", val);
}
void put32(int fd, uint32_t val) {
    printf("%x", val);
}
*/
