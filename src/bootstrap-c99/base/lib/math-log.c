#ifndef __C1_LOG_C
#define __C1_LOG_C
#include "math-exp.c"

static struct __c1_dd __c1_dd_ratio(struct __c1_dd a, struct __c1_dd b) {
    struct __c1_dd q, p, r;
    q.hi = a.hi / b.hi;
    q.lo = 0;
    p = __c1_dd_mul(q, b);
    p.hi = -p.hi;
    p.lo = -p.lo;
    r = __c1_dd_add(a, p);
    return __c1_dd_sum(q.hi, (r.hi + r.lo) / b.hi);
}

static unsigned long long __c1_log_bits(unsigned long long input, int type, int base) {
    union {
        double f;
        unsigned long long u;
    } value;

    union {
        float f;
        unsigned int u;
    } single;

    unsigned long long sign = __c1_m_sign(type), magnitude = input & (sign - 1), normalized, result;
    unsigned long long infinity = type == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    unsigned long long quiet = type == 13 ? 0x400000ULL : 0x8000000000000ULL;
    unsigned long long one = type == 13 ? 0x3f800000ULL : 0x3ff0000000000000ULL;
    struct __c1_dd z, z2, term, sum, part, coefficient;
    fenv_t saved;
    int exponent, i, negative, mode, changed = 0;
    double power;
    if (magnitude > infinity) {
        if (!(magnitude & quiet))
            feraiseexcept(FE_INVALID);
        return input | quiet;
    }
    if (!magnitude) {
        errno = ERANGE;
        feraiseexcept(FE_DIVBYZERO);
        return sign | infinity;
    }
    if (input & sign) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return infinity | quiet;
    }
    if (magnitude == infinity)
        return infinity;
    if (magnitude == one)
        return 0;
    normalized = __c1_m_frexp(input, type, &exponent);
    if (base == 2 && normalized == (type == 13 ? 0x3f000000ULL : 0x3fe0000000000000ULL)) {
        if (type == 13) {
            single.f = (float)(exponent - 1);
            return single.u;
        }
        value.f = (double)(exponent - 1);
        return value.u;
    }
    feholdexcept(&saved);
    fesetenv(FE_DFL_ENV);
    if (type == 13) {
        single.u = (unsigned int)input;
        value.f = single.f;
    } else
        value.u = input;
    if (base == 10 && value.f >= 10.0) {
        power = 10.0;
        for (i = 1; i <= (type == 13 ? 10 : 22); i++) {
            if (value.f == power) {
                if (type == 13) {
                    single.f = (float)i;
                    result = single.u;
                } else {
                    value.f = (double)i;
                    result = value.u;
                }
                fesetenv(&saved);
                return result;
            }
            power *= 10.0;
        }
    }
    if (type == 13) {
        single.u = (unsigned int)normalized;
        value.f = single.f;
    } else
        value.u = normalized;
    if (value.f < 0x1.6a09e667f3bcdp-1) {
        value.f *= 2.0;
        exponent--;
    }
    z = __c1_dd_ratio(__c1_dd_sum(value.f, -1.0), __c1_dd_sum(value.f, 1.0));
    z2 = __c1_dd_mul(z, z);
    term = z;
    sum = z;
    for (i = 3; i <= 75; i += 2) {
        term = __c1_dd_mul(term, z2);
        sum = __c1_dd_add(sum, __c1_dd_divide(term, i));
    }
    sum.hi *= 2.0;
    sum.lo *= 2.0;
    part.hi = (double)exponent;
    part.lo = 0;
    coefficient.hi = 0x1.62e42fefa39efp-1;
    coefficient.lo = 0x1.abc9e3b39803fp-56;
    sum = __c1_dd_add(sum, __c1_dd_mul(part, coefficient));
    if (base == 10) {
        coefficient.hi = 0x1.bcb7b1526e50ep-2;
        coefficient.lo = 0x1.95355baaafad3p-57;
        sum = __c1_dd_mul(sum, coefficient);
    }
    if (base == 2) {
        coefficient.hi = 0x1.71547652b82fep+0;
        coefficient.lo = 0x1.777d0ffda0d24p-56;
        sum = __c1_dd_mul(sum, coefficient);
    }
    negative = sum.hi < 0;
    if (negative) {
        sum.hi = -sum.hi;
        sum.lo = -sum.lo;
    }
    fesetenv(&saved);
    mode = fegetround();
    if (negative && (mode == FE_UPWARD || mode == FE_DOWNWARD)) {
        fesetround(mode == FE_UPWARD ? FE_DOWNWARD : FE_UPWARD);
        changed = 1;
    }
    result = __c1_exp_pack(sum, 0, type);
    if (changed)
        fesetround(mode);
    return result | (negative ? sign : 0);
}

#define __C1_LOG_FUNCTION(T,U,S,K) \
T log##S(T x){union{T f;U u;}v;v.f=x;v.u=(U)__c1_log_bits(v.u,K,0);return v.f;} \
T log10##S(T x){union{T f;U u;}v;v.f=x;v.u=(U)__c1_log_bits(v.u,K,10);return v.f;} \
T log2##S(T x){union{T f;U u;}v;v.f=x;v.u=(U)__c1_log_bits(v.u,K,2);return v.f;}
__C1_LOG_FUNCTION(double, unsigned long long, , 14)
__C1_LOG_FUNCTION(float, unsigned int, f, 13)
__C1_LOG_FUNCTION(long double, unsigned long long, l, 15)
#undef __C1_LOG_FUNCTION
#endif
