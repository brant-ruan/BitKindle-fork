#ifndef __C1_WOUT_FORMAT_HEX_C
#define __C1_WOUT_FORMAT_HEX_C

static int __c1_wout_format_hex(__c1_wout_format_sink *s, double value, int upper, int precision,
                                int width, int left, int plus, int space, int alternate, int zero) {
    union {
        double f;
        unsigned long long u;
    } a;

    unsigned long long m, tail, half, kept;
    int sign, e, p, i, n = 0, shift, up, mode, negative;
    char fraction[13], reverse[8], exponent[8], leading, prefix;
    size_t body, padding, used;
    a.f = value;
    negative = (int)(a.u >> 63);
    e = (int)((a.u >> 52) & 2047);
    if (e == 2047)
        return __c1_wout_format_decimal(s, value, upper ? 'F' : 'f', precision, width, left, plus,
                                        space, alternate, zero);
    m = a.u & 0xfffffffffffffULL;
    if (e) {
        m |= 0x10000000000000ULL;
        e -= 1023;
    } else {
        e = -1022;
        if (m) {
            while (m < 0x10000000000000ULL) {
                m <<= 1;
                e--;
            }
        } else
            e = 0;
    }
    p = precision < 0 ? 13 : precision;
    if (p < 13) {
        shift = 52 - p * 4;
        tail = m & ((1ULL << shift) - 1);
        half = 1ULL << (shift - 1);
        kept = m >> shift;
        mode = fegetround();
        up = 0;
        if (tail) {
            if (mode == FE_UPWARD)
                up = !negative;
            else if (mode == FE_DOWNWARD)
                up = negative;
            else if (mode == FE_TONEAREST)
                up = tail > half || (tail == half && (kept & 1));
        }
        if (up)
            kept++;
        m = kept << shift;
        if (m == 0x20000000000000ULL) {
            m >>= 1;
            e++;
        }
    }
    for (i = 0; i < 13; i++)
        fraction[i] = (upper ? "0123456789ABCDEF" : "0123456789abcdef")[(m >> (48 - i * 4)) & 15];
    if (precision < 0)
        while (p && fraction[p - 1] == '0')
            p--;
    leading = m ? '1' : '0';
    prefix = negative ? '-' : plus ? '+' : ' ';
    sign = negative || plus || space;
    i = e < 0 ? -e : e;
    do {
        reverse[n++] = '0' + i % 10;
        i /= 10;
    } while (i);
    exponent[0] = upper ? 'P' : 'p';
    exponent[1] = e < 0 ? '-' : '+';
    for (i = 0; i < n; i++)
        exponent[i + 2] = reverse[n - i - 1];
    body = 3 + (size_t)p + ((p || alternate) ? 1 : 0) + n + 2;
    if (body + sign > 2147483647U - s->count) {
        errno = ERANGE;
        s->error = 1;
        return -1;
    }
    padding = (size_t)width > body + sign ? width - body - sign : 0;
    if (!left && !zero)
        __c1_wout_format_pad(s, ' ', padding);
    if (sign)
        __c1_wout_format_write(s, &prefix, 1);
    __c1_wout_format_write(s, upper ? "0X" : "0x", 2);
    if (!left && zero)
        __c1_wout_format_pad(s, '0', padding);
    __c1_wout_format_write(s, &leading, 1);
    if (p || alternate)
        __c1_wout_format_write(s, ".", 1);
    used = p > 13 ? 13 : p;
    __c1_wout_format_write(s, fraction, used);
    if (p > 13)
        __c1_wout_format_pad(s, '0', (size_t)p - 13);
    __c1_wout_format_write(s, exponent, n + 2);
    if (left)
        __c1_wout_format_pad(s, ' ', padding);
    return s->error ? -1 : 0;
}

#endif
