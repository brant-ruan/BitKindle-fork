#include <wchar.h>
#include <stdlib.h>
#include <errno.h>

int main(void) {
    wchar_t *end, wide[128];
    char ascii[128], reverse[128];
    char *nend;
    const wchar_t *p;
    uintmax_t limits[4] = {LONG_MAX, ULONG_MAX, LLONG_MAX, ULLONG_MAX};
    uintmax_t value, digit;
    int base, which, n, i, negative;
    errno = 17;
    if (wcstol(L"-2147483648!", &end, 10) != LONG_MIN || *end != '!' || errno != 17)
        return 1;
    if (wcstol(L"2147483648!", &end, 10) != LONG_MAX || *end != '!' || errno != ERANGE)
        return 2;
    errno = 0;
    if (wcstol(L"-2147483649!", &end, 10) != LONG_MIN || *end != '!' || errno != ERANGE)
        return 3;
    errno = 17;
    if (wcstoul(L"-1!", &end, 10) != ULONG_MAX || *end != '!' || errno != 17)
        return 4;
    if (wcstoul(L"-4294967295", NULL, 10) != 1 || errno != 17)
        return 5;
    if (wcstoul(L"-4294967296!", &end, 10) != ULONG_MAX || *end != '!' || errno != ERANGE)
        return 6;
    errno = 17;
    if (wcstoll(L"-9223372036854775808!", &end, 10) != LLONG_MIN || *end != '!' || errno != 17)
        return 7;
    if (wcstoll(L"9223372036854775808!", &end, 10) != LLONG_MAX || *end != '!' || errno != ERANGE)
        return 8;
    errno = 17;
    if (wcstoull(L"-1", NULL, 10) != ULLONG_MAX || errno != 17)
        return 9;
    if (wcstoull(L"18446744073709551616!", &end, 10) != ULLONG_MAX || *end != '!' ||
        errno != ERANGE)
        return 10;
    p = L" \t+!";
    if (wcstol(p, &end, 0) || end != p)
        return 11;
    p = L"\x0131";
    if (wcstoll(p, &end, 10) || end != p)
        return 12;
    if (wcstol(L"0x!", &end, 0) || *end != 'x' || wcstol(L"052", NULL, 0) != 42 ||
        wcstol(L"0X2a", NULL, 0) != 42)
        return 13;
    /* Compare independently exposed narrow/wide conversions for every radix. */
    for (which = 0; which < 4; which++)
        for (base = 2; base <= 36; base++)
            for (negative = 0; negative < 2; negative++) {
                value = limits[which];
                n = 0;
                do {
                    digit = value % (unsigned int)base;
                    reverse[n++] = (char)(digit < 10 ? '0' + digit : 'A' + digit - 10);
                    value /= (unsigned int)base;
                } while (value);
                if (negative) {
                    ascii[0] = '-';
                    wide[0] = '-';
                }
                for (i = 0; i < n; i++) {
                    ascii[i + negative] = reverse[n - i - 1];
                    wide[i + negative] = (unsigned char)ascii[i + negative];
                }
                n += negative;
                ascii[n] = '!';
                ascii[n + 1] = 0;
                wide[n] = '!';
                wide[n + 1] = 0;
                errno = 0;
                if (which == 0) {
                    long x = strtol(ascii, &nend, base);
                    if (wcstol(wide, &end, base) != x)
                        return 14;
                }
                if (which == 1) {
                    unsigned long x = strtoul(ascii, &nend, base);
                    if (wcstoul(wide, &end, base) != x)
                        return 15;
                }
                if (which == 2) {
                    long long x = strtoll(ascii, &nend, base);
                    if (wcstoll(wide, &end, base) != x)
                        return 16;
                }
                if (which == 3) {
                    unsigned long long x = strtoull(ascii, &nend, base);
                    if (wcstoull(wide, &end, base) != x)
                        return 17;
                }
                if (errno || nend != ascii + n || end != wide + n)
                    return 18;
                wide[n] = '0';
                wide[n + 1] = '!';
                wide[n + 2] = 0;
                errno = 0;
                if (which == 0 && wcstol(wide, &end, base) != (negative ? LONG_MIN : LONG_MAX))
                    return 19;
                if (which == 1 && wcstoul(wide, &end, base) != ULONG_MAX)
                    return 20;
                if (which == 2 && wcstoll(wide, &end, base) != (negative ? LLONG_MIN : LLONG_MAX))
                    return 21;
                if (which == 3 && wcstoull(wide, &end, base) != ULLONG_MAX)
                    return 22;
                if (errno != ERANGE || end != wide + n + 1)
                    return 23;
            }
    return 0;
}
