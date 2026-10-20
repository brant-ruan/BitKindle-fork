#ifndef __C1_MATH_ROUNDING_C
#define __C1_MATH_ROUNDING_C
#include "math-order.c"

static unsigned long long __c1_m_rint(unsigned long long bits, int k, int report) {
    int p = k == 13 ? 23 : 52, bias = k == 13 ? 127 : 1023, field, shift, mode;
    unsigned long long sign = __c1_m_sign(k), mag = bits & (sign - 1), result, frac, half;
    unsigned long long infinity = k == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    unsigned long long quiet = 1ULL << (p - 1);
    if (mag >= infinity) {
        if (mag > infinity) {
            if (!(mag & quiet))
                feraiseexcept(FE_INVALID);
            return bits | quiet;
        }
        return bits;
    }
    field = (int)(mag >> p);
    if (!mag || field >= bias + p)
        return bits;
    mode = fegetround() >> 13;
    if (mode == 1)
        result = __c1_m_round(bits, k, -1);
    else if (mode == 2)
        result = __c1_m_round(bits, k, 1);
    else if (mode == 3)
        result = __c1_m_round(bits, k, 0);
    else if (field < bias) {
        half = (unsigned long long)(bias - 1) << p;
        result = (bits & sign) | (mag > half ? ((unsigned long long)bias << p) : 0);
    } else {
        shift = p - (field - bias);
        half = 1ULL << (shift - 1);
        frac = mag & ((1ULL << shift) - 1);
        result = mag & ~((1ULL << shift) - 1);
        if (frac > half || (frac == half && ((mag >> shift) & 1)))
            result += 1ULL << shift;
        result |= bits & sign;
    }
    if (report && result != bits)
        feraiseexcept(FE_INEXACT);
    return result;
}

static unsigned long long __c1_m_round_integer(unsigned long long bits, int k, int width,
                                               int away) {
    int p = k == 13 ? 23 : 52, bias = k == 13 ? 127 : 1023, exponent;
    unsigned long long sign = __c1_m_sign(k), rounded, magnitude, limit, value;
    rounded = away ? __c1_m_round(bits, k, 2) : __c1_m_rint(bits, k, 0);
    magnitude = rounded & (sign - 1);
    limit = (unsigned long long)(bias + width - 1) << p;
    if ((rounded & sign) ? magnitude > limit : magnitude >= limit) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return width == 64 ? 0x8000000000000000ULL : 0xffffffff80000000ULL;
    }
    if (!magnitude)
        value = 0;
    else {
        exponent = (int)(magnitude >> p) - bias;
        value = (magnitude & ((1ULL << p) - 1)) | (1ULL << p);
        if (exponent >= p)
            value <<= exponent - p;
        else
            value >>= p - exponent;
        if (rounded & sign)
            value = 0ULL - value;
    }
    if (!away && rounded != bits)
        feraiseexcept(FE_INEXACT);
    return value;
}

#define __C1_RINT_IMPL(T,U,S,K) \
T rint##S(T x){union{T f;U u;}a;a.f=x;a.u=(U)__c1_m_rint(a.u,K,1);return a.f;} \
T nearbyint##S(T x){union{T f;U u;}a;a.f=x;a.u=(U)__c1_m_rint(a.u,K,0);return a.f;} \
long lrint##S(T x){union{T f;U u;}a;a.f=x;return (long)__c1_m_round_integer(a.u,K,32,0);} \
long long llrint##S(T x){union{T f;U u;}a;a.f=x;return (long long)__c1_m_round_integer(a.u,K,64,0);} \
long lround##S(T x){union{T f;U u;}a;a.f=x;return (long)__c1_m_round_integer(a.u,K,32,1);} \
long long llround##S(T x){union{T f;U u;}a;a.f=x;return (long long)__c1_m_round_integer(a.u,K,64,1);}
__C1_RINT_IMPL(double, unsigned long long, , 14)
__C1_RINT_IMPL(float, unsigned int, f, 13)
__C1_RINT_IMPL(long double, unsigned long long, l, 15)
#undef __C1_RINT_IMPL

#endif
