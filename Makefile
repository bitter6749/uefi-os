# ==============================================================================
# UEFI OS Build Makefile (C language & Assembly hybrid)
# ==============================================================================

# Compilers & Tools
CC          := x86_64-w64-mingw32-gcc
ASM         := nasm
LD          := x86_64-w64-mingw32-ld
OBJCOPY     := x86_64-w64-mingw32-objcopy
QEMU        := qemu-system-x86_64
MFORMAT     := mformat
MMD         := mmd
MCOPY       := mcopy
DD          := dd

# Compiler & Linker Flags
# -ffreestanding: 標準ライブラリのない自作OS/UEFI環境用
# -fno-stack-protector -fno-stack-check: スタック保護用ランタイム依存を排除
# -mno-red-zone: x86_64のRed Zone(128バイト)を無効化（割り込み処理でのスタック破壊防止）
# -nostdlib: ホストOSの標準ライブラリをリンクしない
CFLAGS      := -Wall -Wextra -O2 -ffreestanding -fno-stack-protector \
               -fno-stack-check -mno-red-zone -nostdlib

# または find で include は以下の全ディレクトリを自動取得する場合
INCDIRS := $(shell find include -type d)
CFLAGS += $(addprefix -I, $(INCDIRS)) -Isrc

ASMFLAGS    := -f win64
LDFLAGS     := -e efi_main --subsystem 10

# OVMF Firmware Path (Auto-detected if not specified)
OVMF        ?= $(firstword $(wildcard \
                 /usr/share/ovmf/OVMF.fd \
                 /usr/share/qemu/OVMF.fd \
                 /usr/share/OVMF/OVMF_CODE_4M.fd \
                 /usr/share/OVMF/OVMF.fd \
                 /usr/share/edk2-ovmf/x64/OVMF.fd))

# Directories & Targets
SRC_DIR     := src
BUILD_DIR   := build

TARGET_EFI  := $(BUILD_DIR)/BOOTX64.EFI
TARGET_IMG  := $(BUILD_DIR)/disk.img

# Font Binary Asset
FONT_BIN    := assets/font.bin
FONT_OBJ    := $(BUILD_DIR)/font_bin.o

# Source files (all .c, .asm, and .S files under src/)
C_SRCS      := $(shell find $(SRC_DIR) -name "*.c")
NASM_SRCS   := $(shell find $(SRC_DIR) -name "*.asm")
GAS_SRCS    := $(shell find $(SRC_DIR) -name "*.S")

C_OBJS      := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SRCS))
NASM_OBJS   := $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%_asm.o,$(NASM_SRCS))
GAS_OBJS    := $(patsubst $(SRC_DIR)/%.S,$(BUILD_DIR)/%_S.o,$(GAS_SRCS))
ALL_OBJS    := $(C_OBJS) $(NASM_OBJS) $(GAS_OBJS) $(FONT_OBJ)

# Default Target
.PHONY: all
all: $(TARGET_IMG)

# Compile C source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble NASM source files to object files
$(BUILD_DIR)/%_asm.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@

# Assemble GAS source files (.S) to object files
$(BUILD_DIR)/%_S.o: $(SRC_DIR)/%.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Convert binary font into linkable PE-COFF object file
$(FONT_OBJ): $(FONT_BIN)
	@mkdir -p $(BUILD_DIR)
	$(OBJCOPY) -I binary -O pe-x86-64 -B i386:x86-64 $< $@

# Link object files into PE32+ UEFI executable
$(TARGET_EFI): $(ALL_OBJS)
	@mkdir -p $(BUILD_DIR)
	@if [ -z "$(ALL_OBJS)" ]; then \
		echo "Error: No source files found in $(SRC_DIR)/ (.c or .asm)."; \
		exit 1; \
	fi
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)

# Create bootable FAT32 disk image using mtools
$(TARGET_IMG): $(TARGET_EFI)
	@mkdir -p $(BUILD_DIR)
	@echo "Creating 64MB FAT32 disk image..."
	$(DD) if=/dev/zero of=$@ bs=1M count=64 status=none
	$(MFORMAT) -i $@ -F ::
	$(MMD) -i $@ ::/EFI
	$(MMD) -i $@ ::/EFI/BOOT
	$(MCOPY) -i $@ $< ::/EFI/BOOT/BOOTX64.EFI
	@echo "Disk image created at $@"

# Run under QEMU with UEFI (OVMF)
.PHONY: run
run: $(TARGET_IMG)
	@if [ -z "$(OVMF)" ]; then \
		echo "Error: OVMF firmware not found. Please install ovmf or set OVMF=/path/to/OVMF.fd"; \
		exit 1; \
	fi
	$(QEMU) -bios $(OVMF) -drive file=$(TARGET_IMG),format=raw -net none

# Flash disk image to physical USB drive
# Usage: make flash DRIVE=/dev/sdX
.PHONY: flash
flash: $(TARGET_IMG)
	@if [ -z "$(DRIVE)" ]; then \
		echo "Error: DRIVE variable not specified."; \
		echo "Usage: make flash DRIVE=/dev/sdX"; \
		exit 1; \
	fi
	@echo "WARNING: All data on $(DRIVE) will be overwritten!"
	@echo -n "Are you sure you want to proceed? [y/N]: "
	@read ans && [ "$$ans" = "y" -o "$$ans" = "Y" ] || (echo "Aborted." && exit 1)
	sudo $(DD) if=$(TARGET_IMG) of=$(DRIVE) bs=4M status=progress conv=fsync

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/*
