.set MAGIC, 0x1badb002
.set FLAGS, (1<<0 | 1<<1)
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
	.long MAGIC
	.long FLAGS
	.long CHECKSUM

.section .text
.extern kmain
.global loader

loader:
	lea kernel_stack_end, %esp
	push %eax
	push %ebx
	call kmain

_stop:
	cli
	hlt
	jmp _stop

.section .bss
	.align 16
	.space 2*1024*1024
kernel_stack:
	.skip 8192
kernel_stack_end:
