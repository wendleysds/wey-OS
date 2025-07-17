#ifndef _COMPILE_H
#define _COMPILE_H

#define unlikely(x) __builtin_expect(!!(x), 0)
#define likely(x) __builtin_expect(!!(x), 1)
#define __must_check __attribute__((__warn_unused_result__))

#endif