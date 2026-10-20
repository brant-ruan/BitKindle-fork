#include <wchar.h>
#include <stdlib.h>
#include <errno.h>
#include <fenv.h>
static const char *inputs[] = {"",
                               " \t+!",
                               "0",
                               "-0",
                               "0.1!",
                               "1.5",
                               "1e+!",
                               "0x1.8p+2!",
                               "0x!",
                               "INF!",
                               "-infinity!",
                               "NaN(payload)!",
                               "nan(unclosed",
                               "1e99999!",
                               "-1e99999!",
                               "1e-99999!",
                               "-1e-99999!",
                               "0x1p-1074",
                               "0x1p-149",
                               "2.2250738585072014e-308",
                               "3.40282346638528859811704183484516925440e38"};

static int compare(const char *text) {
    wchar_t wide[4096], *wend;
    char *end;
    int i, kind, e, flags;

    union {
        double f;
        unsigned long long u;
    } d1, d2;

    union {
        float f;
        unsigned int u;
    } f1, f2;

    union {
        long double f;
        unsigned long long u;
    } l1, l2;

    for (i = 0; text[i]; i++)
        wide[i] = (unsigned char)text[i];
    wide[i] = 0;
    for (kind = 0; kind < 3; kind++) {
        errno = 17;
        feclearexcept(FE_ALL_EXCEPT);
        if (kind == 0)
            d1.f = strtod(text, &end);
        else if (kind == 1)
            f1.f = strtof(text, &end);
        else
            l1.f = strtold(text, &end);
        e = errno;
        flags = fetestexcept(FE_ALL_EXCEPT);
        errno = 17;
        feclearexcept(FE_ALL_EXCEPT);
        if (kind == 0)
            d2.f = wcstod(wide, &wend);
        else if (kind == 1)
            f2.f = wcstof(wide, &wend);
        else
            l2.f = wcstold(wide, &wend);
        if (errno != e || fetestexcept(FE_ALL_EXCEPT) != flags || wend - wide != end - text)
            return 1;
        if (kind == 0 && d1.u != d2.u)
            return 2;
        if (kind == 1 && f1.u != f2.u)
            return 3;
        if (kind == 2 && l1.u != l2.u)
            return 4;
    }
    return 0;
}

int main(void) {
    int modes[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    int i, j;
    char longtext[3008];
    const wchar_t *w;
    wchar_t *end;

    union {
        double f;
        unsigned long long u;
    } d;

    union {
        float f;
        unsigned int u;
    } f;

    for (i = 0; i < 4; i++) {
        if (fesetround(modes[i]))
            return 1;
        for (j = 0; j < 21; j++)
            if (compare(inputs[j]))
                return 2;
        longtext[0] = '1';
        longtext[1] = '.';
        for (j = 2; j < 3002; j++)
            longtext[j] = '0';
        longtext[3002] = '1';
        longtext[3003] = '!';
        longtext[3004] = 0;
        if (compare(longtext))
            return 3;
        d.f = wcstod(L"0.1", NULL);
        f.f = wcstof(L"0.1", NULL);
        if (d.u != (i == 1 || i == 3 ? 0x3fb9999999999999ULL : 0x3fb999999999999aULL))
            return 4;
        if (f.u != (i == 1 || i == 3 ? 0x3dccccccU : 0x3dcccccdU))
            return 5;
    }
    fesetround(FE_TONEAREST);
    w = L"1\x0132";
    if (wcstod(w, &end) != 1 || end != w + 1)
        return 6;
    w = L"\x0120"
        L"1";
    if (wcstod(w, &end) != 0 || end != w)
        return 7;
    w = L"\x0149"
        L"NF";
    if (wcstod(w, &end) != 0 || end != w)
        return 8;
    w = L"NA\x014e";
    if (wcstod(w, &end) != 0 || end != w)
        return 9;
    w = L"0x\x0131";
    if (wcstod(w, &end) != 0 || end != w + 1)
        return 10;
    return 0;
}
