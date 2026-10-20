#ifndef __C1_MATH_ADJACENT_C
#define __C1_MATH_ADJACENT_C
#include "math-scale.c"
#include "math-classify.c"

/* Development extension to the verified math foundation. All decisions use
   integer representations, including the wider direction of nexttowardf.
   Range errors follow C99 7.12.11.3 and WG14 N1630 guidance:
   https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1630.htm */
static unsigned long long __c1_m_next(unsigned long long x, unsigned long long y, int k) {
    unsigned long long sign = __c1_m_sign(k), quiet, infinity, ax, ay, key, result;
    unsigned int order;

    union {
        float f;
        unsigned int u;
    } a;

    quiet = k == 13 ? 0x400000ULL : 0x8000000000000ULL;
    infinity = k == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    ax = x & (sign - 1);
    ay = y & 0x7fffffffffffffffULL;
    if (ax > infinity || ay > 0x7ff0000000000000ULL) {
        if ((ax > infinity && !(ax & quiet)) ||
            (ay > 0x7ff0000000000000ULL && !(ay & 0x8000000000000ULL)))
            feraiseexcept(FE_INVALID);
        if (ax > infinity)
            return x | quiet;
        if (k != 13)
            return y | quiet;
        return ((y >> 63) ? sign : 0) | infinity | quiet | ((ay & 0xfffffffffffffULL) >> 29);
    }
    if (k == 13) {
        a.u = (unsigned int)x;
        key = __c1_fpkeyf(a.f);
    } else
        key = x;
    order = __c1_fpcmp(key, y);
    if (order == 1)
        return (x & (sign - 1)) | ((y >> 63) ? sign : 0);
    if (!ax)
        result = ((y >> 63) ? sign : 0) | 1;
    else if ((order == 0) == !(x & sign))
        result = x + 1;
    else
        result = x - 1;
    ax = result & (sign - 1);
    if (ax == infinity) {
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW | FE_INEXACT);
    } else if (ax < (k == 13 ? 0x800000ULL : 0x10000000000000ULL)) {
        errno = ERANGE;
        feraiseexcept(FE_UNDERFLOW | FE_INEXACT);
    }
    return result;
}

double nextafter(double x, double y) {
    union {
        double f;
        unsigned long long u;
    } a, b;

    a.f = x;
    b.f = y;
    a.u = __c1_m_next(a.u, b.u, 14);
    return a.f;
}

float nextafterf(float x, float y) {
    union {
        float f;
        unsigned int u;
    } a;

    a.f = x;
    a.u = (unsigned int)__c1_m_next(a.u, __c1_fpkeyf(y), 13);
    return a.f;
}

long double nextafterl(long double x, long double y) {
    union {
        long double f;
        unsigned long long u;
    } a, b;

    a.f = x;
    b.f = y;
    a.u = __c1_m_next(a.u, b.u, 15);
    return a.f;
}

double nexttoward(double x, long double y) {
    union {
        double f;
        unsigned long long u;
    } a;

    union {
        long double f;
        unsigned long long u;
    } b;

    a.f = x;
    b.f = y;
    a.u = __c1_m_next(a.u, b.u, 14);
    return a.f;
}

float nexttowardf(float x, long double y) {
    union {
        float f;
        unsigned int u;
    } a;

    union {
        long double f;
        unsigned long long u;
    } b;

    a.f = x;
    b.f = y;
    a.u = (unsigned int)__c1_m_next(a.u, b.u, 13);
    return a.f;
}

long double nexttowardl(long double x, long double y) {
    return nextafterl(x, y);
}

#endif
