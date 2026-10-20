#include <wchar.h>

int main(void) {
    wchar_t a[64], b[64], before[64], expected[64], tokens1[32], tokens2[32];
    wchar_t *state1, *state2, *p;
    const wchar_t *s = L"ab\x1234"
                       L"ab";
    int i, j, k, n;
    if (wcslen(s) != 5 || wcslen(L""))
        return 1;
    wmemset(a, 0xabcd, 64);
    if (wcscpy(a, s) != a || wmemcmp(a, s, 6) || a[6] != 0xabcd)
        return 2;
    wmemset(a, 0xabcd, 64);
    if (wcsncpy(a, L"xy", 5) != a || a[0] != 'x' || a[1] != 'y' || a[2] || a[3] || a[4] ||
        a[5] != 0xabcd)
        return 3;
    wmemset(a, 0xabcd, 64);
    wcsncpy(a, L"xyz", 2);
    if (a[0] != 'x' || a[1] != 'y' || a[2] != 0xabcd)
        return 4;
    wcsncpy(a, L"q", 0);
    if (a[0] != 'x')
        return 5;
    wcscpy(a, L"ab");
    if (wcscat(a, L"cd") != a || wcscmp(a, L"abcd"))
        return 6;
    if (wcsncat(a, L"efgh", 2) != a || wcscmp(a, L"abcdef"))
        return 7;
    wcsncat(a, L"ignored", 0);
    if (wcscmp(a, L"abcdef"))
        return 8;
    if (wcscmp(L"\xffff", L"\x0100") <= 0 || wcsncmp(L"abx", L"aby", 2) ||
        wcsncmp(L"abx", L"aby", 3) >= 0 || wcsncmp(L"x", L"y", 0))
        return 9;
    if (wcscoll(L"abc", L"abd") >= 0 || wcscoll(L"abc", L"abc"))
        return 10;
    if (wcsxfrm(NULL, s, 0) != 5)
        return 11;
    wmemset(a, 0xabcd, 64);
    if (wcsxfrm(a, s, 3) != 5 || wmemcmp(a, s, 3) || a[3] != 0xabcd)
        return 12;
    if (wcsxfrm(a, s, 6) != 5 || wcscmp(a, s))
        return 13;
    if (wcschr(s, 0x1234) != s + 2 || wcsrchr(s, 'a') != s + 3 || wcschr(s, 0) != s + 5 ||
        wcsrchr(s, 0) != s + 5 || wcschr(s, 'x'))
        return 14;
    if (wcsspn(s, L"ba") != 2 || wcscspn(s, L"\x1234") != 2 || wcsspn(s, L"") ||
        wcscspn(s, L"") != 5)
        return 15;
    if (wcspbrk(s, L"x\x1234") != s + 2 || wcspbrk(s, L""))
        return 16;
    if (wcsstr(s, L"ab") != s ||
        wcsstr(s, L"\x1234"
                  L"ab") != s + 2 ||
        wcsstr(s, L"") != s || wcsstr(s, L"abc") || wcsstr(L"a", L"abcdef"))
        return 17;
    wcscpy(tokens1, L",,ab:cd,,ef");
    wcscpy(tokens2, L"X/Y");
    p = wcstok(tokens1, L",:", &state1);
    if (!p || wcscmp(p, L"ab"))
        return 18;
    p = wcstok(tokens2, L"/", &state2);
    if (!p || wcscmp(p, L"X"))
        return 19;
    p = wcstok(NULL, L",", &state1);
    if (!p || wcscmp(p, L"cd"))
        return 20;
    p = wcstok(NULL, L"", &state2);
    if (!p || wcscmp(p, L"Y"))
        return 21;
    p = wcstok(NULL, L",", &state1);
    if (!p || wcscmp(p, L"ef") || wcstok(NULL, L",", &state1) || wcstok(NULL, L"/", &state2))
        return 22;
    wcscpy(tokens1, L",,,");
    if (wcstok(tokens1, L",", &state1))
        return 23;
    for (i = 0; i < 64; i++)
        before[i] = (wchar_t)(0x1200 + i);
    for (i = 0; i < 32; i++)
        for (j = 0; j < 32; j++)
            for (n = 0; n <= 16; n++) {
                for (k = 0; k < 64; k++) {
                    a[k] = before[k];
                    expected[k] = before[k];
                }
                for (k = 0; k < n; k++)
                    expected[j + k] = before[i + k];
                if (wmemmove(a + j, a + i, n) != a + j)
                    return 24;
                for (k = 0; k < 64; k++)
                    if (a[k] != expected[k])
                        return 25;
            }
    if (wmemcpy(b, before, 64) != b || wmemcmp(b, before, 64))
        return 26;
    if (wmemchr(b, 0x123f, 64) != b + 63 || wmemchr(b, 0x123f, 63) || wmemchr(b, b[0], 0))
        return 27;
    b[2] = 0;
    b[3] = 0xffff;
    a[2] = 0;
    a[3] = 0;
    if (wmemcmp(b + 2, a + 2, 2) <= 0 || wmemcmp(b, a, 0))
        return 28;
    if (wmemset(b, 0xffff, 64) != b)
        return 29;
    for (i = 0; i < 64; i++)
        if (b[i] != 0xffff)
            return 30;
    return 0;
}
