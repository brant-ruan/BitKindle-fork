#ifndef __C1_WIDE_SCAN_C
#define __C1_WIDE_SCAN_C
#include "wchar.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

/* Shared wscanf reader and conversions for the current stateless C locale. */
typedef struct {
    FILE *file;
    const wchar_t *text;
    size_t count;
    int look;
} __c1_wscan;

static int __c1_wscan_peek(__c1_wscan *s) {
    if (s->look == -2)
        s->look = s->file ? (int)fgetwc(s->file) : *s->text ? *s->text : EOF;
    return s->look;
}

static int __c1_wscan_take(__c1_wscan *s) {
    int c = __c1_wscan_peek(s);
    if (c != EOF) {
        s->look = -2;
        s->count++;
        if (!s->file)
            s->text++;
    }
    return c;
}

static void __c1_wscan_space(__c1_wscan *s) {
    while (iswspace(__c1_wscan_peek(s)))
        __c1_wscan_take(s);
}

static void __c1_wscan_store(void *p, int length, int uns, unsigned long long n) {
    if (uns) {
        if (length == 2)
            *(unsigned char *)p = (unsigned char)n;
        else if (length == 1)
            *(unsigned short *)p = (unsigned short)n;
        else if (length == 3)
            *(unsigned long *)p = (unsigned long)n;
        else if (length >= 4)
            *(unsigned long long *)p = n;
        else
            *(unsigned int *)p = (unsigned int)n;
    } else {
        if (length == 2)
            *(signed char *)p = (signed char)n;
        else if (length == 1)
            *(short *)p = (short)n;
        else if (length == 3)
            *(long *)p = (long)n;
        else if (length >= 4)
            *(long long *)p = (long long)n;
        else
            *(int *)p = (int)n;
    }
}

