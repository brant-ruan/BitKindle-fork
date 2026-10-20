#ifndef __C1_MATH_BITS_C
#define __C1_MATH_BITS_C
#include <math.h>

/* Exact representation operations: no host CRT, no floating arithmetic,
   no dependence on rounding mode, FTZ/DAZ, or exception masks. */
static unsigned long long __c1_m_sign(int k) {
    return k == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
}

static unsigned long long __c1_m_frexp(unsigned long long bits, int k, int *out) {
    int p = k == 13 ? 23 : 52, bias = k == 13 ? 127 : 1023, shift = 0, field;
    unsigned long long sign = bits & __c1_m_sign(k), mantissa, leading = 1ULL << p;
    field = (int)((bits >> p) & (k == 13 ? 255ULL : 2047ULL));
    mantissa = bits & (leading - 1);
    *out = 0;
    if (field == (k == 13 ? 255 : 2047) || (!field && !mantissa))
        return bits;
    if (!field) {
        while (!(mantissa & leading)) {
            mantissa <<= 1;
            shift++;
        }
        *out = 2 - bias - shift;
    } else
        *out = field - bias + 1;
    return sign | ((unsigned long long)(bias - 1) << p) | (mantissa & (leading - 1));
}

static unsigned long long __c1_m_modf(unsigned long long bits, int k, unsigned long long *whole) {
    int p = k == 13 ? 23 : 52, bias = k == 13 ? 127 : 1023, field, e, shift = 0;
    unsigned long long sign = bits & __c1_m_sign(k), fraction, mask, leading = 1ULL << p;
    field = (int)((bits >> p) & (k == 13 ? 255ULL : 2047ULL));
    e = field - bias;
    *whole = bits;
    if (field == (k == 13 ? 255 : 2047))
        return (bits & (leading - 1)) ? bits : sign;
    if (e < 0) {
        *whole = sign;
        return bits;
    }
    if (e >= p)
        return sign;
    mask = (1ULL << (p - e)) - 1;
    fraction = bits & mask;
    *whole = bits & ~mask;
    if (!fraction)
        return sign;
    while (!(fraction & leading)) {
        fraction <<= 1;
        shift++;
    }
    return sign | ((unsigned long long)(field - shift) << p) | (fraction & (leading - 1));
}

static unsigned long long __c1_m_round(unsigned long long bits, int k, int mode) {
    int p = k == 13 ? 23 : 52, bias = k == 13 ? 127 : 1023, field, e, increment = 0;
    unsigned long long whole, fraction, sign = __c1_m_sign(k);
    field = (int)((bits >> p) & (k == 13 ? 255ULL : 2047ULL));
    if (field == (k == 13 ? 255 : 2047))
        return bits;
    fraction = __c1_m_modf(bits, k, &whole) & (sign - 1);
    if (!fraction || !mode)
        return whole;
    if (mode == -1)
        increment = (bits & sign) != 0;
    if (mode == 1)
        increment = (bits & sign) == 0;
    if (mode == 2)
        increment = fraction >= ((unsigned long long)(bias - 1) << p);
    if (!increment)
        return whole;
    e = field - bias;
    if (e < 0)
        return (bits & sign) | ((unsigned long long)bias << p);
    return whole + (1ULL << (p - e));
}

#define __C1_MATH_IMPL(T,U,S,K) \
T fabs##S(T x){union{T f;U u;}a;a.f=x;a.u=(U)((unsigned long long)a.u&(__c1_m_sign(K)-1));return a.f;} \
T copysign##S(T x,T y){union{T f;U u;}a,b;a.f=x;b.f=y;a.u=(U)(((unsigned long long)a.u&(__c1_m_sign(K)-1))|((unsigned long long)b.u&__c1_m_sign(K)));return a.f;} \
T frexp##S(T x,int *exp){union{T f;U u;}a;a.f=x;a.u=(U)__c1_m_frexp(a.u,K,exp);return a.f;} \
T modf##S(T x,T *ip){union{T f;U u;}a,b;unsigned long long whole;a.f=x;a.u=(U)__c1_m_modf(a.u,K,&whole);b.u=(U)whole;*ip=b.f;return a.f;} \
T floor##S(T x){union{T f;U u;}a;a.f=x;a.u=(U)__c1_m_round(a.u,K,-1);return a.f;} \
T ceil##S(T x){union{T f;U u;}a;a.f=x;a.u=(U)__c1_m_round(a.u,K,1);return a.f;} \
T trunc##S(T x){union{T f;U u;}a;a.f=x;a.u=(U)__c1_m_round(a.u,K,0);return a.f;} \
T round##S(T x){union{T f;U u;}a;a.f=x;a.u=(U)__c1_m_round(a.u,K,2);return a.f;}
__C1_MATH_IMPL(double, unsigned long long, , 14)
__C1_MATH_IMPL(float, unsigned int, f, 13)
__C1_MATH_IMPL(long double, unsigned long long, l, 15)
#undef __C1_MATH_IMPL

#endif
