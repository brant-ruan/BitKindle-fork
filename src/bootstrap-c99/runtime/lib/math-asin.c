#ifndef __C1_ASIN_C
#define __C1_ASIN_C
#include "math-atan.c"

static struct __c1_dd __c1_dd_root(struct __c1_dd a) {
    struct __c1_dd q, p, r;
    q.hi = sqrt(a.hi);
    q.lo = 0;
    p = __c1_dd_mul(q, q);
    p.hi = -p.hi;
    p.lo = -p.lo;
    r = __c1_dd_add(a, p);
    return __c1_dd_sum(q.hi, (r.hi + r.lo) / (2 * q.hi));
}

static unsigned long long __c1_asin_bits(unsigned long long input, int type, int cosine) {
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
    struct __c1_dd x, a, b, root, sum, pi2;
    fenv_t saved;
    int input_negative = (input & sign) != 0, negative = input_negative && !cosine,
        mode = fegetround(), changed = 0;
    if (magnitude > infinity) {
        if (!(magnitude & quiet))
            feraiseexcept(FE_INVALID);
        return input | quiet;
    }
    if (magnitude > one) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return infinity | quiet;
    }
    if (!cosine && !magnitude)
        return input;
    if (cosine && input == one)
        return 0;
    if (!cosine && magnitude < tiny) {
        result = magnitude;
        if ((!negative && mode == FE_UPWARD) || (negative && mode == FE_DOWNWARD))
            result++;
        if (result < minimum) {
            errno = ERANGE;
            feraiseexcept(FE_UNDERFLOW);
        }
        feraiseexcept(FE_INEXACT);
        return result | (negative ? sign : 0);
    }
    feholdexcept(&saved);
    fesetenv(FE_DFL_ENV);
    if (type == 13) {
        single.u = (unsigned int)magnitude;
        value.f = single.f;
    } else
        value.u = magnitude;
    pi2.hi = 0x1.921fb54442d18p+0;
    pi2.lo = 0x1.1a62633145c07p-54;
    if (magnitude == one) {
        sum = pi2;
        if (cosine) {
            sum.hi *= 2;
            sum.lo *= 2;
        }
    } else {
        x.hi = value.f;
        x.lo = 0;
        a = __c1_dd_sum(1, -value.f);
        b = __c1_dd_sum(1, value.f);
        root = __c1_dd_root(__c1_dd_mul(a, b));
        if (cosine && value.f >= 0.5) {
            sum = __c1_atan_kernel(__c1_dd_ratio(root, x));
            if (input_negative) {
                sum.hi = -sum.hi;
                sum.lo = -sum.lo;
                pi2.hi *= 2;
                pi2.lo *= 2;
                sum = __c1_dd_add(pi2, sum);
            }
        } else {
            sum = __c1_atan_kernel(__c1_dd_ratio(x, root));
            if (cosine) {
                if (!input_negative) {
                    sum.hi = -sum.hi;
                    sum.lo = -sum.lo;
                }
                sum = __c1_dd_add(pi2, sum);
            }
        }
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

#define __C1_ASIN_FUNCTION(T,U,S,K) \
T asin##S(T x){union{T f;U u;}v;int saved=__c1_math_policy_begin(K);v.f=x;v.u=(U)__c1_asin_bits(v.u,K,0);v.u=(U)__c1_math_policy_end(v.u,K,saved);return v.f;} \
T acos##S(T x){union{T f;U u;}v;int saved=__c1_math_policy_begin(K);v.f=x;v.u=(U)__c1_asin_bits(v.u,K,1);v.u=(U)__c1_math_policy_end(v.u,K,saved);return v.f;}
__C1_ASIN_FUNCTION(double, unsigned long long, , 14)
__C1_ASIN_FUNCTION(float, unsigned int, f, 13)
__C1_ASIN_FUNCTION(long double, unsigned long long, l, 15)
#undef __C1_ASIN_FUNCTION
#endif
