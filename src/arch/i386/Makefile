obj-y += kernel/ mm/ entry/ usr/

realmode_cflags := -ffreestanding -nostdlib -m16 -nostartfiles $(KINCLUDE)

ifdef DEBUG_KERNEL
realmode_cflags += -g
endif

realmode_cflags += -Wall -Werror -std=gnu11 \
	-Wno-int-to-pointer-cast -Wno-attribute-alias -Wno-cpp \
	-Wno-unused-function -Wno-unused-parameter \
	-falign-jumps -falign-functions -falign-loops -falign-labels \
	-fstrength-reduce -finline-functions -march=i386

export realmode_cflags

boot-y := arch/i386/boot/
export boot-y

kernelsecs := $(srctree)/$(boot-y)kernelsecs.h

bzImage: $(BIN_DIR)/kernel.bin $(kernelsecs)
	$(Q)$(MAKE) $(build)=$(boot-y)
	@echo "  GEN        $@"
	(dd if=$(BIN_DIR)/setup.bin bs=4k conv=sync status=none; cat $(BIN_DIR)/kernel.bin) > $(BUILD_DIR)/$@
	@echo "  Kernel Ready: $(BUILD_DIR)/$@"

$(BIN_DIR)/kernel.bin: $(BIN_DIR)/kernel.elf | $(BIN_DIR)
	@echo "  OBJCOPY    $@"
	$(Q)$(OBJCOPY) -O binary $< $@

$(kernelsecs): $(BIN_DIR)/kernel.elf
	@echo "  NM         $@"
	$(Q)$(NM) $^ | sed -n 's/^\([0-9a-fA-F]*\) [A-Za-z] \(__kernel.*\)$$/#define \2 0x\1/p' > $@

$(KBUILD_LDS): $(OBJ_DIR)/arch/i386/built-in.o ;

kallsyms_src := $(OBJ_DIR)/kallsyms_data.c
kallsyms_obj := $(OBJ_DIR)/kallsyms_data.o

$(kallsyms_src): $(KBUILD_LDS) $(core-builtins) | $(BIN_DIR)
	@mkdir -p $(dir $@)
	$(Q)$(LD) $(KBUILD_LDFLAGS) -T $(KBUILD_LDS) -o $(BIN_DIR)/kernel.tmp.elf $(core-builtins)
	@echo "  GEN        kallsyms_data.c"
	$(Q)python3 $(srcroot)/scripts/gen_kallsyms.py $(BIN_DIR)/kernel.tmp.elf $@ $(NM)

$(kallsyms_obj): $(kallsyms_src)
	@echo "  CC         kallsyms_data.c"
	$(Q)$(CC) $(KBUILD_CFLAGS) -c $< -o $@

$(BIN_DIR)/kernel.elf: $(KBUILD_LDS) $(core-builtins) $(kallsyms_obj) | $(BIN_DIR)
	@echo "  LD         $@"
	@mkdir -p $(dir $@)
	$(Q)$(LD) $(KBUILD_LDFLAGS) -T $(KBUILD_LDS) -o $@ $(core-builtins) $(kallsyms_obj)

