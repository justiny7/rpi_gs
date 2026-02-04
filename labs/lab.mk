# labs/lab.mk - shared Makefile fragment for all labs
#
# Each lab Makefile should define:
#   LAB_NAME   - used for output kernel name (e.g., gpio_interrupts -> kernel-gpio_interrupts.img)
#   LAB_SRCS   - (optional) additional .c files beyond main.c
#
# Then include this file:
#   include ../lab.mk

# path to OS root (relative to lab directory)
OS_ROOT := ../..

# toolchain
CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-as
OBJCOPY := arm-none-eabi-objcopy

# flags
CFLAGS  := -mcpu=arm1176jzf-s -fpic -ffreestanding -O2 -Wall -Wextra -nostdlib \
           -I$(OS_ROOT)/include -I$(OS_ROOT)/lib
ASFLAGS := -mcpu=arm1176jzf-s
LDFLAGS := -T $(OS_ROOT)/linker.ld -nostdlib

# build directory (local to lab)
BUILD_DIR := build

# OS sources
OS_SRC_DIR := $(OS_ROOT)/src
OS_DRV_DIR := $(OS_ROOT)/src/drivers
OS_LIB_DIR := $(OS_ROOT)/lib

OS_SRCS_C := $(wildcard $(OS_SRC_DIR)/*.c) $(wildcard $(OS_DRV_DIR)/*.c)
OS_SRCS_S := $(wildcard $(OS_SRC_DIR)/*.S)
LIB_SRCS_C := $(wildcard $(OS_LIB_DIR)/*.c)
LIB_SRCS_S := $(wildcard $(OS_LIB_DIR)/*.S)

# filter out the default main.c from OS sources
OS_SRCS_C := $(filter-out $(OS_SRC_DIR)/main.c, $(OS_SRCS_C))

# OS objects
OS_OBJS := \
  $(patsubst $(OS_ROOT)/%.S, $(BUILD_DIR)/%.S.o, $(OS_SRCS_S)) \
  $(patsubst $(OS_ROOT)/%.c, $(BUILD_DIR)/%.c.o, $(OS_SRCS_C))

LIB_OBJS := \
  $(patsubst $(OS_ROOT)/%.S, $(BUILD_DIR)/%.S.o, $(LIB_SRCS_S)) \
  $(patsubst $(OS_ROOT)/%.c, $(BUILD_DIR)/%.c.o, $(LIB_SRCS_C))

# lab sources: main.c + any extra LAB_SRCS
LAB_SRCS ?=
LAB_ALL_SRCS := main.c $(LAB_SRCS)
LAB_OBJS := $(patsubst %.c, $(BUILD_DIR)/lab/%.c.o, $(LAB_ALL_SRCS))

# all objects
ALL_OBJS := $(OS_OBJS) $(LIB_OBJS) $(LAB_OBJS)

# output kernel name
KERNEL := kernel-$(LAB_NAME).img

# default target
all: $(KERNEL)

$(KERNEL): $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) $< -O binary $@

$(BUILD_DIR)/kernel.elf: $(ALL_OBJS)
	$(CC) $(LDFLAGS) $(ALL_OBJS) -lgcc -o $@

# compile OS .S files
$(BUILD_DIR)/%.S.o: $(OS_ROOT)/%.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# compile OS .c files
$(BUILD_DIR)/%.c.o: $(OS_ROOT)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# compile lab .c files
$(BUILD_DIR)/lab/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(KERNEL)

.PHONY: all clean
