#ifndef _COMPILE_H
#define _COMPILE_H

#define unlikely(x) __builtin_expect(!!(x), 0)
#define likely(x) __builtin_expect(!!(x), 1)
#define __must_check __attribute__((__warn_unused_result__))
#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#define __non_zero(e) (sizeof(char[1 - 2 * !!(e)]) - 1)

#ifndef __builtin_choose_expr
#define __builtin_choose_expr(cond, if_true, if_false) ((cond) ? (if_true) : (if_false))
#endif

#define __no_return __attribute__((noreturn))

#define asmlinkage __attribute__((regparm(0)))
#define stringfy(s) #s

#define __packed __attribute__((packed))
#define __cdecl __attribute__((cdecl))

#endif