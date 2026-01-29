#include "tty_util.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static const char *ttyusb_prefixes[] = {
    "cu.usbserial",
	0
};

#define TTYUSB_MAX_FILENAME_LEN 256

static int filter(const struct dirent *d) {
    for (const char** s = ttyusb_prefixes; *s; s++) {
        if (strstr(d->d_name, *s)) {
            return 1;
        }
    }

    return 0;
}

char* find_ttyusb() {
    struct dirent** namelist;
    int n;

    if ((n = scandir("/dev", &namelist, NULL, alphasort)) < 0) {
        printf("ERROR: find_ttyusb - can't scan dir\n");
        exit(1);
    }

    char* device = NULL;
    while (n--) {
        if (filter(namelist[n])) {
            if (!device) {
                device = namelist[n]->d_name;
            } else {
                printf("ERROR: find_ttyusb - find_ttyusb failed: more than one device\n");
                exit(1);
            }
        }
    }

    if (!device) {
        printf("ERROR: find_ttyusb - find_ttyusb failed: no devices found\n");
        exit(1);
    }

    char buf[TTYUSB_MAX_FILENAME_LEN] = "/dev/";
    strcat(buf, device);
    return strdup(buf);
}

int open_tty(const char* device) {
    int fd;
    
    for (int i = 0; i < OPEN_TTY_MAX_RETRIES; i++) {
        if ((fd = open(device, O_RDWR | O_NOCTTY | O_SYNC)) >= 0) {
            break;
        }
    }

    if (fd == -1) {
        printf("ERROR: can't open USB device\n");
        exit(1);
    }

    return fd;
}

int set_tty_to_8n1(int fd, uint32_t speed) {
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0) {
        printf("ERROR: set_tty_to_8n1 - tcgetattr failed\n");
        exit(1);
    }

    memset (&tty, 0, sizeof tty);

    // https://github.com/rlcheng/raspberry_pi_workshop
    cfsetspeed(&tty, speed);

    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays

    // blocking reads
    tty.c_cc[VMIN] = 1;
    // no timeout
    tty.c_cc[VTIME] = 0;

	/*
	 * Setup TTY for 8n1 mode, used by the pi UART.
	 */

    // Disables the Parity Enable bit(PARENB),So No Parity 
    tty.c_cflag &= ~PARENB; 	
    // CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit 
    tty.c_cflag &= ~CSTOPB;   	
    // Clears the mask for setting the data size     
    tty.c_cflag &= ~CSIZE;	 	
    // Set the data bits = 8
    tty.c_cflag |=  CS8; 		
    // No Hardware flow Control 
    tty.c_cflag &= ~CRTSCTS;
    // Enable receiver,Ignore Modem Control lines 
    tty.c_cflag |= CREAD | CLOCAL; 	
    	
    // Disable XON/XOFF flow control both i/p and o/p
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);    
    // Non Cannonical mode 
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);  
    // No Output Processing
    tty.c_oflag &= ~OPOST;	

    if (tcsetattr (fd, TCSANOW, &tty) != 0) {
        printf("ERROR: can't set tty to 8n1\n");
        exit(1);
    }

    return fd;
}

