#ifndef _SYSCALL_H
#define _SYSCALL_H

static inline long syscall(
	long no, long arg1, long agr2, long arg3, long arg4){
		__asm__ volatile (
			"mov %0, %%eax\n\t"
			"mov %1, %%ebx\n\t"
			"mov %2, %%ecx\n\t"
			"mov %3, %%edx\n\t"
			"mov %4, %%esx\n\t"
			"int 0x80\n\t"
			:
			: "r"(no), "r"(arg1), "r"(agr2), "r"(arg3), "r"(arg4)
			: "eax", "ebx", "ecx", "edx", "esx"
		);
	}

#endif