#include "src/bootstrap-c99/include/math.h"
#include <fenv.h>
#include <errno.h>
#include "src/bootstrap-c99/complex/lib/gamma-core.h"

static struct __ga_dd __ga_c(double x) {
    struct __ga_dd r;
    r.hi = x;
    r.lo = 0;
    return r;
}

static struct __ga_dd __ga_neg(struct __ga_dd x) {
    x.hi = -x.hi;
    x.lo = -x.lo;
    return x;
}

static struct __ga_dd __ga_sub(struct __ga_dd x, struct __ga_dd y) {
    return __ga_dd_add(x, __ga_neg(y));
}

static struct __ga_dd __ga_log1p(struct __ga_dd x) {
    struct __ga_dd z, s, t, sum;
    int i;
    z = __ga_dd_ratio(x, __ga_dd_add(__ga_c(2), x));
    s = __ga_dd_mul(z, z);
    t = z;
    sum = z;
    for (i = 3; i <= 101; i += 2) {
        t = __ga_dd_mul(t, s);
        sum = __ga_dd_add(sum, __ga_dd_divide(t, i));
    }
    return __ga_dd_mul(__ga_c(2), sum);
}

static struct __ga_dd __ga_log(struct __ga_dd x) {
    struct __ga_dd ln2, z;
    int e;
    x.hi = frexp(x.hi, &e);
    x.lo = ldexp(x.lo, -e);
    if (x.hi < 0x1.6a09e667f3bcdp-1) {
        x.hi *= 2;
        x.lo *= 2;
        e--;
    }
    z = __ga_log1p(__ga_sub(x, __ga_c(1)));
    ln2.hi = 0x1.62e42fefa39efp-1;
    ln2.lo = 0x1.abc9e3b39803fp-56;
    return __ga_dd_add(z, __ga_dd_mul(__ga_c(e), ln2));
}

/* B_(2k)/(2k(2k-1)), stored as exact integer ratios. */
static struct __ga_dd __ga_coefficient(int k) {
    double num[12] = {1, -1, 1, -1, 1, -691, 1, -3617, 43867, -174611, 77683, -236364091};
    double den[12] = {12,  360,    1260,   1680,   1188, 360360,
                      156, 122400, 244188, 125400, 5796, 1506960};
    return __ga_dd_ratio(__ga_c(num[k]), __ga_c(den[k]));
}

/* Stable Stirling difference at 64, followed by log1p recurrence.
   Every term is proportional to z, even next to the zero at x=1. */
static struct __ga_dd __ga_nearone(double z) {
    struct __ga_dd a, b, r, d, p, p0, delta, c, sum, logn;
    int j, k;
    a = __ga_dd_sum(64, z);
    logn = __ga_log(__ga_c(64));
    sum = __ga_dd_mul(__ga_c(z), __ga_sub(logn, __ga_c(1)));
    b = __ga_log1p(__ga_dd_divide(__ga_c(z), 64));
    sum = __ga_dd_add(sum, __ga_dd_mul(__ga_dd_sum(63.5, z), b));
    r = __ga_dd_ratio(__ga_c(1), a);
    d = __ga_dd_ratio(__ga_c(-z), __ga_dd_mul(__ga_c(64), a));
    p = r;
    p0 = __ga_c(1.0 / 64);
    delta = d;
    k = 0;
    for (j = 1; j <= 23; j++) {
        if (j & 1) {
            c = __ga_coefficient(k++);
            sum = __ga_dd_add(sum, __ga_dd_mul(c, delta));
        }
        delta = __ga_dd_add(__ga_dd_mul(delta, r), __ga_dd_mul(p0, d));
        p = __ga_dd_mul(p, r);
        p0 = __ga_dd_divide(p0, 64);
    }
    for (j = 1; j < 64; j++)
        sum = __ga_sub(sum, __ga_log1p(__ga_dd_divide(__ga_c(z), j)));
    return sum;
}

static struct __ga_dd __ga_positive(double x) {
    struct __ga_dd sum, z, r, p, c, logx;
    int n, k;
    if (x < 0.5)
        return __ga_sub(__ga_nearone(x), __ga_log(__ga_c(x)));
    if (x < 64) {
        n = (int)x;
        z = __ga_dd_sum(x, -(double)n);
        if (z.hi > 0.5) {
            n++;
            z.hi -= 1;
        }
        sum = __ga_nearone(z.hi);
        if (!n)
            return __ga_sub(sum, __ga_log(__ga_c(x)));
        for (k = 1; k < n; k++)
            sum = __ga_dd_add(sum, __ga_log(__ga_dd_sum(k, z.hi)));
        return sum;
    }
    logx = __ga_log(__ga_c(x));
    sum = __ga_sub(__ga_dd_mul(__ga_dd_sum(x, -0.5), logx), __ga_c(x));
    c.hi = 0x1.d67f1c864beb5p-1;
    c.lo = -0x1.65b5a1b7ff5dfp-55;
    sum = __ga_dd_add(sum, c);
    r = __ga_dd_ratio(__ga_c(1), __ga_c(x));
    p = r;
    for (k = 0; k < 12; k++) {
        sum = __ga_dd_add(sum, __ga_dd_mul(__ga_coefficient(k), p));
        p = __ga_dd_mul(p, __ga_dd_mul(r, r));
    }
    return sum;
}

static struct __ga_dd __ga_sinpi(double f) {
    struct __ga_dd pi, z, s, t, sum;
    int i;
    if (f > 0.5)
        f = 1 - f;
    pi.hi = 0x1.921fb54442d18p1;
    pi.lo = 0x1.1a62633145c07p-53;
    z = __ga_dd_mul(__ga_c(f), pi);
    s = __ga_neg(__ga_dd_mul(z, z));
    t = z;
    sum = z;
    for (i = 3; i <= 51; i += 2) {
        t = __ga_dd_divide(__ga_dd_mul(t, s), i * (i - 1));
        sum = __ga_dd_add(sum, t);
    }
    return sum;
}

