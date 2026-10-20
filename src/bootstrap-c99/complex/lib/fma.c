#include "src/bootstrap-c99/include/math.h"
#include <fenv.h>
#include <errno.h>

/* Exact product and sum on a fixed binary lattice. No floating intermediates. */
static int __fm_bit(const unsigned int *a, int b) {
    return (a[b / 32] >> (b % 32)) & 1;
}

static unsigned long long __fm_bits(unsigned long long x, unsigned long long y,
                                    unsigned long long z, int p, int bias) {
    unsigned int a[136], b[136], u[2], v[2], prod[4];
    unsigned long long sign = 1ULL << (p + (p == 23 ? 8 : 11)),
                       inf = ((unsigned long long)(2 * bias + 1)) << p, lead = 1ULL << p;
    unsigned long long xx = x & (sign - 1), yy = y & (sign - 1), zz = z & (sign - 1), mx, my, mz, t,
                       carry, q, s = (x ^ y) & sign, sz = z & sign;
    int i, j, ex, ey, ez, base = p == 23 ? -298 : -2148, n = p == 23 ? 20 : 136, shift, h, H,
                          cmp = 0, borrow, mode = fegetround(), guard, sticky = 0, lost, inc = 0;
    if ((xx > inf && !(xx & (lead >> 1))) || (yy > inf && !(yy & (lead >> 1))) ||
        (zz > inf && !(zz & (lead >> 1))))
        feraiseexcept(FE_INVALID);
    if ((!xx && yy == inf) || (!yy && xx == inf)) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        if (xx <= inf && yy <= inf && zz <= inf)
            return inf | (lead >> 1);
    }
    if (xx > inf)
        return x | (lead >> 1);
    if (yy > inf)
        return y | (lead >> 1);
    if (zz > inf)
        return z | (lead >> 1);
    if (xx == inf || yy == inf) {
        if (zz == inf && s != sz) {
            errno = EDOM;
            feraiseexcept(FE_INVALID);
            return inf | (lead >> 1);
        }
        return s | inf;
    }
    if (zz == inf)
        return z;
    ex = (int)(xx >> p);
    ey = (int)(yy >> p);
    ez = (int)(zz >> p);
    mx = xx & (lead - 1);
    my = yy & (lead - 1);
    mz = zz & (lead - 1);
    if (ex)
        mx |= lead;
    if (ey)
        my |= lead;
    if (ez)
        mz |= lead;
    ex = (ex ? ex : 1) - bias - p;
    ey = (ey ? ey : 1) - bias - p;
    ez = (ez ? ez : 1) - bias - p;
    for (i = 0; i < n; i++) {
        a[i] = 0;
        b[i] = 0;
    }
    for (i = 0; i < 4; i++)
        prod[i] = 0;
    u[0] = (unsigned int)mx;
    u[1] = (unsigned int)(mx >> 32);
    v[0] = (unsigned int)my;
    v[1] = (unsigned int)(my >> 32);
    for (i = 0; i < 2; i++) {
        carry = 0;
        for (j = 0; j < 2; j++) {
            t = (unsigned long long)u[i] * v[j] + prod[i + j] + carry;
            prod[i + j] = (unsigned int)t;
            carry = t >> 32;
        }
        prod[i + 2] = (unsigned int)carry;
    }
    shift = ex + ey - base;
    for (i = 0; i < 128; i++)
        if ((prod[i / 32] >> (i % 32)) & 1)
            a[(i + shift) / 32] |= 1U << ((i + shift) % 32);
    shift = ez - base;
    for (i = 0; i <= p; i++)
        if ((mz >> i) & 1)
            b[(i + shift) / 32] |= 1U << ((i + shift) % 32);
    if (s == sz) {
        carry = 0;
        for (i = 0; i < n; i++) {
            t = (unsigned long long)a[i] + b[i] + carry;
            a[i] = (unsigned int)t;
            carry = t >> 32;
        }
    } else {
        for (i = n - 1; i >= 0; i--)
            if (a[i] != b[i]) {
                cmp = a[i] > b[i] ? 1 : -1;
                break;
            }
        borrow = 0;
        for (i = 0; i < n; i++) {
            t = (unsigned long long)(cmp >= 0 ? b[i] : a[i]) + borrow;
            q = cmp >= 0 ? a[i] : b[i];
            a[i] = (unsigned int)(q - t);
            borrow = q < t;
        }
        if (cmp < 0)
            s = sz;
    }
    h = n * 32 - 1;
    while (h >= 0 && !__fm_bit(a, h))
        h--;
    if (h < 0)
        return s == sz ? s : (mode == FE_DOWNWARD ? sign : 0);
    H = h + base;
    shift = h - p;
    if (shift < 1 - bias - p - base)
        shift = 1 - bias - p - base;
    q = 0;
    for (i = 0; i <= p; i++)
        if (__fm_bit(a, shift + i))
            q |= 1ULL << i;
    guard = __fm_bit(a, shift - 1);
    for (i = 0; i < shift - 1; i++)
        if (__fm_bit(a, i)) {
            sticky = 1;
            break;
        }
    lost = guard || sticky;
    if (lost) {
        if (mode == FE_TONEAREST)
            inc = guard && (sticky || (q & 1));
        else if (mode == FE_UPWARD)
            inc = !s;
        else if (mode == FE_DOWNWARD)
            inc = s != 0;
    }
    q += inc;
    if (H >= 1 - bias && q == (lead << 1)) {
        q >>= 1;
        H++;
    }
    if (H > bias) {
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW | FE_INEXACT);
        return s |
               ((mode == FE_TONEAREST || (mode == FE_UPWARD && !s) || (mode == FE_DOWNWARD && s))
                    ? inf
                    : inf - 1);
    }
    if (lost) {
        if (H < 1 - bias && q < lead) {
            errno = ERANGE;
            feraiseexcept(FE_UNDERFLOW | FE_INEXACT);
        } else
            feraiseexcept(FE_INEXACT);
    }
    if (H < 1 - bias)
        return s | q;
    return s | ((unsigned long long)(H + bias) << p) | (q & (lead - 1));
}

#define FM(T,U,S,P,B) T fma##S(T x,T y,T z){union{T f;U u;}a,b,c;a.f=x;b.f=y;c.f=z;a.u=(U)__fm_bits(a.u,b.u,c.u,P,B);return a.f;}
FM(float, unsigned int, f, 23, 127)
FM(double, unsigned long long, , 52, 1023)
FM(long double, unsigned long long, l, 52, 1023)
#undef FM
