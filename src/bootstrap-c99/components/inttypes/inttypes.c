#include "inttypes.h"
#include <stdlib.h>
#include <errno.h>

intmax_t imaxabs(intmax_t value) {
    return value < 0 ? -value : value;
}

imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom) {
    imaxdiv_t result;
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

intmax_t strtoimax(const char *restrict text, char **restrict end, int base) {
    return strtoll(text, end, base);
}

uintmax_t strtoumax(const char *restrict text, char **restrict end, int base) {
    return strtoull(text, end, base);
}

static int __c1_wide_integer_digit(wchar_t c) {
    if (c >= 48 && c <= 57)
        return c - 48;
    if (c >= 65 && c <= 90)
        return c - 65 + 10;
    if (c >= 97 && c <= 122)
        return c - 97 + 10;
    return -1;
}

/* C locale: classify full wide units, never truncate to unsigned char. */
static uintmax_t __c1_wide_integer(const wchar_t *text, wchar_t **end, int base, uintmax_t maximum,
                                   int signed_result, int *negative, int *overflow) {
    const wchar_t *p = text;
    uintmax_t value = 0, limit = maximum, cutoff, remainder;
    int digit, any = 0;
    *negative = 0;
    *overflow = 0;
    if (end)
        *end = (wchar_t *)text;
    if (base && (base < 2 || base > 36)) {
        errno = EINVAL;
        return 0;
    }
    while (*p == 32 || (*p >= 9 && *p <= 13))
        p++;
    if (*p == 43 || *p == 45) {
        *negative = *p == 45;
        p++;
    }
    if ((!base || base == 16) && p[0] == 48 && (p[1] == 120 || p[1] == 88)) {
        digit = __c1_wide_integer_digit(p[2]);
        if (digit >= 0 && digit < 16) {
            p += 2;
            base = 16;
        }
    }
    if (!base)
        base = *p == 48 ? 8 : 10;
    if (signed_result && *negative)
        limit++;
    cutoff = limit / (unsigned int)base;
    remainder = limit % (unsigned int)base;
    for (;;) {
        digit = __c1_wide_integer_digit(*p);
        if (digit < 0 || digit >= base)
            break;
        any = 1;
        if (!*overflow) {
            if (value > cutoff || (value == cutoff && (unsigned int)digit > remainder))
                *overflow = 1;
            else
                value = value * (unsigned int)base + (unsigned int)digit;
        }
        p++;
    }
    if (any && end)
        *end = (wchar_t *)p;
    if (*overflow) {
        errno = ERANGE;
        return limit;
    }
    return value;
}

intmax_t wcstoimax(const wchar_t *restrict text, wchar_t **restrict end, int base) {
    int negative, overflow;
    uintmax_t value = __c1_wide_integer(text, end, base, INTMAX_MAX, 1, &negative, &overflow);
    if (negative) {
        if (value == (uintmax_t)INTMAX_MAX + 1)
            return INTMAX_MIN;
        return -(intmax_t)value;
    }
    return (intmax_t)value;
}

uintmax_t wcstoumax(const wchar_t *restrict text, wchar_t **restrict end, int base) {
    int negative, overflow;
    uintmax_t value = __c1_wide_integer(text, end, base, UINTMAX_MAX, 0, &negative, &overflow);
    if (overflow)
        return UINTMAX_MAX;
    return negative ? (uintmax_t)0 - value : value;
}
