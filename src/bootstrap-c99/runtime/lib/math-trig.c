#ifndef __C1_TRIG_C
#define __C1_TRIG_C
#include "math-atan2.c"
#include "two-over-pi.h"

static int __c1_trig_bit(unsigned int *p, int b) {
    return b < 0 || b >= 1376 ? 0 : (p[b / 32] >> (b % 32)) & 1;
}

/* Multiply the exact significand by the certified 1280-bit reciprocal.
   Keep the fractional distance to the nearest integer before converting
   to floating point, so cancellation near quadrants loses no leading bits. */
static struct __c1_dd __c1_trig_reduce(double x, int *quadrant) {
    union {
        double f;
        unsigned long long u;
    } v;

    unsigned int p[43], m[2];
    unsigned long long t, carry, a, b;
    int i, j, d, up, top;
    struct __c1_dd r, pi2;
    v.f = x;
    if (x <= 0x1.921fb54442d18p-1) {
        *quadrant = 0;
        r.hi = x;
        r.lo = 0;
        return r;
    }
    a = (v.u & 0xfffffffffffffULL) | 0x10000000000000ULL;
    m[0] = (unsigned int)a;
    m[1] = (unsigned int)(a >> 32);
    for (i = 0; i < 43; i++)
        p[i] = 0;
    for (j = 0; j < 2; j++) {
        carry = 0;
        for (i = 0; i < 40; i++) {
            t = (unsigned long long)__c1_two_over_pi[i] * m[j] + p[i + j] + carry;
            p[i + j] = (unsigned int)t;
            carry = t >> 32;
        }
        p[40 + j] = (unsigned int)carry;
    }
    d = 1332 - ((int)(v.u >> 52) - 1023);
    up = __c1_trig_bit(p, d - 1);
    *quadrant = (__c1_trig_bit(p, d) + 2 * __c1_trig_bit(p, d + 1) + up) & 3;
    for (i = (d + 31) / 32; i < 43; i++)
        p[i] = 0;
    if (d % 32)
        p[d / 32] &= (1U << (d % 32)) - 1;
    if (up) {
        carry = 1;
        for (i = 0; i < (d + 31) / 32; i++) {
            t = (unsigned long long)(~p[i]) + carry;
            p[i] = (unsigned int)t;
            carry = t >> 32;
        }
        if (d % 32)
            p[d / 32] &= (1U << (d % 32)) - 1;
    }
    top = d - 1;
    while (top >= 0 && !__c1_trig_bit(p, top))
        top--;
    a = 0;
    b = 0;
    for (i = 0; i < 53; i++) {
        a = (a << 1) | __c1_trig_bit(p, top - i);
        b = (b << 1) | __c1_trig_bit(p, top - 53 - i);
    }
    r = __c1_dd_sum(ldexp((double)a, top - d - 52), ldexp((double)b, top - d - 105));
    if (up) {
        r.hi = -r.hi;
        r.lo = -r.lo;
    }
    pi2.hi = 0x1.921fb54442d18p+0;
    pi2.lo = 0x1.1a62633145c07p-54;
    return __c1_dd_mul(r, pi2);
}

static unsigned long long __c1_trig_bits(unsigned long long input, int type, int kind) {
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
    struct __c1_dd r, z, s, c, term, sum;
    fenv_t saved;
    int negative = (input & sign) != 0, mode = fegetround(), q, i, changed = 0;
    if (a > infinity) {
        if (!(a & quiet))
            feraiseexcept(FE_INVALID);
        return input | quiet;
    }
    if (a == infinity) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return infinity | quiet;
    }
    if (!a)
        return kind == 1 ? one : input;
    if (a < (type == 13 ? 0x32000000ULL : 0x3e40000000000000ULL)) {
        if (kind == 1)
            result = one - ((mode == FE_DOWNWARD || mode == FE_TOWARDZERO) ? 1 : 0);
        else {
            result = a;
            if (kind == 0 &&
                (mode == FE_TOWARDZERO || (negative ? mode == FE_UPWARD : mode == FE_DOWNWARD)))
                result--;
            if (kind == 2 && (negative ? mode == FE_DOWNWARD : mode == FE_UPWARD))
                result++;
            if (result < (type == 13 ? 0x800000ULL : 0x10000000000000ULL)) {
                errno = ERANGE;
                feraiseexcept(FE_UNDERFLOW);
            }
            result |= input & sign;
        }
        feraiseexcept(FE_INEXACT);
        return result;
    }
    feholdexcept(&saved);
    fesetenv(FE_DFL_ENV);
    if (type == 13) {
        f.u = (unsigned int)a;
        v.f = f.f;
    } else
        v.u = a;
    r = __c1_trig_reduce(v.f, &q);
    z = __c1_dd_mul(r, r);
    z.hi = -z.hi;
    z.lo = -z.lo;
    s = r;
    term = r;
    for (i = 3; i <= 49; i += 2) {
        term = __c1_dd_divide(__c1_dd_mul(term, z), i * (i - 1));
        s = __c1_dd_add(s, term);
    }
    c.hi = 1;
    c.lo = 0;
    term = c;
    for (i = 2; i <= 48; i += 2) {
        term = __c1_dd_divide(__c1_dd_mul(term, z), i * (i - 1));
        c = __c1_dd_add(c, term);
    }
    if (kind == 0) {
        sum = (q & 1) ? c : s;
        if (q & 2) {
            sum.hi = -sum.hi;
            sum.lo = -sum.lo;
        }
    } else if (kind == 1) {
        sum = (q & 1) ? s : c;
        if (q == 1 || q == 2) {
            sum.hi = -sum.hi;
            sum.lo = -sum.lo;
        }
        negative = 0;
    } else {
        sum = (q & 1) ? __c1_dd_ratio(c, s) : __c1_dd_ratio(s, c);
        if (q & 1) {
            sum.hi = -sum.hi;
            sum.lo = -sum.lo;
        }
    }
    if (sum.hi < 0) {
        sum.hi = -sum.hi;
        sum.lo = -sum.lo;
        negative = !negative;
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

#define __C1_TRIG_FUNCTION(T,U,S,K) \
T sin##S(T x){union{T f;U u;}v;int saved=__c1_math_policy_begin(K);v.f=x;v.u=(U)__c1_trig_bits(v.u,K,0);v.u=(U)__c1_math_policy_end(v.u,K,saved);return v.f;} \
T cos##S(T x){union{T f;U u;}v;int saved=__c1_math_policy_begin(K);v.f=x;v.u=(U)__c1_trig_bits(v.u,K,1);v.u=(U)__c1_math_policy_end(v.u,K,saved);return v.f;} \
T tan##S(T x){union{T f;U u;}v;int saved=__c1_math_policy_begin(K);v.f=x;v.u=(U)__c1_trig_bits(v.u,K,2);v.u=(U)__c1_math_policy_end(v.u,K,saved);return v.f;}
__C1_TRIG_FUNCTION(double, unsigned long long, , 14)
__C1_TRIG_FUNCTION(float, unsigned int, f, 13)
__C1_TRIG_FUNCTION(long double, unsigned long long, l, 15)
#undef __C1_TRIG_FUNCTION
#endif