static int __c1_wscan_digit(int c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

#include "float.c"

static int __c1_wscan_member(int c, const wchar_t *p, const wchar_t *end, int invert) {
    int found = 0, first, last;
    while (p < end) {
        first = *p++;
        if (end - p >= 2 && *p == '-' && first <= p[1]) {
            last = p[1];
            p += 2;
            if (c >= first && c <= last)
                found = 1;
        } else if (c == first)
            found = 1;
    }
    return invert ? !found : found;
}

static int __c1_wscan_run(__c1_wscan *s, const wchar_t *format, va_list ap) {
    int assigned = 0, completed = 0, c, conversion, suppress, width, length, base, automatic;
    int negative, digits, digit, invert, i, j, taken, specified, input_failure = 0;
    const wchar_t *set_begin, *set_end;
    unsigned long long number;
    void *target;
    while (*format) {
        if (iswspace(*format)) {
            while (iswspace(*format))
                format++;
            __c1_wscan_space(s);
            continue;
        }
        if (*format != '%') {
            c = __c1_wscan_peek(s);
            if (c == EOF) {
                input_failure = 1;
                break;
            }
            if (c != *format)
                break;
            __c1_wscan_take(s);
            format++;
            continue;
        }
        format++;
        suppress = 0;
        if (*format == '*') {
            suppress = 1;
            format++;
        }
        width = 0;
        while (*format >= '0' && *format <= '9') {
            i = *format++ - '0';
            if (width > (2147483647 - i) / 10)
                goto invalid;
            width = width * 10 + i;
        }
        specified = width != 0;
        if (!width)
            width = 2147483647;
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
        conversion = *format;
        if (!conversion)
            goto invalid;
        format++;
        if (conversion == 'n') {
            if (length == 8)
                goto invalid;
            if (!suppress)
                __c1_wscan_store(va_arg(ap, void *), length, 0, s->count);
            completed = 1;
            continue;
        }
        if (conversion != 'c' && conversion != '[')
            __c1_wscan_space(s);
        if (conversion == '%') {
            c = __c1_wscan_peek(s);
            if (c == EOF) {
                input_failure = 1;
                break;
            }
            if (c != '%')
                break;
            __c1_wscan_take(s);
            completed = 1;
            continue;
        }
        if (conversion == 's' || conversion == 'c' || conversion == '[') {
            if (length && length != 3)
                goto invalid;
            if (conversion == 'c' && !specified)
                width = 1;
            if (conversion == '[') {
                invert = 0;
                if (*format == '^') {
                    invert = 1;
                    format++;
                }
                set_begin = format;
                if (*format == ']')
                    format++;
                while (*format && *format != ']')
                    format++;
                if (*format != ']')
                    goto invalid;
                set_end = format++;
            }
            target = suppress ? NULL : va_arg(ap, void *);
            taken = 0;
            while (taken < width) {
                c = __c1_wscan_peek(s);
                if (c == EOF)
                    break;
                if (conversion == 's' && iswspace(c))
                    break;
                if (conversion == '[' && !__c1_wscan_member(c, set_begin, set_end, invert))
                    break;
                if (!suppress && length != 3 && c > 255) {
                    errno = EILSEQ;
                    input_failure = 1;
                    goto finish;
                }
                __c1_wscan_take(s);
                if (target) {
                    if (length == 3)
                        ((wchar_t *)target)[taken] = (wchar_t)c;
                    else
                        ((char *)target)[taken] = (char)c;
                }
                taken++;
            }
            if (!taken) {
                input_failure = __c1_wscan_peek(s) == EOF;
                break;
            }
            if (conversion == 'c' && taken != width)
                break;
            if (target && conversion != 'c') {
                if (length == 3)
                    ((wchar_t *)target)[taken] = 0;
                else
                    ((char *)target)[taken] = 0;
            }
            if (!suppress)
                assigned++;
            completed = 1;
            continue;
        }
        if (conversion == 'f' || conversion == 'F' || conversion == 'e' || conversion == 'E' ||
            conversion == 'g' || conversion == 'G' || conversion == 'a' || conversion == 'A') {
            char *token;
            int status;
            if (length && length != 3 && length != 8)
                goto invalid;
            if (__c1_wscan_peek(s) == EOF) {
                input_failure = 1;
                break;
            }
            status = __c1_wscan_float_token(s, width, &token);
            if (status <= 0) {
                if (status < 0)
                    input_failure = 1;
                break;
            }
            if (!suppress) {
                if (length == 8)
                    *va_arg(ap, long double *) = strtold(token, NULL);
                else if (length == 3)
                    *va_arg(ap, double *) = strtod(token, NULL);
                else
                    *va_arg(ap, float *) = strtof(token, NULL);
                assigned++;
            }
            free(token);
            completed = 1;
            continue;
        }
        if (conversion != 'd' && conversion != 'i' && conversion != 'u' && conversion != 'o' &&
            conversion != 'x' && conversion != 'X' && conversion != 'p')
            goto invalid;
        if (length == 8)
            goto invalid;
        c = __c1_wscan_peek(s);
        if (c == EOF) {
            input_failure = 1;
            break;
        }
        negative = 0;
        if (c == '+' || c == '-') {
            negative = c == '-';
            __c1_wscan_take(s);
            width--;
        }
        number = 0;
        digits = 0;
        automatic = conversion == 'i';
        base = conversion == 'o'                                               ? 8
               : (conversion == 'x' || conversion == 'X' || conversion == 'p') ? 16
                                                                               : 10;
        if (width && __c1_wscan_peek(s) == '0') {
            __c1_wscan_take(s);
            width--;
            digits = 1;
            if (automatic)
                base = 8;
            if (width && (automatic || base == 16) &&
                (__c1_wscan_peek(s) == 'x' || __c1_wscan_peek(s) == 'X')) {
                __c1_wscan_take(s);
                width--;
                base = 16;
                digits = 0;
            }
        }
        while (width) {
            digit = __c1_wscan_digit(__c1_wscan_peek(s));
            if (digit < 0 || digit >= base)
                break;
            __c1_wscan_take(s);
            width--;
            digits = 1;
            number = number * base + digit;
        }
        if (!digits)
            break;
        if (negative)
            number = 0ULL - number;
        if (!suppress) {
            target = va_arg(ap, void *);
            if (conversion == 'p')
                *(void **)target = (void *)number;
            else
                __c1_wscan_store(target, length, conversion != 'd' && conversion != 'i', number);
            assigned++;
        }
        completed = 1;
    }
finish:
    return input_failure && !completed ? EOF : assigned;
invalid:
    errno = EINVAL;
    return assigned;
}

int vswscanf(const wchar_t *text, const wchar_t *format, va_list ap) {
    __c1_wscan s;
    s.file = NULL;
    s.text = text;
    s.count = 0;
    s.look = -2;
    return __c1_wscan_run(&s, format, ap);
}

int vfwscanf(FILE *file, const wchar_t *format, va_list ap) {
    __c1_wscan s;
    int result;
    s.file = file;
    s.text = NULL;
    s.count = 0;
    s.look = -2;
    if (__c1_file_orient(file, 1))
        return EOF;
    result = __c1_wscan_run(&s, format, ap);
    if (s.look >= 0)
        ungetwc((wint_t)s.look, file);
    return result;
}

int vwscanf(const wchar_t *format, va_list ap) {
    return vfwscanf(stdin, format, ap);
}

int swscanf(const wchar_t *text, const wchar_t *format, ...) {
    va_list ap;
    int result;
    va_start(ap, format);
    result = vswscanf(text, format, ap);
    va_end(ap);
    return result;
}

int fwscanf(FILE *file, const wchar_t *format, ...) {
    va_list ap;
    int result;
    va_start(ap, format);
    result = vfwscanf(file, format, ap);
    va_end(ap);
    return result;
}

int wscanf(const wchar_t *format, ...) {
    va_list ap;
    int result;
    va_start(ap, format);
    result = vwscanf(format, ap);
    va_end(ap);
    return result;
}

#endif
