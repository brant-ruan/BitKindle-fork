#ifndef __C1_MATH_REMAINDER_C
#define __C1_MATH_REMAINDER_C
#include "math-scale.c"

/* Exact binary long division. No rounded quotient or floating subtraction. */
static unsigned long long __c1_m_fmod(unsigned long long x, unsigned long long y, int k) {
    int p = k == 13 ? 23 : 52, bias = k == 13 ? 127 : 1023, ex, ey, i, field;
    unsigned long long sign = __c1_m_sign(k), leading = 1ULL << p;
    unsigned long long infinity = (unsigned long long)(bias * 2 + 1) << p;
    unsigned long long ax = x & (sign - 1), ay = y & (sign - 1), mx, my, quiet = leading >> 1;
    if (ax > infinity || ay > infinity) {
        if ((ax > infinity && !(ax & quiet)) || (ay > infinity && !(ay & quiet)))
            feraiseexcept(FE_INVALID);
        return (ax > infinity ? x : y) | quiet;
    }
    if (ax == infinity || !ay) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return infinity | quiet;
    }
    if (ax < ay)
        return x;
    if (ax == ay)
        return x & sign;
    mx = (__c1_m_frexp(x, k, &ex) & (leading - 1)) | leading;
    my = (__c1_m_frexp(y, k, &ey) & (leading - 1)) | leading;
    for (i = ex - ey; i >= 0; i--) {
        if (mx >= my)
            mx -= my;
        if (!mx)
            return x & sign;
        if (i)
            mx <<= 1;
    }
    field = ey + bias - 1;
    while (!(mx & leading)) {
        mx <<= 1;
        field--;
    }
    if (field > 0)
        return (x & sign) | ((unsigned long long)field << p) | (mx & (leading - 1));
    return (x & sign) | (mx >> (1 - field));
}

#define __C1_FMOD_IMPL(T,U,S,K) \
T fmod##S(T x,T y){union{T f;U u;}a,b;a.f=x;b.f=y;a.u=(U)__c1_m_fmod(a.u,b.u,K);return a.f;}
__C1_FMOD_IMPL(double, unsigned long long, , 14)
__C1_FMOD_IMPL(float, unsigned int, f, 13)
__C1_FMOD_IMPL(long double, unsigned long long, l, 15)
#undef __C1_FMOD_IMPL

#endif
