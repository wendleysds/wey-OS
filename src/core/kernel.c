#include "def/compile.h"
#include "def/err.h"
#include <def/init.h>
#include <mm/slab.h>
#include <mm/page.h>
#include <wey/interrupt.h>
#include <wey/mmu.h>
#include <wey/panic.h>

#include <lib/font.h>
#include <wey/terminal_struct.h>
#include <wey/terminal.h>
#include <wey/printk.h>
#include <mm/kheap.h>

#include <uapi/headers.h>

#define halt() do {__asm__ volatile("hlt");}while(1)

extern struct boot_header boot_header;

extern __init void setup_arch();

static inline __no_return void module_failed(const char* module, int res){
	panic("%s failed with status %d!", module, res);
	halt();
}

__no_return void kmain(){
	int res;

	setup_arch();

	interrupts_disable();

	res = PTR_ERR(
		page_init(boot_header.e820_table, boot_header.e820_entries_count)
	);

	if(IS_ERR_VALUE(res)){
		module_failed("page allocator", res);
	}

	slab_init();

	res = mmu_init();
	if(IS_ERR_VALUE(res)){
		module_failed("MMU", res);
	}

	res = terminal_init();
	if(IS_ERR_VALUE(res)){
		module_failed("VTerminal", res);
	}

	interrupts_enable();

	printk("OK");

	halt();

	__builtin_unreachable();
}
