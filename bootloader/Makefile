# Toolchain
CC_PI   = arm-none-eabi-gcc
CC_MAC  = gcc
OBJCOPY = arm-none-eabi-objcopy

# Flags
# Note: Added -Iinclude to CFLAGS_PI to ensure it finds bootloader-specific headers
CFLAGS_PI  = -mcpu=arm1176jzf-s -fpic -ffreestanding -Wall -Wextra -nostdlib -Iinclude -I../include -I../lib
CFLAGS_MAC = -O2 -Wall -Iinclude
LDFLAGS    = -T linker.ld -nostdlib

# Directories
BUILD_DIR     = build
BUILD_PI_DIR  = $(BUILD_DIR)/pi
BUILD_MAC_DIR = $(BUILD_DIR)/mac

# VPATH allows make to find sources in these directories automatically
VPATH = src ../src ../src/drivers ../lib

# Sources
PI_C_SRCS  = main.c uart.c gpio.c sys_timer.c lib.c
PI_S_SRCS  = boot.S lib.S
MAC_C_SRCS = put_code.c tty_util.c io_util.c pi_echo.c

# Objects (Separated by platform directory)
PI_OBJS  = $(addprefix $(BUILD_PI_DIR)/, $(PI_C_SRCS:.c=.c.o))
PI_OBJS += $(addprefix $(BUILD_PI_DIR)/, $(PI_S_SRCS:.S=.S.o))
MAC_OBJS = $(addprefix $(BUILD_MAC_DIR)/, $(MAC_C_SRCS:.c=.o))

# Targets
all: kernel.img rpi-install

# --- Raspberry Pi Build Rules ---

kernel.img: $(BUILD_PI_DIR)/kernel.elf
	$(OBJCOPY) $< -O binary $@

$(BUILD_PI_DIR)/kernel.elf: $(PI_OBJS)
	$(CC_PI) $(LDFLAGS) $(PI_OBJS) -o $@

# Pi C compilation
$(BUILD_PI_DIR)/%.c.o: %.c
	@mkdir -p $(BUILD_PI_DIR)
	$(CC_PI) $(CFLAGS_PI) -c $< -o $@

# Pi Assembly compilation (using GCC as the assembler to handle preprocessor directives)
$(BUILD_PI_DIR)/%.S.o: %.S
	@mkdir -p $(BUILD_PI_DIR)
	$(CC_PI) $(CFLAGS_PI) -c $< -o $@

# --- Mac Installer Build Rules ---

rpi-install: rpi_install.c $(MAC_OBJS)
	@mkdir -p bin
	$(CC_MAC) $(CFLAGS_MAC) $(MAC_OBJS) $< -o bin/$@

# Mac C compilation
$(BUILD_MAC_DIR)/%.o: %.c
	@mkdir -p $(BUILD_MAC_DIR)
	$(CC_MAC) $(CFLAGS_MAC) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) kernel.img rpi-install bin

.PHONY: all clean
