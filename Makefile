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
CPP     = $(CC) -E
AS      = nasm
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
AR      = $(CROSS_COMPILE)ar
NM      = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

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
BUILD_DIR ?= $(srcroot)/build
OBJ_DIR := $(BUILD_DIR)/objs
BIN_DIR := $(BUILD_DIR)/bin
TOOLS_DIR := $(srcroot)/tools

BUILD_DIRS := $(OBJ_DIR) $(BIN_DIR)
export BUILD_DIR OBJ_DIR BIN_DIR

build := -f $(srcroot)/scripts/Makefile.build obj

export build

KBUILD_LDS := $(OBJ_DIR)/arch/$(ARCH)/kernel/linker.lds

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

# --------- Rules ---------------

#core-y := core/ block/  memory/ drivers/
core-y := core/ memory/ drivers/ fs/
lib-y := lib/
arch-y := arch/$(ARCH)/

all-y := $(core-y) $(lib-y) $(arch-y)

dirs := $(foreach dir, $(all-y), $(srcroot)/$(dir))
core-builtins := $(foreach dir, $(all-y), $(OBJ_DIR)/$(dir)built-in.o)

export core-builtins

all: bzImage

$(BUILD_DIRS):
	$(Q)mkdir -p $@

$(dirs): chksyscalls $(BUILD_DIRS)
	$(Q)$(MAKE) $(build)=$@

bzImage: $(dirs)

isoimage:
	@echo "not supported yet"

cdrom:
	@echo "not supported yet"

chksyscalls:
	@echo "  CALL       $(srcroot)/scripts/chksyscalls.sh"
	$(srcroot)/scripts/chksyscalls.sh \
		$(srctree)/arch/$(ARCH)/entry/syscalls/syscall_32.tbl \
		$(srctree)/arch/$(ARCH)/entry/syscalls/syscalltbl.h

clean:
	@echo "  CLEAN    $(BUILD_DIR)"
	$(Q)rm -rf $(BUILD_DIR)