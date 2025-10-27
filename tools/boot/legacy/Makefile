TARGET := bootloader.bin

ASFLAGS := -f bin
AS := nasm

$(TARGET): loader.asm
	@$(AS) $(ASFLAGS) $^ -o $@