#ifndef __C1_MATH_ORDER_C
#define __C1_MATH_ORDER_C
#include "math-adjacent.c"

/* C99 7.12.12.2/3: numeric operands take precedence over NaNs.
   Signed-zero ties select +0 for maximum and -0 for minimum. */
static unsigned long long __c1_m_minmax(unsigned long long x, unsigned long long y, int k,
                                        int maximum) {
    unsigned long long sign = __c1_m_sign(k), ax = x & (sign - 1), ay = y & (sign - 1);
    unsigned long long infinity = k == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    unsigned long long quiet = k == 13 ? 0x400000ULL : 0x8000000000000ULL;
    unsigned long long xx = x, yy = y;
    unsigned int order;

    union {
        float f;
        unsigned int u;
    } a, b;

    if (ax > infinity || ay > infinity) {
        if ((ax > infinity && !(ax & quiet)) || (ay > infinity && !(ay & quiet)))
            feraiseexcept(FE_INVALID);
        if (ax > infinity)
            return ay > infinity ? x | quiet : y;
        return x;
    }
    if (!ax && !ay)
        return maximum ? x & y : x | y;
    if (k == 13) {
        a.u = (unsigned int)x;
        b.u = (unsigned int)y;
        xx = __c1_fpkeyf(a.f);
        yy = __c1_fpkeyf(b.f);
    }
    order = __c1_fpcmp(xx, yy);
    return maximum ? (order == 0 ? y : x) : (order == 2 ? y : x);
}

#define __C1_MINMAX_IMPL(T,U,S,K) \
T fmax##S(T x,T y){union{T f;U u;}a,b;a.f=x;b.f=y;a.u=(U)__c1_m_minmax(a.u,b.u,K,1);return a.f;} \
T fmin##S(T x,T y){union{T f;U u;}a,b;a.f=x;b.f=y;a.u=(U)__c1_m_minmax(a.u,b.u,K,0);return a.f;}
__C1_MINMAX_IMPL(double, unsigned long long, , 14)
__C1_MINMAX_IMPL(float, unsigned int, f, 13)
__C1_MINMAX_IMPL(long double, unsigned long long, l, 15)
#undef __C1_MINMAX_IMPL

/* C99 7.12.6.5/11. Chosen ilogb domain values and errno policy. */
static int __c1_m_ilogb(unsigned long long bits, int k) {
    unsigned long long magnitude = bits & (__c1_m_sign(k) - 1);
    unsigned long long infinity = k == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    int exponent;
    if (!magnitude || magnitude >= infinity) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return magnitude ? 2147483647 : FP_ILOGB0;
    }
    __c1_m_frexp(bits, k, &exponent);
    return exponent - 1;
}

#define __C1_LOGB_IMPL(T,U,S,K) \
int ilogb##S(T x){union{T f;U u;}a;a.f=x;return __c1_m_ilogb(a.u,K);} \
T logb##S(T x){ \
 union{T f;U u;}a;unsigned long long magnitude,infinity,quiet;int exponent; \
 a.f=x;magnitude=a.u&(__c1_m_sign(K)-1); \
 infinity=K==13?0x7f800000ULL:0x7ff0000000000000ULL; \
 quiet=K==13?0x400000ULL:0x8000000000000ULL; \
 if(!magnitude){errno=ERANGE;feraiseexcept(FE_DIVBYZERO);a.u=(U)(__c1_m_sign(K)|infinity);return a.f;} \
 if(magnitude==infinity){a.u=(U)infinity;return a.f;} \
 if(magnitude>infinity){if(!(magnitude&quiet))feraiseexcept(FE_INVALID);a.u|=(U)quiet;return a.f;} \
 __c1_m_frexp(a.u,K,&exponent);return (T)(exponent-1); \
}
__C1_LOGB_IMPL(double, unsigned long long, , 14)
__C1_LOGB_IMPL(float, unsigned int, f, 13)
__C1_LOGB_IMPL(long double, unsigned long long, l, 15)
#undef __C1_LOGB_IMPL

#endif
