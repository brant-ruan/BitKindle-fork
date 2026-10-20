#ifndef __C1_STDIO_SCAN_C
#define __C1_STDIO_SCAN_C
#include "stdio-format.c"
#include <stdio.h>

/* Shared scanf reader and conversions for the current stateless C locale. */
typedef struct {
    FILE *file;
    const unsigned char *text;
    size_t count;
    int look;
} __c1_scan;

static int __c1_scan_peek(__c1_scan *s) {
    if (s->look == -2)
        s->look = s->file ? fgetc(s->file) : *s->text ? *s->text : EOF;
    return s->look;
}

static int __c1_scan_take(__c1_scan *s) {
    int c = __c1_scan_peek(s);
    if (c != EOF) {
        s->look = -2;
        s->count++;
        if (!s->file)
            s->text++;
    }
    return c;
}

static void __c1_scan_space(__c1_scan *s) {
    while (isspace(__c1_scan_peek(s)))
        __c1_scan_take(s);
}

static void __c1_scan_store(void *p, int length, int uns, unsigned long long n) {
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

static int __c1_scan_digit(int c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

#include "stdio-scan-float.c"

static int __c1_scan_run(__c1_scan *s, const char *format, va_list ap) {
    int assigned = 0, completed = 0, c, conversion, suppress, width, length, base, automatic;
    int negative, digits, digit, invert, i, j, taken, specified, input_failure = 0;
    unsigned char set[256];
    unsigned long long number;
    void *target;
    while (*format) {
        if (isspace((unsigned char)*format)) {
            while (isspace((unsigned char)*format))
                format++;
            __c1_scan_space(s);
            continue;
        }
        if (*format != '%') {
            c = __c1_scan_peek(s);
            if (c == EOF) {
                input_failure = 1;
                break;
            }
            if (c != (unsigned char)*format)
                break;
            __c1_scan_take(s);
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
                __c1_scan_store(va_arg(ap, void *), length, 0, s->count);
            completed = 1;
            continue;
        }
        if (conversion != 'c' && conversion != '[')
            __c1_scan_space(s);
        if (conversion == '%') {
            c = __c1_scan_peek(s);
            if (c == EOF) {
                input_failure = 1;
                break;
            }
            if (c != '%')
                break;
            __c1_scan_take(s);
            completed = 1;
            continue;
        }
        if (conversion == 's' || conversion == 'c' || conversion == '[') {
            if (length && length != 3)
                goto invalid;
            if (conversion == 'c' && !specified)
                width = 1;
            if (conversion == '[') {
                memset(set, 0, 256);
                invert = 0;
                if (*format == '^') {
                    invert = 1;
                    format++;
                }
                if (*format == ']') {
                    set[']'] = 1;
                    format++;
                }
                while (*format && *format != ']') {
                    i = (unsigned char)*format++;
                    if (*format == '-' && format[1] && format[1] != ']' &&
                        i <= (unsigned char)format[1]) {
                        j = (unsigned char)format[1];
                        format += 2;
                        while (i <= j)
                            set[i++] = 1;
                    } else
                        set[i] = 1;
                }
                if (*format != ']')
                    goto invalid;
                format++;
                if (invert)
                    for (i = 0; i < 256; i++)
                        set[i] = !set[i];
            }
            target = suppress ? NULL : va_arg(ap, void *);
            taken = 0;
            while (taken < width) {
                c = __c1_scan_peek(s);
                if (c == EOF)
                    break;
                if (conversion == 's' && isspace(c))
                    break;
                if (conversion == '[' && !set[c])
                    break;
                __c1_scan_take(s);
                if (target) {
                    if (length == 3)
                        ((wchar_t *)target)[taken] = (wchar_t)c;
                    else
                        ((char *)target)[taken] = (char)c;
                }
                taken++;
            }
            if (!taken) {
                input_failure = __c1_scan_peek(s) == EOF;
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
            if (__c1_scan_peek(s) == EOF) {
                input_failure = 1;
                break;
            }
            status = __c1_scan_float_token(s, width, &token);
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
        c = __c1_scan_peek(s);
        if (c == EOF) {
            input_failure = 1;
            break;
        }
        negative = 0;
        if (c == '+' || c == '-') {
            negative = c == '-';
            __c1_scan_take(s);
            width--;
        }
        number = 0;
        digits = 0;
        automatic = conversion == 'i';
        base = conversion == 'o'                                               ? 8
               : (conversion == 'x' || conversion == 'X' || conversion == 'p') ? 16
                                                                               : 10;
        if (width && __c1_scan_peek(s) == '0') {
            __c1_scan_take(s);
            width--;
            digits = 1;
            if (automatic)
                base = 8;
            if (width && (automatic || base == 16) &&
                (__c1_scan_peek(s) == 'x' || __c1_scan_peek(s) == 'X')) {
                __c1_scan_take(s);
                width--;
                base = 16;
                digits = 0;
            }
        }
        while (width) {
            digit = __c1_scan_digit(__c1_scan_peek(s));
            if (digit < 0 || digit >= base)
                break;
            __c1_scan_take(s);
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
                __c1_scan_store(target, length, conversion != 'd' && conversion != 'i', number);
            assigned++;
        }
        completed = 1;
    }
    return input_failure && !completed ? EOF : assigned;
invalid:
    errno = EINVAL;
    return assigned;
}

int vsscanf(const char *text, const char *format, va_list ap) {
    __c1_scan s;
    s.file = NULL;
    s.text = (const unsigned char *)text;
    s.count = 0;
    s.look = -2;
    return __c1_scan_run(&s, format, ap);
}

int vfscanf(FILE *file, const char *format, va_list ap) {
    __c1_scan s;
    int result;
    s.file = file;
    s.text = NULL;
    s.count = 0;
    s.look = -2;
    result = __c1_scan_run(&s, format, ap);
    if (s.look >= 0)
        ungetc(s.look, file);
    return result;
}

int vscanf(const char *format, va_list ap) {
    return vfscanf(stdin, format, ap);
}

int sscanf(const char *text, const char *format, ...) {
    va_list ap;
    int result;
    va_start(ap, format);
    result = vsscanf(text, format, ap);
    va_end(ap);
    return result;
}

int fscanf(FILE *file, const char *format, ...) {
    va_list ap;
    int result;
    va_start(ap, format);
    result = vfscanf(file, format, ap);
    va_end(ap);
    return result;
}

int scanf(const char *format, ...) {
    va_list ap;
    int result;
    va_start(ap, format);
    result = vscanf(format, ap);
    va_end(ap);
    return result;
}

#endif
