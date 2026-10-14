#ifndef _CLIB_FLOAT_H
#define _CLIB_FLOAT_H

#define FLT_ROUNDS          (__builtin_flt_rounds())
#define FLT_EVAL_METHOD     (__builtin_flt_eval_method())

#ifndef INFINITY
#define INFINITY            (__builtin_inff())
#endif

#ifndef NAN
#define NAN                 (__builtin_nanf(""))
#endif

#define FLT_RADIX           2

/* Float (Single Precision)                                                   */
#define FLT_MANT_DIG        24
#define FLT_DIG             6
#define FLT_MIN_EXP         (-126)
#define FLT_MIN_10_EXP      (-37)
#define FLT_MAX_EXP         128
#define FLT_MAX_10_EXP      38

#define FLT_MIN             1.1754943508222875e-38F
#define FLT_MAX             3.4028234663852887e+38F
#define FLT_EPSILON         1.1920928955078125e-07F

/* C11 / Subnormais */
#define FLT_TRUE_MIN        1.40129846432481707092e-45F
#define FLT_HAS_SUBNORM     1
#define FLT_DECIMAL_DIG     9

/* Double (Double Precision)                                                  */
#define DBL_MANT_DIG        53
#define DBL_DIG             15
#define DBL_MIN_EXP         (-1022)
#define DBL_MIN_10_EXP      (-308)
#define DBL_MAX_EXP         1024
#define DBL_MAX_10_EXP      38

#define DBL_MIN             2.2250738585072014e-308
#define DBL_MAX             1.7976931348623157e+308
#define DBL_EPSILON         2.2204460492503131e-16

#define DBL_TRUE_MIN        4.94065645841246544177e-324
#define DBL_HAS_SUBNORM     1
#define DBL_DECIMAL_DIG     17

/* Long Double (Extended Precision - 80 bits in x86/x86_64)                   */
#define LDBL_MANT_DIG       64
#define LDBL_DIG            18
#define LDBL_MIN_EXP        (-16382)
#define LDBL_MIN_10_EXP     (-4931)
#define LDBL_MAX_EXP        16384
#define LDBL_MAX_10_EXP     4932

#define LDBL_MIN            3.36210314311209350626e-4932L
#define LDBL_MAX            1.18973149535723176505e+4932L
#define LDBL_EPSILON        1.08420217248550443401e-19L

#define LDBL_TRUE_MIN       3.64519958461062958614e-4951L
#define LDBL_HAS_SUBNORM    1
#define LDBL_DECIMAL_DIG    21

#define DECIMAL_DIG         LDBL_DECIMAL_DIG

#endif /* _CLIB_FLOAT_H */