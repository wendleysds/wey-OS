.PHONY: all build clean

TARGET   := bash
LKR      := linker.ld
SRC      := test.asm
OBJ      := $(TARGET).o

ASM      := nasm
ASMFLAGS := -f elf

LD       := i686-elf-ld
LDFLAGS  := -m elf_i386

all: build

build: $(TARGET)

$(OBJ): $(SRC)
	$(ASM) $(ASMFLAGS) -o $@ $<

$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)
