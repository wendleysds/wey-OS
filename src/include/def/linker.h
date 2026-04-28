#ifndef _LINKER_SYMB_H
#define _LINKER_SYMB_H

#ifndef __ASSEMBLY__
extern char __boot_start[];
extern char __boot_end[];
extern char __kernel_text_start[];
extern char __kernel_text_end[];
extern char __kernel_rodata_start[];
extern char __kernel_rodata_end[];
extern char __kernel_data_start[];
extern char __kernel_data_end[];
extern char __init_begin[];
extern char __init_text_start[];
extern char __init_text_end[];
extern char __init_end[];
extern char __kernel_bss_start[];
extern char __kernel_bss_end[];
extern char __kernel_high_start[];
extern char __kernel_size[];
extern char _end_of_kernel_reserve[];
extern char __brk_base[];
extern char __brk_limit[];
extern char _end[];
#else
extern __boot_start
extern __boot_end
extern __kernel_text_start
extern __kernel_text_end
extern __kernel_rodata_start
extern __kernel_rodata_end
extern __kernel_data_start
extern __kernel_data_end
extern __init_begin
extern __init_text_start
extern __init_text_end
extern __init_end
extern __kernel_bss_start
extern __kernel_bss_end
extern __kernel_high_start
extern __kernel_size
extern _end_of_kernel_reserve
extern __brk_base
extern __brk_limit
extern _end
#endif

#endif
