#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "format-tests.c"

int main(void) {
    char *end;
    wchar_t *wend;
    const wchar_t *w;
    imaxdiv_t d;
    char text[96];
    wchar_t wide[96];
    uintmax_t number, v;
    int base, n, i;
    if (format_tests())
        return 1;
    if (sprintf(text, "%" PRIdMAX, INTMAX_MIN) != 20 || strcmp(text, "-9223372036854775808"))
        return 24;
    if (sprintf(text, "%" PRIuMAX, UINTMAX_MAX) != 20 || strcmp(text, "18446744073709551615"))
        return 25;
    if (sprintf(text, "%" PRIXPTR, (uintptr_t)UINTPTR_MAX) != 16 ||
        strcmp(text, "FFFFFFFFFFFFFFFF"))
        return 26;
    if (imaxabs(-42) != 42 || imaxabs(INTMAX_MAX) != INTMAX_MAX)
        return 2;
    d = imaxdiv(-43, 7);
    if (d.quot != -6 || d.rem != -1)
        return 3;
    d = imaxdiv(43, -7);
    if (d.quot != -6 || d.rem != 1)
        return 4;
    d = imaxdiv(INTMAX_MIN, 1);
    if (d.quot != INTMAX_MIN || d.rem)
        return 5;
    errno = 0;
    if (strtoimax("-9223372036854775808!", &end, 10) != INTMAX_MIN || *end != '!' || errno)
        return 6;
    if (strtoumax("18446744073709551615!", &end, 10) != UINTMAX_MAX || *end != '!' || errno)
        return 7;
    if (strtoimax("9223372036854775808", 0, 10) != INTMAX_MAX || errno != ERANGE)
        return 8;
    errno = 0;
    if (wcstoimax(L" \t-9223372036854775808!", &wend, 10) != INTMAX_MIN || *wend != '!' || errno)
        return 9;
    if (wcstoimax(L"9223372036854775808tail", &wend, 10) != INTMAX_MAX || *wend != 't' ||
        errno != ERANGE)
        return 10;
    errno = 0;
    if (wcstoimax(L"-9223372036854775809tail", &wend, 10) != INTMAX_MIN || *wend != 't' ||
        errno != ERANGE)
        return 11;
    errno = 0;
    if (wcstoumax(L"18446744073709551615!", &wend, 10) != UINTMAX_MAX || *wend != '!' || errno)
        return 12;
    if (wcstoumax(L"-18446744073709551616tail", &wend, 10) != UINTMAX_MAX || *wend != 't' ||
        errno != ERANGE)
        return 13;
    errno = 17;
    if (wcstoumax(L"-1", 0, 10) != UINTMAX_MAX || errno != 17)
        return 14;
    w = L"  +z";
    if (wcstoimax(w, &wend, 10) != 0 || wend != w)
        return 15;
    if (wcstoimax(L"0x!", &wend, 0) != 0 || *wend != 'x')
        return 16;
    if (wcstoimax(L"0X2a!", &wend, 0) != 42 || *wend != '!')
        return 17;
    if (wcstoimax(L"052!", &wend, 0) != 42 || *wend != '!')
        return 18;
    w = L"\x0131";
    if (wcstoimax(w, &wend, 10) != 0 || wend != w)
        return 19;
    w = L"\x0120"
        L"42";
    if (wcstoimax(w, &wend, 10) != 0 || wend != w)
        return 20;
    w = L"42";
    errno = 0;
    if (wcstoimax(w, &wend, 1) != 0 || wend != w || errno != EINVAL)
        return 21;
    /* All supported radices, including long input and unsigned overflow. */
    for (base = 2; base <= 36; base++) {
        number = UINTMAX_MAX;
        n = 0;
        do {
            v = number % (unsigned int)base;
            text[n++] = (char)(v < 10 ? '0' + v : 'a' + v - 10);
            number /= (unsigned int)base;
        } while (number);
        for (i = 0; i < n; i++)
            wide[i] = (wchar_t)text[n - i - 1];
        wide[n] = 0;
        errno = 0;
        if (wcstoumax(wide, &wend, base) != UINTMAX_MAX || wend != wide + n || errno)
            return 22;
        wide[n] = '0';
        wide[n + 1] = '!';
        wide[n + 2] = 0;
        if (wcstoumax(wide, &wend, base) != UINTMAX_MAX || wend != wide + n + 1 || errno != ERANGE)
            return 23;
    }
    return 0;
}
