#include <lib/assert.h>

__no_return void __assert_fail(
	const char* expr,
	const char* file,
	int line,
	const char* func
){
	panic(
		"Assertion failed: (%s)\n"
		"  at %s:%d\n"
		"  in %s()\n",
		expr, file, line, func
	);
}
