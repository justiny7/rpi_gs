#include "fat32.h"
#include "fat32-helpers.h"
#include "uart.h"
#include "lib.h"

/******************************************************************************
 * boot record helpers.
 */
#define is_pow2(x) (((x) & -(x)) == (x))

static int is_fat32(int t) { return t == 0xb; }

void fat32_volume_id_check(fat32_boot_sec_t *b) {
    // has to be a multiple of 512 or the sd.c driver won't really work.
    // currently assume 512
    assert(b->bytes_per_sec == 512, "bytes_per_sec is not 512");
    assert(b->nfats == 2, "nfats is not 2");
    assert(b->sig == 0xAA55, "sig is not 0xAA55");
    assert(is_pow2(b->sec_per_cluster), "sec_per_cluster is not a power of 2");
    unsigned n = b->bytes_per_sec;
    assert(n == 512 || n == 1024 || n == 2048 || n == 4096, "bytes_per_sec is not 512, 1024, 2048, or 4096");
    assert(!b->max_files, "max_files is not 0");
    // 0 if size does not fit in 2bytes, which should always be true.
    assert(b->fs_nsec == 0, "fs_nsec is not 0");
    // removable disk.
    // assert(b->media_type == 0xf0, "media_type is not 0xf0");
    assert(b->zero == 0, "zero is not 0");
    assert(b->nsec_in_fs != 0, "nsec_in_fs is 0");
    // usually these apparently.
    assert(b->info_sec_num == 1, "info_sec_num is not 1");
    assert(b->backup_boot_loc == 6, "backup_boot_loc is not 6");
    assert(b->extended_sig == 0x29, "extended_sig is not 0x29");
}

void fat32_volume_id_print(const char *msg, fat32_boot_sec_t *b) {
    uart_puts(msg);
    uart_puts(":\n");
    char oem[9];
    memcpy(oem, b->oem, 8);
    oem[8] = 0;
    char label[12];
    memcpy(label, b->volume_label, 11);
    oem[11] = 0;
    char type[9];
    memcpy(type, b->fs_type, 8);
    type[8] = 0;
    uart_puts(" \toem               = <");
    uart_puts(oem);
    uart_puts(">\n");
    uart_puts(" \tbytes_per_sec     = ");
    uart_putd(b->bytes_per_sec);
    uart_puts("\n");
    uart_puts(" \tsec_per_cluster   = ");
    uart_putd(b->sec_per_cluster);
    uart_puts("\n");
    uart_puts(" \treserved size     = ");
    uart_putd(b->reserved_area_nsec);
    uart_puts("\n");
    uart_puts(" \tnfats             = ");
    uart_putd(b->nfats);
    uart_puts("\n");
    uart_puts(" \tmax_files         = ");
    uart_putd(b->max_files);
    uart_puts("\n");
    uart_puts(" \tfs n sectors      = ");
    uart_putd(b->fs_nsec);
    uart_puts("\n");
    uart_puts(" \tmedia type        = ");
    uart_putx(b->media_type);
    uart_puts("\n");
    uart_puts(" \tsec per track     = ");
    uart_putd(b->sec_per_track);
    uart_puts("\n");
    uart_puts(" \tn heads           = ");
    uart_putd(b->n_heads);
    uart_puts("\n");
    uart_puts(" \tn hidden secs     = ");
    uart_putd(b->hidden_secs);
    uart_puts("\n");
    uart_puts(" \tn nsec in FS      = ");
    uart_putd(b->nsec_in_fs);
    uart_puts("\n");
    fat32_volume_id_check(b);
}

/****************************************************************************************
 * fsinfo helpers.
 */
void fat32_fsinfo_print(const char *msg, struct fsinfo *f) {
    uart_puts(msg);
    uart_puts(":\n");
    uart_puts(" \tsig1              = ");
    uart_putx(f->sig1);
    uart_puts("\n");
    uart_puts(" \tsig2              = ");
    uart_putx(f->sig2);
    uart_puts("\n");
    uart_puts(" \tsig3              = ");
    uart_putx(f->sig3);
    uart_puts("\n");
    uart_puts(" \tfree cluster cnt  = ");
    uart_putd(f->free_cluster_count);
    uart_puts("\n");
    uart_puts(" \tnext free cluster = ");
    uart_putx(f->next_free_cluster);
    uart_puts("\n");
}

void fat32_fsinfo_check(struct fsinfo *info) {
    assert(info->sig1 ==  0x41615252, "sig1 is not 0x41615252");
    assert(info->sig2 ==  0x61417272, "sig2 is not 0x61417272");
    assert(info->sig3 ==  0xaa550000, "sig3 is not 0xaa550000");
}


/******************************************************************************
 * FAT table helpers.
 */
