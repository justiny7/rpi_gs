.include <vc4.qinc>

.set stride, ra0
.set N, ra1
.set qpu_num, ra2

.set arr, ra5
.set indices, ra7
.set output, ra3
.set loop_counter, rb4

.set base_row, ra6

.macro mem_to_vpm_vec16, addr_reg, offset
    # read 1 row of length 16 at VPM row [offset]
    mov r0, vdr_setup_0(1, 16, 1, vdr_h32(1, offset, 0))

    # shift by 4 to switch to [base_row]'th row, add to vr_setup
    shl r1, base_row, 4
    add vr_setup, r0, r1

    # read [addr_reg] into ([base_row] + [offset])'th row of VPM
    mov vr_addr, addr_reg
    mov -, vr_wait
.endm

.macro vpm_to_mem_vec16, addr_reg, offset
    # write 1 row of length 16 at VPM row [offset]
    mov r0, vdw_setup_0(1, 16, dma_h32(offset, 0))

    # shift by 7 to switch to [base_row]'th row, add to vw_setup
    shl r1, base_row, 7
    add vw_setup, r0, r1

    # write ([base_row + [offset])'th row of VPM into [addr_reg]
    mov vw_addr, addr_reg
    mov -, vw_wait
.endm

.macro reg_to_vpm_vec16, src_reg, offset
    # set up vpm at row [offset]
    mov r0, vpm_setup(1, 1, h32(offset))

    # add [base_row] to [offset] and kick off VPM write
    add vw_setup, r0, base_row
    mov vpm, src_reg
.endm

.macro vpm_to_reg_vec16, dst_reg, offset
    # set up vpm at row [offset]
    mov r0, vpm_setup(1, 1, h32(offset))

    # add [base_row] to [offset] and kick off VPM read
    add vr_setup, r0, base_row
    mov dst_reg, vpm
.endm

mov stride, unif
mov N, unif
mov qpu_num, unif

mov arr, unif
mov indices, unif
mov output, unif

shl loop_counter, qpu_num, 4
shl base_row, qpu_num, 2

mov tmurs, 1

:loop
    mem_to_vpm_vec16 indices, 0
    vpm_to_reg_vec16 r1, 0

    shl r1, r1, 2

    add t0s, arr, r1
    add t0s, arr, r1
    add t0s, arr, r1
    ldtmu0
    ldtmu0
    ldtmu0

    reg_to_vpm_vec16 r4, 0
    vpm_to_mem_vec16 output, 0

    shl r0, stride, 2
    add output, output, r0
    add indices, indices, r0

    add r0, loop_counter, stride
    sub.setf -, r0, N

    brr.anyc -, :loop
    mov loop_counter, r0
    nop
    nop

nop; thrend
nop
nop
