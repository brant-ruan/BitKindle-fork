#ifndef __C1_STRTOFLOAT_CORE_C
#define __C1_STRTOFLOAT_CORE_C
/* Own integer rational converter, adapted for runtime rounding and a sticky tail. */
static int __c1_sf_mode, __c1_sf_negative, __c1_sf_sticky;
static int __c1_sf_num[2050];
static int __c1_sf_den[2050];
static int __c1_sf_nn;
static int __c1_sf_dn;
static int __c1_sf_type;
static int __c1_sf_inexact;
static int __c1_sf_underflow;
static unsigned long long __c1_sf_bits;

static int __c1_sf_digit(int c, int base) {
    int d = -1;
    if (c >= '0' && c <= '9')
        d = c - '0';
    if (c >= 'a' && c <= 'f')
        d = c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        d = c - 'A' + 10;
    return d < base ? d : -1;
}

static int __c1_sf_mul_add(int *a, int n, int m, int carry) {
    int i, x;
    for (i = 0; i < n; i++) {
        x = a[i] * m + carry;
        a[i] = x & 65535;
        carry = x >> 16;
    }
    if (carry) {
        if (n == 2050)
            return 0;
        a[n++] = carry;
    }
    return n;
}

static int __c1_sf_bit_length(int *a, int n) {
    int bits = (n - 1) * 16, top = a[n - 1];
    while (top) {
        bits++;
        top >>= 1;
    }
    return bits;
}

static int __c1_sf_shifted_word(int *a, int n, int shift, int word) {
    int k = word - shift / 16, r = shift % 16, x = 0;
    if (k >= 0 && k < n)
        x = (a[k] << r) & 65535;
    if (r && k > 0 && k <= n)
        x |= a[k - 1] >> (16 - r);
    return x;
}

static int __c1_sf_compare(int *a, int an, int ash, int *b, int bn, int bsh) {
    int ab = __c1_sf_bit_length(a, an), bb = __c1_sf_bit_length(b, bn), i, x, y;
    if (ab)
        ab += ash;
    if (bb)
        bb += bsh;
    if (ab != bb)
        return ab > bb ? 1 : -1;
    for (i = (ab + 15) / 16 - 1; i >= 0; i--) {
        x = __c1_sf_shifted_word(a, an, ash, i);
        y = __c1_sf_shifted_word(b, bn, bsh, i);
        if (x != y)
            return x > y ? 1 : -1;
    }
    return 0;
}

static int __c1_sf_shift(int *a, int n, int shift) {
    int words = shift / 16, r = shift % 16, extra, i, x;
    if (!__c1_sf_bit_length(a, n))
        return 1;
    extra = r && (a[n - 1] >> (16 - r)) != 0;
    if (n + words + extra > 2050)
        return 0;
    for (i = n + words + extra - 1; i >= 0; i--) {
        x = __c1_sf_shifted_word(a, n, shift, i);
        a[i] = x;
    }
    return n + words + extra;
}

static int __c1_sf_subtract(int *a, int an, int *b, int bn, int shift) {
    int i, x, borrow = 0;
    for (i = 0; i < an; i++) {
        x = a[i] - __c1_sf_shifted_word(b, bn, shift, i) - borrow;
        borrow = x < 0;
        if (borrow)
            x += 65536;
        a[i] = x;
    }
    while (an > 1 && !a[an - 1])
        an--;
    return an;
}

static int __c1_sf_round(int binary_exponent) {
    int p = __c1_sf_type == 13 ? 24 : 53, emin = __c1_sf_type == 13 ? -126 : -1022;
    int emax = __c1_sf_type == 13 ? 127 : 1023, bias = __c1_sf_type == 13 ? 127 : 1023;
    int e, ratio, cmp, qexp, shift, i;
    unsigned long long q = 0, leading = 1ULL << (p - 1);
    ratio =
        __c1_sf_bit_length(__c1_sf_num, __c1_sf_nn) - __c1_sf_bit_length(__c1_sf_den, __c1_sf_dn);
    if (ratio >= 0)
        cmp = __c1_sf_compare(__c1_sf_num, __c1_sf_nn, 0, __c1_sf_den, __c1_sf_dn, ratio);
    else
        cmp = __c1_sf_compare(__c1_sf_num, __c1_sf_nn, -ratio, __c1_sf_den, __c1_sf_dn, 0);
    if (cmp < 0)
        ratio--;
    e = ratio + binary_exponent;
    if (e > emax)
        return 2;
    if (e < emin - p) {
        __c1_sf_inexact = 1;
        __c1_sf_underflow = 1;
        __c1_sf_bits =
            ((__c1_sf_mode == 1 && __c1_sf_negative) || (__c1_sf_mode == 2 && !__c1_sf_negative))
                ? 1
                : 0;
        return 0;
    }
    qexp = (e < emin ? emin : e) - (p - 1);
    shift = binary_exponent - qexp;
    if (shift >= 0)
        __c1_sf_nn = __c1_sf_shift(__c1_sf_num, __c1_sf_nn, shift);
    else
        __c1_sf_dn = __c1_sf_shift(__c1_sf_den, __c1_sf_dn, -shift);
    if (!__c1_sf_nn || !__c1_sf_dn)
        return 3;
    i = __c1_sf_bit_length(__c1_sf_num, __c1_sf_nn) - __c1_sf_bit_length(__c1_sf_den, __c1_sf_dn);
    if (i > 63)
        return 3;
    for (; i >= 0; i--) {
        if (__c1_sf_compare(__c1_sf_num, __c1_sf_nn, 0, __c1_sf_den, __c1_sf_dn, i) >= 0) {
            __c1_sf_nn = __c1_sf_subtract(__c1_sf_num, __c1_sf_nn, __c1_sf_den, __c1_sf_dn, i);
            q |= 1ULL << i;
        }
    }
    __c1_sf_inexact = __c1_sf_sticky || __c1_sf_bit_length(__c1_sf_num, __c1_sf_nn) != 0;
    cmp = __c1_sf_compare(__c1_sf_num, __c1_sf_nn, 1, __c1_sf_den, __c1_sf_dn, 0);
    if (__c1_sf_mode == 0) {
        if (cmp > 0 || (cmp == 0 && ((q & 1) || __c1_sf_sticky)))
            q++;
    } else if (__c1_sf_inexact && ((__c1_sf_mode == 1 && __c1_sf_negative) ||
                                   (__c1_sf_mode == 2 && !__c1_sf_negative)))
        q++;
    if (e < emin) {
        __c1_sf_bits = q;
        __c1_sf_underflow = __c1_sf_inexact && q < leading;
    } else {
        if (q == (leading << 1)) {
            q >>= 1;
            e++;
        }
        if (e > emax)
            return 2;
        __c1_sf_bits = ((unsigned long long)(e + bias) << (p - 1)) | (q - leading);
    }
    return 0;
}

#endif
