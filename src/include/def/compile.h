#ifndef _COMPILE_H
#define _COMPILE_H

#if defined(__GNUC__) || defined(__clang__)
    #define HAS_BUILTINS 1
#else
    #define HAS_BUILTINS 0
#endif

#if HAS_BUILTINS
    #define unlikely(x) __builtin_expect(!!(x), 0)
    #define likely(x)   __builtin_expect(!!(x), 1)
#elif defined(__cplusplus) && (__cplusplus >= 202002L) /* C++20 */
    #define unlikely(x) (x) [[unlikely]]
    #define likely(x)   (x) [[likely]]
#else
    #define unlikely(x) (x)
    #define likely(x)   (x)
#endif

#if HAS_BUILTINS
    #define __must_check __attribute__((__warn_unused_result__))
    #define __no_return  __attribute__((noreturn))
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202311L) /* C23 */
    #define __must_check [[nodiscard]]
    #define __no_return  [[noreturn]]
#else
    #define __must_check
    #define __no_return
#endif

#if HAS_BUILTINS
    #define __used   __attribute__((used))
    #define __unused __attribute__((unused))
    #define __cold   __attribute__((cold))
#else
    #define __used
    #define __unused
    #define __cold
#endif

#if HAS_BUILTINS
    #define __always_inline inline __attribute__((always_inline))
    #define __section(x)    __attribute__((section(x)))
#else
    #define __always_inline inline
    #define __section(x)
#endif

#if HAS_BUILTINS
    #define __packed     __attribute__((packed))
    #define __aligned(x) __attribute__((aligned(x)))
#else
    #define __packed
    #define __aligned(x)
#endif

#if defined(_M_IX86) || defined(__i386__)
    #if HAS_BUILTINS
        #define __cdecl      __attribute__((cdecl))
        #define __regparm(x) __attribute__((regparm(x)))
        #define asmlinkage   __attribute__((regparm(0)))
    #elif defined(_MSC_VER)
        #define __cdecl      __cdecl
        #define __regparm(x)
        #define asmlinkage   __cdecl
    #endif
#else
    #define __cdecl
    #define __regparm(x)
    #define asmlinkage
#endif

#if HAS_BUILTINS
    #define __no_stack_protector __attribute__((__no_stack_protector__))
    #define notrace              __attribute__((no_instrument_function))
#else
    #define __no_stack_protector
    #define notrace
#endif

#if HAS_BUILTINS
    #define __same_type(a, b) __builtin_types_compatible_p(__typeof__(a), __typeof__(b))
    
    #ifndef __builtin_choose_expr
        #define __builtin_choose_expr(cond, if_true, if_false) __builtin_choose_expr(cond, if_true, if_false)
    #endif
#else
    #define __same_type(a, b) (sizeof(a) == sizeof(b)) 
    
    #ifndef __builtin_choose_expr
        #define __builtin_choose_expr(cond, if_true, if_false) ((cond) ? (if_true) : (if_false))
    #endif
#endif

#define __non_zero(e) (sizeof(char[1 - 2 * !!(e)]) - 1)
#define stringfy(s) #s

#endif