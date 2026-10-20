#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

static int __c1_integer_digit(unsigned char c) {
    if (c >= 48 && c <= 57)
        return c - 48;
    if (c >= 65 && c <= 90)
        return c - 65 + 10;
    if (c >= 97 && c <= 122)
        return c - 97 + 10;
    return -1;
}

static unsigned long long __c1_parse_integer(const char *text, char **end, int base,
                                             unsigned long long maximum, int signed_result,
                                             int *negative, int *overflow) {
    const char *p = text;
    unsigned long long value = 0, limit = maximum, cutoff, remainder;
    int digit, any = 0;
    *negative = 0;
    *overflow = 0;
    if (end)
        *end = (char *)text;
    if (base && (base < 2 || base > 36)) {
        errno = EINVAL;
        return 0;
    }
    while (isspace((unsigned char)*p))
        p++;
    if (*p == 43 || *p == 45) {
        *negative = *p == 45;
        p++;
    }
    if ((!base || base == 16) && p[0] == 48 && (p[1] == 120 || p[1] == 88)) {
        digit = __c1_integer_digit((unsigned char)p[2]);
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
    while (1) {
        digit = __c1_integer_digit((unsigned char)*p);
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
        *end = (char *)p;
    if (*overflow) {
        errno = ERANGE;
        return limit;
    }
    return value;
}

long strtol(const char *text, char **end, int base) {
    unsigned long long value;
    int negative, overflow;
    value = __c1_parse_integer(text, end, base, LONG_MAX, 1, &negative, &overflow);
    if (negative) {
        if (value == (unsigned long long)LONG_MAX + 1)
            return LONG_MIN;
        return -(long)value;
    }
    return (long)value;
}

long long strtoll(const char *text, char **end, int base) {
    unsigned long long value;
    int negative, overflow;
    value = __c1_parse_integer(text, end, base, LLONG_MAX, 1, &negative, &overflow);
    if (negative) {
        if (value == (unsigned long long)LLONG_MAX + 1)
            return LLONG_MIN;
        return -(long long)value;
    }
    return (long long)value;
}

unsigned long strtoul(const char *text, char **end, int base) {
    unsigned long long value;
    int negative, overflow;
    value = __c1_parse_integer(text, end, base, ULONG_MAX, 0, &negative, &overflow);
    if (overflow)
        return ULONG_MAX;
    if (negative)
        return 0UL - (unsigned long)value;
    return (unsigned long)value;
}

unsigned long long strtoull(const char *text, char **end, int base) {
    unsigned long long value;
    int negative, overflow;
    value = __c1_parse_integer(text, end, base, ULLONG_MAX, 0, &negative, &overflow);
    if (overflow)
        return ULLONG_MAX;
    if (negative)
        return 0ULL - value;
    return value;
}

int atoi(const char *text) {
    return (int)strtol(text, NULL, 10);
}

long atol(const char *text) {
    return strtol(text, NULL, 10);
}

long long atoll(const char *text) {
    return strtoll(text, NULL, 10);
}
