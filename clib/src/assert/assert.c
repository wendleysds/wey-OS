#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void __assert_fail(const char *assertion, const char *file, unsigned int line, const char *function) {
	fprintf(stderr, "Assertion failed: %s, at file %s:%u in function %s\n", assertion, file, line, function);
	abort();
}