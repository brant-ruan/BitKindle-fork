#ifndef __C1_POW_C
#define __C1_POW_C
#include "math-trig.c"
#include "math-integer-power.c"

/* Integer classification without conversion, including exponents beyond
   the exact integer range of a machine integer. 0=fractional, 1=odd, 2=even. */
static int __c1_pow_integer(unsigned long long a, int type) {
    int p = type == 13 ? 23 : 52, bias = type == 13 ? 127 : 1023, e = (int)(a >> p) - bias;
    unsigned long long m = (a & ((1ULL << p) - 1)) | (1ULL << p);
    if (e < 0)
        return a ? 0 : 2;
    if (e > p)
        return 2;
    if (e < p && (m & ((1ULL << (p - e)) - 1)))
        return 0;
    return (m >> (p - e)) & 1 ? 1 : 2;
}

static struct __c1_dd __c1_pow_log(unsigned long long bits, int *exponent) {
    union {
        double f;
        unsigned long long u;
    } v;
    struct __c1_dd z, square, term, sum, n, ln2;
    int i;
    v.u = __c1_m_frexp(bits, 14, exponent);
    if (v.f < 0x1.6a09e667f3bcdp-1) {
        v.f *= 2;
        (*exponent)--;
    }
    z = __c1_dd_ratio(__c1_dd_sum(v.f, -1), __c1_dd_sum(v.f, 1));
    square = __c1_dd_mul(z, z);
    term = z;
    sum = z;
    for (i = 3; i <= 75; i += 2) {
        term = __c1_dd_mul(term, square);
        sum = __c1_dd_add(sum, __c1_dd_divide(term, i));
    }
    sum.hi *= 2;
    sum.lo *= 2;
    n.hi = *exponent;
    n.lo = 0;
    ln2.hi = 0x1.62e42fefa39efp-1;
    ln2.lo = 0x1.abc9e3b39803fp-56;
    return __c1_dd_add(sum, __c1_dd_mul(n, ln2));
}

