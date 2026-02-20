# toolchain
CC      = arm-none-eabi-gcc
AS      = arm-none-eabi-as
LD      = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

# flags
CFLAGS  = -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -fpic -ffreestanding -O2 -Wall -Wextra -nostdlib -Iinclude -Isrc/kernels
ASFLAGS = -mcpu=arm1176jzf-s -mfpu=vfp
LDFLAGS = -T linker.ld -nostdlib -mfloat-abi=hard -mfpu=vfp

# CFLAGS  = -mcpu=arm1176jzf-s -fpic -ffreestanding -O2 -Wall -Wextra -nostdlib -Iinclude
# ASFLAGS = -mcpu=arm1176jzf-s

# directories
SRC_DIR   = src
DRV_DIR   = src/drivers
STARTUP_DIR = src/startup
LIB_DIR = src/lib
KERNEL_DIR = src/kernels
BUILD_DIR = build
HEADER_DIR = include

# sources
SRCS_QASM = $(wildcard $(KERNEL_DIR)/*.qasm)
SRCS_C = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(DRV_DIR)/*.c) $(wildcard $(STARTUP_DIR)/*.c) $(SRCS_QASM:.qasm=.c)
SRCS_S = $(wildcard $(SRC_DIR)/*.S) $(wildcard $(STARTUP_DIR)/*.S)
SRCS_LIB_C = $(wildcard $(LIB_DIR)/*.c)
SRCS_LIB_S = $(wildcard $(LIB_DIR)/*.S)

.PRECIOUS: $(KERNEL_DIR)/%.c

# objects (preserve directory structure under build/ to avoid name collisions)
OBJS = \
  $(patsubst %.S,$(BUILD_DIR)/%.S.o,$(SRCS_S)) \
  $(patsubst %.c,$(BUILD_DIR)/%.c.o,$(SRCS_C))

LIB_OBJS = \
  $(patsubst %.S,$(BUILD_DIR)/%.S.o,$(SRCS_LIB_S)) \
  $(patsubst %.c,$(BUILD_DIR)/%.c.o,$(SRCS_LIB_C))

# targets
all: kernel.img

kernel.img: $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) $< -O binary $@

$(BUILD_DIR)/kernel.elf: $(OBJS) $(LIB_OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIB_OBJS) -lm -lgcc -o $@

# asm rules
$(BUILD_DIR)/%.S.o: %.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# c rules
$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# qasm rules
$(KERNEL_DIR)/%.c $(KERNEL_DIR)/%.h &: $(KERNEL_DIR)/%.qasm
	/opt/homebrew/opt/vc4asm/bin/vc4asm -h $(KERNEL_DIR)/$*.h -c $(KERNEL_DIR)/$*.c $<

$(OBJS): $(SRCS_QASM:$(KERNEL_DIR)/%.qasm=$(KERNEL_DIR)/%.h)

clean:
	rm -rf $(BUILD_DIR) kernel.img

.PHONY: all clean
