#ifndef __C1_EXP_C
#define __C1_EXP_C
#include "math-scale.c"

struct __c1_dd {
    double hi, lo;
};

static struct __c1_dd __c1_dd_sum(double a, double b) {
    struct __c1_dd z;
    double v;
    z.hi = a + b;
    v = z.hi - a;
    z.lo = (a - (z.hi - v)) + (b - v);
    return z;
}

static struct __c1_dd __c1_dd_add(struct __c1_dd a, struct __c1_dd b) {
    struct __c1_dd z = __c1_dd_sum(a.hi, b.hi);
    return __c1_dd_sum(z.hi, z.lo + a.lo + b.lo);
}

static struct __c1_dd __c1_dd_product(double a, double b) {
    struct __c1_dd z;
    double t, ah, al, bh, bl;
    t = 134217729.0 * a;
    ah = t - (t - a);
    al = a - ah;
    t = 134217729.0 * b;
    bh = t - (t - b);
    bl = b - bh;
    z.hi = a * b;
    z.lo = ((ah * bh - z.hi) + ah * bl + al * bh) + al * bl;
    return z;
}

static struct __c1_dd __c1_dd_mul(struct __c1_dd a, struct __c1_dd b) {
    struct __c1_dd z = __c1_dd_product(a.hi, b.hi);
    return __c1_dd_sum(z.hi, z.lo + a.hi * b.lo + a.lo * b.hi);
}

static struct __c1_dd __c1_dd_divide(struct __c1_dd a, int n) {
    double q = a.hi / n;
    struct __c1_dd p = __c1_dd_product(q, (double)n);
    return __c1_dd_sum(q, ((a.hi - p.hi) - p.lo + a.lo) / n);
}

/* Positive double-double value times 2^scale, rounded directly to target
   precision, including subnormals. The residual breaks halfway cases. */
static unsigned long long __c1_exp_pack(struct __c1_dd y, int scale, int type) {
    union {
        double f;
        unsigned long long u;
    } high, low;

    unsigned long long m, q, rest = 0, half = 0, bits, infinity;
    int p = type == 13 ? 23 : 52, bias = type == 13 ? 127 : 1023, maximum = type == 13 ? 255 : 2047;
    int field, shift, mode = fegetround(), negative, residual;
    high.f = y.hi;
    low.f = y.lo;
    negative = (low.u >> 63) != 0;
    residual = (low.u & 0x7fffffffffffffffULL) != 0;
    field = (int)(high.u >> 52) - 1023 + scale + bias;
    infinity = (unsigned long long)maximum << p;
    if (field >= maximum) {
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW | FE_INEXACT);
        return mode == FE_TONEAREST || mode == FE_UPWARD ? infinity : infinity - 1;
    }
    m = (high.u & 0xfffffffffffffULL) | 0x10000000000000ULL;
    shift = 52 - p + (field <= 0 ? 1 - field : 0);
    if (shift >= 64) {
        q = 0;
        rest = m;
    } else if (shift) {
        q = m >> shift;
        rest = m & ((1ULL << shift) - 1);
        half = 1ULL << (shift - 1);
    } else
        q = m;
    if (mode == FE_UPWARD) {
        if (rest || (residual && !negative))
            q++;
    } else if (mode == FE_DOWNWARD || mode == FE_TOWARDZERO) {
        if (!rest && residual && negative)
            q--;
    } else if (shift > 0 && shift < 64) {
        if (rest > half || (rest == half && (residual ? !negative : (q & 1) != 0)))
            q++;
    }
    bits = field > 0 ? ((unsigned long long)(field - 1) << p) + q : q;
    if (bits >= infinity) {
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW | FE_INEXACT);
        return infinity;
    }
    if (bits < (1ULL << p)) {
        errno = ERANGE;
        feraiseexcept(FE_UNDERFLOW | FE_INEXACT);
    } else
        feraiseexcept(FE_INEXACT);
    return bits;
}

static unsigned long long __c1_exp_bits(unsigned long long input, int type) {
    union {
        double f;
        unsigned long long u;
    } value;

    union {
        float f;
        unsigned int u;
    } single;
    struct __c1_dd x, r, ln2, n, term, sum;
    fenv_t saved;
    unsigned long long magnitude, sign = type == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
    unsigned long long infinity = type == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    unsigned long long quiet = type == 13 ? 0x400000ULL : 0x8000000000000ULL;
    int scale, i;
    magnitude = input & (sign - 1);
    if (magnitude > infinity) {
        if (!(magnitude & quiet))
            feraiseexcept(FE_INVALID);
        return input | quiet;
    }
    if (magnitude == infinity)
        return input & sign ? 0 : infinity;
    if (!magnitude)
        return type == 13 ? 0x3f800000ULL : 0x3ff0000000000000ULL;
    feholdexcept(&saved);
    fesetenv(FE_DFL_ENV);
    if (type == 13) {
        single.u = (unsigned int)input;
        value.f = single.f;
    } else
        value.u = input;
    if (value.f > 710.0 || value.f < -746.0) {
        scale = value.f > 0 ? 2000 : -2000;
        fesetenv(&saved);
        return __c1_m_scale(type == 13 ? 0x3f800000ULL : 0x3ff0000000000000ULL, type, scale);
    }
    scale = (int)(value.f * 0x1.71547652b82fep+0 + (value.f > 0 ? 0.5 : -0.5));
    x.hi = value.f;
    x.lo = 0;
    n.hi = -(double)scale;
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
    fesetenv(&saved);
    return __c1_exp_pack(sum, scale, type);
}

double exp(double x) {
    union {
        double f;
        unsigned long long u;
    } v;

    int saved = __c1_math_policy_begin(14);
    v.f = x;
    v.u = __c1_exp_bits(v.u, 14);
    v.u = __c1_math_policy_end(v.u, 14, saved);
    return v.f;
}

float expf(float x) {
    union {
        float f;
        unsigned int u;
    } v;

    v.f = x;
    v.u = (unsigned int)__c1_exp_bits(v.u, 13);
    return v.f;
}

long double expl(long double x) {
    union {
        long double f;
        unsigned long long u;
    } v;

    v.f = x;
    v.u = __c1_exp_bits(v.u, 15);
    return v.f;
}
#endif
