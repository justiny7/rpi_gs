.section ".text.boot"

.global _start
_start:
    ldr r0, =0x8000
    ldr r1, =__code_start__
    ldr r2, =__data_end__ // copy all the way until end of data

copy_loop:
    ldmia r0!, {r3-r10}
    stmia r1!, {r3-r10}
    cmp r1, r2
    blo copy_loop

    ldr r3, =skip
    bx r3

skip:
    mov sp, #0x8000000

    ldr r0, =__bss_start__
    ldr r1, =__bss_end__
    mov r2, #0

bss_loop:
    cmp r0, r1
    bge bss_done
    str r2, [r0], #4
    b bss_loop

bss_done:
    bl main

halt:
    wfe
    b halt
