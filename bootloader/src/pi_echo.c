#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <poll.h>

#include "tty_util.h"

#define BUF_SIZE 4096
#define CTRL_D 4

struct termios orig_termios;

// hack-y state machine to indicate when we've seen the special string
// 'DONE!!!' from the pi telling us to shutdown.
int pi_done(unsigned char *s) {
    static unsigned pos = 0;
    const char exit_string[] = "DONE!!!\n";

    for(; *s; s++) {
        if(*s != exit_string[pos++]) {
            pos = 0;
            return pi_done(s + 1);
        }

        if(pos == sizeof(exit_string) - 1) {
            return 1;
        }
    }

    return 0;
}

void remove_nonprint(uint8_t* buf, int n) {
    for(int i = 0; i < n; i++) {
        uint8_t *p = &buf[i];
        if(isprint(*p) || (isspace(*p) && *p != '\r'))
            continue;
        *p = ' ';
    }
}

void reset_terminal_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
void set_terminal_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(reset_terminal_mode);

    struct termios raw = orig_termios;
    cfmakeraw(&raw);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void write_clean_output(int fd, uint8_t* buf, uint32_t n) {
    for (uint8_t* c = buf; c < buf + n; c++) {
        if (*c == '\n') {
            uint8_t cr = '\r';
            write(fd, &cr, 1);
            write(fd, c, 1);
        } else if (isprint(*c) || (isspace(*c) && *c != '\r')) {
            write(fd, c, 1);
        } else {
            uint8_t sp = ' ';
            write(fd, &sp, 1);
        }
    }

    fflush(stdout);
}

void pi_echo(int pi_fd, const char* device) {
    struct pollfd fds[2];
    fds[0].fd = pi_fd;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    set_terminal_raw_mode();

    printf("\r\n----------- PI OUTPUT ------------\r\n");
    while (1) {
        if (poll(fds, 2, -1) < 0) {
            perror("Poll error\n");
            exit(1);
        }

        if (fds[0].revents & POLLIN) {
            uint8_t buf[BUF_SIZE];

            int n = read(pi_fd, buf, BUF_SIZE - 1);
            if (n < 0 || (n == 0 && tty_disconnected(device))) {
                printf("\r\n---------- DISCONNECTED ----------\r\n");
                exit(1);
            }

            write_clean_output(STDOUT_FILENO, buf, n);

            if (pi_done(buf)) {
                printf("\r\n---------- PROGRAM DONE ----------\r\n");
                exit(0);
            }
        }

        if (fds[1].revents & POLLIN) {
            uint8_t c;
            int n = read(STDIN_FILENO, &c, 1);
            if (n > 0) {
                if (c == CTRL_D) {
                    printf("\r\n----------- TERMINATED -----------\r\n");
                    exit(0);
                }

                if (c == '\r') {
                    c = '\n';
                }

                write(pi_fd, &c, 1);
            }
        }
    }
}

