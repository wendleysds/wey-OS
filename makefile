TARGET = kernel.img

# Compilers
CC = i686-elf-gcc
CFLAGS = -Isrc/include -ffreestanding -nostdlib -Wall -Werror

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
LINKER_B16_FILE = linker16.ld
LINKER_B32_FILE = linker32.ld

# Binaries
BOOTLOADER_BIN = $(BIN_DIR)/bootloader.bin
KERNEL_BIN = $(BIN_DIR)/kernel.bin
STEP1_BIN = $(BIN_DIR)/step1.bin

# Source Files
SRC_C16_FILES = $(shell find $(SRC_DIR)/boot/ -type f -name "*.c")
SRC_C32_FILES = $(shell find $(SRC_DIR) -type f -name "*.c" ! -path "$(SRC_DIR)/boot/*")

EXCLUDE_B16_ASM_FILES = $(SRC_DIR)/boot/entry16.asm $(SRC_DIR)/boot/loader.asm
SRC_B16_ASM_FILES = $(filter-out $(EXCLUDE_B16_ASM_FILES), $(shell find $(SRC_DIR)/boot/ -type f -name "*.asm"))

EXCLUDE_B32_ASM_FILES = $(SRC_DIR)/core/entry32.asm
SRC_B32_ASM_FILES = $(filter-out $(EXCLUDE_B32_ASM_FILES), $(shell find $(SRC_DIR) -type f -name "*.asm" ! -path "$(SRC_DIR)/boot/*"))

# Object files

KERNEL16_ENTRY = $(OBJ_DIR)/boot/entry16.asm.o
KERNEL32_ENTRY= $(OBJ_DIR)/core/entry32.asm.o

OBJ_C16_FILES = $(patsubst $(SRC_DIR)/boot/%.c, $(OBJ_DIR)/%.o, $(SRC_C16_FILES))
OBJ_C32_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_C32_FILES))

OBJ_ASM16_FILES = $(patsubst $(SRC_DIR)/boot/%.asm, $(OBJ_DIR)/%.asm.o, $(SRC_B16_ASM_FILES))
OBJ_ASM32_FILES = $(patsubst $(SRC_DIR)/%.asm, $(OBJ_DIR)/%.asm.o, $(SRC_B32_ASM_FILES))

# Create directories
$(BUILD_DIRS):
	@echo "Creating directories..."
	mkdir -p $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR) $(IMG_DIR)

SECTOR_SIZE = 512

# Create kernel.img
$(TARGET): $(BOOTLOADER_BIN) $(KERNEL_BIN) $(STEP1_BIN)
	@echo "Creating os image..."
	mkdir -p $(IMG_DIR)
	dd if=/dev/zero of=$(IMG_DIR)/$@ bs=1048576 count=16
	dd if=$(BOOTLOADER_BIN) of=$(IMG_DIR)/$@ conv=notrunc
	dd if=$(STEP1_BIN) of=$(IMG_DIR)/$@ bs=$(SECTOR_SIZE) seek=1 conv=notrunc
	dd if=$(KERNEL_BIN) of=$(IMG_DIR)/$@ bs=$(SECTOR_SIZE) seek=20 conv=notrunc
	@echo "Created IMG in $(IMG_DIR)/$@"

# Compile files
# Bootlodaer
$(BOOTLOADER_BIN): $(SRC_DIR)/boot/loader.asm
	@echo "Compiling bootloader..."
	$(ASM) $(ASMFLAGS) -f bin $^ -o $(BOOTLOADER_BIN)

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

# Include dependency files
-include $(C_OBJ_FILES:.o=.d)

all:
	make clean
	make
	make kernel.img

run:
	qemu-system-i386 -serial stdio -drive format=raw,file=$(IMG_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

disassembly-img:
	i686-elf-objdump -D -b binary -m i386 -M intel build/img/kernel.img | less

