#include <wchar.h>
#include <stdio.h>
#include <errno.h>

int main(void) {
    mbstate_t state = {0};
    wchar_t wc, wide[40], winput[40];
    char byte[40], input[40];
    const char *src, *start;
    const wchar_t *wsrc, *wstart;
    int i, j, len, cap, written;
    if (!mbsinit(NULL) || !mbsinit(&state) || btowc(EOF) != WEOF || wctob(WEOF) != EOF)
        return 1;
    for (i = 0; i < 256; i++) {
        byte[0] = (char)i;
        wc = 999;
        errno = 17;
        if (btowc(i) != (wint_t)i || wctob(i) != i)
            return 2;
        if (mbrtowc(&wc, byte, 1, &state) != (size_t)(i ? 1 : 0) || wc != i || !mbsinit(&state))
            return 3;
        if (mbrlen(byte, 1, NULL) != (size_t)(i ? 1 : 0))
            return 4;
        byte[0] = 42;
        if (wcrtomb(byte, (wchar_t)i, &state) != 1 || (unsigned char)byte[0] != i || errno != 17)
            return 5;
    }
    for (i = 256; i < 65536; i++)
        if (wctob(i) != EOF)
            return 6;
    wc = 123;
    errno = 17;
    if (mbrtowc(&wc, "x", 0, &state) != (size_t)-2 || wc != 123 || errno != 17)
        return 7;
    if (mbrtowc(&wc, NULL, 0, &state) || wc != 123 || wcrtomb(NULL, 0xffff, &state) != 1)
        return 8;
    byte[0] = 42;
    errno = 0;
    if (wcrtomb(byte, 256, &state) != (size_t)-1 || errno != EILSEQ || byte[0] != 42)
        return 9;
    state.__state = 0;
    src = "ab";
    start = src;
    if (mbsrtowcs(NULL, &src, 0, &state) != 2 || src != start)
        return 10;
    wide[0] = 99;
    if (mbsrtowcs(wide, &src, 0, &state) || src != start || wide[0] != 99)
        return 11;
    wide[1] = 99;
    if (mbsrtowcs(wide, &src, 1, &state) != 1 || src != start + 1 || wide[0] != 'a' ||
        wide[1] != 99)
        return 12;
    if (mbsrtowcs(wide, &src, 2, &state) != 1 || src || wide[0] != 'b' || wide[1])
        return 13;
    wsrc = L"ab";
    wstart = wsrc;
    if (wcsrtombs(NULL, &wsrc, 0, &state) != 2 || wsrc != wstart)
        return 14;
    byte[0] = 99;
    if (wcsrtombs(byte, &wsrc, 0, &state) || wsrc != wstart || byte[0] != 99)
        return 15;
    byte[1] = 99;
    if (wcsrtombs(byte, &wsrc, 1, &state) != 1 || wsrc != wstart + 1 || byte[0] != 'a' ||
        byte[1] != 99)
        return 16;
    if (wcsrtombs(byte, &wsrc, 2, &state) != 1 || wsrc || byte[0] != 'b' || byte[1])
        return 17;
    wsrc = L"a\x1234";
    wstart = wsrc;
    errno = 0;
    if (wcsrtombs(NULL, &wsrc, 0, &state) != (size_t)-1 || wsrc != wstart || errno != EILSEQ)
        return 18;
    state.__state = 0;
    byte[1] = 99;
    if (wcsrtombs(byte, &wsrc, 8, &state) != (size_t)-1 || wsrc != wstart + 1 || byte[0] != 'a' ||
        byte[1] != 99)
        return 19;
    state.__state = 0;
    src = "\xff";
    if (mbsrtowcs(wide, &src, 8, NULL) != 1 || src || wide[0] != 255 || wide[1])
        return 20;
    wsrc = L"\xff";
    if (wcsrtombs(byte, &wsrc, 8, NULL) != 1 || wsrc || (unsigned char)byte[0] != 255 || byte[1])
        return 21;
    for (len = 0; len <= 32; len++)
        for (cap = 0; cap <= 33; cap++) {
            for (i = 0; i < len; i++) {
                input[i] = (char)((i * 37 + 11) % 255 + 1);
                winput[i] = (unsigned char)input[i];
            }
            input[len] = 0;
            winput[len] = 0;
            src = input;
            wsrc = winput;
            for (i = 0; i < 40; i++) {
                byte[i] = 90;
                wide[i] = 0xabcd;
            }
            written = cap < len ? cap : len;
            if (mbsrtowcs(wide, &src, cap, &state) != (size_t)written)
                return 22;
            if (wcsrtombs(byte, &wsrc, cap, &state) != (size_t)written)
                return 23;
            if (cap > len) {
                if (src || wsrc)
                    return 24;
            } else if (src != input + written || wsrc != winput + written)
                return 25;
            for (j = 0; j < 40; j++) {
                if (j < written) {
                    if (wide[j] != winput[j] || (unsigned char)byte[j] != (unsigned char)input[j])
                        return 26;
                } else if (j == len && cap > len) {
                    if (wide[j] || byte[j])
                        return 27;
                } else if (wide[j] != 0xabcd || byte[j] != 90)
                    return 28;
            }
        }
    wsrc = L"a\x1234";
    errno = 17;
    if (wcsrtombs(byte, &wsrc, 1, &state) != 1 || errno != 17 || *wsrc != 0x1234)
        return 29;
    if (wcsrtombs(byte, &wsrc, 1, &state) != (size_t)-1 || errno != EILSEQ || *wsrc != 0x1234)
        return 30;
    return 0;
}
