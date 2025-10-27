obj-y += entry_32.asm.o syscall.o

all:
	$(srcroot)/tools/syscall/gen_syscalls.sh \
		$(srctree)/arch/$(ARCH)/entry/syscalls/syscall_32.tbl \
		$(srctree)/arch/$(ARCH)/entry/syscalls/syscalltbl.h