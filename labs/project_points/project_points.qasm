.include <vc4.qinc>

.set stride, ra0
.set NUM_GAUSSIANS, ra1
.set qpu_num, ra2

.set pos_x, rb0
.set pos_y, rb1
.set pos_z, rb2

.set depth, ra3
.set screen_x, ra4
.set screen_y, ra5

.set base_row, ra6          # (qpu_num * 3)
.set ptr_stride_len, rb3 # (stride * sizeof(float))
.set loop_counter, rb4

.set tx_reg, ra7
.set ty_reg, ra8
.set tz_reg, ra9

.set w2c0, ra10
.set w2c1, rb5
.set w2c2, ra11
.set w2c3, rb6
.set w2c4, ra12
.set w2c5, rb7
.set w2c6, ra13
.set w2c7, rb8
.set w2c8, ra14
.set w2c9, rb9
.set w2c10, ra15
.set w2c11, rb10

.set fx, ra16
.set cx, rb11
.set fy, ra17
.set cy, rb12

mov stride, unif
mov NUM_GAUSSIANS, unif
mov qpu_num, unif

mov pos_x, unif    # pos_x
mov pos_y, unif    # pos_y
mov pos_z, unif    # pos_z

mov depth, unif    # depth
mov screen_x, unif    # screen_x
mov screen_y, unif    # screen_y

mov w2c0, unif
mov w2c1, unif
mov w2c2, unif
mov w2c3, unif
mov w2c4, unif
mov w2c5, unif
mov w2c6, unif
mov w2c7, unif
mov w2c8, unif
mov w2c9, unif
mov w2c10, unif
mov w2c11, unif

mov fx, unif
mov cx, unif
mov fy, unif
mov cy, unif

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

.macro mul_and_accum, offset, w2c_reg1, w2c_reg2, w2c_reg3
    # load p.x/y/z into r0
    vpm_to_reg_vec16 r0, offset

    mov r1, w2c_reg1
    fmul r1, r1, r0
    fadd tx_reg, tx_reg, r1

    mov r1, w2c_reg2
    fmul r1, r1, r0
    fadd ty_reg, ty_reg, r1

    mov r1, w2c_reg3
    fmul r1, r1, r0
    fadd tz_reg, tz_reg, r1
.endm

.macro calc_uv, offset0, offset1, offset2
    # move depth to VPM (can overrwrite pos)
    mov r2, tz_reg
    reg_to_vpm_vec16 r2, offset0

    # get r2 reciprocal
    mov sfu_recip, r2

    # calculate u: load cx first
    mov r3, cx   # c->cx
    mov r1, fx   # c->fx
    fmul r1, r1, tx_reg
    fmul r1, r1, r4
    fadd r3, r3, r1
    reg_to_vpm_vec16 r3, offset1

    # calculate v: load cy first
    mov r3, cy   # c->cy
    mov r1, fy   # c->fy
    fmul r1, r1, ty_reg
    fmul r1, r1, r4
    fadd r3, r3, r1
    reg_to_vpm_vec16 r3, offset2
.endm

# loop counter (qpu_num += stride until >= NUM_GAUSSIANS)
mov loop_counter, qpu_num

# pointer stride length = stride * sizeof(uint32_t)
shl ptr_stride_len, stride, 2

# multiply qpu_num by 3 to get VPM base row
mul24 base_row, qpu_num, 3

:loop
    # load pos_x, pos_y, pos_z into rows qpu_num * 3 + 0/1/2
    mem_to_vpm_vec16 pos_x, 0
    mem_to_vpm_vec16 pos_y, 1
    mem_to_vpm_vec16 pos_z, 2

    # initialize with w2c[3 / 7 / 11]
    mov tx_reg, w2c3
    mov ty_reg, w2c7
    mov tz_reg, w2c11
    mul_and_accum 0, w2c0, w2c4, w2c8
    mul_and_accum 1, w2c1, w2c5, w2c9
    mul_and_accum 2, w2c2, w2c6, w2c10
    nop
    nop

    calc_uv 0, 1, 2

    # write back to depth, screen_x, screen_y
    vpm_to_mem_vec16 depth, 0
    vpm_to_mem_vec16 screen_x, 1
    vpm_to_mem_vec16 screen_y, 2

    # increment pointers += stride * sizeof(float), check if reached array end
    mov r0, ptr_stride_len
    add pos_x, pos_x, r0
    add pos_y, pos_y, r0
    add pos_z, pos_z, r0
    add depth, depth, r0
    add screen_x, screen_x, r0
    add screen_y, screen_y, r0

    add r0, loop_counter, stride
    sub.setf -, r0, NUM_GAUSSIANS

    brr.anyc -, :loop
    mov loop_counter, r0
    nop
    nop

nop; thrend
nop
nop