const char * fat32_fat_entry_type_str(uint32_t x) {
    switch(x) {
    case FREE_CLUSTER:      return "FREE_CLUSTER";
    case RESERVED_CLUSTER:  return "RESERVED_CLUSTER";
    case BAD_CLUSTER:       return "BAD_CLUSTER";
    case LAST_CLUSTER:      return "LAST_CLUSTER";
    case USED_CLUSTER:      return "USED_CLUSTER";
    default: uart_puts("bad value: ");
    uart_putx(x);
    uart_puts("\n");
    panic("bad value");
    }
}

int fat32_fat_entry_type(uint32_t x) {
    // eliminate upper 4 bits: error to not do this.
    x = (x << 4) >> 4;
    switch(x) {
    case FREE_CLUSTER: 
    case RESERVED_CLUSTER:
    case BAD_CLUSTER:
        return x;
    }
    if(x >= 0x2 && x <= 0xFFFFFEF)
        return USED_CLUSTER;
    if(x >= 0xFFFFFF0 && x <= 0xFFFFFF6)
        uart_puts("reserved value: ");
    uart_putx(x);
    uart_puts("\n");
    panic("reserved value");
    if(x >=  0xFFFFFF8  && x <= 0xFFFFFFF)
        return LAST_CLUSTER;
    uart_puts("impossible type value: ");
    uart_putx(x);
    uart_puts("\n");
    panic("impossible type value");
}

/****************************************************************************************
 * directory helpers.
 */

int fat32_dirent_free(fat32_dirent_t *d) {
    uint8_t x = d->filename[0];

    if(d->attr == FAT32_LONG_FILE_NAME)
        return fat32_dirent_is_deleted_lfn(d);
    return x == 0 || x == 0xe5;
}

const char * fat32_dir_attr_str(int attr) {
    if(attr == FAT32_LONG_FILE_NAME)
        return " LONG FILE NAME";

    static char buf[128];
    buf[0] = 0;
    if(fat32_is_attr(attr, FAT32_RO)) {
        strcat(buf, "R/O");
        attr &= ~FAT32_RO;
    }
    if(fat32_is_attr(attr, FAT32_HIDDEN)) {
        strcat(buf, " HIDDEN");
        attr &= ~FAT32_HIDDEN;
    }

    switch(attr) {
    case FAT32_SYSTEM_FILE:     strcat(buf, " SYSTEM FILE"); break;
    case FAT32_VOLUME_LABEL:    strcat(buf, " VOLUME LABEL"); break;
    case FAT32_DIR:             strcat(buf, " DIR"); break;
    case FAT32_ARCHIVE:         strcat(buf, " ARCHIVE"); break;
    default: uart_puts("unhandled attr=");
    uart_putx(attr);
    uart_puts("\n");
    panic("unhandled attr");
    }
    return buf;
}

// trim spaces from the first 8.
// not re-entrant, gross.
static const char *to_8dot3(const char p[11]) {
    static char s[14];
    const char *suffix = &p[8];

    // find last non-space
    memcpy(s, p, 8);
    int i = 7; 
    for(; i >= 0; i--) {
        if(s[i] != ' ') {
            i++;
            break;
        }
    }
    s[i++] = '.';
    // probably need to handle spaces here too.
    for(int j = 0; j < 3; j++)
        s[i++] = suffix[j];
    s[i++] = 0;
    return s;
}


void fat32_dirent_name(fat32_dirent_t *d, char *name) {
  strcpy(name, to_8dot3(d->filename));
}

int fat32_is_valid_name(char *name) {
  int n = strlen(name);
  int has_dot = 0;
  for (int i = 0; i < n; i++) {
    if (name[i] == '.') has_dot = 1;
  }
  if (has_dot) {
    if (n > 12) return 0;
    if (n < 5) return 0;
    if (name[n-4] != '.') return 0;
    for (int i = 0; i < n; i++) {
      if ((name[i] < 'A' || name[i] > 'Z') && (name[i] < '0' || name[i] > '9'))
        if (i != n-4)
          return 0;
    }
  } else {
    if (n > 8) return 0;
    if (n < 1) return 0;
    for (int i = 0; i < n; i++) {
      if ((name[i] < 'A' || name[i] > 'Z') && (name[i] < '0' || name[i] > '9'))
        return 0;
    }
  }
  return 1;
}

void fat32_dirent_set_name(fat32_dirent_t *d, char *name) {
  int n = strlen(name);
  assert(fat32_is_valid_name(name), "name is not valid");
  int has_dot = 0;
  for (int i = 0; i < n; i++) {
    if (name[i] == '.') has_dot = 1;
  }
  memset(d->filename, ' ', 11);
  if (has_dot) {
    memcpy(d->filename, name, n - 4);
    memcpy(d->filename + 8, name + n - 3, 3);
  } else {
    memcpy(d->filename, name, n);
  }
}


