LIB_NAME  ?= libc.a
BUILD_DIR ?= build
OBJ_DIR   ?= $(BUILD_DIR)/objs
TARGET    ?= $(BUILD_DIR)/$(LIB_NAME)

CROSS_COMPILE ?=

# Tools
CC      = $(CROSS_COMPILE)gcc
AS      = nasm
AR      = $(CROSS_COMPILE)ar
RANLIB  = $(CROSS_COMPILE)ranlib

# CFLAGS
CFLAGS := -m32 \
    -Iinclude \
    -std=gnu11 \
    -ffreestanding \
    -nostdlib \
    -nostdinc \
    -nostartfiles \
    -nodefaultlibs \
    -Wall \
    -Werror \
    -Wextra \
    -Wno-int-to-pointer-cast \
    -Wno-attribute-alias \
    -Wno-cpp \
    -Wno-unused-function \
    -Wno-unused-parameter \
    -Wno-address-of-packed-member \
    -Wno-implicit-fallthrough \
    -ffunction-sections \
    -fdata-sections \
    -fno-strict-aliasing \
    -falign-jumps \
    -falign-functions \
    -falign-loops \
    -falign-labels \
    -fno-asynchronous-unwind-tables \
    -fno-unwind-tables \
    -fno-ident \
    -fno-pic \
    -fno-stack-protector \
    -fno-omit-frame-pointer \
    -mfpmath=387 \
    -mno-sse \
    -mno-mmx \
    -mno-sse2 \
    -mno-3dnow \
    -mno-avx \
    -mno-sse4a \
    -MMD -MP

ASFLAGS := -f elf32 -Iinclude/

# ARFLAGS
ARFLAGS := rcs

# Sources and Objects
SRC_DIR       := src
SRC_C_FILES   := $(shell find $(SRC_DIR) -type f -name "*.c")
SRC_ASM_FILES := $(shell find $(SRC_DIR) -type f -name "*.asm")

OBJ_C_FILES   := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_C_FILES))
OBJ_ASM_FILES := $(patsubst $(SRC_DIR)/%.asm, $(OBJ_DIR)/%.asm.o, $(SRC_ASM_FILES))
ALL_OBJS      := $(OBJ_C_FILES) $(OBJ_ASM_FILES)

DEPS_C        := $(OBJ_C_FILES:.o=.d)
DEPS_ASM      := $(OBJ_ASM_FILES:.asm.o=.asm.d)
ALL_DEPS      := $(DEPS_C) $(DEPS_ASM)

.PHONY: all clean

all: $(TARGET)

# Create the static library .a containing all isolated objects
$(TARGET): $(ALL_OBJS)
	@mkdir -p $(BUILD_DIR)
	@echo " [AR]  $@"
	@$(AR) $(ARFLAGS) $@ $^

# Compile C code
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo " [CC]  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile Assembly (NASM) com geração de dependências (-MD)
$(OBJ_DIR)/%.asm.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	@echo " [AS]  $<"
	@$(AS) $(ASFLAGS) -MD $(OBJ_DIR)/$*.asm.d $< -o $@

clean:
	@echo " [CLEAN]"
	@rm -rf $(BUILD_DIR)

-include $(ALL_DEPS)