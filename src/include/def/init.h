#ifndef _INIT_H
#define _INIT_H

#include <def/compile.h>

#define __init  __section(".init.text")
#define __initdata __section(".init.data")
#define __initconst __section(".init.rodata")

#define __exit __section(".exit.text") __used __cold notrace
#define __exitdata __section(".exit.data")
#define __exit_call __used __section(".exitcall.exit")

#define __define_initcall(fn, level) \
	static initcall_t __initcall_##fn __used \
	__section(".initcall" #level ".init") = fn

#define pure_initcall(fn)      __define_initcall(fn, 0)
#define core_initcall(fn)      __define_initcall(fn, 1)
#define postcore_initcall(fn)  __define_initcall(fn, 2)
#define arch_initcall(fn)      __define_initcall(fn, 3)
#define subsys_initcall(fn)    __define_initcall(fn, 4)
#define fs_initcall(fn) 	   __define_initcall(fn, 5)
#define rootfs_initcall(fn)    __define_initcall(fn, rootfs)
#define device_initcall(fn)    __define_initcall(fn, 6)
#define late_initcall(fn)      __define_initcall(fn, 7)

#define __initcall(fn) device_initcall(fn)
#define __exitcall(fn) \
	static exitcall_t __exitcall_##fn __exit_call = fn

typedef int (*initcall_t)();
typedef void (*exitcall_t)();
typedef initcall_t initcall_entry_t;

// Linker defined symbols
extern initcall_entry_t __initcall_start[];
extern initcall_entry_t __initcall0_start[];
extern initcall_entry_t __initcall1_start[];
extern initcall_entry_t __initcall2_start[];
extern initcall_entry_t __initcall3_start[];
extern initcall_entry_t __initcall4_start[];
extern initcall_entry_t __initcall5_start[];
extern initcall_entry_t __initcall6_start[];
extern initcall_entry_t __initcall7_start[];
extern initcall_entry_t __initcall_end[];

static inline initcall_t initcall_from_entry(initcall_entry_t *entry){
	return *entry;
}

#endif
