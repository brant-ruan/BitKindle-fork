#ifndef __C1_STDIO_FORMAT_C
#define __C1_STDIO_FORMAT_C
#include "stdio-temporary.c"
#include <stdio.h>

/* Shared output engine; wide conversions use the currently supported C locale. */
typedef struct {
    FILE *file;
    char *text;
    size_t capacity, count;
    int error;
} __c1_format_sink;

static int __c1_format_write(__c1_format_sink *s, const char *p, size_t n) {
    size_t amount;
    if (s->error)
        return -1;
    if (n > 2147483647U - s->count) {
        errno = ERANGE;
        s->error = 1;
        return -1;
    }
    if (s->file) {
        if (fwrite(p, 1, n, s->file) != n) {
            s->error = 1;
            return -1;
        }
    } else if (s->capacity && s->count < s->capacity - 1) {
        amount = s->capacity - 1 - s->count;
        if (amount > n)
            amount = n;
        memcpy(s->text + s->count, p, amount);
    }
    s->count += n;
    return 0;
}

static int __c1_format_pad(__c1_format_sink *s, int c, size_t n) {
    char b[64];
    size_t amount;
    if (s->error)
        return -1;
    if (n > 2147483647U - s->count) {
        errno = ERANGE;
        s->error = 1;
        return -1;
    }
    if (!s->file) {
        if (s->capacity && s->count < s->capacity - 1) {
            amount = s->capacity - 1 - s->count;
            if (amount > n)
                amount = n;
            memset(s->text + s->count, c, amount);
        }
        s->count += n;
        return 0;
    }
    memset(b, c, 64);
    while (n) {
        amount = n < 64 ? n : 64;
        if (__c1_format_write(s, b, amount))
            return -1;
        n -= amount;
    }
    return 0;
}

#include "stdio-format-decimal.c"
#include "stdio-format-hex.c"

static int __c1_format_wide(__c1_format_sink *s, const wchar_t *text, int code, int single,
                            int precision, int width, int left) {
    size_t n = 0, i, padding;
    char byte;
    if (single) {
        if (wctomb(&byte, (wchar_t)code) < 0) {
            s->error = 1;
            return -1;
        }
        n = 1;
    } else {
        while ((precision < 0 || n < (size_t)precision) && text[n]) {
            if (wctomb(&byte, text[n]) < 0) {
                s->error = 1;
                return -1;
            }
            n++;
        }
    }
    padding = (size_t)width > n ? width - n : 0;
    if (!left)
        __c1_format_pad(s, ' ', padding);
    if (single)
        __c1_format_write(s, &byte, 1);
    else
        for (i = 0; i < n && !s->error; i++) {
            wctomb(&byte, text[i]);
            __c1_format_write(s, &byte, 1);
        }
    if (left)
        __c1_format_pad(s, ' ', padding);
    return s->error ? -1 : 0;
}

static int __c1_format_number(const char **p) {
    int n = 0, d;
    while (**p >= '0' && **p <= '9') {
        d = *(*p)++ - '0';
        if (n > (2147483647 - d) / 10)
            return -1;
        n = n * 10 + d;
    }
    return n;
}

static int __c1_format_run(__c1_format_sink *s, const char *format, va_list ap) {
    int left, plus, space, alternate, zero, width, precision, length, c, negative, base, upper;
    int digits, prefixlen, padding, zeros;
    unsigned long long value;
    long long signedvalue;
    char reverse[65], number[65], prefix[3], character;
    const char *p, *begin;
    size_t n;
    int i;
    while (*format && !s->error) {
        if (*format != '%') {
            begin = format;
            while (*format && *format != '%')
                format++;
            __c1_format_write(s, begin, format - begin);
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
            width = __c1_format_number(&format);
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
                precision = __c1_format_number(&format);
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
            __c1_format_write(s, "%", 1);
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
                    __c1_format_wide(s, NULL, va_arg(ap, int), 1, precision, width, left);
                else
                    __c1_format_wide(s, va_arg(ap, wchar_t *), 0, 0, precision, width, left);
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
                __c1_format_pad(s, ' ', padding);
            __c1_format_write(s, p, n);
            if (left)
                __c1_format_pad(s, ' ', padding);
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
                __c1_format_hex(s, fp, c == 'A', precision, width, left, plus, space, alternate,
                                zero);
            else
                __c1_format_decimal(s, fp, c, precision, width, left, plus, space, alternate, zero);
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
            __c1_format_pad(s, ' ', padding);
        __c1_format_write(s, prefix, prefixlen);
        if (!left && zero && precision < 0)
            __c1_format_pad(s, '0', padding);
        __c1_format_pad(s, '0', zeros);
        __c1_format_write(s, number, digits);
        if (left)
            __c1_format_pad(s, ' ', padding);
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

int vsnprintf(char *text, size_t capacity, const char *format, va_list ap) {
    __c1_format_sink sink;
    int result;
    sink.file = NULL;
    sink.text = text;
    sink.capacity = capacity;
    sink.count = 0;
    sink.error = 0;
    result = __c1_format_run(&sink, format, ap);
    if (capacity)
        text[sink.count < capacity ? sink.count : capacity - 1] = 0;
    return result;
}

int vsprintf(char *text, const char *format, va_list ap) {
    return vsnprintf(text, (size_t)-1, format, ap);
}

int vfprintf(FILE *file, const char *format, va_list ap) {
    __c1_format_sink sink;
    if (__c1_file_orient(file, -1))
        return -1;
    sink.file = file;
    sink.text = NULL;
    sink.capacity = 0;
    sink.count = 0;
    sink.error = 0;
    return __c1_format_run(&sink, format, ap);
}

int vprintf(const char *format, va_list ap) {
    return vfprintf(stdout, format, ap);
}

int snprintf(char *text, size_t capacity, const char *format, ...) {
    va_list ap;
    int result;
    va_start(ap, format);
    result = vsnprintf(text, capacity, format, ap);
    va_end(ap);
    return result;
}

int sprintf(char *text, const char *format, ...) {
    va_list ap;
    int result;
    va_start(ap, format);
    result = vsprintf(text, format, ap);
    va_end(ap);
    return result;
}

int fprintf(FILE *file, const char *format, ...) {
    va_list ap;
    int result;
    va_start(ap, format);
    result = vfprintf(file, format, ap);
    va_end(ap);
    return result;
}

int printf(const char *format, ...) {
    va_list ap;
    int result;
    va_start(ap, format);
    result = vprintf(format, ap);
    va_end(ap);
    return result;
}

#endif
