#include "src/bootstrap-c99/include/math.h"
#include <fenv.h>

/* Six base-2^32 limbs suffice for a 54-bit midpoint cubed (162 bits). */
static void __cb_mul(const unsigned int *a, int na, const unsigned int *b, int nb,
                     unsigned int *out) {
    int i, j;
    unsigned long long carry, v;
    for (i = 0; i < 6; i++)
        out[i] = 0;
    for (i = 0; i < na; i++) {
        carry = 0;
        for (j = 0; j < nb; j++) {
            v = (unsigned long long)a[i] * b[j] + out[i + j] + carry;
            out[i + j] = (unsigned int)v;
            carry = v >> 32;
        }
        out[i + nb] = (unsigned int)carry;
    }
}

static int __cb_compare(unsigned long long q, const unsigned int *n) {
    unsigned int a[2], square[6], cube[6];
    int i;
    a[0] = (unsigned int)q;
    a[1] = (unsigned int)(q >> 32);
    __cb_mul(a, 2, a, 2, square);
    __cb_mul(square, 4, a, 2, cube);
    for (i = 5; i >= 0; i--) {
        if (cube[i] > n[i])
            return 1;
        if (cube[i] < n[i])
            return -1;
    }
    return 0;
}

static unsigned long long __cb_bits(unsigned long long input, int type) {
    int p = type == 13 ? 23 : 52, bias = type == 13 ? 127 : 1023, e, k, rem, i, shift, mode, cmp,
        increment = 0;
    unsigned long long sign = type == 13 ? 0x80000000ULL : 0x8000000000000000ULL;
    unsigned long long infinity = type == 13 ? 0x7f800000ULL : 0x7ff0000000000000ULL;
    unsigned long long quiet = type == 13 ? 0x400000ULL : 0x8000000000000ULL;
    unsigned long long a = input & (sign - 1), leading = 1ULL << p, m, low, high, mid, v, carry;
    unsigned int n[6], eight[6];
    if (a > infinity) {
        if (!(a & quiet))
            feraiseexcept(FE_INVALID);
        return input | quiet;
    }
    if (!a || a == infinity)
        return input;
    e = (int)(a >> p) - bias;
    m = (a & (leading - 1)) | leading;
    if (!(a >> p)) {
        e = 1 - bias;
        m = a;
        while (m < leading) {
            m <<= 1;
            e--;
        }
    }
    k = e / 3;
    rem = e % 3;
    if (rem < 0) {
        rem += 3;
        k--;
    }
    /* The desired root significand is cbrt(m * 2^(2*p+rem)). */
    shift = 2 * p + rem;
    for (i = 0; i < 6; i++)
        n[i] = 0;
    for (i = 0; i <= p; i++)
        if ((m >> i) & 1)
            n[(i + shift) / 32] |= 1U << ((i + shift) % 32);
    low = leading;
    high = leading << 1;
    while (high - low > 1) {
        mid = (low + high) >> 1;
        if (__cb_compare(mid, n) <= 0)
            low = mid;
        else
            high = mid;
    }
    if (__cb_compare(low, n) != 0) {
        mode = fegetround();
        if (mode == FE_TONEAREST) {
            carry = 0;
            for (i = 0; i < 6; i++) {
                v = ((unsigned long long)n[i] << 3) | carry;
                eight[i] = (unsigned int)v;
                carry = v >> 32;
            }
            cmp = __cb_compare(2 * low + 1, eight);
            increment = cmp < 0 || (cmp == 0 && (low & 1));
        } else if (mode == FE_UPWARD)
            increment = (input & sign) == 0;
        else if (mode == FE_DOWNWARD)
            increment = (input & sign) != 0;
        if (increment)
            low++;
        feraiseexcept(FE_INEXACT);
    }
    if (low == (leading << 1)) {
        low >>= 1;
        k++;
    }
    /* Every nonzero binary32/binary64 cube root is normal and finite. */
    return (input & sign) | ((unsigned long long)(k + bias) << p) | (low & (leading - 1));
}

#define CBRT(T,U,S,K) \
T cbrt##S(T x){union{T f;U u;}v;v.f=x;v.u=(U)__cb_bits(v.u,K);return v.f;}
CBRT(float, unsigned int, f, 13)
CBRT(double, unsigned long long, , 14)
CBRT(long double, unsigned long long, l, 15)
#undef CBRT
