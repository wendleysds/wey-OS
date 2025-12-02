TARGET_PLATFORM ?= bios
CROSS_COMPILE ?= i686-elf-

# tools

CPP     = $(CC) -E
AS      = nasm
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
AR      = $(CROSS_COMPILE)ar
NM      = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

# dirs

SRC_DIR = src

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR:=$(BUILD_DIR)/bin

BUILD_DIRS := $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR)

# Flags

KINCLUDES := -I../../../src/include
INCLUDE := -Iinclude -I$(SRC_DIR)/$(TARGET_PLATFORM)/include

CFLAGS := -ffreestanding -nostdlib -m16 -march=i386 -nostartfiles \
        $(KINCLUDES) $(INCLUDE)

CFLAGS += -Wall -Werror -std=gnu11 \
          -Wno-int-to-pointer-cast \
          -Wno-attribute-alias -Wno-cpp \
          -Wno-unused-function -Wno-unused-parameter

CFLAGS += -falign-jumps -falign-functions -falign-loops \
          -falign-labels -fstrength-reduce -finline-functions

ifeq ($(TARGET_PLATFORM), bios)
    CFLAGS += -D_BIOS
else ifeq ($(TARGET_PLATFORM), uefi)
    CFLAGS += -D_UEFI
endif

ASFLAGS := -f elf32

LDFLAGS := -m elf_i386 --no-warn-rwx-segments

# files

STAGE1_FILE = $(SRC_DIR)/$(TARGET_PLATFORM)/stage1.asm
STAGE1_BIN = $(BIN_DIR)/stage1.bin

STAGE2_ELF = $(BIN_DIR)/stage2.elf
STAGE2_BIN = $(BIN_DIR)/stage2.bin

LD_FILE = linker.ld

C_FILES   :=
ASM_FILES :=

ifeq ($(TARGET_PLATFORM), bios)
    C_FILES   = $(shell find $(SRC_DIR) -type f -name "*.c" ! -path "$(SRC_DIR)/uefi/*")
	ASM_FILES =  $(filter-out $(STAGE1_FILE), $(shell find $(SRC_DIR) -type f -name "*.asm" ! -path "$(SRC_DIR)/uefi/*"))
else ifeq ($(TARGET_PLATFORM), uefi)
    C_FILES   = $(shell find $(SRC_DIR) -type f -name "*.c" ! -path "$(SRC_DIR)/bios/*)
    ASM_FILES =  $(filter-out $(STAGE1_FILE), $(shell find $(SRC_DIR) -type f -name "*.asm" ! -path "$(SRC_DIR)/bios/*"))
else
	@echo "Unknow Platform!"
	exit(1)
endif

C_OBJS   = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(C_FILES))
ASM_OBJS = $(patsubst $(SRC_DIR)/%.asm, $(OBJ_DIR)/%.asm.o, $(ASM_FILES))

# Compile rules


all: $(BUILD_DIRS) $(STAGE1_BIN) $(STAGE2_BIN)
	

$(BUILD_DIRS):
	@mkdir $@


$(STAGE1_BIN): $(STAGE1_FILE)
	$(AS) -f bin -o $@ $<

$(STAGE2_ELF): $(C_OBJS) $(ASM_OBJS)
	$(LD) $(LDFLAGS) -T $(LD_FILE) -o $@ $^

$(STAGE2_BIN): $(STAGE2_ELF)
	$(OBJCOPY) -O binary $< $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.asm.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $< 

r: all
	qemu-system-i386 -drive format=raw,file=$(IMG)

d: all
	qemu-system-i386 -drive format=raw,file=$(IMG) -s -S &

clean:
	rm -rf $(BUILD_DIR)
