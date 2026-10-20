#include "wctype.h"
#include <ctype.h>
#include <string.h>

int iswalnum(wint_t c) {
    return c <= 127 ? isalnum((int)c) : 0;
}

int iswalpha(wint_t c) {
    return c <= 127 ? isalpha((int)c) : 0;
}

int iswblank(wint_t c) {
    return c <= 127 ? isblank((int)c) : 0;
}

int iswcntrl(wint_t c) {
    return c <= 127 ? iscntrl((int)c) : 0;
}

int iswdigit(wint_t c) {
    return c <= 127 ? isdigit((int)c) : 0;
}

int iswgraph(wint_t c) {
    return c <= 127 ? isgraph((int)c) : 0;
}

int iswlower(wint_t c) {
    return c <= 127 ? islower((int)c) : 0;
}

int iswprint(wint_t c) {
    return c <= 127 ? isprint((int)c) : 0;
}

int iswpunct(wint_t c) {
    return c <= 127 ? ispunct((int)c) : 0;
}

int iswspace(wint_t c) {
    return c <= 127 ? isspace((int)c) : 0;
}

int iswupper(wint_t c) {
    return c <= 127 ? isupper((int)c) : 0;
}

int iswxdigit(wint_t c) {
    return c <= 127 ? isxdigit((int)c) : 0;
}

wctype_t wctype(const char *name) {
    if (!strcmp(name, "alnum"))
        return 1;
    if (!strcmp(name, "alpha"))
        return 2;
    if (!strcmp(name, "blank"))
        return 3;
    if (!strcmp(name, "cntrl"))
        return 4;
    if (!strcmp(name, "digit"))
        return 5;
    if (!strcmp(name, "graph"))
        return 6;
    if (!strcmp(name, "lower"))
        return 7;
    if (!strcmp(name, "print"))
        return 8;
    if (!strcmp(name, "punct"))
        return 9;
    if (!strcmp(name, "space"))
        return 10;
    if (!strcmp(name, "upper"))
        return 11;
    if (!strcmp(name, "xdigit"))
        return 12;
    return 0;
}

int iswctype(wint_t c, wctype_t desc) {
    switch (desc) {
    case 1:
        return iswalnum(c);
    case 2:
        return iswalpha(c);
    case 3:
        return iswblank(c);
    case 4:
        return iswcntrl(c);
    case 5:
        return iswdigit(c);
    case 6:
        return iswgraph(c);
    case 7:
        return iswlower(c);
    case 8:
        return iswprint(c);
    case 9:
        return iswpunct(c);
    case 10:
        return iswspace(c);
    case 11:
        return iswupper(c);
    case 12:
        return iswxdigit(c);
    default:
        return 0;
    }
}

wint_t towlower(wint_t c) {
    return c >= 65 && c <= 90 ? c + 32 : c;
}

wint_t towupper(wint_t c) {
    return c >= 97 && c <= 122 ? c - 32 : c;
}

wctrans_t wctrans(const char *name) {
    if (!strcmp(name, "tolower"))
        return 1;
    if (!strcmp(name, "toupper"))
        return 2;
    return 0;
}

wint_t towctrans(wint_t c, wctrans_t desc) {
    if (desc == 1)
        return towlower(c);
    if (desc == 2)
        return towupper(c);
    return c;
}
