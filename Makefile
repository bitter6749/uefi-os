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
BUILD_DIR   := build
TARGET_EFI  := $(BUILD_DIR)/BOOTX64.EFI
TARGET_IMG  := $(BUILD_DIR)/disk.img

# --- Bootloader Settings ---
BOOT_SRC    := bootloader/src
BOOT_INC    := -Ibootloader/include -Ishared/include
BOOT_CSRCS  := $(shell find $(BOOT_SRC) -name "*.c")
BOOT_OBJS   := $(patsubst $(BOOT_SRC)/%.c,$(BUILD_DIR)/bootloader/%.o,$(BOOT_CSRCS))

# --- Kernel Settings ---
KERN_SRC    := kernel/src
KERN_INC    := $(addprefix -I, $(shell find kernel/include -type d)) -Ishared/include
KERN_CSRCS  := $(shell find $(KERN_SRC) -name "*.c")
KERN_SSRCS  := $(shell find $(KERN_SRC) -name "*.S")
KERN_COBJS  := $(patsubst $(KERN_SRC)/%.c,$(BUILD_DIR)/kernel/%.o,$(KERN_CSRCS))
KERN_SOBJS  := $(patsubst $(KERN_SRC)/%.S,$(BUILD_DIR)/kernel/%_S.o,$(KERN_SSRCS))

# Font Binary Asset
FONT_BIN    := assets/font.bin
FONT_OBJ    := $(BUILD_DIR)/font_bin.o

ALL_OBJS		:= $(BOOT_OBJS) $(KERN_COBJS) $(KERN_SOBJS) $(FONT_OBJ)

# Default Target
.PHONY: all
all: $(TARGET_IMG)

# Compile C source files to object files
$(BUILD_DIR)/bootloader/%.o: $(BOOT_SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(BOOT_INC) -c $< -o $@

$(BUILD_DIR)/kernel/%.o: $(KERN_SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(KERN_INC) -c $< -o $@

# Assemble GAS source files (.S) to object files
$(BUILD_DIR)/kernel/%_S.o: $(KERN_SRC)/%.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(KERN_INC) -c $< -o $@

# Convert binary font into linkable PE-COFF object file
$(FONT_OBJ): $(FONT_BIN)
	@mkdir -p $(BUILD_DIR)
	$(OBJCOPY) -I binary -O pe-x86-64 -B i386:x86-64 $< $@

# Link object files into PE32+ UEFI executable
$(TARGET_EFI): $(ALL_OBJS)
	@mkdir -p $(BUILD_DIR)
	@if [ -z "$(ALL_OBJS)" ]; then \
		echo "Error: No source files found in $(KERN_SRC)/ (.c or .asm)."; \
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

# Check if KVM is available
USE_KVM := $(shell test -w /dev/kvm && echo "-enable-kvm -cpu host" || echo "-cpu max,smap=on,smep=on")

# Run under QEMU with UEFI (OVMF)
.PHONY: run
run: $(TARGET_IMG)
	@if [ -z "$(OVMF)" ]; then \
		echo "Error: OVMF firmware not found. Please install ovmf or set OVMF=/path/to/OVMF.fd"; \
		exit 1; \
	fi
	$(QEMU) $(USE_KVM) \
					-bios $(OVMF) \
					-m 2048 \
					-smp 2 \
					-drive file=$(TARGET_IMG),format=raw \
					-net none \
					-d int,cpu_reset \

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
