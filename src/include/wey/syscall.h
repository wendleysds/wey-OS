#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include <def/compile.h>
#include <stdint.h>

#define SYSCALL_INTERRUPT_NUM 0x80

#define __MAP0(m, ...)
#define __MAP1(m, t, a, ...) m(t, a)
#define __MAP2(m, t, a, ...) m(t, a), __MAP1(m, __VA_ARGS__)
#define __MAP3(m, t, a, ...) m(t, a), __MAP2(m, __VA_ARGS__)
#define __MAP4(m, t, a, ...) m(t, a), __MAP3(m, __VA_ARGS__)
#define __MAP5(m, t, a, ...) m(t, a), __MAP4(m, __VA_ARGS__)
#define __MAP6(m, t, a, ...) m(t, a), __MAP5(m, __VA_ARGS__)
#define __MAP(n, ...) __MAP##n(__VA_ARGS__)

#define __SC_DECL(t, a) t a
#define __TYPE_AS(t, v) __same_type((t)0, v)
#define __TYPE_IS_LL(t) (__TYPE_AS(t, 0LL) || __TYPE_AS(t, 0ULL))
#define __SC_LONG(t, a) typeof(__builtin_choose_expr(__TYPE_IS_LL(t), 0LL, 0L)) a
#define __SC_CAST(t, a) (t) a
#define __SC_ARGS(t, a) a
#define __SC_TEST(t, a) (void)__non_zero(!__TYPE_IS_LL(t) && sizeof(t) > sizeof(long))

#ifndef __SYSCALL_DEFINEx
#define __SYSCALL_DEFINEx(x, name, ...)                                  \
    asmlinkage long sys##name(__MAP(x, __SC_DECL, __VA_ARGS__))          \
        __attribute__((alias(stringfy(__se_sys##name))));                \
    static inline long __do_sys##name(__MAP(x, __SC_DECL, __VA_ARGS__)); \
    asmlinkage long __se_sys##name(__MAP(x, __SC_LONG, __VA_ARGS__));    \
    asmlinkage long __se_sys##name(__MAP(x, __SC_LONG, __VA_ARGS__))     \
    {                                                                    \
        long ret = __do_sys##name(__MAP(x, __SC_CAST, __VA_ARGS__));     \
        __MAP(x, __SC_TEST, __VA_ARGS__);                                \
        return ret;                                                      \
    }                                                                    \
    static inline long __do_sys##name(__MAP(x, __SC_DECL, __VA_ARGS__))
#endif

#define SYSCALL_DEFINEx(x, sname, ...) \
    __SYSCALL_DEFINEx(x, sname, __VA_ARGS__)

#define SYSCALL_DEFINE0(sname) \
    asmlinkage long sys##name(void)                         \
        __attribute__((alias(stringfy(__se_sys##name))));   \
    asmlinkage long __se_sys##name(void);                   \
    asmlinkage long __se_sys##name(void)

#define SYSCALL_DEFINE1(name, ...) SYSCALL_DEFINEx(1, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE2(name, ...) SYSCALL_DEFINEx(2, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE3(name, ...) SYSCALL_DEFINEx(3, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE4(name, ...) SYSCALL_DEFINEx(4, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE5(name, ...) SYSCALL_DEFINEx(5, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE6(name, ...) SYSCALL_DEFINEx(6, _##name, __VA_ARGS__)

typedef asmlinkage long (*sys_fn_t)(long long, long long, long long, long long, long long, long long);

void syscalls_init();

sys_fn_t _syscall(long no);

#endif