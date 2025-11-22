obj-y += main.o printf.o regs.o video.o
obj-y += pm.asm.o bioscall.asm.o
obj-y += headerc_s.o

KBUILD_CFLAGS := $(realmode_cflags)
KBUILD_LDFLAGS += -T $(srctree)/$(boot-y)setup.ld

relocable := n
target := $(BIN_DIR)/setup.elf

$(BIN_DIR)/setup.bin: $(BIN_DIR)/setup.elf
	@echo "  OBJCOPY    $@"
	$(Q)$(OBJCOPY) -O binary $< $@
