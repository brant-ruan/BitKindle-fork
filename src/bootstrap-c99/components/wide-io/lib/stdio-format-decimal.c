#ifndef __C1_STDIO_FORMAT_DECIMAL_C
#define __C1_STDIO_FORMAT_DECIMAL_C

/* Exact binary64 -> decimal digits; only integer arithmetic after decoding. */
typedef struct {
    char digit[1100];
    int length, exponent, sign, special;
} __c1_decimal;

static void __c1_decimal_exact(__c1_decimal *d, double value) {
    union {
        double f;
        unsigned long long u;
    } a;

    unsigned long long m;
    unsigned char little[1100];
    int exponent, n = 0, i, j, carry, v, mult;
    a.f = value;
    d->sign = (int)(a.u >> 63);
    exponent = (int)((a.u >> 52) & 2047);
    m = a.u & 0xfffffffffffffULL;
    d->special = exponent == 2047 ? (m ? 2 : 1) : 0;
    if (d->special) {
        d->length = 0;
        d->exponent = 0;
        return;
    }
    if (exponent)
        m |= 0x10000000000000ULL;
    exponent = exponent ? exponent - 1075 : -1074;
    if (!m) {
        d->digit[0] = '0';
        d->length = 1;
        d->exponent = 0;
        return;
    }
    while (m) {
        little[n++] = (unsigned char)(m % 10);
        m /= 10;
    }
    mult = exponent < 0 ? 5 : 2;
    j = exponent < 0 ? -exponent : exponent;
    while (j--) {
        carry = 0;
        for (i = 0; i < n; i++) {
            v = little[i] * mult + carry;
            little[i] = v % 10;
            carry = v / 10;
        }
        while (carry) {
            little[n++] = carry % 10;
            carry /= 10;
        }
    }
    d->exponent = n - 1 + (exponent < 0 ? exponent : 0);
    for (i = 0; i < n; i++)
        d->digit[i] = '0' + little[n - i - 1];
    while (n > 1 && d->digit[n - 1] == '0')
        n--;
    d->length = n;
}

static void __c1_decimal_round(__c1_decimal *d, int keep) {
    int i, nonzero = 0, guard = 0, sticky = 0, up = 0, mode = fegetround();
    if (keep >= d->length || d->special)
        return;
    for (i = keep > 0 ? keep : 0; i < d->length; i++)
        if (d->digit[i] != '0')
            nonzero = 1;
    if (!nonzero)
        return;
    if (keep >= 0) {
        guard = d->digit[keep] - '0';
        for (i = keep + 1; i < d->length; i++)
            if (d->digit[i] != '0')
                sticky = 1;
    }
    if (mode == FE_UPWARD)
        up = !d->sign;
    else if (mode == FE_DOWNWARD)
        up = d->sign;
    else if (mode == FE_TONEAREST)
        up =
            guard > 5 || (guard == 5 && (sticky || (keep > 0 && ((d->digit[keep - 1] - '0') & 1))));
    if (keep <= 0) {
        if (up) {
            d->exponent = d->exponent - keep + 1;
            d->digit[0] = '1';
        } else {
            d->exponent = 0;
            d->digit[0] = '0';
        }
        d->length = 1;
        return;
    }
    d->length = keep;
    if (up) {
        i = keep - 1;
        while (i >= 0 && d->digit[i] == '9')
            d->digit[i--] = '0';
        if (i >= 0)
            d->digit[i]++;
        else {
            d->digit[0] = '1';
            d->length = 1;
            d->exponent++;
        }
    }
    while (d->length > 1 && d->digit[d->length - 1] == '0')
        d->length--;
}

/* Emit a virtual digit interval. Leading/trailing zero runs need no allocation. */
static void __c1_decimal_digits(__c1_format_sink *s, __c1_decimal *d, int start, size_t count) {
    size_t n;
    if (start < 0) {
        n = (size_t)(-start);
        if (n > count)
            n = count;
        __c1_format_pad(s, '0', n);
        start += (int)n;
        count -= n;
    }
    if (count && start < d->length) {
        n = d->length - start;
        if (n > count)
            n = count;
        __c1_format_write(s, d->digit + start, n);
        start += (int)n;
        count -= n;
    }
    if (count)
        __c1_format_pad(s, '0', count);
}

