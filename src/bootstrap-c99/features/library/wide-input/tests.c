#include <wchar.h>
#include <stdio.h>
#include <errno.h>
#include <fenv.h>
#include <string.h>
#include <stdint.h>

static int memory(const wchar_t *text, const wchar_t *format, ...) {
    va_list ap;
    int r;
    va_start(ap, format);
    r = vswscanf(text, format, ap);
    va_end(ap);
    return r;
}

static int stream(FILE *f, const wchar_t *format, ...) {
    va_list ap;
    int r;
    va_start(ap, format);
    r = vfwscanf(f, format, ap);
    va_end(ap);
    return r;
}

static int standard(const wchar_t *format, ...) {
    va_list ap;
    int r;
    va_start(ap, format);
    r = vwscanf(format, ap);
    va_end(ap);
    return r;
}

int main(void) {
    wchar_t b[32], c, source[256], format[32];
    char text[32];
    int n, i, j, k, a, z;
    long long si;
    unsigned long long ui;
    double d;
    FILE *f;
    int modes[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    const char *numbers[] = {"0.1", "-2.5", "0x1.fffffffffffffp1023", "0x1p-1074", "1e-5000", "-0",
                             "inf", "nan"};
    const char *forms[] = {"%lf%n", "%la%n", "%lg%n"};

    union {
        double d;
        unsigned long long u;
    } one, two;

    signed char nhh;
    short nh;
    long nl;
    long long nll;
    intmax_t nj;
    intptr_t nz;
    ptrdiff_t nt;
    if (memory(L"\x4e2d -42 0xff abc", L"\x4e2d %d %i %ls%n", &a, &z, b, &n) != 3 || a != -42 ||
        z != 255 || wcscmp(b, L"abc") || n != 14)
        return 1;
    if (swscanf(L"-9223372036854775808 18446744073709551615", L"%lld %llu", &si, &ui) != 2 ||
        si != (-9223372036854775807LL - 1) || ui != 18446744073709551615ULL)
        return 2;
    if (swscanf(L"\x4e2d\x4e2e]Q", L"%3l[\x4e2d-\x4e2f]%lc", b, &c) != 2 ||
        wcscmp(b, L"\x4e2d\x4e2e") || c != ']')
        return 3;
    if (swscanf(L"]-abcQ", L"%l[]a-c-]%lc", b, &c) != 2 || wcscmp(b, L"]-abc") || c != 'Q')
        return 4;
    if (swscanf(L"\xffff A", L"%l[^ ] %c", b, text) != 2 || b[0] != 65535 || b[1] || text[0] != 'A')
        return 5;
    text[0] = 'Q';
    errno = 0;
    if (swscanf(L"\x4e2d", L"%s", text) != EOF || errno != EILSEQ || text[0] != 'Q')
        return 6;
    if (swscanf(L"\x4e2d 42", L"%*s%d", &a) != 1 || a != 42)
        return 7;
    if (swscanf(L"", L"%d", &a) != EOF || swscanf(L"Q", L"%d", &a) != 0)
        return 8;
    if (swscanf(L"\x0131", L"%d", &a) != 0)
        return 9;
    if (swscanf(L"abc", L"%2lc%n", b, &n) != 1 || b[0] != 'a' || b[1] != 'b' || n != 2)
        return 10;
    nhh = -1;
    nh = -1;
    nl = -1;
    nll = -1;
    nj = -1;
    nz = -1;
    nt = -1;
    if (swscanf(L"ABC", L"%*3lc%hhn%hn%ln%lln%jn%zn%tn", &nhh, &nh, &nl, &nll, &nj, &nz, &nt) !=
            0 ||
        nhh != 3 || nh != 3 || nl != 3 || nll != 3 || nj != 3 || nz != 3 || nt != 3)
        return 26;
    for (i = 0; i <= 16; i++)
        for (j = 1; j <= 16; j++) {
            for (k = 0; k < i; k++)
                source[k] = 'A';
            source[i] = 0;
            for (k = 0; k < 32; k++)
                b[k] = 0xabcd;
            swprintf(format, 32, L"%%%dls%%n", j);
            n = -1;
            a = swscanf(source, format, b, &n);
            z = i < j ? i : j;
            if (!i) {
                if (a != EOF || n != -1 || b[0] != 0xabcd)
                    return 27;
            } else {
                if (a != 1 || n != z || b[z])
                    return 28;
                for (k = 0; k < z; k++)
                    if (b[k] != 'A')
                        return 29;
                for (k = z + 1; k < 32; k++)
                    if (b[k] != 0xabcd)
                        return 30;
            }
        }
    for (i = 1; i <= 65535; i++) {
        source[0] = (wchar_t)i;
        source[1] = 0;
        b[0] = 0;
        b[1] = 0xabcd;
        n = -1;
        if (swscanf(source, L"%lc%n", b, &n) != 1 || b[0] != i || b[1] != 0xabcd || n != 1)
            return 21;
        text[0] = 'Q';
        text[1] = 'Z';
        errno = 0;
        a = swscanf(source, L"%c", text);
        if (i <= 255) {
            if (a != 1 || (unsigned char)text[0] != i || text[1] != 'Z')
                return 22;
        } else if (a != EOF || errno != EILSEQ || text[0] != 'Q' || text[1] != 'Z')
            return 23;
        if (i != ']' && i != '^' && i != '-') {
            format[0] = '%';
            format[1] = 'l';
            format[2] = '[';
            format[3] = (wchar_t)i;
            format[4] = ']';
            format[5] = 0;
            b[0] = 0;
            b[1] = 1;
            b[2] = 0xabcd;
            if (swscanf(source, format, b) != 1 || b[0] != i || b[1] || b[2] != 0xabcd)
                return 24;
            source[0] = (wchar_t)(i == 65535 ? 1 : i + 1);
            if (swscanf(source, format, b) != 0)
                return 25;
        }
    }
    for (i = 0; i < 4; i++) {
        fesetround(modes[i]);
        for (j = 0; j < 8; j++) {
            for (k = 0; numbers[j][k]; k++)
                source[k] = (unsigned char)numbers[j][k];
            source[k] = 0;
            for (z = 0; z < 3; z++) {
                for (k = 0; forms[z][k]; k++)
                    format[k] = (unsigned char)forms[z][k];
                format[k] = 0;
                n = -1;
                a = -1;
                if (sscanf(numbers[j], forms[z], &one.d, &n) != 1 ||
                    swscanf(source, format, &two.d, &a) != 1 || n != a || one.u != two.u)
                    return 11;
            }
        }
    }
    fesetround(FE_TONEAREST);
    f = tmpfile();
    if (!f)
        return 12;
    if (fputws(L"12 0x1.8p+1 Q", f) < 0)
        return 13;
    rewind(f);
    if (stream(f, L"%d %lf", &a, &d) != 2 || a != 12 || d != 3 || fgetwc(f) != ' ')
        return 14;
    if (fwscanf(f, L" %lc", &c) != 1 || c != 'Q' || fwscanf(f, L"%lc", &c) != EOF)
        return 15;
    rewind(f);
    if (fwscanf(f, L"%d", &a) != 1 || a != 12)
        return 16;
    if (fclose(f))
        return 17;
    f = tmpfile();
    if (!f)
        return 18;
    fputws(L"1e+Q", f);
    rewind(f);
    if (fwscanf(f, L"%lf", &d) != 0 || fgetwc(f) != 'Q')
        return 19;
    fclose(f);
    f = tmpfile();
    if (!f)
        return 31;
    if (fwscanf(f, L"") != 0 || fwide(f, 0) <= 0)
        return 32;
    if (fputwc(0, f) != 0 || fputwc(65535, f) != WEOF)
        return 33;
    clearerr(f);
    rewind(f);
    c = 42;
    n = -1;
    if (fwscanf(f, L"%lc%n", &c, &n) != 1 || c || n != 1)
        return 34;
    if (ungetwc(65535, f) != 65535 || fwscanf(f, L"%lc", &c) != 1 || c != 65535)
        return 35;
    fclose(f);
    if (wscanf(L"%d", &a) != 1 || a != 17 || standard(L"%d", &z) != 1 || z != 25)
        return 20;
    return 0;
}
