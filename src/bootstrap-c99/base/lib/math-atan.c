#ifndef __C1_ATAN_C
#define __C1_ATAN_C
#include "math-hyperbolic.c"

static struct __c1_dd __c1_atan_kernel(struct __c1_dd a) {
    struct __c1_dd b, z, square, term, sum, pi2, pi4;
    int invert = 0, offset = 0, i;
    pi2.hi = 0x1.921fb54442d18p+0;
    pi2.lo = 0x1.1a62633145c07p-54;
    pi4.hi = 0x1.921fb54442d18p-1;
    pi4.lo = 0x1.1a62633145c07p-55;
    if (a.hi >= 0x1p60)
        sum = pi2;
    else {

        if (a.hi > 1) {
            b.hi = 1;
            b.lo = 0;
            a = __c1_dd_ratio(b, a);
            invert = 1;
        }
        if (a.hi > 0x1.a827999fcef32p-2) {
            b.hi = -1;
            b.lo = 0;
            z = __c1_dd_add(a, b);
            b.hi = 1;
            b = __c1_dd_add(a, b);
            z = __c1_dd_ratio(z, b);
            offset = 1;
        } else
            z = a;
        square = __c1_dd_mul(z, z);
        square.hi = -square.hi;
        square.lo = -square.lo;
        term = z;
        sum = z;
        for (i = 3; i <= 145; i += 2) {
            term = __c1_dd_mul(term, square);
            sum = __c1_dd_add(sum, __c1_dd_divide(term, i));
        }
        if (offset)
            sum = __c1_dd_add(pi4, sum);
        if (invert) {
            sum.hi = -sum.hi;
            sum.lo = -sum.lo;
            sum = __c1_dd_add(pi2, sum);
        }
    }
    return sum;
}

static unsigned long long __c1_atan_bits(unsigned long long input, int type) {
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
    unsigned long long tiny = type == 13 ? 0x32000000ULL : 0x3e40000000000000ULL;
    unsigned long long minimum = type == 13 ? 0x800000ULL : 0x10000000000000ULL;
    struct __c1_dd a, b, z, square, term, sum, pi2, pi4;
    fenv_t saved;
    int negative = (input & sign) != 0, mode = fegetround(), invert = 0, offset = 0, i, changed = 0;
    if (magnitude > infinity) {
        if (!(magnitude & quiet))
            feraiseexcept(FE_INVALID);
        return input | quiet;
    }
    if (!magnitude)
        return input;
    if (magnitude < tiny) {
        result = magnitude;
        if (mode == FE_TOWARDZERO || (!negative && mode == FE_DOWNWARD) ||
            (negative && mode == FE_UPWARD))
            result--;
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
    a.hi = value.f;
    a.lo = 0;
    sum = __c1_atan_kernel(a);
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

double atan(double x) {
    union {
        double f;
        unsigned long long u;
    } v;

    int saved = __c1_math_policy_begin(14);
    v.f = x;
    v.u = __c1_atan_bits(v.u, 14);
    v.u = __c1_math_policy_end(v.u, 14, saved);
    return v.f;
}

float atanf(float x) {
    union {
        float f;
        unsigned int u;
    } v;

    v.f = x;
    v.u = (unsigned int)__c1_atan_bits(v.u, 13);
    return v.f;
}

long double atanl(long double x) {
    union {
        long double f;
        unsigned long long u;
    } v;

    v.f = x;
    v.u = __c1_atan_bits(v.u, 15);
    return v.f;
}
#endif
