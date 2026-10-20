#include "src/bootstrap-c99/include/math.h"
#include <fenv.h>
#include <errno.h>
#include "src/bootstrap-c99/complex/lib/erf-core.h"

static struct __ef_dd __ef_const(double x) {
    struct __ef_dd r;
    r.hi = x;
    r.lo = 0;
    return r;
}

static struct __ef_dd __ef_neg(struct __ef_dd x) {
    x.hi = -x.hi;
    x.lo = -x.lo;
    return x;
}

static struct __ef_dd __ef_erf(double x) {
    struct __ef_dd square, term, sum, c;
    int i;
    square = __ef_neg(__ef_dd_product(x, x));
    term = __ef_const(x);
    sum = term;
    for (i = 1; i <= 100; i++) {
        term = __ef_dd_divide(__ef_dd_mul(term, square), i);
        sum = __ef_dd_add(sum, __ef_dd_divide(term, 2 * i + 1));
    }
    c.hi = 0x1.20dd750429b6dp0;
    c.lo = 0x1.1ae3a914fed8p-56;
    return __ef_dd_mul(sum, c);
}

static struct __ef_dd __ef_tail(double x, int *scale) {
    struct __ef_dd square, tail, den, factor, c;
    int i;
    square = __ef_dd_product(x, x);
    tail = __ef_const(0);
    for (i = 400; i >= 1; i--) {
        den = __ef_dd_add(__ef_dd_mul(__ef_const(2), square), __ef_const(4 * i + 1));
        den = __ef_dd_add(den, __ef_neg(tail));
        tail = __ef_dd_ratio(__ef_const((2 * i) * (2 * i - 1)), den);
    }
    den = __ef_dd_add(__ef_dd_mul(__ef_const(2), square), __ef_const(1));
    den = __ef_dd_add(den, __ef_neg(tail));
    factor = __ef_dd_ratio(__ef_const(2 * x), den);
    c.hi = 0x1.20dd750429b6dp-1;
    c.lo = 0x1.1ae3a914fed8p-57;
    factor = __ef_dd_mul(factor, c);
    return __ef_dd_mul(factor, __ef_exp(__ef_neg(square), scale));
}

static unsigned long long __ef_bits(unsigned long long input, int type, int complement) {
    unsigned long long sign = type == 13 ? 0x80000000ULL : 0x8000000000000000ULL,
                       inf = type == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    unsigned long long quiet = type == 13 ? 0x400000ULL : 0x8000000000000ULL,
                       one = type == 13 ? 0x3f800000ULL : 0x3ff0000000000000ULL,
                       two = type == 13 ? 0x40000000ULL : 0x4000000000000000ULL,
                       a = input & (sign - 1), out;

    union {
        double f;
        unsigned long long u;
    } v;

    union {
        float f;
        unsigned int u;
    } vf;
    struct __ef_dd y, c;
    fenv_t saved;
    int negative = (input & sign) != 0, mode = fegetround(), scale = 0, e, flip;
    if (a > inf) {
        if (!(a & quiet))
            feraiseexcept(FE_INVALID);
        return input | quiet;
    }
    if (a == inf)
        return complement ? (negative ? two : 0) : (one | (input & sign));
    if (!a)
        return complement ? one : input;
    if (type == 13) {
        vf.u = (unsigned int)a;
        v.f = vf.f;
    } else
        v.u = a;
    if (v.f >= 6 && (!complement || negative)) {
        out = complement ? two : one;
        if (complement) {
            if (mode == FE_DOWNWARD || mode == FE_TOWARDZERO)
                out--;
        } else if (mode == FE_TOWARDZERO || (mode == FE_DOWNWARD && !negative) ||
                   (mode == FE_UPWARD && negative))
            out--;
        feraiseexcept(FE_INEXACT);
        return out | (!complement && negative ? sign : 0);
    }
    if (complement && v.f >= 28) {
        errno = ERANGE;
        feraiseexcept(FE_UNDERFLOW | FE_INEXACT);
        return mode == FE_UPWARD ? 1 : 0;
    }
    if (complement && v.f < 0x1p-60) {
        out = one;
        if (negative) {
            if (mode == FE_UPWARD)
                out++;
        } else if (mode == FE_DOWNWARD || mode == FE_TOWARDZERO)
            out--;
        feraiseexcept(FE_INEXACT);
        return out;
    }
    feholdexcept(&saved);
    fesetenv(FE_DFL_ENV);
    if (!complement && v.f < 0x1p-60) {
        v.f = frexp(v.f, &scale);
        c.hi = 0x1.20dd750429b6dp0;
        c.lo = 0x1.1ae3a914fed8p-56;
        y = __ef_dd_mul(__ef_const(v.f), c);
    } else if (v.f < 2) {
        y = __ef_erf(v.f);
        if (complement)
            y = __ef_dd_add(__ef_const(1), negative ? y : __ef_neg(y));
    } else {
        y = __ef_tail(v.f, &scale);
        if (!complement || negative) {
            y.hi = ldexp(y.hi, scale);
            y.lo = ldexp(y.lo, scale);
            scale = 0;
            y = __ef_dd_add(__ef_const(complement ? 2 : 1), __ef_neg(y));
        }
    }
    /* Normalize before target packing, including the minimum input's erf. */
    y.hi = frexp(y.hi, &e);
    y.lo = ldexp(y.lo, -e);
    scale += e;
    fesetenv(&saved);
    flip = !complement && negative && (mode == FE_DOWNWARD || mode == FE_UPWARD);
    if (flip)
        fesetround(mode == FE_DOWNWARD ? FE_UPWARD : FE_DOWNWARD);
    out = __ef_pack(y, scale, type);
    if (flip)
        fesetround(mode);
    return out | (!complement && negative ? sign : 0);
}

#define EF(T,U,S,K) T erf##S(T x){union{T f;U u;}v;v.f=x;v.u=(U)__ef_bits(v.u,K,0);return v.f;} T erfc##S(T x){union{T f;U u;}v;v.f=x;v.u=(U)__ef_bits(v.u,K,1);return v.f;}
EF(float, unsigned int, f, 13)
EF(double, unsigned long long, , 14)
EF(long double, unsigned long long, l, 15)
#undef EF
