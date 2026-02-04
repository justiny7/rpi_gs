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
    mov r0, #0
    mcr p15, 0, r0, c7, c5, 4
    ldr r0, =_interrupt_table
    mcr p15, 0, r0, c12, c0, 0
    cpsie i

    bl main

halt:
    wfe
    b halt


.global _interrupt_table
_interrupt_table:
    ldr pc, =reset_vector
    ldr pc, =undefined_instruction_vector
    ldr pc, =software_interrupt_vector
    ldr pc, =prefetch_abort_vector
    ldr pc, =data_abort_vector
    ldr pc, =reset_vector
    ldr pc, =interrupt_vector
    ldr pc, =fast_interrupt_vector

reset_vector:
    bl halt
