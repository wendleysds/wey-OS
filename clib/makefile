TARGET=$(BUILD_DIR)/clib.o

# Compilers
CC = i686-elf-gcc

CFLAGS = -Iinclude -Wall -Werror

# Bare-metal Flags
CFLAGS += -ffreestanding -nostdlib -nostartfiles
# Warnig Suppresion Flags
CFLAGS += -Wno-unused-function -Wno-unused-parameter -Wno-cpp
# Alignment Flags
CFLAGS += -falign-jumps -falign-functions -falign-loops -falign-labels
# Optimzation Flags
CFLAGS += -fstrength-reduce -finline-functions

ASM = nasm
ASMFLAGS =

LD = i686-elf-ld
LDFLAGS = -m elf_i386 -relocatable

SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/objs

# Create directories
BUILD_DIRS = $(OBJ_DIR)

SRC_C_FILES = $(shell find $(SRC_DIR) -type f -name "*.c")
SRC_ASM_FILES = $(shell find $(SRC_DIR) -type f -name "*.asm")

OBJ_C_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_C_FILES))
OBJ_ASM_FILES = $(patsubst $(SRC_DIR)/%.asm, $(OBJ_DIR)/%.asm.o, $(SRC_ASM_FILES))

# Build Target
$(TARGET): $(OBJ_C_FILES) $(OBJ_ASM_FILES) | $(BUILD_DIRS)
	@echo "Creating CLib Linkable Object in $@"
	$(LD) $(LDFLAGS) -o $@ $^

# Create directories
$(BUILD_DIRS):
	mkdir -p $@

# Compile Objects
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

clean:
	rm -rf $(BUILD_DIR)