// not re-entrant, gross.
static const char *add_nul(const char filename[11]) {
    static char s[14];
    memcpy(s, filename, 11);
    s[11] = 0;
    return s;
}

void fat32_dirent_print_helper(fat32_dirent_t *d) {
    if(fat32_dirent_free(d))  {
        uart_puts("\tdirent is not allocated\n");
        return;
    }
    if(d->attr == FAT32_LONG_FILE_NAME) {
        uart_puts("\tdirent is an LFN\n");
        return;
    }
    if(fat32_is_attr(d->attr, FAT32_ARCHIVE)) {
        uart_puts("[ARCHIVE]: asssuming short part of LFN:");
    } else if(!fat32_is_attr(d->attr, FAT32_DIR)) {
        uart_puts("need to handle attr ");
        uart_putx(d->attr);
        uart_puts(" (");
        uart_puts(fat32_dir_attr_str(d->attr));
        uart_puts(")\n");
        if(fat32_is_attr(d->attr, FAT32_ARCHIVE))
            return;
    }
    uart_puts("\n");
    uart_puts("\tfilename      = raw=");
    uart_puts(add_nul(d->filename));
    uart_puts("> 8.3=");
    uart_puts(to_8dot3(d->filename));
    uart_puts("\n");
    uart_puts("\tbyte version  = {");
    for(int i = 0; i < sizeof d->filename; i++) {
        if(i==8) {
            uart_puts("\n"); uart_puts("\t\t\t\t");
        }
        uart_puts("'");
        uart_putc(d->filename[i]);
        uart_puts("'/");
        uart_putx(d->filename[i]);
        uart_puts(",");
    }
    uart_puts("}\n");
        
    uart_puts("\tattr         = ");
    uart_putx(d->attr);
    uart_puts(" ");
    if(d->attr != FAT32_LONG_FILE_NAME) {
        if(d->attr&FAT32_RO)
            uart_puts(" [Read-only]\n");
        if(d->attr&FAT32_HIDDEN)
            uart_puts(" [HIDDEN]\n");
        if(d->attr&FAT32_SYSTEM_FILE)
            uart_puts(" [SYSTEM FILE: don't move]\n");
        uart_puts("\n");
    }
#if 0
    // clutters output without much value (for lab 13 at least)
    printk("\tctime_tenths = %d\n", d->ctime_tenths);
    printk("\tctime         = %d\n", d->ctime);
    printk("\tcreate_date   = %d\n", d->create_date);
    printk("\taccess_date   = %d\n", d->access_date);
    printk("\tmod_time      = %d\n", d->mod_time);
    printk("\tmod_date      = %d\n", d->mod_date);
#endif
    uart_puts("\thi_start      = ");
    uart_putx(d->hi_start);
    uart_puts("\n");
    uart_puts("\tlo_start      = ");
    uart_putd(d->lo_start);
    uart_puts("\n");
    uart_puts("\tfile_nbytes   = ");
    uart_putd(d->file_nbytes);
    uart_puts("\n");
}

void fat32_dirent_print(const char *msg, fat32_dirent_t *d) {
    uart_puts(msg);
    uart_puts(": ");
    fat32_dirent_print_helper(d);
}

int fat32_dir_lookup(const char *name, fat32_dirent_t *dirs, unsigned n) {
    for(int i = 0; i < n; i++) {
        fat32_dirent_t *d = dirs+i;
        if(fat32_dirent_free(d) || fat32_dirent_is_lfn(d))
            continue;
        if(memcmp(name, d->filename, sizeof d->filename) == 0)
            return i;
    }
    return -1;
}


/****************************************************************************************
 * general purpose utilities.
 */

// print [p, p+n) as a string: use for ascii filled files.
void print_as_string(const char *msg, uint8_t *p, int n) {
    uart_puts(msg);
    uart_puts("\n");
    for(int i = 0; i < n; i++) {
        char c = p[i];
        uart_putc(c);
    }
    uart_puts("\n");
}

void print_bytes(const char *msg, uint8_t *p, int n) {
    uart_puts(msg);
    uart_puts("\n");
    for(int i = 0; i < n; i++) {
        if(i % 16 == 0)
            uart_puts("\n\t");
        uart_putx(p[i]);
        uart_puts(", ");
    }
    uart_puts("\n");
}
void print_words(const char *msg, uint32_t *p, int n) {
    uart_puts(msg);
    uart_puts("\n");
    for(int i = 0; i < n; i++) {
        if(i % 16 == 0)
            uart_puts("\n\t");
        uart_putx(p[i]);
        uart_puts(", ");
    }
    uart_puts("\n");
}
