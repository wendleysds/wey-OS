#
# Based on linux kernel build (kbuild) system
#
# Will enter recursively into sub-makefiles and compile everything
# in a built-in.o
#
# Overview
#
# make
#  ├─> make -f scripts/Makefile.build obj=src/core
#  │     ├─ Read src/core/Makefile
#  │     ├─ Compile main.o
#  │     ├─ Enter src/core/sched/ if subdir-y += sched/
#  │     │     ├─ Read src/core/sched/Makefile
#  │     │     ├─ Compile sched.o
#  │     │     └─ LD build/objs/core/sched/built-in.o
#  │     └─ LD build/objs/core/built-in.o (main.o + sched/built-in.o)
#  │
#  ├─> make -f scripts/Makefile.build obj=src/memory
#  │     ├─ Compile mmu.o
#  │     ├─ Entra em src/memory/heap/ if subdir-y += sched/
#  │     │     ├─ Compile heap.o, kheap.o
#  │     │     └─ LD build/objs/memory/heap/built-in.o
#  │     └─ LD build/objs/memory/built-in.o (mmu.o + heap/built-in.o)
#  │
#  └─> LD build/bin/kernel.elf

ARCH ?= i386
CROSS_COMPILE ?= i686-elf-

# Tools
AS      := nasm
CC      := $(CROSS_COMPILE)gcc
LD      := $(CROSS_COMPILE)ld
AR      := $(CROSS_COMPILE)ar
NM      := $(CROSS_COMPILE)nm
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

export ARCH CROSS_COMPILE AS CC CPP LD AR NM OBJCOPY OBJDUMP

this-makefile := $(lastword $(MAKEFILE_LIST))
abs_srcroot:= $(realpath $(dir $(this-makefile)))
abs_output := $(CURDIR)

MAKEFLAGS += -rR # Remove make intenal compile rules
MAKEFLAGS += --warn-undefined-variables
#MAKEFLAGS += --always-make

# Verbose
ifeq ("$(origin V)", "command line")
    KBUILD_VERBOSE = $(V)
endif

ifneq ($(findstring 1, $(KBUILD_VERBOSE)),)
    Q =
else
    Q = @
    MAKEFLAGS += --quiet
    MAKEFLAGS += --no-print-directory
endif

export Q KBUILD_VERBOSE

srcroot := $(CURDIR)

ifeq ($(srcroot),$(CURDIR))
    srcroot := .
else ifeq ($(srcroot)/,$(dir $(CURDIR)))
    srcroot := ..
endif

export srcroot

srctree := $(srcroot)/src
export srctree

# Output directory
BUILD_DIR := $(srcroot)/build
OBJ_DIR := $(BUILD_DIR)/objs
BIN_DIR := $(BUILD_DIR)/bin
TOOLS_DIR := $(srcroot)/tools

BUILD_DIRS := $(OBJ_DIR) $(BIN_DIR)
export BUILD_DIR OBJ_DIR BIN_DIR

build := -f $(srcroot)/scripts/Makefile.build obj

export build

KBUILD_LDS := $(srctree)/arch/$(ARCH)/kernel/linker.ld

export KBUILD_LDS

# Flags
UINCLUDE := \

KINCLUDE := \
	-I$(srctree)/arch/$(ARCH)/include \
	-I$(srctree)/include $(UINCLUDE)

KBUILD_CFLAGS = -Wall -Werror -std=gnu11 $(KINCLUDE)

KBUILD_CFLAGS += -m32

ifdef DEBUG_KERNEL
KBUILD_CFLAGS += -g
endif

KBUILD_CFLAGS += -ffreestanding -nostdlib -nostartfiles
KBUILD_CFLAGS += -Wno-unused-function -Wno-unused-parameter \
	-Wno-int-to-pointer-cast -Wno-attribute-alias -Wno-cpp
KBUILD_CFLAGS += -falign-jumps -falign-functions -falign-loops -falign-labels
KBUILD_CFLAGS += -fstrength-reduce -finline-functions

KBUILD_ASFLAGS = -f elf32

KBUILD_LDFLAGS += -m elf_i386 --no-warn-rwx-segments

export KBUILD_CFLAGS KBUILD_ASFLAGS KBUILD_LDFLAGS KINCLUDE

# core-y := block/ core/ drivers/ fs/ init/ init/ lib/ memory/
#
# Main Directories
# dirs := $(foreach dir, $(core-y), $(srcroot)/$(dir))
# 
# # built-in.o targets
# builtins := $(foreach dir, $(core-y), $(OBJ_DIR)/$(dir)built-in.o)
# 
# all: $(BIN_DIR)/kernel.elf
# 
# $(BUILD_DIRS):
# 	$(Q)mkdir -p $@
# 
# $(dirs): $(BUILD_DIRS)
# 	$(Q)$(MAKE) $(build)=$@
# 
# $(builtins): $(dirs)
# 
# $(BIN_DIR)/kernel.elf: $(builtins)
# #	$(Q)$(LD) -T $(KBUILD_LDS) -o $@ $^
# 	@echo "$^ > $@"
# 

isoimage := $(BUILD_DIR)/isoimage

core-y := core/ block/ fs/ memory/ drivers/
lib-y := lib/
arch-y := arch/$(ARCH)/

boot := $(arch-y)boot/

all-y := $(core-y) $(lib-y) $(arch-y)

dirs := $(foreach dir, $(all-y), $(srcroot)/$(dir))
builtins := $(foreach dir, $(all-y), $(OBJ_DIR)/$(dir)built-in.o)

all: $(isoimage)

$(BUILD_DIRS):
	$(Q)mkdir -p $@

$(builtins): $(dirs)

$(dirs): $(BUILD_DIRS)
	$(Q)$(MAKE) $(build)=$@

# ---- ELF Files (Symbols) ----
$(BIN_DIR)/kernel.elf: $(builtins) | $(BIN_DIR)
	@echo "  LD    $@"
	@mkdir -p $(dir $@)
	$(Q)$(LD) $(KBUILD_LDFLAGS) -T $(KBUILD_LDS) -o $@ $^

$(BIN_DIR)/setup.elf: | $(BIN_DIR)
	$(Q)$(MAKE) -f $(srctree)/$(boot)Makefile obj=$(boot) target=$(BIN_DIR)/setup.elf

# ---- Binary Files ----
$(BIN_DIR)/kernel.bin: $(BIN_DIR)/kernel.elf | $(BIN_DIR)
	@echo "  OBJCOPY    $@"
	$(Q)$(OBJCOPY) --pad-to=0x2000 -O binary $< $@

$(BIN_DIR)/setup.bin: $(BIN_DIR)/setup.elf | $(BIN_DIR)
	@echo "  OBJCOPY    $@"
	$(Q)$(OBJCOPY) --pad-to=0x2000 -O binary $< $@

$(isoimage): $(BIN_DIR)/kernel.bin $(BIN_DIR)/setup.bin
	@echo "Creating os image in $@"
	dd if=/dev/zero of=$@ bs=512 count=65536
	$(TOOLS_DIR)/boot/legacy/Install $@
	$(TOOLS_DIR)/fs/fat/main.py init

run: $(isoimage)
	qemu-system-i386 -serial stdio -drive format=raw,file=$(isoimage)

clean:
	@echo "  CLEAN    $(BUILD_DIR)"
	$(Q)rm -rf $(BUILD_DIR)