static int __c1_format_decimal(__c1_format_sink *s, double value, int c, int precision, int width,
                               int left, int plus, int space, int alternate, int zero) {
    __c1_decimal d;
    int upper = c >= 'A' && c <= 'Z', kind = upper ? c + 32 : c;
    int scientific, p, sign, expdigits = 0, e, i;
    char expbuf[12], exponent[12], prefix;
    size_t body, padding, fraction, integer;
    long long keep;
    __c1_decimal_exact(&d, value);
    sign = d.sign || plus || space;
    prefix = d.sign ? '-' : plus ? '+' : ' ';
    if (d.special) {
        padding = width > 3 + sign ? width - 3 - sign : 0;
        if (!left)
            __c1_format_pad(s, ' ', padding);
        if (sign)
            __c1_format_write(s, &prefix, 1);
        __c1_format_write(s, d.special == 1 ? (upper ? "INF" : "inf") : (upper ? "NAN" : "nan"), 3);
        if (left)
            __c1_format_pad(s, ' ', padding);
        return s->error ? -1 : 0;
    }
    p = precision < 0 ? 6 : precision;
    if (kind == 'g' && !p)
        p = 1;
    keep = kind == 'f' ? (long long)d.exponent + 1 + p : kind == 'e' ? (long long)p + 1 : p;
    if (keep < d.length)
        __c1_decimal_round(&d, (int)keep);
    scientific = kind == 'e' || (kind == 'g' && (d.exponent < -4 || d.exponent >= p));
    if (kind == 'g') {
        if (scientific)
            fraction = p - 1;
        else
            fraction = p > d.exponent + 1 ? (size_t)((long long)p - d.exponent - 1) : 0;
        if (!alternate) {
            i = scientific ? d.length - 1 : d.length - d.exponent - 1;
            if (i < 0)
                i = 0;
            if (fraction > (size_t)i)
                fraction = i;
        }
    } else
        fraction = p;
    integer = scientific ? 1 : d.exponent >= 0 ? (size_t)d.exponent + 1 : 1;
    body = integer + fraction + ((fraction || alternate) ? 1 : 0);
    if (scientific) {
        e = d.exponent < 0 ? -d.exponent : d.exponent;
        do {
            expbuf[expdigits++] = '0' + e % 10;
            e /= 10;
        } while (e);
        while (expdigits < 2)
            expbuf[expdigits++] = '0';
        exponent[0] = upper ? 'E' : 'e';
        exponent[1] = d.exponent < 0 ? '-' : '+';
        for (i = 0; i < expdigits; i++)
            exponent[i + 2] = expbuf[expdigits - i - 1];
        body += expdigits + 2;
    }
    if (body + sign > 2147483647U - s->count) {
        errno = ERANGE;
        s->error = 1;
        return -1;
    }
    padding = (size_t)width > body + sign ? width - body - sign : 0;
    if (!left && !zero)
        __c1_format_pad(s, ' ', padding);
    if (sign)
        __c1_format_write(s, &prefix, 1);
    if (!left && zero)
        __c1_format_pad(s, '0', padding);
    if (scientific)
        __c1_decimal_digits(s, &d, 0, 1);
    else if (d.exponent < 0)
        __c1_format_write(s, "0", 1);
    else
        __c1_decimal_digits(s, &d, 0, integer);
    if (fraction || alternate)
        __c1_format_write(s, ".", 1);
    if (fraction)
        __c1_decimal_digits(s, &d, scientific ? 1 : d.exponent + 1, fraction);
    if (scientific)
        __c1_format_write(s, exponent, expdigits + 2);
    if (left)
        __c1_format_pad(s, ' ', padding);
    return s->error ? -1 : 0;
}

#endif