static unsigned long long __ga_bits(unsigned long long input, int type, int logarithm) {
    unsigned long long sign = type == 13 ? 0x80000000ULL : 0x8000000000000000ULL,
                       inf = type == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    unsigned long long quiet = type == 13 ? 0x400000ULL : 0x8000000000000ULL,
                       a = input & (sign - 1), out;

    union {
        double f;
        unsigned long long u;
    } v;

    union {
        float f;
        unsigned int u;
    } vf;
    struct __ga_dd y, c;
    fenv_t saved;
    int negative = (input & sign) != 0, resultnegative = 0, mode = fegetround(), scale = 0, e, flip,
        n;
    double x, f, t;
    if (a > inf) {
        if (!(a & quiet))
            feraiseexcept(FE_INVALID);
        return input | quiet;
    }
    if (a == inf) {
        if (logarithm || !negative)
            return inf;
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return inf | quiet;
    }
    if (!a) {
        errno = ERANGE;
        feraiseexcept(FE_DIVBYZERO);
        return inf | (!logarithm && negative ? sign : 0);
    }
    if (type == 13) {
        vf.u = (unsigned int)a;
        x = vf.f;
    } else {
        v.u = a;
        x = v.f;
    }
    if (negative && floor(x) == x) {
        if (logarithm) {
            errno = ERANGE;
            feraiseexcept(FE_DIVBYZERO);
            return inf;
        }
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return inf | quiet;
    }
    if (!negative && (x == 1 || x == 2))
        return logarithm ? 0 : (type == 13 ? 0x3f800000ULL : 0x3ff0000000000000ULL);
    if (!logarithm && !negative && x <= (type == 13 ? 14 : 23) && floor(x) == x) {
        t = 1;
        for (n = 2; n < (int)x; n++)
            t *= n;
        if (type == 13) {
            vf.f = (float)t;
            return vf.u;
        }
        v.f = t;
        return v.u;
    }
    /* Gamma(x)=1/x-EulerGamma+O(x). For these tiny inputs the
    correction cannot cross a nonexact reciprocal's rounding boundary.
    Exact reciprocal powers need the directed neighbor toward -infinity. */
    if (!logarithm && x < 0x1p-60 && (x < 0x1p-120 || frexp(x, &e) == 0.5)) {
        feholdexcept(&saved);
        if (type == 13) {
            vf.f = (float)(negative ? -x : x);
            vf.f = 1.0f / vf.f;
            out = vf.u;
        } else {
            v.f = negative ? -x : x;
            v.f = 1.0 / v.f;
            out = v.u;
        }
        if (!fetestexcept(FE_INEXACT)) {
            if (negative) {
                if (mode == FE_DOWNWARD)
                    out++;
            } else if (mode == FE_DOWNWARD || mode == FE_TOWARDZERO)
                out--;
        }
        if (fetestexcept(FE_OVERFLOW))
            errno = ERANGE;
        feraiseexcept(FE_INEXACT);
        feupdateenv(&saved);
        return out;
    }
    feholdexcept(&saved);
    fesetenv(FE_DFL_ENV);
    if (logarithm && !negative && x > 0x1p100) {
        y = __ga_sub(__ga_log(__ga_c(x)), __ga_c(1));
        t = frexp(x, &scale);
        y = __ga_dd_mul(__ga_c(t), y);
    } else if (!logarithm && !negative && x >= 172) {
        y = __ga_c(1);
        scale = 2000;
    } else {
        if (negative) {
            resultnegative = fmod(floor(x), 2) == 0;
            if (x < 0.5)
                y = __ga_sub(__ga_nearone(-x), __ga_log(__ga_c(x)));
            else {
                f = x - floor(x);
                c.hi = 0x1.921fb54442d18p1;
                c.lo = 0x1.1a62633145c07p-53;
                y = __ga_sub(__ga_log(c), __ga_log(__ga_sinpi(f)));
                y = __ga_sub(__ga_sub(y, __ga_positive(x)), __ga_log(__ga_c(x)));
            }
        } else
            y = __ga_positive(x);
        if (logarithm) {
            resultnegative = y.hi < 0;
            if (resultnegative)
                y = __ga_neg(y);
        } else {
            if (y.hi > 710) {
                y = __ga_c(1);
                scale = 2000;
            } else if (y.hi < -746) {
                y = __ga_c(1);
                scale = -2000;
            } else
                y = __ga_exp(y, &scale);
        }
    }
    y.hi = frexp(y.hi, &e);
    y.lo = ldexp(y.lo, -e);
    scale += e;
    fesetenv(&saved);
    flip = resultnegative && (mode == FE_UPWARD || mode == FE_DOWNWARD);
    if (flip)
        fesetround(mode == FE_DOWNWARD ? FE_UPWARD : FE_DOWNWARD);
    out = __ga_pack(y, scale, type);
    if (flip)
        fesetround(mode);
    return out | (resultnegative ? sign : 0);
}

#define GA(T,U,S,K) T lgamma##S(T x){union{T f;U u;}v;v.f=x;v.u=(U)__ga_bits(v.u,K,1);return v.f;} T tgamma##S(T x){union{T f;U u;}v;v.f=x;v.u=(U)__ga_bits(v.u,K,0);return v.f;}
GA(float, unsigned int, f, 13)
GA(double, unsigned long long, , 14)
GA(long double, unsigned long long, l, 15)
#undef GA
