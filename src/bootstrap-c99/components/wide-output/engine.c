static int __c1_wout_format_number(const wchar_t **p) {
    int n = 0, d;
    while (**p >= '0' && **p <= '9') {
        d = *(*p)++ - '0';
        if (n > (2147483647 - d) / 10)
            return -1;
        n = n * 10 + d;
    }
    return n;
}

static int __c1_wout_format_run(__c1_wout_format_sink *s, const wchar_t *format, va_list ap) {
    int left, plus, space, alternate, zero, width, precision, length, c, negative, base, upper;
    int digits, prefixlen, padding, zeros;
    unsigned long long value;
    long long signedvalue;
    char reverse[65], number[65], prefix[3], character;
    const char *p;
    const wchar_t *begin;
    size_t n;
    int i;
    while (*format && !s->error) {
        if (*format != '%') {
            begin = format;
            while (*format && *format != '%')
                format++;
            __c1_wout_format_units(s, begin, format - begin);
            continue;
        }
        format++;
        left = 0;
        plus = 0;
        space = 0;
        alternate = 0;
        zero = 0;
        while (1) {
            c = *format;
            if (c == '-')
                left = 1;
            else if (c == '+')
                plus = 1;
            else if (c == ' ')
                space = 1;
            else if (c == '#')
                alternate = 1;
            else if (c == '0')
                zero = 1;
            else
                break;
            format++;
        }
        if (*format == '*') {
            format++;
            width = va_arg(ap, int);
            if (width == (-2147483647 - 1))
                goto overflow;
            if (width < 0) {
                left = 1;
                width = -width;
            }
        } else {
            width = __c1_wout_format_number(&format);
            if (width < 0)
                goto overflow;
        }
        precision = -1;
        if (*format == '.') {
            format++;
            if (*format == '*') {
                format++;
                precision = va_arg(ap, int);
                if (precision < 0)
                    precision = -1;
            } else {
                precision = __c1_wout_format_number(&format);
                if (precision < 0)
                    goto overflow;
            }
        }
        length = 0;
        if (*format == 'h') {
            format++;
            length = 1;
            if (*format == 'h') {
                format++;
                length = 2;
            }
        } else if (*format == 'l') {
            format++;
            length = 3;
            if (*format == 'l') {
                format++;
                length = 4;
            }
        } else if (*format == 'j') {
            format++;
            length = 5;
        } else if (*format == 'z') {
            format++;
            length = 6;
        } else if (*format == 't') {
            format++;
            length = 7;
        } else if (*format == 'L') {
            format++;
            length = 8;
        }
        c = *format;
        if (!c)
            goto invalid;
        format++;
        if (c == '%') {
            __c1_wout_format_write(s, "%", 1);
            continue;
        }
        if (c == 'n') {
            if (length == 2)
                *va_arg(ap, signed char *) = (signed char)s->count;
            else if (length == 1)
                *va_arg(ap, short *) = (short)s->count;
            else if (length == 3)
                *va_arg(ap, long *) = (long)s->count;
            else if (length >= 4 && length <= 7)
                *va_arg(ap, long long *) = (long long)s->count;
            else if (!length)
                *va_arg(ap, int *) = (int)s->count;
            else
                goto invalid;
            continue;
        }
        if (c == 's' || c == 'c') {
            if (length == 3) {
                if (c == 'c')
                    __c1_wout_format_wide(s, NULL, va_arg(ap, wint_t), 1, precision, width, left);
                else
                    __c1_wout_format_wide(s, va_arg(ap, wchar_t *), 0, 0, precision, width, left);
                continue;
            }
            if (length)
                goto invalid;
            if (c == 'c') {
                character = (char)va_arg(ap, int);
                p = &character;
                n = 1;
            } else {
                p = va_arg(ap, char *);
                if (!p)
                    p = "(null)";
                n = 0;
                while ((precision < 0 || n < (size_t)precision) && p[n])
                    n++;
            }
            padding = (size_t)width > n ? width - (int)n : 0;
            if (!left)
                __c1_wout_format_pad(s, ' ', padding);
            __c1_wout_format_write(s, p, n);
            if (left)
                __c1_wout_format_pad(s, ' ', padding);
            continue;
        }
        if (c == 'f' || c == 'F' || c == 'e' || c == 'E' || c == 'g' || c == 'G' || c == 'a' ||
            c == 'A') {
            double fp;
            if (length == 8)
                fp = va_arg(ap, long double);
            else if (!length || length == 3)
                fp = va_arg(ap, double);
            else
                goto invalid;
            if (c == 'a' || c == 'A')
                __c1_wout_format_hex(s, fp, c == 'A', precision, width, left, plus, space,
                                     alternate, zero);
            else
                __c1_wout_format_decimal(s, fp, c, precision, width, left, plus, space, alternate,
                                         zero);
            continue;
        }
        if (c != 'd' && c != 'i' && c != 'u' && c != 'o' && c != 'x' && c != 'X' && c != 'p')
            goto invalid;
        if (length == 8)
            goto invalid;
        negative = 0;
        prefixlen = 0;
        upper = c == 'X';
        base = c == 'o' ? 8 : (c == 'x' || c == 'X' || c == 'p') ? 16 : 10;
        if (c == 'p')
            value = (unsigned long long)va_arg(ap, void *);
        else if (c == 'd' || c == 'i') {
            if (length >= 4)
                signedvalue = va_arg(ap, long long);
            else if (length == 3)
                signedvalue = va_arg(ap, long);
            else {
                signedvalue = va_arg(ap, int);
                if (length == 1)
                    signedvalue = (short)signedvalue;
                else if (length == 2)
                    signedvalue = (signed char)signedvalue;
            }
            negative = signedvalue < 0;
            value = (unsigned long long)signedvalue;
            if (negative)
                value = 0ULL - value;
        } else {
            if (length >= 4)
                value = va_arg(ap, unsigned long long);
            else if (length == 3)
                value = va_arg(ap, unsigned long);
            else {
                value = va_arg(ap, unsigned int);
                if (length == 1)
                    value = (unsigned short)value;
                else if (length == 2)
                    value = (unsigned char)value;
            }
        }
        if (c == 'd' || c == 'i') {
            if (negative)
                prefix[prefixlen++] = '-';
            else if (plus)
                prefix[prefixlen++] = '+';
            else if (space)
                prefix[prefixlen++] = ' ';
        }
        if (c == 'p' || ((c == 'x' || c == 'X') && alternate && value)) {
            prefix[prefixlen++] = '0';
            prefix[prefixlen++] = upper ? 'X' : 'x';
        }
        digits = 0;
        if (value || precision != 0 || c == 'p') {
            do {
                reverse[digits++] = (upper ? "0123456789ABCDEF" : "0123456789abcdef")[value % base];
                value /= base;
            } while (value);
        }
        for (i = 0; i < digits; i++)
            number[i] = reverse[digits - i - 1];
        if (c == 'o' && alternate && (!digits || number[0] != '0') && precision <= digits)
            precision = digits + 1;
        zeros = precision > digits ? precision - digits : 0;
        if (zeros > 2147483647 - digits - prefixlen)
            goto overflow;
        padding = width > prefixlen + zeros + digits ? width - prefixlen - zeros - digits : 0;
        if (!left && !(zero && precision < 0))
            __c1_wout_format_pad(s, ' ', padding);
        __c1_wout_format_write(s, prefix, prefixlen);
        if (!left && zero && precision < 0)
            __c1_wout_format_pad(s, '0', padding);
        __c1_wout_format_pad(s, '0', zeros);
        __c1_wout_format_write(s, number, digits);
        if (left)
            __c1_wout_format_pad(s, ' ', padding);
    }
    return s->error ? -1 : (int)s->count;
overflow:
    errno = ERANGE;
    s->error = 1;
    return -1;
invalid:
    errno = EINVAL;
    s->error = 1;
    return -1;
}
