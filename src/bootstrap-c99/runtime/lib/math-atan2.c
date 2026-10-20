#ifndef __C1_ATAN2_C
#define __C1_ATAN2_C
#include "math-asin.c"

static unsigned long long __c1_atan2_bits(unsigned long long y, unsigned long long x, int type) {
    union {
        double f;
        unsigned long long u;
    } value;

    union {
        float f;
        unsigned int u;
    } single;

    unsigned long long sign = __c1_m_sign(type), ay = y & (sign - 1), ax = x & (sign - 1), normal_y,
                       normal_x, result;
    unsigned long long infinity = type == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    unsigned long long quiet = type == 13 ? 0x400000ULL : 0x8000000000000ULL;
    struct __c1_dd a, b, sum, pi, pi2;
    fenv_t saved;
    int negative = (y & sign) != 0, left = (x & sign) != 0, mode = fegetround(), changed = 0, ey,
        ex, invert, scale = 0;
    if (ay > infinity || ax > infinity) {
        if ((ay > infinity && !(ay & quiet)) || (ax > infinity && !(ax & quiet)))
            feraiseexcept(FE_INVALID);
        return (ay > infinity ? y : x) | quiet;
    }
    if ((!ay && !left) || (ax == infinity && ay != infinity && !left))
        return y & sign;
    feholdexcept(&saved);
    fesetenv(FE_DFL_ENV);
    pi2.hi = 0x1.921fb54442d18p+0;
    pi2.lo = 0x1.1a62633145c07p-54;
    pi.hi = 0x1.921fb54442d18p+1;
    pi.lo = 0x1.1a62633145c07p-53;
    if (!ay)
        sum = pi;
    else if (!ax)
        sum = pi2;
    else if (ay == infinity && ax == infinity) {
        sum = pi2;
        sum.hi *= 0.5;
        sum.lo *= 0.5;
        if (left) {
            sum.hi = -sum.hi;
            sum.lo = -sum.lo;
            sum = __c1_dd_add(pi, sum);
        }
    } else if (ay == infinity)
        sum = pi2;
    else if (ax == infinity)
        sum = pi;
    else {
        invert = ay > ax;
        normal_y = __c1_m_frexp(invert ? ax : ay, type, &ey);
        normal_x = __c1_m_frexp(invert ? ay : ax, type, &ex);
        if (type == 13) {
            single.u = (unsigned int)normal_y;
            a.hi = single.f;
            single.u = (unsigned int)normal_x;
            b.hi = single.f;
        } else {
            value.u = normal_y;
            a.hi = value.f;
            value.u = normal_x;
            b.hi = value.f;
        }
        a.lo = 0;
        b.lo = 0;
        a = __c1_dd_ratio(a, b);
        scale = ey - ex;
        if (scale < -60) {
            if (!left && !invert) {
                /* atan(r) is strictly below r. Preserve this direction when the
       normalized quotient is exact, including subnormal halfway cases. */
                if (a.lo == 0)
                    a.lo = -0x1p-110;
                sum = a;
            } else {
                sum = invert ? pi2 : pi;
                scale = 0;
            }
        } else {
            a.hi = ldexp(a.hi, scale);
            a.lo = ldexp(a.lo, scale);
            scale = 0;
            sum = __c1_atan_kernel(a);
            if (invert) {
                sum.hi = -sum.hi;
                sum.lo = -sum.lo;
                sum = __c1_dd_add(pi2, sum);
            }
            if (left) {
                sum.hi = -sum.hi;
                sum.lo = -sum.lo;
                sum = __c1_dd_add(pi, sum);
            }
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

#define __C1_ATAN2_FUNCTION(T,U,S,K) \
T atan2##S(T y,T x){union{T f;U u;}a,b;int saved=__c1_math_policy_begin(K);a.f=y;b.f=x;a.u=(U)__c1_atan2_bits(a.u,b.u,K);a.u=(U)__c1_math_policy_end(a.u,K,saved);return a.f;}
__C1_ATAN2_FUNCTION(double, unsigned long long, , 14)
__C1_ATAN2_FUNCTION(float, unsigned int, f, 13)
__C1_ATAN2_FUNCTION(long double, unsigned long long, l, 15)
#undef __C1_ATAN2_FUNCTION
#endif
