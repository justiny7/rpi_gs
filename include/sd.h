/*
 * Referenced https://github.com/bztsrc/raspi3-tutorial/tree/master/0D_readfile
 */

#define SD_OK                0
#define SD_TIMEOUT          -1
#define SD_ERROR            -2

int sd_init();
int sd_readblock(unsigned int lba, unsigned char *buffer, unsigned int num);
