#ifndef _TYPES_H
#define _TYPES_H

#define NULL ((void*)0)

#ifdef __cplusplus
extern "C" {
#endif

typedef __INT8_TYPE__      int8_t;
typedef __UINT8_TYPE__     uint8_t;
typedef __INT16_TYPE__     int16_t;
typedef __UINT16_TYPE__    uint16_t;
typedef __INT32_TYPE__     int32_t;
typedef __UINT32_TYPE__    uint32_t;
typedef __INT64_TYPE__     int64_t;
typedef __UINT64_TYPE__    uint64_t;
typedef __SIZE_TYPE__      size_t;

typedef int8_t   int_least8_t;
typedef uint8_t  uint_least8_t;
typedef int16_t  int_least16_t;
typedef uint16_t uint_least16_t;
typedef int32_t  int_least32_t;
typedef uint32_t uint_least32_t;
typedef int64_t  int_least64_t;
typedef uint64_t uint_least64_t;

typedef __INT_FAST8_TYPE__   int_fast8_t;
typedef __UINT_FAST8_TYPE__  uint_fast8_t;
typedef __INT_FAST16_TYPE__  int_fast16_t;
typedef __UINT_FAST16_TYPE__ uint_fast16_t;
typedef __INT_FAST32_TYPE__  int_fast32_t;
typedef __UINT_FAST32_TYPE__ uint_fast32_t;
typedef __INT_FAST64_TYPE__  int_fast64_t;
typedef __UINT_FAST64_TYPE__ uint_fast64_t;

typedef __INTPTR_TYPE__  intptr_t;
typedef __UINTPTR_TYPE__ uintptr_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

typedef __INTMAX_TYPE__  intmax_t;
typedef __UINTMAX_TYPE__ uintmax_t;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uint64_t phys_addr_t;
typedef uintptr_t virt_addr_t;
typedef __UINTPTR_TYPE__ register_t;

#define INT8_MIN   (-__INT8_C(127) - 1)
#define INT8_MAX   __INT8_C(127)
#define UINT8_MAX  __INT8_C(255)

#define INT16_MIN  (-__INT16_C(32768) - 1)
#define INT16_MAX  __INT16_C(32767)
#define UINT16_MAX __INT16_C(65535)

#define INT32_MIN  (-__INT32_C(2147483647) - 1)
#define INT32_MAX  __INT32_C(2147483647)
#define UINT32_MAX __INT32_C(4294967295)

#define INT64_MIN  (-__INT64_C(9223372036854775807) - 1)
#define INT64_MAX  __INT64_C(9223372036854775807)
#define UINT64_MAX __UINT64_C(18446744073709551615)

#define INTPTR_MIN  __INTPTR_MIN__
#define INTPTR_MAX  __INTPTR_MAX__
#define UINTPTR_MAX __UINTPTR_MAX__

#define INTMAX_MIN  __INTMAX_MIN__
#define INTMAX_MAX  __INTMAX_MAX__
#define UINTMAX_MAX __UINTMAX_MAX__

#define INT8_C(v)  v
#define UINT8_C(v) v##U
#define INT16_C(v) v
#define UINT16_C(v) v##U
#define INT32_C(v) v
#define UINT32_C(v) v##U
#define INT64_C(v) __INT64_C(v)
#define UINT64_C(v) __UINT64_C(v)

// Sanity
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    _Static_assert(sizeof(int8_t)  == 1, "int8_t wrong size");
    _Static_assert(sizeof(int16_t) == 2, "int16_t wrong size");
    _Static_assert(sizeof(int32_t) == 4, "int32_t wrong size");
    _Static_assert(sizeof(int64_t) == 8, "int64_t wrong size");
    _Static_assert(sizeof(uintptr_t) == sizeof(void*), "uintptr_t wrong size");
#endif

#ifdef __cplusplus
}
#endif

#endif
