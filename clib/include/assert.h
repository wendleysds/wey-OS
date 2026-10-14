#ifndef CLIB_ASSERT_H
#define CLIB_ASSERT_H

#ifndef NDEBUG
	void __assert_fail(const char *assertion, const char *file, unsigned int line, const char *function);
	
	#define assert(assertion) \
		((assertion) ? (void)0 : __assert_fail (#assertion, __FILE__, __LINE__, __func__))
#else
	#define assert(assertion) (void)0
#endif

#endif /* CLIB_ASSERT_H */
