/* Integer-only conversion of C floating tokens to IEEE binary32/binary64.
   Built by the existing custom C compiler. No host floating arithmetic.
   fp_parse accepts an ASCII int array without a sign or whitespace.
   Return: 0 success, 1 invalid token, 2 overflow, 3 resource limit.
   fp_type: 13 float, 14 double, 15 long double (Windows binary64).
   fp_inexact and fp_underflow describe rounding; tininess is after rounding.
   Limits: 1 MiB token, 8192 significant mantissa digits, 32800-bit workspace.
   The workspaces make this helper non-reentrant. */
int fp_num[2050];
int fp_den[2050];
int fp_nn;
int fp_dn;
int fp_type;
int fp_inexact;
int fp_underflow;
unsigned long long fp_bits;

int fp_digit(int c, int base) {
    int d = -1;
    if (c >= '0' && c <= '9')
        d = c - '0';
    if (c >= 'a' && c <= 'f')
        d = c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        d = c - 'A' + 10;
    return d < base ? d : -1;
}

int fp_mul_add(int *a, int n, int m, int carry) {
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

int fp_bit_length(int *a, int n) {
    int bits = (n - 1) * 16, top = a[n - 1];
    while (top) {
        bits++;
        top >>= 1;
    }
    return bits;
}

int fp_shifted_word(int *a, int n, int shift, int word) {
    int k = word - shift / 16, r = shift % 16, x = 0;
    if (k >= 0 && k < n)
        x = (a[k] << r) & 65535;
    if (r && k > 0 && k <= n)
        x |= a[k - 1] >> (16 - r);
    return x;
}

int fp_compare(int *a, int an, int ash, int *b, int bn, int bsh) {
    int ab = fp_bit_length(a, an), bb = fp_bit_length(b, bn), i, x, y;
    if (ab)
        ab += ash;
    if (bb)
        bb += bsh;
    if (ab != bb)
        return ab > bb ? 1 : -1;
    for (i = (ab + 15) / 16 - 1; i >= 0; i--) {
        x = fp_shifted_word(a, an, ash, i);
        y = fp_shifted_word(b, bn, bsh, i);
        if (x != y)
            return x > y ? 1 : -1;
    }
    return 0;
}

int fp_shift(int *a, int n, int shift) {
    int words = shift / 16, r = shift % 16, extra, i, x;
    if (!fp_bit_length(a, n))
        return 1;
    extra = r && (a[n - 1] >> (16 - r)) != 0;
    if (n + words + extra > 2050)
        return 0;
    for (i = n + words + extra - 1; i >= 0; i--) {
        x = fp_shifted_word(a, n, shift, i);
        a[i] = x;
    }
    return n + words + extra;
}

int fp_subtract(int *a, int an, int *b, int bn, int shift) {
    int i, x, borrow = 0;
    for (i = 0; i < an; i++) {
        x = a[i] - fp_shifted_word(b, bn, shift, i) - borrow;
        borrow = x < 0;
        if (borrow)
            x += 65536;
        a[i] = x;
    }
    while (an > 1 && !a[an - 1])
        an--;
    return an;
}

int fp_round(int binary_exponent) {
    int p = fp_type == 13 ? 24 : 53, emin = fp_type == 13 ? -126 : -1022;
    int emax = fp_type == 13 ? 127 : 1023, bias = fp_type == 13 ? 127 : 1023;
    int e, ratio, cmp, qexp, shift, i;
    unsigned long long q = 0, leading = 1ULL << (p - 1);
    ratio = fp_bit_length(fp_num, fp_nn) - fp_bit_length(fp_den, fp_dn);
    if (ratio >= 0)
        cmp = fp_compare(fp_num, fp_nn, 0, fp_den, fp_dn, ratio);
    else
        cmp = fp_compare(fp_num, fp_nn, -ratio, fp_den, fp_dn, 0);
    if (cmp < 0)
        ratio--;
    e = ratio + binary_exponent;
    if (e > emax)
        return 2;
    if (e < emin - p) {
        fp_inexact = 1;
        fp_underflow = 1;
        return 0;
    }
    qexp = (e < emin ? emin : e) - (p - 1);
    shift = binary_exponent - qexp;
    if (shift >= 0)
        fp_nn = fp_shift(fp_num, fp_nn, shift);
    else
        fp_dn = fp_shift(fp_den, fp_dn, -shift);
    if (!fp_nn || !fp_dn)
        return 3;
    i = fp_bit_length(fp_num, fp_nn) - fp_bit_length(fp_den, fp_dn);
    if (i > 63)
        return 3;
    for (; i >= 0; i--) {
        if (fp_compare(fp_num, fp_nn, 0, fp_den, fp_dn, i) >= 0) {
            fp_nn = fp_subtract(fp_num, fp_nn, fp_den, fp_dn, i);
            q |= 1ULL << i;
        }
    }
    fp_inexact = fp_bit_length(fp_num, fp_nn) != 0;
    cmp = fp_compare(fp_num, fp_nn, 1, fp_den, fp_dn, 0);
    if (cmp > 0 || (cmp == 0 && (q & 1)))
        q++;
    if (e < emin) {
        fp_bits = q;
        fp_underflow = fp_inexact && q < leading;
    } else {
        if (q == (leading << 1)) {
            q >>= 1;
            e++;
        }
        if (e > emax)
            return 2;
        fp_bits = ((unsigned long long)(e + bias) << (p - 1)) | (q - leading);
    }
    return 0;
}

int fp_parse(int *s, int length) {
    int i = 0, base = 10, point = 0, exponent_seen = 0, digits = 0, fractional = 0;
    int significant = 0, first = 0, oversized = 0, d, c, exponent = 0, negative = 0;
    int k, order, top, precision, minimum, maximum;
    fp_bits = 0;
    fp_type = 14;
    fp_inexact = 0;
    fp_underflow = 0;
    fp_nn = 1;
    fp_dn = 1;
    fp_num[0] = 0;
    fp_den[0] = 1;
    if (length < 1)
        return 1;
    if (length > 1048576)
        return 3;
    if (length >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        i = 2;
    }
    while (i < length) {
        c = s[i];
        d = fp_digit(c, base);
        if (d >= 0) {
            digits++;
            if (point)
                fractional++;
            if (significant || d) {
                if (!significant)
                    first = d;
                significant++;
                if (significant > 8192)
                    oversized = 1;
                else {
                    fp_nn = fp_mul_add(fp_num, fp_nn, base, d);
                    if (!fp_nn)
                        oversized = 1;
                }
            }
            i++;
        } else if (c == '.' && !point) {
            point = 1;
            i++;
        } else
            break;
    }
    if (!digits)
        return 1;
    if (i < length && ((base == 10 && (s[i] == 'e' || s[i] == 'E')) ||
                       (base == 16 && (s[i] == 'p' || s[i] == 'P')))) {
        exponent_seen = 1;
        i++;
        if (i < length && (s[i] == '+' || s[i] == '-')) {
            negative = s[i] == '-';
            i++;
        }
        if (i == length || s[i] < '0' || s[i] > '9')
            return 1;
        while (i < length && s[i] >= '0' && s[i] <= '9') {
            if (exponent < 10000000) {
                exponent = exponent * 10 + s[i] - '0';
                if (exponent > 10000000)
                    exponent = 10000000;
            }
            i++;
        }
    }
    if (base == 16 && !exponent_seen)
        return 1;
    if (base == 10 && !point && !exponent_seen)
        return 1;
    if (i < length) {
        c = s[i++];
        if (c == 'f' || c == 'F')
            fp_type = 13;
        else if (c == 'l' || c == 'L')
            fp_type = 15;
        else
            return 1;
    }
    if (i != length)
        return 1;
    if (!significant)
        return 0;
    if (negative)
        exponent = -exponent;
    precision = fp_type == 13 ? 24 : 53;
    minimum = fp_type == 13 ? -126 : -1022;
    maximum = fp_type == 13 ? 127 : 1023;
    if (base == 16) {
        k = exponent - fractional * 4;
        top = 0;
        while (first > 1) {
            first >>= 1;
            top++;
        }
        order = (significant - 1) * 4 + top + k;
        if (order > maximum)
            return 2;
        if (order < minimum - precision) {
            fp_inexact = 1;
            fp_underflow = 1;
            return 0;
        }
        if (oversized)
            return 3;
        return fp_round(k);
    }
    k = exponent - fractional;
    order = significant - 1 + k;
    if (order > (fp_type == 13 ? 38 : 308))
        return 2;
    if (order < (fp_type == 13 ? -46 : -324)) {
        fp_inexact = 1;
        fp_underflow = 1;
        return 0;
    }
    if (oversized)
        return 3;
    if (k >= 0) {
        for (i = 0; i < k; i++) {
            fp_nn = fp_mul_add(fp_num, fp_nn, 5, 0);
            if (!fp_nn)
                return 3;
        }
    } else {
        for (i = 0; i < -k; i++) {
            fp_dn = fp_mul_add(fp_den, fp_dn, 5, 0);
            if (!fp_dn)
                return 3;
        }
    }
    return fp_round(k);
}
