#include "parse.c"

long wcstol(const wchar_t *restrict text, wchar_t **restrict end, int base) {
    int negative, overflow;
    uintmax_t value = __c1_wchar_integer(text, end, base, LONG_MAX, 1, &negative, &overflow);
    if (negative) {
        if (value == (uintmax_t)LONG_MAX + 1)
            return LONG_MIN;
        return -(long)value;
    }
    return (long)value;
}

unsigned long wcstoul(const wchar_t *restrict text, wchar_t **restrict end, int base) {
    int negative, overflow;
    uintmax_t value = __c1_wchar_integer(text, end, base, ULONG_MAX, 0, &negative, &overflow);
    if (overflow)
        return ULONG_MAX;
    return negative ? 0UL - (unsigned long)value : (unsigned long)value;
}

long long wcstoll(const wchar_t *restrict text, wchar_t **restrict end, int base) {
    int negative, overflow;
    uintmax_t value = __c1_wchar_integer(text, end, base, LLONG_MAX, 1, &negative, &overflow);
    if (negative) {
        if (value == (uintmax_t)LLONG_MAX + 1)
            return LLONG_MIN;
        return -(long long)value;
    }
    return (long long)value;
}

unsigned long long wcstoull(const wchar_t *restrict text, wchar_t **restrict end, int base) {
    int negative, overflow;
    uintmax_t value = __c1_wchar_integer(text, end, base, ULLONG_MAX, 0, &negative, &overflow);
    if (overflow)
        return ULLONG_MAX;
    return negative ? 0ULL - value : value;
}
