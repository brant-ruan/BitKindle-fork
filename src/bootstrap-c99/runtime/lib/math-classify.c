#ifndef __C1_MATH_CLASSIFY_C
#define __C1_MATH_CLASSIFY_C
#include <math.h>

/* Inspect the argument's semantic representation without FP arithmetic.
   The macro dispatch keeps float subnormals in binary32 rather than
   promoting them to a normal binary64 value before classification. */
int __c1_fpclassifyf(float x) {
    union {
        float f;
        unsigned int u;
    } a;

    unsigned int magnitude;
    a.f = x;
    magnitude = a.u & 0x7fffffffU;
    if (magnitude > 0x7f800000U)
        return FP_NAN;
    if (magnitude == 0x7f800000U)
        return FP_INFINITE;
    if (!magnitude)
        return FP_ZERO;
    if (magnitude < 0x00800000U)
        return FP_SUBNORMAL;
    return FP_NORMAL;
}

int __c1_fpclassifyl(long double x) {
    union {
        long double f;
        unsigned long long u;
    } a;

    unsigned long long magnitude;
    a.f = x;
    magnitude = a.u & 0x7fffffffffffffffULL;
    if (magnitude > 0x7ff0000000000000ULL)
        return FP_NAN;
    if (magnitude == 0x7ff0000000000000ULL)
        return FP_INFINITE;
    if (!magnitude)
        return FP_ZERO;
    if (magnitude < 0x0010000000000000ULL)
        return FP_SUBNORMAL;
    return FP_NORMAL;
}

int __c1_signbitf(float x) {
    union {
        float f;
        unsigned int u;
    } a;

    a.f = x;
    return (int)(a.u >> 31);
}

int __c1_signbitl(long double x) {
    union {
        long double f;
        unsigned long long u;
    } a;

    a.f = x;
    return (int)(a.u >> 63);
}

/* Widen binary32 using integer fields, including subnormals and NaNs.
   A hardware float->double conversion could signal for sNaN or honor DAZ. */
unsigned long long __c1_fpkeyf(float x) {
    union {
        float f;
        unsigned int u;
    } a;

    unsigned long long sign;
    unsigned int field, fraction;
    int exponent;
    a.f = x;
    sign = (unsigned long long)(a.u >> 31) << 63;
    field = (a.u >> 23) & 255;
    fraction = a.u & 0x7fffffU;
    if (field == 255)
        return sign | 0x7ff0000000000000ULL | ((unsigned long long)fraction << 29);
    if (!field) {
        if (!fraction)
            return sign;
        exponent = -126;
        while (!(fraction & 0x800000U)) {
            fraction <<= 1;
            exponent--;
        }
        field = (unsigned int)(exponent + 1023);
        fraction &= 0x7fffffU;
    } else
        field += 896;
    return sign | ((unsigned long long)field << 52) | ((unsigned long long)fraction << 29);
}

unsigned long long __c1_fpkeyl(long double x) {
    union {
        long double f;
        unsigned long long u;
    } a;

    a.f = x;
    return a.u;
}

/* 0 less, 1 equal, 2 greater, 3 unordered. */
unsigned int __c1_fpcmp(unsigned long long a, unsigned long long b) {
    unsigned long long aa = a & 0x7fffffffffffffffULL, bb = b & 0x7fffffffffffffffULL;
    if (aa > 0x7ff0000000000000ULL || bb > 0x7ff0000000000000ULL)
        return 3;
    if (a == b || (!aa && !bb))
        return 1;
    if ((a ^ b) >> 63)
        return (a >> 63) ? 0 : 2;
    if (a >> 63)
        return aa > bb ? 0 : 2;
    return aa < bb ? 0 : 2;
}

#endif
