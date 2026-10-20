/* Private copies of the self-built runtime DD arithmetic and exponential kernel. */
struct __ef_dd {
    double hi, lo;
};

static struct __ef_dd __ef_dd_sum(double a, double b) {
    struct __ef_dd z;
    double v;
    z.hi = a + b;
    v = z.hi - a;
    z.lo = (a - (z.hi - v)) + (b - v);
    return z;
}

static struct __ef_dd __ef_dd_add(struct __ef_dd a, struct __ef_dd b) {
    struct __ef_dd z = __ef_dd_sum(a.hi, b.hi);
    return __ef_dd_sum(z.hi, z.lo + a.lo + b.lo);
}

static struct __ef_dd __ef_dd_product(double a, double b) {
    struct __ef_dd z;
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

static struct __ef_dd __ef_dd_mul(struct __ef_dd a, struct __ef_dd b) {
    struct __ef_dd z = __ef_dd_product(a.hi, b.hi);
    return __ef_dd_sum(z.hi, z.lo + a.hi * b.lo + a.lo * b.hi);
}

static struct __ef_dd __ef_dd_divide(struct __ef_dd a, int n) {
    double q = a.hi / n;
    struct __ef_dd p = __ef_dd_product(q, (double)n);
    return __ef_dd_sum(q, ((a.hi - p.hi) - p.lo + a.lo) / n);
}

/* Positive double-double value times 2^scale, rounded directly to target
   precision, including subnormals. The residual breaks halfway cases. */
static unsigned long long __ef_pack(struct __ef_dd y, int scale, int type) {
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

static struct __ef_dd __ef_dd_ratio(struct __ef_dd a, struct __ef_dd b) {
    struct __ef_dd q, p, r;
    q.hi = a.hi / b.hi;
    q.lo = 0;
    p = __ef_dd_mul(q, b);
    p.hi = -p.hi;
    p.lo = -p.lo;
    r = __ef_dd_add(a, p);
    return __ef_dd_sum(q.hi, (r.hi + r.lo) / b.hi);
}

static struct __ef_dd __ef_exp(struct __ef_dd x, int *scale) {
    struct __ef_dd n, ln2, r, term, sum;
    int i;
    *scale = (int)(x.hi * 0x1.71547652b82fep+0 + (x.hi > 0 ? 0.5 : -0.5));
    n.hi = -(double)*scale;
    n.lo = 0;
    ln2.hi = 0x1.62e42fefa39efp-1;
    ln2.lo = 0x1.abc9e3b39803fp-56;
    r = __ef_dd_add(x, __ef_dd_mul(n, ln2));
    sum.hi = 1;
    sum.lo = 0;
    term = sum;
    for (i = 1; i <= 26; i++) {
        term = __ef_dd_divide(__ef_dd_mul(term, r), i);
        sum = __ef_dd_add(sum, term);
    }
    return sum;
}
