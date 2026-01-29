#include "put_code.h"
#include "io_util.h"
#include "boot_codes.h"
#include "boot_crc32.h"

#include <stdio.h>
#include <stdlib.h>

// #define VERBOSE

uint32_t wait_for_op(int fd) {
    while (1) {
        uint32_t op = get32(fd);
        if (op == BOOT_ERROR) {
            printf("BOOT ERROR\n");
            exit(1);
        }
        if (op != PRINT_STRING) {
            return op;
        }
    }
}

void put_code(int fd, const uint8_t* bin, uint32_t code_len) {
    uint32_t crc = crc32((void*) bin, code_len);

#ifdef VERBOSE
    printf("Waiting for GET_INFO...\n");
#endif
    while (wait_for_op(fd) != GET_INFO) {
        get8(fd); // offset by one byte
    }

#ifdef VERBOSE
    printf("Putting info...\n");
#endif
    put32(fd, PUT_INFO);
    put32(fd, BASE_ADDR);
    put32(fd, code_len);
    put32(fd, crc);

#ifdef VERBOSE
    printf("Waiting for GET_CODE...\n");
#endif
    while (wait_for_op(fd) != GET_CODE);
    uint32_t crc_rec = get32(fd);
    if (crc_rec != crc) {
        printf("ERROR: crc received is incorrect\n");
        exit(1);
    }

#ifdef VERBOSE
    printf("Putting code...\n");
#endif
    put32(fd, PUT_CODE);
    for (uint32_t i = 0; i < code_len; i++) {
        put8(fd, bin[i]);
    }

    if (wait_for_op(fd) != BOOT_SUCCESS) {
        printf("ERROR: put_code - boot error\n");
        exit(1);
    }

    printf("Boot success!\n");
    printf("----------- PI OUTPUT ------------\n");

    while (1) {
        putchar(get8(fd));
        fflush(stdout);
    }
}

