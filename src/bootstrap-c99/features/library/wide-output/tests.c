#include <wchar.h>
#include <stdio.h>
#include <fenv.h>
#include <errno.h>

static int memory(wchar_t *out, size_t n, const wchar_t *format, ...) {
    va_list ap;
    int r;
    va_start(ap, format);
    r = vswprintf(out, n, format, ap);
    va_end(ap);
    return r;
}

static int stream(FILE *out, const wchar_t *format, ...) {
    va_list ap;
    int r;
    va_start(ap, format);
    r = vfwprintf(out, format, ap);
    va_end(ap);
    return r;
}

static int standard(const wchar_t *format, ...) {
    va_list ap;
    int r;
    va_start(ap, format);
    r = vwprintf(format, ap);
    va_end(ap);
    return r;
}

int main(void) {
    wchar_t b[512], format[64];
    char narrow[512];
    int i, j, k, n, r, count;
    int modes[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    const char *forms[] = {"%.0f", "%+.8e", "%#.12g", "%020.3a", "%-20.6G", "%.17g"};
    double values[6] = {0.0, -0.0, 0.1, -2.5, 1e100, 0x1p-1074};
    FILE *f;
    if (swprintf(b, 512, L"\x4e2d:%ls:%lc:%s:%c%n", L"\x1234", (wint_t)0xffff, "AB", 255, &count) !=
            10 ||
        count != 10)
        return 1;
    if (b[0] != 0x4e2d || b[2] != 0x1234 || b[4] != 0xffff || b[6] != 'A' || b[7] != 'B' ||
        b[9] != 255 || b[10])
        return 2;
    if (memory(b, 512, L"[%*.*ls]", -6, 2,
               L"\x4e2d\x1234"
               L"abc") != 8 ||
        wcscmp(b, L"[\x4e2d\x1234    ]"))
        return 3;
    if (swprintf(b, 512, L"%lld|%llu|%#x", (-9223372036854775807LL - 1), 18446744073709551615ULL,
                 42U) != 46 ||
        wcscmp(b, L"-9223372036854775808|18446744073709551615|0x2a"))
        return 4;
    for (n = 0; n <= 12; n++) {
        for (i = 0; i < 512; i++)
            b[i] = 0xabcd;
        r = swprintf(b, n, L"\x4e2d[%08x]", 42U);
        if (n < 12) {
            if (r >= 0)
                return 5;
        } else if (r != 11 || wcscmp(b, L"\x4e2d[0000002a]"))
            return 6;
        for (i = n; i < 512; i++)
            if (b[i] != 0xabcd)
                return 7;
    }
    if (swprintf(b, 2, L"%2147483647d", 1) >= 0)
        return 8;
    for (i = 0; i < 4; i++) {
        fesetround(modes[i]);
        for (j = 0; j < 6; j++) {
            for (k = 0; forms[j][k]; k++)
                format[k] = (unsigned char)forms[j][k];
            format[k] = 0;
            for (k = 0; k < 6; k++) {
                n = snprintf(narrow, 512, forms[j], values[k]);
                r = swprintf(b, 512, format, values[k]);
                if (n != r)
                    return 9;
                for (count = 0; count <= n; count++)
                    if (b[count] != (unsigned char)narrow[count])
                        return 10;
            }
        }
    }
    fesetround(FE_TONEAREST);
    f = tmpfile();
    if (!f || stream(f, L"%ls:%d\n", L"AB", 42) != 6 || fwide(f, 0) <= 0)
        return 11;
    rewind(f);
    if (fgetws(b, 512, f) != b || wcscmp(b, L"AB:42\n"))
        return 12;
    rewind(f);
    errno = 0;
    if (fwprintf(f, L"\x4e2d") >= 0 || errno != EILSEQ || !ferror(f))
        return 13;
    if (fclose(f))
        return 14;
    if (wprintf(L"%ls:", L"W") != 2 || standard(L"%d\n", 42) != 3)
        return 15;
    return 0;
}
