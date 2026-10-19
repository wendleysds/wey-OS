#ifndef _CLIB_LIMITS_H
#define _CLIB_LIMITS_H

/* Signed / Unsigned Char */
#define SCHAR_MAX __SCHAR_MAX__
#define SCHAR_MIN (-__SCHAR_MAX__ - 1)
#define UCHAR_MAX (__SCHAR_MAX__ * 2 + 1)

#ifdef __CHAR_UNSIGNED__
  #define CHAR_MAX UCHAR_MAX
  #define CHAR_MIN 0
#else
  #define CHAR_MAX SCHAR_MAX
  #define CHAR_MIN SCHAR_MIN
#endif

/* Short */
#define SHRT_MAX __SHRT_MAX__
#define SHRT_MIN (-__SHRT_MAX__ - 1)
#define USHRT_MAX (__SHRT_MAX__ * 2U + 1U)

/* Int */
#define INT_MAX __INT_MAX__
#define INT_MIN (-__INT_MAX__ - 1)
#define UINT_MAX (__INT_MAX__ * 2U + 1U)

/* Long */
#define LONG_MAX __LONG_MAX__
#define LONG_MIN (-__LONG_MAX__ - 1L)
#define ULONG_MAX (__LONG_MAX__ * 2UL + 1UL)

/* Long Long */
#define LLONG_MAX __LONG_LONG_MAX__
#define LLONG_MIN (-__LONG_LONG_MAX__ - 1LL)
#define ULLONG_MAX (__LONG_LONG_MAX__ * 2ULL + 1ULL)

#endif /* _CLIB_LIMITS_H */
