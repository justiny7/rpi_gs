.section ".text.boot"

.global _start

_start:
    ldr sp, =0x8000000

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
