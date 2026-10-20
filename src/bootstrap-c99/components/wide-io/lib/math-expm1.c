#ifndef __C1_EXPM1_C
#define __C1_EXPM1_C
#include "math-hyperbolic.c"

static unsigned long long __c1_expm1_bits(unsigned long long input, int type) {
    union {
        double f;
        unsigned long long u;
    } v;

    union {
        float f;
        unsigned int u;
    } f;

    unsigned long long sign = __c1_m_sign(type), a = input & (sign - 1), result;
    unsigned long long infinity = type == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    unsigned long long quiet = type == 13 ? 0x400000ULL : 0x8000000000000ULL;
    unsigned long long one = type == 13 ? 0x3f800000ULL : 0x3ff0000000000000ULL;
    unsigned long long tiny = type == 13 ? 0x33000000ULL : 0x3c90000000000000ULL;
    struct __c1_dd sum, term, x, minusone;
    fenv_t saved;
    int negative = (input & sign) != 0, mode = fegetround(), changed = 0, scale = 0, i;
    if (a > infinity) {
        if (!(a & quiet))
            feraiseexcept(FE_INVALID);
        return input | quiet;
    }
    if (a == infinity)
        return negative ? sign | one : infinity;
    if (!a)
        return input;
    if (a < tiny) {
        result = a;
        if (negative && (mode == FE_UPWARD || mode == FE_TOWARDZERO))
            result--;
        if (!negative && mode == FE_UPWARD)
            result++;
        if (result < (type == 13 ? 0x800000ULL : 0x10000000000000ULL)) {
            errno = ERANGE;
            feraiseexcept(FE_UNDERFLOW);
        }
        feraiseexcept(FE_INEXACT);
        return result | (input & sign);
    }
    feholdexcept(&saved);
    fesetenv(FE_DFL_ENV);
    if (type == 13) {
        f.u = (unsigned int)input;
        v.f = f.f;
    } else
        v.u = input;
    if (v.f < -40) {
        fesetenv(&saved);
        feraiseexcept(FE_INEXACT);
        return sign | (one - ((mode == FE_UPWARD || mode == FE_TOWARDZERO) ? 1 : 0));
    }
    if (v.f > 710) {
        fesetenv(&saved);
        return __c1_m_scale(one, type, 2000);
    }
    if (v.f > -0.5 && v.f < 0.5) {
        x.hi = v.f;
        x.lo = 0;
        sum = x;
        term = x;
        for (i = 2; i <= 40; i++) {
            term = __c1_dd_divide(__c1_dd_mul(term, x), i);
            sum = __c1_dd_add(sum, term);
        }
    } else {
        sum = __c1_hyp_exp(v.f, &scale);
        minusone.hi = -ldexp(1, -scale);
        minusone.lo = 0;
        sum = __c1_dd_add(sum, minusone);
    }
    if (negative) {
        sum.hi = -sum.hi;
        sum.lo = -sum.lo;
    }
    fesetenv(&saved);
    if (negative && (mode == FE_UPWARD || mode == FE_DOWNWARD)) {
        fesetround(mode == FE_UPWARD ? FE_DOWNWARD : FE_UPWARD);
        changed = 1;
    }
    result = __c1_exp_pack(sum, scale, type);
    if (changed)
        fesetround(mode);
    return result | (negative ? sign : 0);
}

#define __C1_EXPM1_FUNCTION(T,U,S,K) \
T expm1##S(T x){union{T f;U u;}v;v.f=x;v.u=(U)__c1_expm1_bits(v.u,K);return v.f;}
__C1_EXPM1_FUNCTION(double, unsigned long long, , 14)
__C1_EXPM1_FUNCTION(float, unsigned int, f, 13)
__C1_EXPM1_FUNCTION(long double, unsigned long long, l, 15)
#undef __C1_EXPM1_FUNCTION
#endif
