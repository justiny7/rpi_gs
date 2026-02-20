.include <vc4.qinc>

mov ra0, unif # start address in output array
mov ra1, unif # num iterations
mov ra2, unif # i
mov ra3, unif # color

mov r2, ra1

:loop
    # allocate a 16-wide vector at VPM row 0
    mov r0, vpm_setup(4, 1, h32(0))

    # add ra2 to r0 to shift the row by i (use i'th row for i'th QPU)
    shl r3, ra2, 2
    add vw_setup, r0, r3

    # move data (i) to VPM
    mov vpm, ra3
    mov vpm, ra3
    mov vpm, ra3
    mov vpm, ra3

    # set up 16-wide DMA transfer at VPM row 0
    mov r0, vdw_setup_0(4, 16, dma_h32(0, 0))

    # shift r1 by 7 for DMA VPM row, then add to r0
    shl r1, r3, 7
    add vw_setup, r0, r1

    # r1 = offset from start address given we're in row r2
    # << 6 --> * 64 --> 4 bytes * 16 bytes/row
    sub.setf r2, r2, 1
    shl r1, r2, 8

    # write to output array
    add vw_addr, ra0, r1
    mov -, vw_wait

    brr.allnz -, :loop
    nop
    nop
    nop

nop; thrend
nop
nop
