#ifndef __C1_MATH_SCALE_C
#define __C1_MATH_SCALE_C
#include "math-bits.c"
#include <errno.h>
#include <fenv.h>
#include "math-policy.h"

/* One rounding at the normal/subnormal boundary. Exponent arithmetic is
   widened before addition, including INT_MIN/LONG_MIN inputs on Windows. */
static unsigned long long __c1_m_scale_policy(unsigned long long bits, int k, long long n,
                                              int c89) {
    int p = k == 13 ? 23 : 52, bias = k == 13 ? 127 : 1023, maximum = k == 13 ? 255 : 2047;
    int exponent, mode, negative, increment = 0;
    long long target, shift;
    unsigned long long sign = __c1_m_sign(k), leading = 1ULL << p;
    unsigned long long magnitude = bits & (sign - 1), fraction, normalized, q, rest, half;
    if (!magnitude || (magnitude >> p) == (unsigned int)maximum)
        return bits;
    negative = (bits & sign) != 0;
    normalized = __c1_m_frexp(bits, k, &exponent);
    fraction = normalized & (leading - 1);
    target = (long long)exponent + n + bias - 1;
    if (target > 0 && target < maximum)
        return (bits & sign) | ((unsigned long long)target << p) | fraction;
    mode = fegetround();
    if (target >= maximum) {
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW | FE_INEXACT);
        if (c89)
            return (bits & sign) | ((unsigned long long)maximum << p);
        if (mode == FE_TONEAREST || (mode == FE_UPWARD && !negative) ||
            (mode == FE_DOWNWARD && negative))
            return (bits & sign) | ((unsigned long long)maximum << p);
        return (bits & sign) | (((unsigned long long)maximum << p) - 1);
    }
    normalized = fraction | leading;
    shift = 1 - target;
    if (shift >= 64) {
        q = 0;
        rest = normalized;
    } else {
        q = normalized >> shift;
        rest = normalized & ((1ULL << shift) - 1);
    }
    if (rest) {
        if (mode == FE_UPWARD)
            increment = !negative;
        else if (mode == FE_DOWNWARD)
            increment = negative;
        else if (mode == FE_TONEAREST && shift < 64) {
            half = 1ULL << (shift - 1);
            increment = rest > half || (rest == half && (q & 1));
        }
        if (increment)
            q++;
        if (q < leading) {
            errno = ERANGE;
            feraiseexcept(FE_UNDERFLOW | FE_INEXACT);
            if (c89)
                return bits & sign;
        } else
            feraiseexcept(FE_INEXACT);
    }
    return (bits & sign) | q;
}

static unsigned long long __c1_m_scale(unsigned long long bits, int k, long long n) {
    return __c1_m_scale_policy(bits, k, n, 0);
}
#if defined(__C1_C89_MATH)
#define __C1_LDEXP_POLICY(K) ((K)==14)
#else
#define __C1_LDEXP_POLICY(K) 0
#endif
#define __C1_SCALE_IMPL(T,U,S,K) \
T ldexp##S(T x,int n){union{T f;U u;}a;a.f=x;a.u=(U)__c1_m_scale_policy(a.u,K,n,__C1_LDEXP_POLICY(K));return a.f;} \
T scalbn##S(T x,int n){union{T f;U u;}a;a.f=x;a.u=(U)__c1_m_scale(a.u,K,n);return a.f;} \
T scalbln##S(T x,long n){union{T f;U u;}a;a.f=x;a.u=(U)__c1_m_scale(a.u,K,n);return a.f;}
__C1_SCALE_IMPL(double, unsigned long long, , 14)
__C1_SCALE_IMPL(float, unsigned int, f, 13)
__C1_SCALE_IMPL(long double, unsigned long long, l, 15)
#undef __C1_SCALE_IMPL
#undef __C1_LDEXP_POLICY

#endif
