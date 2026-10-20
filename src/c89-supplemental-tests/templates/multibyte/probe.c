#include <stdlib.h>
#include <locale.h>

int main(void) {
    int i, n;
    char input[256], output[258], single[2];
    wchar_t wide[258], value;
    if (!setlocale(LC_ALL, "C") || MB_CUR_MAX != 1)
        return 1;
    if (mblen(NULL, 0) || mbtowc(NULL, NULL, 0) || wctomb(NULL, 0))
        return 2;
    for (i = 0; i < 256; i++) {
        single[0] = (char)i;
        single[1] = 0;
        value = 1234;
        n = mbtowc(&value, single, 1);
        if (n != (i != 0) || value != (wchar_t)i)
            return 3;
        if (mblen(single, 1) != (i != 0))
            return 4;
        if (mbtowc(NULL, single, 1) != (i != 0))
            return 5;
        output[0] = 0;
        output[1] = 42;
        if (wctomb(output, (wchar_t)i) != 1 || (unsigned char)output[0] != i || output[1] != 42)
            return 6;
    }
    for (i = 0; i < 255; i++)
        input[i] = (char)(i + 1);
    input[255] = 0;
    for (i = 0; i < 258; i++) {
        wide[i] = 1234;
        output[i] = 42;
    }
    if (mbstowcs(wide, input, 256) != 255 || wide[255] != 0 || wide[256] != 1234)
        return 7;
    for (i = 0; i < 255; i++)
        if (wide[i] != (wchar_t)(i + 1))
            return 8;
    if (wcstombs(output, wide, 256) != 255 || output[255] != 0 || output[256] != 42)
        return 9;
    for (i = 0; i < 255; i++)
        if ((unsigned char)output[i] != i + 1)
            return 10;
    wide[0] = 1234;
    wide[1] = 1234;
    if (mbstowcs(wide, input, 0) != 0 || wide[0] != 1234)
        return 11;
    if (mbstowcs(wide, input, 1) != 1 || wide[0] != 1 || wide[1] != 1234)
        return 12;
    output[0] = 42;
    output[1] = 42;
    if (wcstombs(output, wide, 0) != 0 || output[0] != 42)
        return 13;
    if (wcstombs(output, wide, 1) != 1 || output[0] != 1 || output[1] != 42)
        return 14;
    single[0] = 'A';
    value = 1234;
    if (mbtowc(&value, single, 0) != -1 || mblen(single, 0) != -1)
        return 15;
    if (wctomb(output, (wchar_t)256) != -1)
        return 16;
    wide[0] = 256;
    wide[1] = 0;
    if (wcstombs(output, wide, 2) != (size_t)-1)
        return 17;
    return 0;
}
