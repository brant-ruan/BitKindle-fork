#ifndef __C1_MATH_SQRT_C
#define __C1_MATH_SQRT_C
#include "math-remainder.c"

/* SQRT operates on a normalized positive value. Exact final exponent
   adjustment avoids DAZ/FTZ effects on subnormal inputs. */
static unsigned long long __c1_m_sqrt(unsigned long long bits, int k) {
    int p = k == 13 ? 23 : 52, bias = k == 13 ? 127 : 1023, ex, odd;
    unsigned long long sign = __c1_m_sign(k), infinity = (unsigned long long)(bias * 2 + 1) << p;
    unsigned long long magnitude = bits & (sign - 1), quiet = 1ULL << (p - 1), normalized, result;
    unsigned long long (*leaf)(unsigned long long);
    if (magnitude > infinity) {
        if (!(magnitude & quiet))
            feraiseexcept(FE_INVALID);
        return bits | quiet;
    }
    if (!magnitude)
        return bits;
    if (bits & sign) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return infinity | quiet;
    }
    if (magnitude == infinity)
        return bits;
    normalized = __c1_m_frexp(bits, k, &ex);
    odd = ex & 1;
    if (odd)
        normalized += 1ULL << p;
    if (k == 13)
        leaf = (unsigned long long (*)(
            unsigned long long)) "\x66\x0f\x6e\xc1\xf3\x0f\x51\xc0\x66\x0f\x7e\xc0\xc3";
    else
        leaf = (unsigned long long (*)(
            unsigned long long)) "\x66\x48\x0f\x6e\xc1\xf2\x0f\x51\xc0\x66\x48\x0f\x7e\xc0\xc3";
    result = leaf(normalized);
    return __c1_m_scale(result, k, (ex - odd) / 2);
}

#define __C1_SQRT_IMPL(T,U,S,K) \
T sqrt##S(T x){union{T f;U u;}a;a.f=x;a.u=(U)__c1_m_sqrt(a.u,K);return a.f;}
__C1_SQRT_IMPL(double, unsigned long long, , 14)
__C1_SQRT_IMPL(float, unsigned int, f, 13)
__C1_SQRT_IMPL(long double, unsigned long long, l, 15)
#undef __C1_SQRT_IMPL

#endif
