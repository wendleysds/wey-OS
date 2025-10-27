src := $(srctree)/$(obj)

ccflags := -ffreestanding -nostdlib -nostartfiles $(KINCLUDE)
ccflags += -Wall -Werror -std=gnu11 \
	-Wno-int-to-pointer-cast -Wno-attribute-alias -Wno-cpp \
	-Wno-unused-function -Wno-unused-parameter \
	-falign-jumps -falign-functions -falign-loops -falign-labels \
	-fstrength-reduce -finline-functions

ccflags += -m16

ifdef DEBUG_KERNEL
ccflags += -g
endif

obj-y := main.o bioscall.asm.o pm.asm.o printf.o regs.o video.o
subdir-y := 

$(OBJ_DIR)/$(obj)%.o: $(src)%.c
	@echo "  CC    $<"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(ccflags) -c $< -o $@

$(OBJ_DIR)/$(obj)%.asm.o: $(src)%.asm
	@echo "  AS    $<"
	@mkdir -p $(dir $@)
	$(Q)$(AS) $(KBUILD_ASFLAGS) -o $@ $<


subdir-obj := $(foreach dir, $(subdir-y), $(obj)$(dir)built-in.o)
obj-y := $(addprefix $(OBJ_DIR)/$(obj), $(obj-y))

subdir-files := $(addprefix $(OBJ_DIR)/, $(subdir-obj))

compile-objs := $(sort $(obj-y) $(subdir-files))

all: $(subdir-obj) $(target)

$(target): $(compile-objs)
	@echo "  LD    $@"
	@mkdir -p $(dir $@)
	$(Q)$(LD) $(KBUILD_LDFLAGS) -T $(src)setup.ld -o $@ $(compile-objs)

$(subdir-obj):
	$(Q)$(MAKE) $(build)=$(dir $@)