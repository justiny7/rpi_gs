#ifndef QPU_H
#define QPU_H

#include <stdint.h>

// V3D spec: http://www.broadcom.com/docs/support/videocore/VideoCoreIV-AG100-R.pdf
#define V3D_BASE 0x20C00000

enum {
    V3D_L2CACTL = V3D_BASE + 0x0020,
    V3D_SLCACTL = V3D_BASE + 0x0024,
    V3D_SRQPC = V3D_BASE + 0x0430,
    V3D_SRQUA = V3D_BASE + 0x0434,
    V3D_SRQCS = V3D_BASE + 0x043C,
    V3D_DBCFG = V3D_BASE + 0x0E00,
    V3D_DBQITE = V3D_BASE + 0x0E2C,
    V3D_DBQITC = V3D_BASE + 0x0E30,
};

void qpu_execute(uint32_t num_qpus, uint32_t* mail);
// void qpu_execute(uint32_t num_qpus, uint32_t* unifs, uint32_t* code);

#endif
