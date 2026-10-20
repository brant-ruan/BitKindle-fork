#ifndef __C1_INTEGER_POWER_C
#define __C1_INTEGER_POWER_C

static int __c1_ipow_bit(unsigned int *a, int b) {
    return b < 0 || b >= 3456 ? 0 : (a[b / 32] >> (b % 32)) & 1;
}

/* At most 64 factors of a 53-bit odd significand: 3392 bits suffice.
   Rounding operates on the integer and binary scale together, avoiding
   double rounding when the final result is subnormal. */
static unsigned long long __c1_ipow_pack(unsigned long long m, int n, int scale, int negative,
                                         int type) {
    unsigned int a[108], b[108], factor[2];
    unsigned long long t, carry, q, bits, infinity, sign;
    int i, j, k, top, field, shift, guard, sticky, inexact;
    int p = type == 13 ? 23 : 52, bias = type == 13 ? 127 : 1023, mode = fegetround();
    infinity = (unsigned long long)(2 * bias + 1) << p;
    sign = negative ? __c1_m_sign(type) : 0;
    if (negative) {
        if (mode == FE_UPWARD)
            mode = FE_DOWNWARD;
        else if (mode == FE_DOWNWARD)
            mode = FE_UPWARD;
    }
    factor[0] = (unsigned int)m;
    factor[1] = (unsigned int)(m >> 32);
    for (i = 0; i < 108; i++)
        a[i] = 0;
    a[0] = 1;
    for (k = 0; k < n; k++) {
        for (i = 0; i < 108; i++)
            b[i] = 0;
        for (j = 0; j < 2; j++) {
            carry = 0;
            for (i = 0; i < 108 - j; i++) {
                t = (unsigned long long)a[i] * factor[j] + b[i + j] + carry;
                b[i + j] = (unsigned int)t;
                carry = t >> 32;
            }
        }
        for (i = 0; i < 108; i++)
            a[i] = b[i];
    }
    top = 3455;
    while (!__c1_ipow_bit(a, top))
        top--;
    field = top + scale + bias;
    if (field >= 2 * bias + 1) {
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW | FE_INEXACT);
        return sign | ((mode == FE_TONEAREST || mode == FE_UPWARD) ? infinity : infinity - 1);
    }
    shift = top - p;
    if (shift < 1 - bias - p - scale)
        shift = 1 - bias - p - scale;
    q = 0;
    for (i = 0; i <= p; i++)
        if (__c1_ipow_bit(a, shift + i))
            q |= 1ULL << i;
    guard = shift > 0 ? __c1_ipow_bit(a, shift - 1) : 0;
    sticky = 0;
    for (i = 0; i < shift - 1 && i <= top; i++)
        if (__c1_ipow_bit(a, i)) {
            sticky = 1;
            break;
        }
    inexact = guard || sticky;
    if (mode == FE_UPWARD && inexact)
        q++;
    if (mode == FE_TONEAREST && guard && (sticky || (q & 1)))
        q++;
    bits = field > 0 ? ((unsigned long long)(field - 1) << p) + q : q;
    if (bits >= infinity) {
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW | FE_INEXACT);
    } else if (inexact) {
        if (bits < (1ULL << p)) {
            errno = ERANGE;
            feraiseexcept(FE_UNDERFLOW | FE_INEXACT);
        } else
            feraiseexcept(FE_INEXACT);
    }
    return bits | sign;
}
#endif
