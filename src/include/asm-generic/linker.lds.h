#define INIT_TEXT \
	*(.init.text .init.text.*) \
	*(.text.startup)

#define EXIT_DATA \
	*(.exit.data .exit.data.*)

#define EXIT_TEXT \
	*(.exit.text) \
	*(.text.exit)

#define EXIT_CALL     \
	*(.exitcall.exit)

#define INIT_TEXT_SECTION(init_text_align) \
	. = ALIGN(init_text_align);                       \
	.init.text : AT(ADDR(.init.text) - LOAD_OFFSET) { \
		__init_text_start = .;                        \
		INIT_TEXT                                     \
		__init_text_end = .;                          \
	}

#define INIT_CALLS_LEVEL(level) \
	__initcall##level##_start = .;    \
	KEEP(*(.initcall##level##.init))  \
	KEEP(*(.initcall##level##s.init)) \

#define INIT_CALLS \
	__initcall_start = .;    \
	INIT_CALLS_LEVEL(0)      \
	INIT_CALLS_LEVEL(1)      \
	INIT_CALLS_LEVEL(2)      \
	INIT_CALLS_LEVEL(3)      \
	INIT_CALLS_LEVEL(4)      \
	INIT_CALLS_LEVEL(5)      \
	INIT_CALLS_LEVEL(rootfs) \
	INIT_CALLS_LEVEL(6)      \
	INIT_CALLS_LEVEL(7)      \
	__initcall_end = .;

#define INIT_DATA \
	*(.init.data .init.data.*)     \
	*(.init.rodata .init.rodata.*)

#define INIT_DATA_SECTION(init_setup_align) \
	.init.data : AT(ADDR(.init.data) - LOAD_OFFSET) { \
		INIT_DATA                                     \
		INIT_CALLS                                    \
	}

#define EXIT_DISCARDS \
	EXIT_TEXT         \
	EXIT_DATA

#define COMMON_DISCARDS \
	*(.discard)         \
	*(.discard.*)       \
	*(.export_symbol)   \
	*(.no_trim_symbol)  \
	*(.modinfo)         \
	*(.gnu.version*) 

#define DISCARDS \
	/DISCARD/ : {       \
		EXIT_DISCARDS   \
		EXIT_CALL       \
		COMMON_DISCARDS \
	}
