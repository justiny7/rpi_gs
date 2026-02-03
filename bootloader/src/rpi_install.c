#include "put_code.h"
#include "tty_util.h"
#include "io_util.h"
#include "pi_echo.h"

#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Please only provide one argument (your kernel)!\n");
        return 1;
    }

    const int baudrate = 115200;

    char* device = find_ttyusb();
    int tty = open_tty(device);
    int fd = set_tty_to_8n1(tty, baudrate);
    printf("Found Pi at: %s\n", device);

    uint32_t code_len;
    uint8_t* code = read_file(&code_len, argv[1]);
    printf("Kernel at: %s, code len = %d\n", argv[1], code_len);

    put_code(fd, code, code_len);
    pi_echo(fd, argv[1]);

    return 0;
}
