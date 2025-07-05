IMG = $(IMG_DIR)/kernel.img
TARGET =

# Compilers
CC = i686-elf-gcc

CFLAGS = -Isrc/include -Wall -Werror

# Debug Flags
CFLAGS += -g
# Bare-metal Flags
CFLAGS += -ffreestanding -nostdlib -nostartfiles
# Warnig Suppresion Flags
CFLAGS += -Wno-unused-function -Wno-unused-parameter
# Alignment Flags
CFLAGS += -falign-jumps -falign-functions -falign-loops
# Optimzation Flags
CFLAGS += -fstrength-reduce -finline-functions

ASM = nasm
ASMFLAGS =

LD = i686-elf-ld
LDFLAGS= 

# Directories
SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/objs
BIN_DIR = $(BUILD_DIR)/bin
IMG_DIR = $(BUILD_DIR)/img
MOUNT_DIR = $(BUILD_DIR)/mnt

BUILD_DIRS = $(OBJ_DIR) $(BIN_DIR) $(IMG_DIR) $(MOUNT_DIR)

# Files
LINKER_B16_FILE = linker16.ld
LINKER_B32_FILE = linker32.ld

# Binaries
BOOTLOADER_BIN = $(BIN_DIR)/bootloader.bin
STEP1_BIN = $(BIN_DIR)/step1.bin
KERNEL_BIN = $(BIN_DIR)/kernel.bin

FAT_SIG = $(BIN_DIR)/FATSignature.bin
FAT_EMPTY = $(BIN_DIR)/FAT.bin

# Source Files
EXCLUDED_B16_FILES = $(SRC_DIR)/boot/entry16.asm $(SRC_DIR)/boot/loader.asm
EXCLUDED_B32_FILES = $(SRC_DIR)/core/entry32.asm $(SRC_DIR)/fs/fat/fat.asm $(SRC_DIR)/fs/fat/fatSig.asm

SRC_C16_FILES = $(shell find $(SRC_DIR)/boot/ -type f -name "*.c")
SRC_C32_FILES = $(shell find $(SRC_DIR) -type f -name "*.c" ! -path "$(SRC_DIR)/boot/*")

SRC_B16_ASM_FILES = $(filter-out $(EXCLUDED_B16_FILES), $(shell find $(SRC_DIR)/boot/ -type f -name "*.asm"))
SRC_B32_ASM_FILES = $(filter-out $(EXCLUDED_B32_FILES), $(shell find $(SRC_DIR) -type f -name "*.asm" ! -path "$(SRC_DIR)/boot/*"))

# Object files
KERNEL16_ENTRY = $(OBJ_DIR)/boot/entry16.asm.o
KERNEL32_ENTRY= $(OBJ_DIR)/core/entry32.asm.o

OBJ_C16_FILES = $(patsubst $(SRC_DIR)/boot/%.c, $(OBJ_DIR)/%.o, $(SRC_C16_FILES))
OBJ_C32_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_C32_FILES))

OBJ_ASM16_FILES = $(patsubst $(SRC_DIR)/boot/%.asm, $(OBJ_DIR)/%.asm.o, $(SRC_B16_ASM_FILES))
OBJ_ASM32_FILES = $(patsubst $(SRC_DIR)/%.asm, $(OBJ_DIR)/%.asm.o, $(SRC_B32_ASM_FILES))

SECTOR_SIZE = 512

# Create kernel.img
$(IMG): $(BUILD_DIRS) $(BOOTLOADER_BIN) $(STEP1_BIN) $(KERNEL_BIN) $(FAT_SIG) $(FAT_EMPTY)
	@echo "Creating os image in $@"
	mkdir -p $(IMG_DIR)
	dd if=/dev/zero of=$@ bs=512 count=32768
	dd if=$(BOOTLOADER_BIN) of=$@ conv=notrunc
	dd if=$(FAT_SIG) of=$@ bs=$(SECTOR_SIZE) seek=1 conv=notrunc
	dd if=$(BOOTLOADER_BIN) of=$@ bs=$(SECTOR_SIZE) seek=6 conv=notrunc 
	dd if=$(FAT_EMPTY) of=$@ bs=$(SECTOR_SIZE) seek=32 conv=notrunc
	dd if=$(FAT_EMPTY) of=$@ bs=$(SECTOR_SIZE) seek=160 conv=notrunc
	@echo "Mounting $< in $(MOUNT_DIR) and copying files..."
	sudo mount -t vfat $@ $(MOUNT_DIR)
	sudo cp $(STEP1_BIN) $(MOUNT_DIR)
	sudo cp $(KERNEL_BIN) $(MOUNT_DIR)
	sudo cp -r home $(MOUNT_DIR)
	sudo umount $(MOUNT_DIR)
	@echo "IMG Ready!"

