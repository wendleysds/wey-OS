TARGET = kernel.img

# Compilers
CC = i686-elf-gcc
CFLAGS = -Isrc/include -m32 -ffreestanding -nostdlib -Wall -Werror

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

BUILD_DIRS = $(OBJ_DIR) $(BIN_DIR) $(IMG_DIR)

# Files
LINKER_FILE = linker.ld
BOOTLOADER_BIN = $(BIN_DIR)/bootloader.bin
KERNEL_BIN = $(BIN_DIR)/kernel.bin

KERNEL_ASM = $(OBJ_DIR)/core/kernel.asm.o

SRC_C_FILES = $(shell find $(SRC_DIR) -type f -name "*.c")

EXCLUDE_ASM_FILES = $(SRC_DIR)/core/kernel.asm $(SRC_DIR)/boot/loader.asm
SRC_ASM_FILES = $(filter-out $(EXCLUDE_ASM_FILES), $(shell find $(SRC_DIR) -type f -name "*.asm"))

# Object files
C_OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_C_FILES))
ASM_OBJ_FILES = $(patsubst $(SRC_DIR)/%.asm, $(OBJ_DIR)/%.asm.o, $(SRC_ASM_FILES))

# Create directories
$(BUILD_DIRS):
	@echo "Creating directories..."
	mkdir -p $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR) $(IMG_DIR)

# Create kernel.img
$(TARGET): $(BOOTLOADER_BIN) $(KERNEL_BIN)
	@echo "Creating os image..."
	mkdir -p $(IMG_DIR)
	dd if=/dev/zero of=$(IMG_DIR)/$@ bs=1048576 count=16
	dd if=$(BOOTLOADER_BIN) of=$(IMG_DIR)/$@ conv=notrunc
	dd if=$(KERNEL_BIN) of=$(IMG_DIR)/$@ bs=512 seek=1 conv=notrunc
	@echo "Created img in $(IMG_DIR)/$@"

# Compile files
$(BOOTLOADER_BIN): $(SRC_DIR)/boot/loader.asm
	@echo "Compiling bootloader..."
	$(ASM) $(ASMFLAGS) -f bin $^ -o $(BOOTLOADER_BIN)

$(KERNEL_BIN): $(KERNEL_ASM) $(C_OBJ_FILES) $(ASM_OBJ_FILES) | $(BIN_DIR)
	@echo "Compiling kernel binary..."
	@echo "C files: $(C_OBJ_FILES)"
	@echo "ASM files: $(ASM_OBJ_FILES)"
	$(LD) $(LDFLAGS) -T $(LINKER_FILE) -o $@ $^ --oformat binary

$(KERNEL_ASM): $(SRC_DIR)/core/kernel.asm
	@mkdir -p $(OBJ_DIR)/core
	$(ASM) $(ASMFLAGS) -f elf32 $^ -o $@

# Compile Objects
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling $< ..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.asm.o: $(SRC_DIR)/%.asm | $(OBJ_DIR)
	@echo "Compiling $< ..."
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) -f elf32 $< -o $@

# Include dependency files
-include $(C_OBJ_FILES:.o=.d)

all:
	make clean
	make
	make kernel.img

run:
	qemu-system-i386 -drive format=raw,file=$(IMG_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

disassembly-img:
	i686-elf-objdump -D -b binary -m i386 -M intel build/img/kernel.img | less

