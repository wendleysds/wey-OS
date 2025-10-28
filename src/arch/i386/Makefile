obj-y += kernel/ entry/ 

realmode_cflags := -ffreestanding -nostdlib -m16 -nostartfiles $(KINCLUDE)

ifdef DEBUG_KERNEL
realmode_cflags += -g
endif

realmode_cflags += -Wall -Werror -std=gnu11 \
	-Wno-int-to-pointer-cast -Wno-attribute-alias -Wno-cpp \
	-Wno-unused-function -Wno-unused-parameter \
	-falign-jumps -falign-functions -falign-loops -falign-labels \
	-fstrength-reduce -finline-functions

export realmode_cflags

boot-y := arch/i386/boot/
export boot-y

all:
	$(Q)$(MAKE) $(build)=$(boot-y)