check_TARGET:
	@if [ -z "$(TARGET)" ]; then \
		echo "ERROR: TARGET is empty!"; \
		exit 1; \
	fi

confirm:
	@lsblk -o NAME,SIZE,LABEL,MOUNTPOINT $(TARGET)
	@echo "Is this the correct target? (Y/n)"
	@read ans; \
	if [ "$$ans" != "Y" ] && [ "$$ans" != "y" ] && [ "$$ans" != "" ]; then \
		echo "Aborted."; \
		exit 1; \
	fi

# especify the device: TARGET=/dev/sd#
build_on: check_TARGET confirm $(IMG)
	sudo umount $(TARGET)* || true
	@echo "Building in $(TARGET)"
	sudo dd if=$(IMG) of=$(TARGET)
	@echo "Builded on $(TARGET)"

# Create directories
$(BUILD_DIRS):
	@echo "Creating Building Directories..."
	mkdir -p $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR) $(IMG_DIR) $(MOUNT_DIR)

# Compile files
# Bootlodaer
$(BOOTLOADER_BIN): $(SRC_DIR)/boot/loader.asm
	@echo "Compiling bootloader..."
	$(ASM) $(ASMFLAGS) -f bin $^ -o $@

# Kernel
$(KERNEL_BIN): $(KERNEL32_ENTRY) $(OBJ_C32_FILES) $(OBJ_ASM32_FILES) | $(BIN_DIR)
	@echo "Compiling Kernel binary..."
	@echo "Obj C Files:" $(OBJ_C32_FILES)
	@echo "Obj ASM Files:" $(OBJ_ASM32_FILES)
	$(LD) $(LDFLAGS) -T $(LINKER_B32_FILE) -o $@ $^ --oformat binary

# Step1
$(STEP1_BIN): $(KERNEL16_ENTRY) $(OBJ_C16_FILES) $(OBJ_ASM16_FILES) | $(BIN_DIR)
	@echo "Compiling step1 binary..."
	@echo "Obj C Files:" $(OBJ_C16_FILES)
	@echo "Obj ASM Files:" $(OBJ_ASM16_FILES)
	$(LD) $(LDFLAGS) -T $(LINKER_B16_FILE) -o $@ $^ --oformat binary

# File System
$(FAT_SIG): $(SRC_DIR)/fs/fat/fatSig.asm
	@echo "Compiling FAT Signature..."
	$(ASM) $(ASMFLAGS) -f bin $^ -o $@

$(FAT_EMPTY): $(SRC_DIR)/fs/fat/fat.asm
	@echo "Compiling empty FAT"
	$(ASM) $(ASMFLAGS) -f bin $^ -o $@

# Compile Objects
# Entries
$(KERNEL16_ENTRY): $(SRC_DIR)/boot/entry16.asm
	@mkdir -p $(OBJ_DIR)/boot
	$(ASM) $(ASMFLAGS) -f elf32 $^ -o $@

$(KERNEL32_ENTRY): $(SRC_DIR)/core/entry32.asm
	@mkdir -p $(OBJ_DIR)/core
	$(ASM) $(ASMFLAGS) -f elf32 $^ -o $@

# 16 BITS Files
$(OBJ_DIR)/%.o: $(SRC_DIR)/boot/%.c | $(OBJ_DIR)
	@echo "Compiling $< ..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -m16 -c $< -o $@

$(OBJ_DIR)/%.asm.o: $(SRC_DIR)/boot/%.asm | $(OBJ_DIR)
	@echo "Compiling $< ..."
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) -f elf32 $< -o $@

# 32 BITS Files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling $< ..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -m32 -c $< -o $@

$(OBJ_DIR)/%.asm.o: $(SRC_DIR)/%.asm | $(OBJ_DIR)
	@echo "Compiling $< ..."
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) -f elf32 $< -o $@

all:
	make clean
	make

run:
	make
	qemu-system-i386 -serial stdio -drive format=raw,file=$(IMG)

clean:
	rm -rf $(BUILD_DIR)

disassembly-img:
	i686-elf-objdump -D -b binary -m i386 -M intel build/img/kernel.img | less

debug:
	make update-bins
	qemu-system-i386 -serial stdio -drive format=raw,file=$(IMG) -s -S &
	@echo --QEMU ready for debugging!--

