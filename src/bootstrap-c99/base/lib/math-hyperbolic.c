#ifndef __C1_HYPERBOLIC_C
#define __C1_HYPERBOLIC_C
#include "math-log.c"

static struct __c1_dd __c1_hyp_exp(double value, int *scale) {
    struct __c1_dd x, r, ln2, n, term, sum;
    int i;
    *scale = (int)(value * 0x1.71547652b82fep+0 + (value > 0 ? 0.5 : -0.5));
    x.hi = value;
    x.lo = 0;
    n.hi = -(double)*scale;
    n.lo = 0;
    ln2.hi = 0x1.62e42fefa39efp-1;
    ln2.lo = 0x1.abc9e3b39803fp-56;
    r = __c1_dd_add(x, __c1_dd_mul(n, ln2));
    sum.hi = 1;
    sum.lo = 0;
    term = sum;
    for (i = 1; i <= 26; i++) {
        term = __c1_dd_divide(__c1_dd_mul(term, r), i);
        sum = __c1_dd_add(sum, term);
    }
    return sum;
}

static unsigned long long __c1_hyp_bits(unsigned long long input, int type, int kind) {
    union {
        double f;
        unsigned long long u;
    } value;

    union {
        float f;
        unsigned int u;
    } single;

    unsigned long long sign = __c1_m_sign(type), magnitude = input & (sign - 1), result;
    unsigned long long infinity = type == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    unsigned long long quiet = type == 13 ? 0x400000ULL : 0x8000000000000ULL;
    unsigned long long one = type == 13 ? 0x3f800000ULL : 0x3ff0000000000000ULL;
    unsigned long long tiny = type == 13 ? 0x32000000ULL : 0x3e40000000000000ULL;
    unsigned long long minimum = type == 13 ? 0x800000ULL : 0x10000000000000ULL;
    struct __c1_dd x, square, s, c, term, a, b, sum;
    fenv_t saved;
    int negative = (input & sign) != 0 && kind != 1, mode = fegetround(), scale = 0, other, i,
        changed = 0;
    if (magnitude > infinity) {
        if (!(magnitude & quiet))
            feraiseexcept(FE_INVALID);
        return input | quiet;
    }
    if (magnitude == infinity)
        return (kind == 2 ? one : infinity) | (negative ? sign : 0);
    if (!magnitude)
        return kind == 1 ? one : input;
    if (magnitude < tiny) {
        if (kind == 1)
            result = one + (mode == FE_UPWARD);
        else {
            result = magnitude;
            if (kind == 0 &&
                ((!negative && mode == FE_UPWARD) || (negative && mode == FE_DOWNWARD)))
                result++;
            if (kind == 2 && (mode == FE_TOWARDZERO || (!negative && mode == FE_DOWNWARD) ||
                              (negative && mode == FE_UPWARD)))
                result--;
            if (result < minimum) {
                errno = ERANGE;
                feraiseexcept(FE_UNDERFLOW);
            }
            if (negative)
                result |= sign;
        }
        feraiseexcept(FE_INEXACT);
        return result;
    }
    feholdexcept(&saved);
    fesetenv(FE_DFL_ENV);
    if (type == 13) {
        single.u = (unsigned int)magnitude;
        value.f = single.f;
    } else
        value.u = magnitude;
    if (kind == 2 && value.f >= 20) {
        fesetenv(&saved);
        result = one;
        if (mode == FE_TOWARDZERO || (!negative && mode == FE_DOWNWARD) ||
            (negative && mode == FE_UPWARD))
            result--;
        feraiseexcept(FE_INEXACT);
        return result | (negative ? sign : 0);
    }
    if (value.f > 711) {
        fesetenv(&saved);
        return __c1_m_scale(one | (negative ? sign : 0), type, 2000);
    }
    if (value.f < 0.5) {
        x.hi = value.f;
        x.lo = 0;
        square = __c1_dd_mul(x, x);
        s = x;
        term = x;
        for (i = 3; i <= 41; i += 2) {
            term = __c1_dd_divide(__c1_dd_mul(term, square), i * (i - 1));
            s = __c1_dd_add(s, term);
        }
        c.hi = 1;
        c.lo = 0;
        term = c;
        for (i = 2; i <= 40; i += 2) {
            term = __c1_dd_divide(__c1_dd_mul(term, square), i * (i - 1));
            c = __c1_dd_add(c, term);
        }
        sum = kind == 0 ? s : kind == 1 ? c : __c1_dd_ratio(s, c);
    } else {
        a = __c1_hyp_exp(value.f, &scale);
        b.hi = 0;
        b.lo = 0;
        if (value.f < 40) {
            b = __c1_hyp_exp(-value.f, &other);
            b.hi = ldexp(b.hi, other - scale);
            b.lo = ldexp(b.lo, other - scale);
        }
        c = __c1_dd_add(a, b);
        b.hi = -b.hi;
        b.lo = -b.lo;
        s = __c1_dd_add(a, b);
        if (kind == 2) {
            sum = __c1_dd_ratio(s, c);
            scale = 0;
        } else {
            sum = kind == 0 ? s : c;
            scale--;
        }
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

#define __C1_HYP_FUNCTION(T,U,S,K) \
T sinh##S(T x){union{T f;U u;}v;int saved=__c1_math_policy_begin(K);v.f=x;v.u=(U)__c1_hyp_bits(v.u,K,0);v.u=(U)__c1_math_policy_end(v.u,K,saved);return v.f;} \
T cosh##S(T x){union{T f;U u;}v;int saved=__c1_math_policy_begin(K);v.f=x;v.u=(U)__c1_hyp_bits(v.u,K,1);v.u=(U)__c1_math_policy_end(v.u,K,saved);return v.f;} \
T tanh##S(T x){union{T f;U u;}v;int saved=__c1_math_policy_begin(K);v.f=x;v.u=(U)__c1_hyp_bits(v.u,K,2);v.u=(U)__c1_math_policy_end(v.u,K,saved);return v.f;}
__C1_HYP_FUNCTION(double, unsigned long long, , 14)
__C1_HYP_FUNCTION(float, unsigned int, f, 13)
__C1_HYP_FUNCTION(long double, unsigned long long, l, 15)
#undef __C1_HYP_FUNCTION
#endif
