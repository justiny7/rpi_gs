/*
 * Referenced https://github.com/bztsrc/raspi3-tutorial/tree/master/0D_readfile
 */

int fat_getpartition(void);
unsigned int fat_getcluster(char *fn, unsigned int *file_size);
char *fat_readfile(unsigned int cluster, unsigned int *bytes_read);
