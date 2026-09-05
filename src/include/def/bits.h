#ifndef BITS_H
#define BITS_H

#define BIT(n)                      (1U << (n))

#define BIT_CHECK(var, n)           (((var) & BIT(n)) != 0)
#define BIT_SET(var, n)             ((var) | BIT(n))
#define BIT_CLEAR(var, n)           ((var) & ~BIT(n))
#define BIT_TOGGLE(var, n)          ((var) ^ BIT(n))

#define BIT_WIDTH(msb, lsb)         ((msb) - (lsb) + 1)
#define BIT_MASK(msb, lsb)          (((1U << BIT_WIDTH(msb, lsb)) - 1U) << (lsb))
#define BIT_FIELD_GET(val, msb, lsb) (((val) & BIT_MASK(msb, lsb)) >> (lsb))
#define BIT_FIELD_SET(val, msb, lsb, new_val) \
    (((val) & ~BIT_MASK(msb, lsb)) | (((new_val) << (lsb)) & BIT_MASK(msb, lsb)))


#define IS_POWER_OF_2(x)            (((x) != 0) && (((x) & ((x) - 1)) == 0))

#endif