static struct __c1_dd __c1_pow_exp(struct __c1_dd x, int *scale) {
    struct __c1_dd n, ln2, r, term, sum;
    int i;
    *scale = (int)(x.hi * 0x1.71547652b82fep+0 + (x.hi > 0 ? 0.5 : -0.5));
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

static unsigned long long __c1_pow_bits(unsigned long long x, unsigned long long y, int type) {
    union {
        double f;
        unsigned long long u;
    } vx, vy, v;

    union {
        float f;
        unsigned int u;
    } f;

    unsigned long long sign = __c1_m_sign(type), a = x & (sign - 1), b = y & (sign - 1), result, m,
                       product;
    unsigned long long infinity = type == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    unsigned long long quiet = type == 13 ? 0x400000ULL : 0x8000000000000ULL;
    unsigned long long one = type == 13 ? 0x3f800000ULL : 0x3ff0000000000000ULL;
    unsigned long long half = type == 13 ? 0x3f000000ULL : 0x3fe0000000000000ULL;
    struct __c1_dd logarithm, power, factor;
    fenv_t saved;
    int integer = __c1_pow_integer(b, type), negative = (x & sign) && integer == 1;
    int mode = fegetround(), changed = 0, exponent, scale, n, exact;
    if ((a > infinity && !(a & quiet)) || (b > infinity && !(b & quiet))) {
        feraiseexcept(FE_INVALID);
        return (a > infinity ? x : y) | quiet;
    }
    if (!b || x == one)
        return one;
    if (a > infinity || b > infinity)
        return (a > infinity ? x : y) | quiet;
    if (b == infinity) {
        if (a == one)
            return one;
        return ((a > one) == ((y & sign) == 0)) ? infinity : 0;
    }
#if defined(__C1_C89_MATH)
    if (type == 14 && !a && (y & sign)) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return infinity | quiet;
    }
#endif
    if (!a) {
        if (y & sign) {
            errno = ERANGE;
            feraiseexcept(FE_DIVBYZERO);
            return infinity | (negative ? sign : 0);
        }
        return negative ? sign : 0;
    }
    if (a == infinity)
        return ((y & sign) ? 0 : infinity) | (negative ? sign : 0);
    if ((x & sign) && !integer) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return infinity | quiet;
    }
    if (a == one)
        return one | (negative ? sign : 0);
    if (y == one)
        return x;
    if (y == half) {
        if (type == 13) {
            f.u = (unsigned int)x;
            f.f = sqrtf(f.f);
            return f.u;
        }
        v.u = x;
        v.f = sqrt(v.f);
        return v.u;
    }
    feholdexcept(&saved);
    fesetenv(FE_DFL_ENV);
    if (type == 13) {
        f.u = (unsigned int)a;
        vx.f = f.f;
        f.u = (unsigned int)y;
        vy.f = f.f;
    } else {
        vx.u = a;
        vy.u = y;
    }
    if (vy.f > 0x1p64 || vy.f < -0x1p64) {
        scale = ((a > one) == ((y & sign) == 0)) ? 2000 : -2000;
        fesetenv(&saved);
        return __c1_m_scale(one | (negative ? sign : 0), type, scale);
    }
    /* A tiny exponent gives a nonzero displacement from one smaller than
    half an ulp. Its sign still matters to directed rounding. */
    if (vy.f < 0x1p-64 && vy.f > -0x1p-64) {
        exact = (a > one) == ((y & sign) == 0);
        fesetenv(&saved);
        result = one;
        if (exact && mode == FE_UPWARD)
            result++;
        if (!exact && (mode == FE_DOWNWARD || mode == FE_TOWARDZERO))
            result--;
        feraiseexcept(FE_INEXACT);
        return result;
    }
    v.u = __c1_m_frexp(vx.u, 14, &exponent);
    if (v.u == 0x3fe0000000000000ULL) {
        power = __c1_dd_product((double)(exponent - 1), vy.f);
        v.f = power.hi;
        if (v.f >= 2000 || v.f <= -2000) {
            scale = v.f > 0 ? 2000 : -2000;
            fesetenv(&saved);
            return __c1_m_scale(one | (negative ? sign : 0), type, scale);
        }
        scale = (int)v.f;
        if (v.f == (double)scale && power.lo == 0) {
            fesetenv(&saved);
            return __c1_m_scale(one | (negative ? sign : 0), type, scale);
        }
    }
    /* Positive dyadic powers with an exactly computable integer significand.
    Construct the integer before applying its binary scale, avoiding log/exp
    approximation at exactly representable results and rounding boundaries. */
    if (vy.f > 0 && vy.f <= 64) {
        m = (vx.u & 0xfffffffffffffULL) | 0x10000000000000ULL;
        exponent = (int)(vx.u >> 52) - 1023 - 52;
        if (!(vx.u >> 52)) {
            m = vx.u;
            exponent = -1074;
        }
        while (!(m & 1)) {
            m >>= 1;
            exponent++;
        }
        v.f = vy.f;
        exact = 1;
        /* A dyadic fractional exponent can give an exact dyadic result only
     when each required square root has an integer odd significand and
     an even binary exponent. Verify each root using integer arithmetic. */
        while (v.f <= 64 && v.f != (double)(int)v.f) {
            if (exponent % 2) {
                exact = 0;
                break;
            }
            product = (unsigned long long)sqrt((double)m);
            if (product * product != m) {
                exact = 0;
                break;
            }
            m = product;
            exponent /= 2;
            v.f *= 2;
        }
        if (v.f > 64)
            exact = 0;
        n = (int)v.f;
        if (exact) {
            fesetenv(&saved);
            return __c1_ipow_pack(m, n, exponent * n, negative, type);
        }
    }
    logarithm = __c1_pow_log(vx.u, &exponent);
    factor.hi = vy.f;
    factor.lo = 0;
    power = __c1_dd_mul(logarithm, factor);
    if (power.hi > 710 || power.hi < -746) {
        scale = power.hi > 0 ? 2000 : -2000;
        fesetenv(&saved);
        return __c1_m_scale(one | (negative ? sign : 0), type, scale);
    }
    power = __c1_pow_exp(power, &scale);
    fesetenv(&saved);
    if (negative && (mode == FE_UPWARD || mode == FE_DOWNWARD)) {
        fesetround(mode == FE_UPWARD ? FE_DOWNWARD : FE_UPWARD);
        changed = 1;
    }
    result = __c1_exp_pack(power, scale, type);
    if (changed)
        fesetround(mode);
    return result | (negative ? sign : 0);
}

#define __C1_POW_FUNCTION(T,U,S,K) \
T pow##S(T x,T y){union{T f;U u;}a,b;int saved=__c1_math_policy_begin(K);a.f=x;b.f=y;a.u=(U)__c1_pow_bits(a.u,b.u,K);a.u=(U)__c1_math_policy_end(a.u,K,saved);return a.f;}
__C1_POW_FUNCTION(double, unsigned long long, , 14)
__C1_POW_FUNCTION(float, unsigned int, f, 13)
__C1_POW_FUNCTION(long double, unsigned long long, l, 15)
#undef __C1_POW_FUNCTION
#endif
