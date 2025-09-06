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
LDFLAGS= 

SRC_DIR = src
BUILD_DIR = build