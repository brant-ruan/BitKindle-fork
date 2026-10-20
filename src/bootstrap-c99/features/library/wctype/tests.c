#include <wctype.h>
#include <locale.h>
#include <string.h>
#include <errno.h>
static const char *names[] = {"alnum", "alpha", "blank", "cntrl", "digit", "graph",
                              "lower", "print", "punct", "space", "upper", "xdigit"};
static int (*classify[])(wint_t) = {iswalnum, iswalpha, iswblank, iswcntrl, iswdigit, iswgraph,
                                    iswlower, iswprint, iswpunct, iswspace, iswupper, iswxdigit};

static int expected(wint_t c, int kind) {
    int letter =
        c && c < 128 && strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", (int)c) != 0;
    int digit = c && c < 128 && strchr("0123456789", (int)c) != 0;
    int lower = c && c < 128 && strchr("abcdefghijklmnopqrstuvwxyz", (int)c) != 0;
    int upper = c && c < 128 && strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ", (int)c) != 0;
    switch (kind) {
    case 0:
        return letter || digit;
    case 1:
        return letter;
    case 2:
        return c == ' ' || c == '\t';
    case 3:
        return c < 32 || c == 127;
    case 4:
        return digit;
    case 5:
        return c > 32 && c < 127;
    case 6:
        return lower;
    case 7:
        return c >= 32 && c < 127;
    case 8:
        return c > 32 && c < 127 && !letter && !digit;
    case 9:
        return c && c < 128 && strchr(" \t\n\v\f\r", (int)c) != 0;
    case 10:
        return upper;
    case 11:
        return c && c < 128 && strchr("0123456789abcdefABCDEF", (int)c) != 0;
    }
    return 0;
}

int main(void) {
    wint_t c, lo, up;
    wctype_t desc[12];
    wctrans_t lower, upper;
    int i, want;
    if (!setlocale(LC_ALL, "C"))
        return 1;
    for (i = 0; i < 12; i++) {
        desc[i] = wctype(names[i]);
        if (!desc[i])
            return 2;
    }
    if (wctype("") || wctype("Alpha") || wctype("alphabetic") || wctrans("") || wctrans("TOLOWER"))
        return 3;
    lower = wctrans("tolower");
    upper = wctrans("toupper");
    if (!lower || !upper)
        return 4;
    errno = 17;
    for (c = 0; c <= 65535; c++) {
        for (i = 0; i < 12; i++) {
            want = expected(c, i);
            if (!!classify[i](c) != want || !!iswctype(c, desc[i]) != want)
                return 5;
        }
        lo = c >= 65 && c <= 90 ? c + 32 : c;
        up = c >= 97 && c <= 122 ? c - 32 : c;
        if (towlower(c) != lo || towupper(c) != up || towctrans(c, lower) != lo ||
            towctrans(c, upper) != up)
            return 6;
    }
    for (i = 0; i < 12; i++)
        if (classify[i](WEOF) || iswctype(WEOF, desc[i]))
            return 7;
    if (towlower(WEOF) != WEOF || towupper(WEOF) != WEOF || towctrans(WEOF, lower) != WEOF ||
        towctrans(WEOF, upper) != WEOF)
        return 8;
    if (errno != 17)
        return 9;
    return 0;
}
