# Define the toolchain
CC = i686-elf-gcc
AS = nasm

# Build directories
BUILD_DIR = build
ISO_DIR = isodir
ISO_BOOT = $(ISO_DIR)/boot
ISO_GRUB = $(ISO_BOOT)/grub

# Source files
C_SOURCES = $(shell find src -name '*.c')
ASM_SOURCES = $(shell find src -name '*.asm')

# Object files (placed in build folder)
OBJS = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES)) \
       $(patsubst src/%.asm, $(BUILD_DIR)/%.o, $(ASM_SOURCES))

# Compiler and assembler flags
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Iinclude
ASFLAGS = -f elf32

# Output files
KERNEL_BIN = $(BUILD_DIR)/minios.bin
OS_ISO = minios.iso

all: $(OS_ISO)

# Rule to compile C files
$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS)

# Rule to assemble asm files
$(BUILD_DIR)/%.o: src/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Link the kernel
$(KERNEL_BIN): $(OBJS) linker.ld
	$(CC) -T linker.ld -o $@ -ffreestanding -O2 -nostdlib $(OBJS) -lgcc

# Create ISO image
$(OS_ISO): $(KERNEL_BIN)
	@mkdir -p $(ISO_GRUB)
	cp $(KERNEL_BIN) $(ISO_BOOT)/minios.bin
	echo 'set timeout=0' > $(ISO_GRUB)/grub.cfg
	echo 'set default=0' >> $(ISO_GRUB)/grub.cfg
	echo '' >> $(ISO_GRUB)/grub.cfg
	echo 'menuentry "MINI OS" {' >> $(ISO_GRUB)/grub.cfg
	echo '  multiboot /boot/minios.bin' >> $(ISO_GRUB)/grub.cfg
	echo '  boot' >> $(ISO_GRUB)/grub.cfg
	echo '}' >> $(ISO_GRUB)/grub.cfg
	grub-mkrescue -o $(OS_ISO) $(ISO_DIR)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) $(OS_ISO)

# Run in emulator
run: $(OS_ISO)
	qemu-system-i386 -cdrom $(OS_ISO)

.PHONY: all clean run
