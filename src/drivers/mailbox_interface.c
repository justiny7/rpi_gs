#include "mailbox_interface.h"
#include "lib.h"

static uint32_t mbox_buf[MBOX_BUFFER_SIZE] __attribute__((aligned(16)));

/* GENERAL */
bool mbox_get_property(uint32_t* msg) {
    uint32_t addr = (uint32_t) msg;
    assert((addr & 0xF) == 0, "Mailbox address must be 16-byte aligned");

    mem_barrier_dsb();
    mbox_write(MB_TAGS_ARM_TO_VC, addr);
    mbox_read(MB_TAGS_ARM_TO_VC);
    mem_barrier_dsb();
    return msg[1] == MBOX_REQUEST_SUCCESS;
}

bool mbox_get_property_batch(uint32_t count, ...) {
    va_list args;
    va_start(args, count);

    mbox_buf[0] = (count + 3) * sizeof(uint32_t);
    mbox_buf[1] = MBOX_REQUEST;
    for (uint32_t i = 0; i < count; i++) {
        mbox_buf[i + 2] = va_arg(args, uint32_t);
    }
    mbox_buf[count + 2] = 0;

    va_end(args);
    return mbox_get_property(mbox_buf);
}


/* HARDARE */
uint32_t mbox_get_board_model() {
    assert(mbox_get_property_batch(4,
        MBOX_TAG_GET_BOARD_MODEL, 4, 0, 0
    ), "Get board model failed");

    return mbox_buf[5];
}
uint32_t mbox_get_board_revision() {
    assert(mbox_get_property_batch(4,
        MBOX_TAG_GET_BOARD_REVISION, 4, 0, 0
    ), "Get board revision failed");

    return mbox_buf[5];
}
uint32_t mbox_get_board_serial() {
    assert(mbox_get_property_batch(5,
        MBOX_TAG_GET_BOARD_SERIAL, 8, 0, 0, 0
    ), "Get board serial failed");

    return mbox_buf[5];
}
uint32_t mbox_get_arm_memory() {
    assert(mbox_get_property_batch(5,
        MBOX_TAG_GET_ARM_MEMORY, 8, 0, 0, 0
    ), "Get ARM memory failed");

    return mbox_buf[6];
}
uint32_t mbox_get_vc_memory() {
    assert(mbox_get_property_batch(5,
        MBOX_TAG_GET_VC_MEMORY, 8, 0, 0, 0
    ), "Get VC memory failed");

    return mbox_buf[6];
}


/* TEMPERATURE */
uint32_t mbox_get_temp() {
    assert(mbox_get_property_batch(5,
        MBOX_TAG_GET_TEMP, 8, 0, 0, 0
    ), "Get temperature failed");

    return mbox_buf[6];
}
uint32_t mbox_get_max_temp() {
    assert(mbox_get_property_batch(5,
        MBOX_TAG_GET_MAX_TEMP, 8, 0, 0, 0
    ), "Get max temperature failed");

    return mbox_buf[6];
}


/* CLOCK */
uint32_t mbox_get_clock_rate(uint32_t clock_id) {
    assert(mbox_get_property_batch(5,
        MBOX_TAG_GET_CLOCK_RATE, 8, 0, clock_id, 0
    ), "Get clock rate failed");

    return mbox_buf[6];
}
uint32_t mbox_get_measured_clock_rate(uint32_t clock_id) {
    assert(mbox_get_property_batch(5,
        MBOX_TAG_GET_CLOCK_RATE_MEASURED, 8, 0, clock_id, 0
    ), "Get measured clock rate failed");

    return mbox_buf[6];
}
uint32_t mbox_get_max_clock_rate(uint32_t clock_id) {
    assert(mbox_get_property_batch(5,
        MBOX_TAG_GET_MAX_CLOCK_RATE, 8, 0, clock_id, 0
    ), "Get max clock rate failed");

    return mbox_buf[6];
}
uint32_t mbox_get_min_clock_rate(uint32_t clock_id) {
    assert(mbox_get_property_batch(5,
        MBOX_TAG_GET_MIN_CLOCK_RATE, 8, 0, clock_id, 0
    ), "Get min clock rate failed");

    return mbox_buf[6];
}
void mbox_set_clock_rate(uint32_t clock_id, uint32_t clock_rate) {
    assert(mbox_get_property_batch(6,
        MBOX_TAG_SET_CLOCK_RATE, 12, 0, clock_id, clock_rate, 1
    ), "Set clock rate failed");
}
