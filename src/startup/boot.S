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
    # enable access to CP10 and CP11
    mrc p15, 0, r0, c1, c0, 2
    orr r0, r0, #(0xF << 20)
    mcr p15, 0, r0, c1, c0, 2

    # prefetch buffer
    mov r0, #0
    mcr p15, 0, r0, c7, c5, 4

    # enable VFP (set EN bit 30)
    mov r0, #(1 << 30)
    fmxr fpexc, r0

    # prefetch buffer
    mov r0, #0
    mcr p15, 0, r0, c7, c5, 4

    # load interrupt table
    ldr r0, =_interrupt_table
    mcr p15, 0, r0, c12, c0, 0
    cpsie i

    bl main

halt:
    wfe
    b halt


.p2align 5
.global _interrupt_table
_interrupt_table:
    ldr pc, =reset_vector
    ldr pc, =undefined_instruction_vector
    ldr pc, =software_interrupt_vector
    ldr pc, =prefetch_abort_vector
    ldr pc, =data_abort_vector
    ldr pc, =reset_vector
    ldr pc, interrupt_handler_ptr
    ldr pc, =fast_interrupt_vector

.global interrupt_handler_ptr
interrupt_handler_ptr: .word interrupt_vector

reset_vector:
    bl halt
