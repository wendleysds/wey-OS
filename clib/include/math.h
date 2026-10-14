#ifndef _CLIB_MATH_H
#define _CLIB_MATH_H

typedef double   double_t;
typedef float    float_t;

#define M_PI 3.14159265358979323846
#define M_E  2.71828182845904523536

#define HUGE_VAL  __builtin_huge_val()
#define HUGE_VALF __builtin_huge_valf()
#define HUGE_VALL __builtin_huge_vall()

#ifndef INFINITY
    #define INFINITY  __builtin_inff()
#endif

#ifndef NAN
    #define NAN       __builtin_nanf("")
#endif

/* Single-precision (float) functions */
float powf(float base, float exponent);
float floorf(float x);
float fmodf(float x, float y);
float frexpf(float x, int *exp);
float ldexpf(float x, int exp);
float fabsf(float x);
float expf(float x);
float log10f(float x);
float log2f(float x);
float logf(float x);
float sqrtf(float x);
float ceilf(float x);
float acosf(float x);
float asinf(float x);
float tanf(float x);
float cosf(float x);
float sinf(float x);
float atanf(float x);
float atan2f(float y, float x);

/* Double-precision functions */
double pow(double base, double exponent);
double floor(double x);
double fmod(double x, double y);
double frexp(double x, int *exp);
double ldexp(double x, int exp);
double fabs(double x);
double exp(double x);
double log10(double x);
double log2(double x);
double log(double x);
double sqrt(double x);
double ceil(double x);
double acos(double x);
double asin(double x);
double tan(double x);
double cos(double x);
double sin(double x);
double atan(double x);
double atan2(double y, double x);

#endif /* _CLIB_MATH_H */

