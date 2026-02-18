
# void PUT32(uint32_t addr, uint32_t val)
.global PUT32
PUT32:
    str r1, [r0]
    bx lr

# uint32_t GET32(uint32_t addr)
.global GET32
GET32:
    ldr r0, [r0]
    bx lr

# void OR32(uint32_t addr, uint32_t val)
.global OR32
OR32:
    ldr r2, [r0]
    orr r2, r2, r1
    str r2, [r0]
    bx lr

# void PUT8(uint32_t addr, uint32_t val)
.global PUT8
PUT8:
    strb r1, [r0]
    bx lr

# uint32_t GET8(uint32_t addr)
.global GET8
GET8:
    ldrb r0, [r0]
    bx lr

# void mem_barrier_dsb()
.global mem_barrier_dsb
mem_barrier_dsb:
    mcr p15, 0, r0, c7, c10, 4
    bx lr

# void mem_barrier_dmb()
.global mem_barrier_dmb
mem_barrier_dmb:
    mov r0, #0
    mcr p15, 0, r0, c7, c10, 5
    bx lr
