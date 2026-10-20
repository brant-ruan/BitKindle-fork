#ifndef __C1_LOG1P_C
#define __C1_LOG1P_C
#include "math-expm1.c"

static unsigned long long __c1_log1p_bits(unsigned long long input, int type) {
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
    struct __c1_dd x, y, z, square, term, sum, n, ln2, minusone, plusone;
    fenv_t saved;
    int negative = (input & sign) != 0, mode = fegetround(), changed = 0, e = 0, i;
    if (a > infinity) {
        if (!(a & quiet))
            feraiseexcept(FE_INVALID);
        return input | quiet;
    }
    if (negative && a >= one) {
        if (a == one) {
            errno = ERANGE;
            feraiseexcept(FE_DIVBYZERO);
            return sign | infinity;
        }
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return infinity | quiet;
    }
    if (a == infinity || !a)
        return input;
    if (a < tiny) {
        result = a;
        if (!negative && (mode == FE_DOWNWARD || mode == FE_TOWARDZERO))
            result--;
        if (negative && mode == FE_DOWNWARD)
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
    x.hi = v.f;
    x.lo = 0;
    if (v.f > -0.5 && v.f < 0.5) {
        /* log(1+x)=2*atanh(x/(2+x)), retaining the exact denominator. */
        z = __c1_dd_ratio(x, __c1_dd_sum(2, v.f));
    } else {
        y = __c1_dd_sum(1, v.f);
        v.f = y.hi;
        v.u = __c1_m_frexp(v.u, 14, &e);
        y.hi = v.f;
        y.lo = ldexp(y.lo, -e);
        if (y.hi < 0x1.6a09e667f3bcdp-1) {
            y.hi *= 2;
            y.lo *= 2;
            e--;
        }
        minusone.hi = -1;
        minusone.lo = 0;
        plusone.hi = 1;
        plusone.lo = 0;
        z = __c1_dd_ratio(__c1_dd_add(y, minusone), __c1_dd_add(y, plusone));
    }
    square = __c1_dd_mul(z, z);
    term = z;
    sum = z;
    for (i = 3; i <= 75; i += 2) {
        term = __c1_dd_mul(term, square);
        sum = __c1_dd_add(sum, __c1_dd_divide(term, i));
    }
    sum.hi *= 2;
    sum.lo *= 2;
    n.hi = e;
    n.lo = 0;
    ln2.hi = 0x1.62e42fefa39efp-1;
    ln2.lo = 0x1.abc9e3b39803fp-56;
    sum = __c1_dd_add(sum, __c1_dd_mul(n, ln2));
    if (negative) {
        sum.hi = -sum.hi;
        sum.lo = -sum.lo;
    }
    fesetenv(&saved);
    if (negative && (mode == FE_UPWARD || mode == FE_DOWNWARD)) {
        fesetround(mode == FE_UPWARD ? FE_DOWNWARD : FE_UPWARD);
        changed = 1;
    }
    result = __c1_exp_pack(sum, 0, type);
    if (changed)
        fesetround(mode);
    return result | (negative ? sign : 0);
}

#define __C1_LOG1P_FUNCTION(T,U,S,K) \
T log1p##S(T x){union{T f;U u;}v;v.f=x;v.u=(U)__c1_log1p_bits(v.u,K);return v.f;}
__C1_LOG1P_FUNCTION(double, unsigned long long, , 14)
__C1_LOG1P_FUNCTION(float, unsigned int, f, 13)
__C1_LOG1P_FUNCTION(long double, unsigned long long, l, 15)
#undef __C1_LOG1P_FUNCTION
#endif
